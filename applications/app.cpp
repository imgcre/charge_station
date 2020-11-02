/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-13     imgcr       the first version
 */

#define LOG_TAG "app"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#include <ali_mqtt.h>
#include <rtthread.h>
#include <memory>
#include <relay.h>
#include <rtdevice.h>
#include "rc522.h"
#include "wtn6.h"
#include "state.h"
#include "light.h"
#include "port_state.h"

using namespace std;

#define CURRENT_THRESHOLD 50

//电流通道反过来
//
//{
//    "timer_id": 1,
//    "minutes": 1,
//    "port": 2
//}

PortState portStateA(1), portStateB(2);
PortState* lastInsertPort = nullptr;

rt_timer_t timer, timerWdt;

void tryConeectMqtt();
void printMqttError(rt_err_t connRes);

cJSON* jsonMakeStateItem(int port, int timer_id, int left_minutes, PortState::Value state, float current, float voltage, float consumption) {
    cJSON* obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "port", port);
    cJSON_AddNumberToObject(obj, "timer_id", timer_id);
    cJSON_AddNumberToObject(obj, "left_minutes", left_minutes);
    cJSON_AddNumberToObject(obj, "state", state);
    cJSON_AddNumberToObject(obj, "current", current);
    cJSON_AddNumberToObject(obj, "voltage", voltage);
    cJSON_AddNumberToObject(obj, "consumption", consumption);
    return obj;
}


void postState();

rt_device_t wdt_device;

extern "C"
void run() {
    hlw.config();
    rt_pin_mode(22, PIN_MODE_OUTPUT);
    rt_pin_write(22, PIN_LOW);

    aliMqtt.onConnected([](){
        rt_pin_write(22, PIN_HIGH);
        auto properties = shared_ptr<cJSON>(cJSON_CreateObject(), [](auto p) {
            cJSON_Delete(p);
        });
        cJSON_AddStringToObject(properties.get(), "iccid", aliMqtt.iccid.c_str());
        aliMqtt.setProperties(properties.get());
    });

    aliMqtt.onControl([](auto port, auto minutes, auto timerId){
        LOG_I("开始充电: port=%d, duration=%dmin, timerId=%d", port, minutes, timerId);
        switch(port) {
            case 1:
                relay_ctl(Relay::First, PIN_HIGH);
                light1.setState(Light::State::LoadAndPaid);
                portStateA.startCharging(minutes, timerId);
                break;
            case 2:
                relay_ctl(Relay::Second, PIN_HIGH);
                light2.setState(Light::State::LoadAndPaid);
                portStateB.startCharging(minutes, timerId);
                break;
        }
        wtn6 << VoiceFrg::StartCharing;
        return 1;
    });

    aliMqtt.onStop([](auto port, auto timerId){
        LOG_I("停止充电: port=%d, timerId:%d", port, timerId);
        switch(port) {
            case 1:
                relay_ctl(Relay::First, PIN_LOW);
                light1.setState(Light::State::LoadButNotPay);
                portStateA.stopCharging(timerId);
                wtn6 << VoiceFrg::ChargeCompleted;
                break;
            case 2:
                relay_ctl(Relay::Second, PIN_LOW);
                light2.setState(Light::State::LoadButNotPay);
                portStateB.stopCharging(timerId);
                wtn6 << VoiceFrg::ChargeCompleted;
                break;
        }
        wtn6 << VoiceFrg::ChargeCompleted;
        return 1;
    });

    rc522.onCardInserted([](rt_uint32_t icNumber) {
        if(lastInsertPort == nullptr || lastInsertPort->isCharging()) {
            wtn6 << VoiceFrg::PlugNotReady;
            return;
        }

        auto cvt = shared_ptr<char>(new char[9]);
        rt_sprintf(cvt.get(), "%08x", icNumber);
        aliMqtt.postIcNumberEvent(lastInsertPort->getPort(), cvt.get());
        wtn6 << VoiceFrg::CardDetected;
    });

    lodDetectA.onStateChanged([](auto state){
        if(portStateA.isCharging())
            return;

        if(state) { // 0 -> 1
            if(portStateA.isLoadInserted())
                return;
            wtn6 << VoiceFrg::PortAPluged;
            aliMqtt.postPortPlugedEvent(1);
            light1.setState(Light::State::LoadButNotPay);
            lastInsertPort = &portStateA;
            portStateA.loadInserted();
            LOG_D("A插座已经插入");
        } else {
            wtn6 << VoiceFrg::PortAUnpluged;
            light1.setState(Light::State::LoadNotReady);
            portStateA.loadRemoved();
            if(lastInsertPort == &portStateA) {
                lastInsertPort = nullptr;
            }
            LOG_D("A插座已经拔出");
        }
    });

    lodDetectB.onStateChanged([](auto state){
        if(portStateB.isCharging())
            return;

        if(state) {
            if(portStateB.isLoadInserted())
                return;
            wtn6 << VoiceFrg::PortBPluged;
            aliMqtt.postPortPlugedEvent(2);
            light2.setState(Light::State::LoadButNotPay);
            lastInsertPort = &portStateB;
            portStateB.loadInserted();
            LOG_D("B插座已经插入");
        } else {
            wtn6 << VoiceFrg::PortBUnpluged;
            light2.setState(Light::State::LoadNotReady);
            portStateB.loadRemoved();
            if(lastInsertPort == &portStateB) {
                lastInsertPort = nullptr;
            }
            LOG_D("B插座已经拔出");
        }
    });

    portStateA.onInternalChargeOver([](){
        //if(!aliMqtt.isConnected()) {
        relay_ctl(Relay::First, PIN_LOW);
        light1.setState(Light::State::LoadButNotPay);
        portStateA.stopCharging(0);
        wtn6 << VoiceFrg::ChargeCompleted;

        //}
    });

    portStateB.onInternalChargeOver([](){
        //if(!aliMqtt.isConnected()) {
        relay_ctl(Relay::Second, PIN_LOW);
        light2.setState(Light::State::LoadButNotPay);
        portStateB.stopCharging(0);
        wtn6 << VoiceFrg::ChargeCompleted;
        //}
    });

    portStateA.onResumePortOpenRequired([]() {
        if(!lodDetectA.isInserted())
            return false;
        relay_ctl(Relay::First, PIN_HIGH);
        light1.setState(Light::State::LoadAndPaid);
        return true;
    });


    portStateB.onResumePortOpenRequired([]() {
        if(!lodDetectB.isInserted())
            return false;
        relay_ctl(Relay::Second, PIN_HIGH);
        light2.setState(Light::State::LoadAndPaid);
        return true;
    });

    portStateA.resume();
    portStateB.resume();

    if(lodDetectA.isInserted()) {
        portStateA.loadInserted();
    } else {
        portStateA.loadRemoved();
    }

    if(lodDetectB.isInserted()) {
        portStateB.loadInserted();
    } else {
        portStateB.loadRemoved();
    }

    aliMqtt.onQuery([](){
        postState();
    });

    timer = rt_timer_create(LOG_TAG, [](auto p) {
        postState();
    }, RT_NULL, 10000, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(timer);

    timerWdt = rt_timer_create(LOG_TAG, [](auto p) {
        if(wdt_device) {
            LOG_I("wdt");
            rt_device_control(wdt_device, RT_DEVICE_CTRL_WDT_KEEPALIVE, RT_NULL);
        }
    }, RT_NULL, 5000, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(timerWdt);

    aliMqtt.onTcpClosed([]{
        rt_device_close(wdt_device);
        wdt_device = RT_NULL;
        tryConeectMqtt();
    });

    tryConeectMqtt();
    aliMqtt.poll();

}

void tryConeectMqtt() {
    rt_thread_mdelay(5000);
    while(true) {
        auto connRes = aliMqtt.connect();
        if(connRes != RT_EOK) {
            printMqttError(connRes);
            aliMqtt.resetHW();
            continue;
        }
        break;
    }
    wdt_device = rt_device_find("wdt");
    rt_device_init(wdt_device);

    int timeout = 25;
    rt_device_control(wdt_device, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, (void *)&timeout);
    rt_device_control(wdt_device, RT_DEVICE_CTRL_WDT_START, RT_NULL);
}


void postState() {
    if(!aliMqtt.isConnected())
        return;

    auto properties = shared_ptr<cJSON>(cJSON_CreateObject(), [](auto p) {
        cJSON_Delete(p);
    });

    cJSON *current_data = cJSON_CreateArray();
    cJSON_AddItemToObject(properties.get(), "current_data", current_data);

    rt_err_t err = RT_EOK;
    float iA, iB, u;

    do {
        iA = hlw.getI<Hlw::Port::A>(&err);
        if(err != RT_EOK) {
            LOG_E("IA FATAL");
        }
    } while(err != RT_EOK);

    do {
        iB = hlw.getI<Hlw::Port::B>(&err);
        if(err != RT_EOK) {
            LOG_E("IB FATAL");
        }
    } while(err != RT_EOK);

    do {
        u = hlw.getU(&err);
        if(err != RT_EOK) {
            LOG_E("U FATAL");
        }
    } while(err != RT_EOK);

    cJSON_AddItemToArray(current_data, jsonMakeStateItem(portStateA.getPort(), portStateA.getTimerId(), portStateA.getLeftMinutes(), portStateA.get(), int(iB), int(u), 0));
    cJSON_AddItemToArray(current_data, jsonMakeStateItem(portStateB.getPort(), portStateB.getTimerId(), portStateB.getLeftMinutes(), portStateB.get(), int(iA), int(u), 0));

    cJSON_AddNumberToObject(properties.get(), "signal", aliMqtt.getCSQFromLuat());

    aliMqtt.setProperties(properties.get());

}

void printMqttError(rt_err_t connRes) {
    switch(connRes) { //在mqtt连接成功之后才能上报消息等
        case -RT_ETIMEOUT:
            LOG_E("LUAT连接超时");
            break;
        case -ALI_EDEV_IMEI:
            LOG_E("设备IMEI获取失败");
            break;
        case -ALI_EMQ_STATU:
            LOG_E("MQTT状态获取失败");
            break;
        case -ALI_ENET_GPRS:
            LOG_E("GPRS附着失败");
            break;
        case -ALI_ENET_PDP:
            LOG_E("PDP网络激活失败");
            break;
        case -ALI_EAUTH:
            LOG_E("设备注册失败");
            break;
        case -ALI_EMQ_CONF:
            LOG_E("MQTT配置失败");
            break;
        case -ALI_EMQ_SSL:
            LOG_E("MQTT SSL连接建立失败");
            break;
        case -ALI_EMQ_SESS:
            LOG_E("MQTT会话建立失败");
            break;
        case -ALI_EMQ_TSUB:
            LOG_E("MQTT主题订阅失败");
            break;
    }
}

int init_port_state() {
    portStateA.init();
    portStateB.init();
    return RT_EOK;
}

INIT_APP_EXPORT(init_port_state);
