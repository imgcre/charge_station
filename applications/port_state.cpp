/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-16     imgcr       the first version
 */

#include <rtthread.h>

extern "C" {
#include <at24cxx.h>
}

#include "port_state.h"

#define LOG_TAG "ps"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#include <string.h>
#include <stdlib.h>

at24cxx_device_t at24_dev;


int init_port_state_xx() {
    at24_dev = at24cxx_init("i2c1", 0);
    return RT_EOK;
}

void PortState::init() {
    timer = rt_timer_create("PS", [](auto p) {
        auto self = (PortState*)p;
        if(self->leftSeconds > 0) {
            LOG_I("[%d] left: %d", self->getPort(), self->leftSeconds);
            self->leftSeconds--;
            if(self->leftSeconds == 0) {
                LOG_I("done");
                if(self->onInternalChargeOverCb) {
                    self->onInternalChargeOverCb();
                }
            }
        }
        self->saveTickCnt++;
        self->saveTickCnt %= 60; //*10
        if(self->saveTickCnt == 0) {
            //SAVING DATA
            self->save();
        }
    }, this, 1000, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(timer);

}


void PortState::save() {
    Serialized s = {
        timerId: timerId,
        leftSeconds: leftSeconds,
        charging: charging,
    };
    LOG_I("[%d] saving", portNum);
    at24cxx_write(at24_dev, sizeof(Serialized) * portNum, (uint8_t*)&s, sizeof(Serialized));
    LOG_I("[%d] saved", portNum);
}

void PortState::resume() {
    Serialized s;
    at24cxx_read(at24_dev, sizeof(Serialized) * portNum, (uint8_t*)&s, sizeof(Serialized));
    timerId = s.timerId;
    leftSeconds = s.leftSeconds;
    charging = s.charging;

    if(leftSeconds > 0 && charging) {
        if(!onResumePortOpenRequiredCb || !(onResumePortOpenRequiredCb())) {
            leftSeconds = 0;
            charging = 0;
        }
    }

    LOG_I("[%d] resumed{timerId: %d, leftSeconds: %d, charging: %d}", portNum, timerId, leftSeconds, charging);
}

void at24_write_test(int argc, char** argv) {
    if(argc < 2) {
        LOG_E("too few args");
        return;
    }

    auto size = strlen(argv[1]) + 1;
    at24cxx_write(at24_dev, 0, (uint8_t*)argv[1], size);

}

void at24_read_test(int argc, char** argv) {
    if(argc < 2) {
        LOG_E("too few args");
        return;
    }

    auto size = atoi(argv[1]);

    auto buf = new char[size];


    at24cxx_read(at24_dev, 0, (uint8_t*)buf, size);

    LOG_I("read val: %s", buf);

    delete buf;


}


INIT_COMPONENT_EXPORT(init_port_state_xx);
//MSH_CMD_EXPORT(at24_write_test, at24_write_test);
//MSH_CMD_EXPORT(at24_read_test, at24_read_test);
