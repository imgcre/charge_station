/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-12     imgcr       the first version
 */
#ifndef APPLICATIONS_ALI_MQTT_H_
#define APPLICATIONS_ALI_MQTT_H_

#include <rtthread.h>
#include <cJSON.h>
#include <cJSON_util.h>
#include <string>
#include <functional>

//#define DEVICE_ID "863701042917152"
#define PRODUCT_KEY "a1tltf2GJUn"

#define DEVICE_SECRET "e96d5f79301c20994cb2e984e3cad47b"  //7152
//#define DEVICE_SECRET "83bcb7f84392825fafa83e42e09f3693"  //9224
//#define DEVICE_SECRET "4a1fa000c5c47f354228db5157348f32"  //2088

#define ALI_EAT_E 12 //AT指令执行失败
#define ALI_EAT_P 13 //AT响应解析失败
#define ALI_EMQ_STATU 14 //MQTT连接状态获取失败
#define ALI_ETRY_LIMIT 15 //尝试次数超过限制
#define ALI_ENET_PDP 16
#define ALI_ENET_GPRS 17
#define ALI_EDEV_IMEI 18
#define ALI_EAUTH 19
#define ALI_EMQ_SSL 20
#define ALI_EMQ_SESS 21
#define ALI_EMQ_CONF 22
#define ALI_EMQ_TSUB 23

#define ALI_AT_TIMEOUT 2000
#define ALI_SLL_CONN_TIMEOUT 20000

cJSON* json_make_item(int port, int timer_id, int left_minutes, int state);
int ali_mqtt_set_property(const char* deviceId, const char* productKey, cJSON* property);


////仅是接口
struct AliMqtt {
    rt_err_t connect();

    rt_err_t closeEcho();
    rt_err_t attachGprs();
    rt_err_t activatePdp();

    struct LoginParams {
        std::string username, password;
    };

    rt_err_t mqttConfig(LoginParams& params);
    rt_err_t mqttConnectSsl();
    rt_err_t mqttConnectSess();
    rt_err_t mqttSubTopic(std::string topicSuffix);
    LoginParams getLoginParams();
    std::string makeTopicPrefix();

    void poll();
    void resetHW();

    //事件触发
    rt_err_t postIcNumberEvent(int port, std::string icCard);
    rt_err_t postPortPlugedEvent(int port);

    //由caller负责释放properties
    rt_err_t setProperties(cJSON* properties);

    void onControl(std::function<int(int, int, int)> cb) {
        onControlCb = cb;
    }

    void onStop(std::function<int(int port, int timerId)> cb) {
        onStopCb = cb;
    }

    void onTcpClosed(std::function<void()> cb) {
        onTcpClosedCb = cb;
    }

    void onConnected(std::function<void()> cb) {
        onConnectedCb = cb;
    }

    void onQuery(std::function<void()> cb) {
        onQueryCb = cb;
    }

    bool isConnected() {
        return connected;
    }

//private:

    enum class MqttStatus {
        Offline,
        Online,
        Unauthorized,
    };

    MqttStatus getMqttStatus();
    bool connected = false;

    std::string getImeiFromLuat();
    std::string getIccidFromLuat();
    int getCSQFromLuat();

    std::string imei, iccid;

    std::function<int(int port, int minutes, int timerId)> onControlCb;
    std::function<int(int port, int timerId)> onStopCb;
    std::function<void()> onTcpClosedCb, onConnectedCb, onQueryCb;
    LoginParams params;

};

extern AliMqtt aliMqtt;



#endif /* APPLICATIONS_ALI_MQTT_H_ */
