/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-07-30     imgcr       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>
#include <stdexcept>

#define LOG_TAG "ali.mqtt"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#include <tinycrypt.h>
#include <string.h>

#include <memory>

#include "ali_mqtt.h"

using namespace std;

enum mqtt_event {
    mqtt_event_http_action = 1,
    mqtt_event_conn_ok = 2,
    mqtt_event_conn_ack = 4,
    mqtt_event_closed = 8,
    mqtt_event_suback = 16,
    mqtt_event_already_conn = 32,
};
rt_event_t event;
rt_thread_t thread;
rt_mailbox_t mailbox;

AliMqtt aliMqtt;

char* luat_get_imei();
char* luat_json_escape(cJSON* root);

static void on_http_action(at_client_t client, const char* data, rt_size_t size) {
    LOG_I("on http action");
    rt_event_send(event, mqtt_event_http_action);
}

static void on_conn_ok(at_client_t client, const char* data, rt_size_t size) {
    LOG_I("on conn ok");
    rt_event_send(event, mqtt_event_conn_ok);
}

static void on_conn_ack(at_client_t client, const char* data, rt_size_t size) {
    LOG_I("on conn ack");
    rt_event_send(event, mqtt_event_conn_ack);
}

static void on_closed(at_client_t client, const char* data, rt_size_t size) {
    LOG_I("on closed");
    rt_event_send(event, mqtt_event_closed);
}

static void on_sub_ack(at_client_t client, const char* data, rt_size_t size) {
    LOG_I("on sub ack");
    rt_event_send(event, mqtt_event_suback);
}

static void on_already_conn(at_client_t client, const char* data, rt_size_t size) {
    LOG_I("on already conn");
    rt_event_send(event, mqtt_event_already_conn);
}





char* get_req_id_from_data(const char* data) {
    char* json_str = strstr(data, "{");
    char* id_end = json_str;
    while(*id_end != '"')
        id_end--;

    char* id_begin = id_end;
    while(*id_begin != '/')
        id_begin--;

    id_begin++; //恢复到第一个digit的位置
    id_end--; //移动的最后一个digit的位置

    int str_size = id_end - id_begin + 1;
    char* id_str = (char*)rt_malloc(str_size + 1);
    rt_memset(id_str, '\0', str_size + 1);
    rt_memcpy(id_str, id_begin, str_size);

    return id_str;
}

//NOTE: MQTT消息处理
static void on_mqtt_msg(at_client_t client, const char* data, rt_size_t size) {
    LOG_I("on mqtt msg: %s", data);
    char* json_str = strstr(data, "{");

    cJSON *root;
    if(json_str == RT_NULL)
        return;

    root = cJSON_Parse(json_str); //在这里处理各种json类型

    const char* method = cJSON_item_get_string(root, "method");
    if(strcmp(method, "thing.service.control") == 0 || strcmp(method, "thing.service.stop") || strcmp(method, "thing.service.query") == 0) {
        char* reqId = get_req_id_from_data(data);
        cJSON_AddStringToObject(root, "reqId", reqId);
        rt_mb_send(mailbox, (rt_uint32_t)root);
        rt_free(reqId);
        return;
    }

    cJSON_Delete(root);
}

static struct at_urc urc_table[] = {
    {"+HTTPACTION:", "\r\n", on_http_action},
    {"CONNECT OK", "\r\n", on_conn_ok},
    {"CONNACK OK", "\r\n", on_conn_ack},
    {"+MSUB: ", "\r\n", on_mqtt_msg},
    {"CLOSED", "\r\n", on_closed},
    {"SUBACK", "\r\n", on_sub_ack},
    {"ALREADY CONNECT", "\r\n", on_already_conn},
};


rt_err_t ali_mqtt_service_resp(const char* deviceId, const char* productKey, const char* reqId, cJSON* data) {
    cJSON *root = RT_NULL;

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", 233);
    cJSON_AddNumberToObject(root, "code", 200);
    cJSON_AddItemReferenceToObject(root, "data", data);

    char* escaped = luat_json_escape(root);
    at_exec_cmd(RT_NULL, "AT+MPUB=\"/sys/%s/%s/rrpc/response/%s\",0,0,\"%s\"", productKey, deviceId, reqId, escaped);

    rt_free(escaped);
    rt_free(root);
    return RT_EOK;
}

static int init_mqtt() {
    rt_pin_mode(0, PIN_MODE_OUTPUT);
    rt_pin_write(0, PIN_LOW);
    at_client_init("uart2", 512);
    at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));
    event = rt_event_create("mqtt_event", RT_IPC_FLAG_PRIO);
    mailbox = rt_mb_create(LOG_TAG, 64, RT_IPC_FLAG_FIFO);
    return RT_EOK;
}


char* hex_to_ascii(unsigned char* input, int ilen) {
    char* result = (char*)rt_malloc(ilen * 2 + 1);
    result[ilen * 2] = '\0';

    for(int i = 0; i < ilen; i++) {
        rt_sprintf(&result[i * 2], "%02x", input[i]);
    }
    return result;
}

char* ali_sign(const char* deviceId, const char* productKey, const char* deviceSecret) {
    char* input = (char*)rt_malloc(128);
    unsigned char* output = (unsigned char*)rt_malloc(16);
    char* hash = RT_NULL;

    rt_sprintf(input, "clientId%sdeviceName%sproductKey%s", deviceId, deviceId, productKey);
    tiny_md5_hmac((unsigned char*)deviceSecret, strlen(deviceSecret), (unsigned char*)input, strlen(input), output);
    hash = hex_to_ascii(output, 16);

    rt_free(input);
    rt_free(output);
    return hash;
}

char* luat_json_escape(cJSON* root) {
    char* json_str = cJSON_PrintUnformatted(root);
    int quota_cnt = 0;
    char* token = RT_NULL;
    for(char* p = json_str; *p != '\0'; p++) {
        if(*p == '"')
            quota_cnt++;
    }

    char* escaped = (char*)rt_malloc(strlen(json_str) + quota_cnt * 2 + 1);
    char *p = escaped;

    token = strtok(json_str, "\"");
    while(token != RT_NULL) {
        strcpy(p, token);
        p += strlen(token);
        token = strtok(RT_NULL, "\"");
        if(token != RT_NULL) {
            strcpy(p, "\\22");
            p += 3;
        }
    }

    rt_free(json_str);
    return escaped;
}

int ali_mqtt_event_post(const char* deviceId, const char* productKey, const char* eventName, cJSON* params) {
    cJSON *root = RT_NULL;
    char* method = (char*)rt_malloc(64);
    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", 233);
    cJSON_AddStringToObject(root, "version", "1.0");

    rt_sprintf(method, "thing.event.%s.post", eventName);
    cJSON_AddStringToObject(root, "method", method);
    cJSON_AddItemReferenceToObject(root, "params", params);

    auto resp = shared_ptr<at_response>(at_create_resp(256, 0, RT_WAITING_FOREVER), [](auto p) {
        at_delete_resp(p);
    });

    char* escaped = luat_json_escape(root);
    at_exec_cmd(resp.get(), "AT+MPUB=\"/sys/%s/%s/thing/event/%s/post\",0,0,\"%s\"", productKey, deviceId, eventName, escaped);
    rt_free(escaped);
    cJSON_Delete(root);
    rt_free(method);
    return RT_EOK;
}

//property由本函数负责释放
int ali_mqtt_set_property(const char* deviceId, const char* productKey, cJSON* property) {
    return ali_mqtt_event_post(deviceId, productKey, "property", property);
}

void luat_reset() {
    rt_pin_write(0, PIN_HIGH);
    rt_thread_mdelay(1000);
    rt_pin_write(0, PIN_LOW);
}

rt_err_t AliMqtt::connect() {
    if(at_client_wait_connect(ALI_AT_TIMEOUT) != RT_EOK) return -RT_ETIMEOUT;
    if(closeEcho() != RT_EOK) return -ALI_EDEV_IMEI;
    LOG_I("LUAT已连接");
    imei = getImeiFromLuat();
    if(rt_get_errno() != RT_EOK) {
        return -ALI_EDEV_IMEI;
    }
    LOG_I("imei: %s", imei.c_str());

    iccid = getIccidFromLuat();
    if(rt_get_errno() != RT_EOK) {
        return -ALI_EDEV_IMEI;
    }
    LOG_I("iccid: %s", iccid.c_str());

    auto status = getMqttStatus();
    if(rt_get_errno() != RT_EOK) {
        return -ALI_EMQ_STATU;
    }

    LoginParams params;
    switch(status) {
        case MqttStatus::Online:
            LOG_I("MQTT已连接");
            break;
        case MqttStatus::Offline:
            if(attachGprs() != RT_EOK) return -ALI_ENET_GPRS;
            LOG_I("GPRS已附着");
            if(activatePdp() != RT_EOK) return -ALI_ENET_PDP;
            LOG_I("PDP已网络激活");
            params = getLoginParams();
            if(rt_get_errno() != RT_EOK) return -ALI_EAUTH;
            LOG_I("设备已注册");
            if(mqttConfig(params) != RT_EOK) return -ALI_EMQ_CONF;
            LOG_D("-u: %s, -p: %s", params.username.c_str(), params.password.c_str());
            if(mqttConnectSsl() != RT_EOK) return -ALI_EMQ_SSL;
            //break;
        case MqttStatus::Unauthorized:
            if(mqttConnectSess() != RT_EOK) return -ALI_EMQ_SESS;
            LOG_I("MQTT已连接");
            if(mqttSubTopic("/thing/service/property/set") != RT_EOK) return -ALI_EMQ_TSUB;
            if(mqttSubTopic("/rrpc/request/+") != RT_EOK) return -ALI_EMQ_TSUB;
            LOG_I("MQTT主题已订阅");
            break;
    }

    connected = true;
    onConnectedCb();
    return RT_EOK;
}

rt_err_t AliMqtt::attachGprs() {
    auto resp = shared_ptr<at_response>(at_create_resp(64, 0, ALI_AT_TIMEOUT), [](auto p) {
       at_delete_resp(p);
   });
    int cgatt_val;
    do {
        if(at_exec_cmd(resp.get(), "AT+CGATT?") != RT_EOK) return -ALI_EAT_E;
        if(at_resp_parse_line_args_by_kw(resp.get(), "CGATT:", "+CGATT: %d", &cgatt_val) <= 0) return -ALI_EAT_P;
        if(cgatt_val != 1) {
            rt_thread_mdelay(1000);
        }
    } while(cgatt_val != 1);
    return RT_EOK;
}

rt_err_t AliMqtt::activatePdp() {
    auto resp = shared_ptr<at_response>(at_create_resp(64, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });
    for(auto i = 0; i < 3; i++) {
        int cid, status;
        if(at_exec_cmd(resp.get(), "AT+SAPBR=2,1") != RT_EOK) return -ALI_EAT_E;
        if(at_resp_parse_line_args_by_kw(resp.get(), "+SAPBR:", "+SAPBR: %d,%d", &cid, &status) < 2) return -ALI_EAT_P;
        if(status == 1) return RT_EOK;

        LOG_W("PDP not activated(%d)", i);
        if(at_exec_cmd(resp.get(), "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"") != RT_EOK) return -ALI_EAT_E;
        if(at_exec_cmd(resp.get(), "AT+SAPBR=3,1,\"APN\",\"\"") != RT_EOK) return -ALI_EAT_E;
        if(at_exec_cmd(resp.get(), "AT+SAPBR=1,1") != RT_EOK) return -ALI_EAT_E;
    }
    return -ALI_ETRY_LIMIT;
}

rt_err_t AliMqtt::mqttConfig(LoginParams& params) {
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });
    if(at_exec_cmd(resp.get(), "AT+MCONFIG=\"%s\",\"%s\",\"%s\"", imei.c_str(), params.username.c_str(), params.password.c_str()) != RT_EOK) return -ALI_EAT_E;
    return RT_EOK;
}

rt_err_t AliMqtt::mqttConnectSsl() {
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });

    if(at_exec_cmd(resp.get(), "AT+SSLMIPSTART=\"%s.iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883", PRODUCT_KEY) != RT_EOK) return -ALI_EAT_E;
    rt_uint32_t recved;
    if(rt_event_recv(event, mqtt_event_conn_ok | mqtt_event_already_conn | mqtt_event_closed, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, ALI_SLL_CONN_TIMEOUT, &recved) != RT_EOK) return -RT_ETIMEOUT;
    if((recved & mqtt_event_closed) != 0) return -ALI_EMQ_STATU;
    return RT_EOK;
}

rt_err_t AliMqtt::mqttConnectSess() {
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });

    if(at_exec_cmd(resp.get(), "AT+MCONNECT=1,300") != RT_EOK) return -ALI_EAT_E;
    rt_event_recv(event, mqtt_event_conn_ack, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, ALI_AT_TIMEOUT, RT_NULL);
    return RT_EOK;
}

rt_err_t AliMqtt::mqttSubTopic(string topicSuffix) {
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });

    if(at_exec_cmd(resp.get(), "AT+MSUB=\"%s\",0", (makeTopicPrefix() + topicSuffix).c_str()) != RT_EOK) return RT_EOK;
    rt_event_recv(event, mqtt_event_suback, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, ALI_AT_TIMEOUT, RT_NULL);
    return RT_EOK;
}

rt_err_t AliMqtt::setProperties(cJSON* properties) {
    ali_mqtt_set_property(imei.c_str(), PRODUCT_KEY, properties);
}

string AliMqtt::makeTopicPrefix() {
    return string{"/sys/"} + PRODUCT_KEY + "/" + imei;
}

AliMqtt::LoginParams AliMqtt::getLoginParams() {
    if(!params.username.empty())
        return params;

    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });
    int i;
    for(i = 0; i < 10; i++) {
        if(at_exec_cmd(resp.get(), "AT+HTTPINIT") != RT_EOK) {rt_set_errno(-ALI_EAT_E); return {};}
        if(at_exec_cmd(resp.get(), "AT+HTTPPARA=\"CID\",1") != RT_EOK) {rt_set_errno(-ALI_EAT_E); return {};}
        if(at_exec_cmd(resp.get(), "AT+HTTPPARA=\"URL\",\"https://iot-auth.cn-shanghai.aliyuncs.com/auth/devicename\"") != RT_EOK) {rt_set_errno(-ALI_EAT_E); return {};}
        if(at_exec_cmd(resp.get(), "AT+HTTPPARA=\"USER_DEFINED\",\"Content-Type: application/x-www-form-urlencoded\"") != RT_EOK) {rt_set_errno(-ALI_EAT_E); return {};}

        at_resp_set_info(resp.get(), 128, 1, ALI_AT_TIMEOUT);
        if(at_exec_cmd(resp.get(), "AT+HTTPDATA=112,20000") != RT_EOK) {rt_set_errno(-ALI_EAT_E); return {};}
        at_resp_set_info(resp.get(), 128, 0, ALI_AT_TIMEOUT);

        auto sign = shared_ptr<char>(ali_sign(imei.c_str(), PRODUCT_KEY, DEVICE_SECRET));
        LOG_D("ali sign: %s", sign.get());

        if(at_exec_cmd(resp.get(), "productKey=%s&sign=%s&clientId=%s&deviceName=%s", PRODUCT_KEY, sign.get(), imei.c_str(), imei.c_str()) != RT_EOK) {rt_set_errno(-ALI_EAT_E); return {};}
        if(at_exec_cmd(resp.get(), "AT+HTTPACTION=1") != RT_EOK) {
            at_exec_cmd(resp.get(), "AT+HTTPTERM");
            continue;
        }
        if(rt_event_recv(event, mqtt_event_http_action, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 15000, RT_NULL) == RT_EOK)
            break;
        LOG_W("timeout, try again (%d)", i);
        at_exec_cmd(resp.get(), "AT+HTTPTERM");
    }
    if(i >= 10) {rt_set_errno(-ALI_ETRY_LIMIT); return {};}
    at_resp_set_info(resp.get(), 256, 0, ALI_AT_TIMEOUT);
    if(at_exec_cmd(resp.get(), "AT+HTTPREAD") != RT_EOK) {rt_set_errno(-ALI_EAT_E); return {};}
    const char* http_resp = at_resp_get_line_by_kw(resp.get(), "code");
    at_exec_cmd(resp.get(), "AT+HTTPTERM");
    if(http_resp == RT_NULL) {rt_set_errno(-ALI_EAT_P); return {};}
    auto root = shared_ptr<cJSON>(cJSON_Parse(http_resp), [](auto p) {
        cJSON_Delete(p);
    });
    int code;
    if(cJSON_item_get_number(root.get(), "code", &code) != 0) {rt_set_errno(-ALI_EAT_P); return {};}
    if(code != 200) {rt_set_errno(-ALI_EAUTH); return {};}

    cJSON* data = cJSON_GetObjectItem(root.get(), "data");
    params.username = cJSON_item_get_string(data, "iotId");
    params.password = cJSON_item_get_string(data, "iotToken");
    rt_set_errno(RT_EOK);
    return params;
}


auto AliMqtt::getMqttStatus() -> MqttStatus {
    MqttStatus status;
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });

    if(at_exec_cmd(resp.get(), "AT+MQTTSTATU") != RT_EOK) {
        rt_set_errno(-ALI_EAT_E);
        return MqttStatus::Offline;
    }

    if(at_resp_parse_line_args_by_kw(resp.get(), "+MQTTSTATU :", "+MQTTSTATU :%d", &status) <= 0) {
        rt_set_errno(-ALI_EAT_P);
        return MqttStatus::Offline;
    }

    rt_set_errno(RT_EOK);
    return status;
}

rt_err_t AliMqtt::closeEcho() {
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });
    if(at_exec_cmd(resp.get(), "ATE0") != RT_EOK) return -ALI_EAT_E;
    return RT_EOK;
}

string AliMqtt::getImeiFromLuat() {
    if(!imei.empty())
        return imei;
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });

    if(at_exec_cmd(resp.get(), "AT+CGSN") == RT_EOK) {
        rt_set_errno(RT_EOK);
        return {at_resp_get_line(resp.get(), 2), 15};
    } else {
        rt_set_errno(ALI_EAT_E);
        return { };
    }

}

string AliMqtt::getIccidFromLuat() {
    if(!iccid.empty())
        return iccid;

    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });
    if(at_exec_cmd(resp.get(), "AT+ICCID") == RT_EOK) {
        auto iccid = shared_ptr<char>(new char[22]);
        if(at_resp_parse_line_args_by_kw(resp.get(), "+ICCID:", "+ICCID: %s", iccid.get()) <= 0) {
            rt_set_errno(-ALI_EAT_P);
            return { };
        }
        rt_set_errno(RT_EOK);
        return {iccid.get(), 20};

    } else {
        rt_set_errno(ALI_EAT_E);
        return { };
    }
}

int AliMqtt::getCSQFromLuat() {
    auto resp = shared_ptr<at_response>(at_create_resp(128, 0, ALI_AT_TIMEOUT), [](auto p) {
        at_delete_resp(p);
    });

    if(at_exec_cmd(resp.get(), "AT+CSQ") != RT_EOK) {
        rt_set_errno(ALI_EAT_E);
        return { };
    }

    int result;
    if(at_resp_parse_line_args_by_kw(resp.get(), "+CSQ:", "+CSQ: %d", &result) <= 0) {
        rt_set_errno(-ALI_EAT_P);
        return { };
    }

    rt_set_errno(RT_EOK);
    return result;



}

rt_err_t AliMqtt::postIcNumberEvent(int port, string icCard) {
    auto params = shared_ptr<cJSON>(cJSON_CreateObject(), [](auto p) {
        cJSON_Delete(p);
    });

    cJSON_AddNumberToObject(params.get(), "port", port);
    cJSON_AddStringToObject(params.get(), "ic_number", icCard.c_str());

    ali_mqtt_event_post(imei.c_str(), PRODUCT_KEY, "ic_number", params.get());
    return RT_EOK;
}


rt_err_t AliMqtt::postPortPlugedEvent(int port) {
    auto params = shared_ptr<cJSON>(cJSON_CreateObject(), [](auto p) {
        cJSON_Delete(p);
    });

    cJSON_AddNumberToObject(params.get(), "port", port);

    ali_mqtt_event_post(imei.c_str(), PRODUCT_KEY, "port_access", params.get());
    return RT_EOK;
}

void AliMqtt::poll() {
    rt_uint32_t recved;
    while(true) {
        if(rt_event_recv(event, mqtt_event_closed, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 100, &recved) == RT_EOK) {
            if((recved & mqtt_event_closed) != 0) {
                connected = false;
                onTcpClosedCb();
            }
        }

        rt_ubase_t val;
        if(rt_mb_recv(mailbox, &val, 1000) == RT_EOK) {
            auto req = shared_ptr<cJSON>((cJSON*)(void*)val, [](auto p) {
               cJSON_Delete(p);
            });

            const char* reqId = cJSON_item_get_string(req.get(), "reqId");
            cJSON* params = cJSON_GetObjectItem(req.get(), "params");

            const char* method = cJSON_item_get_string(req.get(), "method");
            if(strcmp(method, "thing.service.control") == 0) {
                int port, minutes, timerId;

                cJSON_item_get_number(params, "port", &port);
                cJSON_item_get_number(params, "minutes", &minutes);
                cJSON_item_get_number(params, "timer_id", &timerId);

                if(aliMqtt.onControlCb) {
                    auto state = aliMqtt.onControlCb(port, minutes, timerId);
                    cJSON* data = cJSON_CreateObject();
                    cJSON_AddNumberToObject(data, "state", state);

                    auto imei = aliMqtt.imei.c_str();
                    ali_mqtt_service_resp(imei, PRODUCT_KEY, reqId, data);
                    cJSON_Delete(data);
                }
            } else if(strcmp(method, "thing.service.stop") == 0) {
                int port, timerId;

                cJSON_item_get_number(params, "port", &port);
                cJSON_item_get_number(params, "timer_id", &timerId);

                if(aliMqtt.onStopCb) {
                    auto state = aliMqtt.onStopCb(port, timerId);
                    cJSON* data = cJSON_CreateObject();
                    cJSON_AddNumberToObject(data, "state", state);
                    ali_mqtt_service_resp(aliMqtt.imei.c_str(), PRODUCT_KEY, reqId, data);
                    cJSON_Delete(data);
                }
            } else if(strcmp(method, "thing.service.query") == 0) {
                if(aliMqtt.onQueryCb) {
                    aliMqtt.onQueryCb();
                    auto data = shared_ptr<cJSON>(cJSON_CreateObject(), [](auto p) {
                        cJSON_Delete(p);
                    });
                    ali_mqtt_service_resp(aliMqtt.imei.c_str(), PRODUCT_KEY, reqId, data.get());
                }
            }
        }
    }
}

void AliMqtt::resetHW() {
    LOG_W("RESET LUAT");
    rt_pin_write(0, PIN_HIGH);
    rt_thread_mdelay(1000);
    rt_pin_write(0, PIN_LOW);
    rt_thread_mdelay(5000);
}

INIT_APP_EXPORT(init_mqtt);
MSH_CMD_EXPORT(luat_reset, reset luat device)
