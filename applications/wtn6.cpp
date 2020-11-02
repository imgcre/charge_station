/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-09     imgcr       the first version
 */


#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <functional>
#include "wtn6.h"

#define LOG_TAG "app.wtn"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

#define WTN_PIN_DATA 31
#define WTN_PIN_BUSY 30

using namespace std;


void Wtn6::init() {
    rt_pin_mode(WTN_PIN_DATA, PIN_MODE_OUTPUT);
    rt_pin_mode(WTN_PIN_BUSY, PIN_MODE_INPUT);
    rt_pin_write(WTN_PIN_DATA, PIN_HIGH);

    tim = rt_device_find("timer2");
    rt_device_open(tim, RT_DEVICE_OFLAG_RDWR);

    auto mode = HWTIMER_MODE_ONESHOT;
    rt_device_control(tim, HWTIMER_CTRL_MODE_SET, &mode);

    event = rt_event_create(LOG_TAG, RT_IPC_FLAG_FIFO);
    writeMailbox = rt_mb_create(LOG_TAG, 32, RT_IPC_FLAG_FIFO);
    writeThread = rt_thread_create(LOG_TAG, writeEntry, this, 256, 3, 1);
    rt_thread_startup(writeThread);
}

void Wtn6::write(uint8_t data) {
    rt_mb_send(writeMailbox, data);
}

void Wtn6::operator << (uint8_t data) {
    write(data);
}

void Wtn6::operator << (VoiceFrg voice) {
    write((rt_uint8_t)voice);
}

bool Wtn6::isBuzy() {
    return rt_pin_read(WTN_PIN_BUSY) == PIN_LOW;
}

void Wtn6::writeEntry(void* p) {
    Wtn6* self = (Wtn6*)p;
    rt_ubase_t data;
    while(true) {
        rt_mb_recv(self->writeMailbox, &data, RT_WAITING_FOREVER);
        rt_pin_write(WTN_PIN_DATA, PIN_LOW);
        self->timeout_us(5000);
        rt_pin_write(WTN_PIN_DATA, PIN_HIGH);

        for(auto i = 0; i < 8; i++) {
            self->writeBit((data & (1 << i)) != 0);
        }

        rt_pin_write(WTN_PIN_DATA, PIN_HIGH);
    }
}

void Wtn6::writeBit(bool bit) {
    auto delayHigh = 200, delayLow = 600;
    if(bit) {
        delayHigh = 600;
        delayLow = 200;
    }

    rt_pin_write(WTN_PIN_DATA, PIN_HIGH);
    timeout_us(delayHigh);
    rt_pin_write(WTN_PIN_DATA, PIN_LOW);
    timeout_us(delayLow);
}

void Wtn6::timeout_us(int timeout) {
    auto to = rt_hwtimerval_t {
        sec: 0,
        usec: timeout,
    };

    rt_device_set_rx_indicate(tim, [](auto dev, auto size) -> rt_err_t {
        rt_event_send(self->event, (rt_uint32_t)events::timeout);
        return RT_EOK;
    });
    rt_device_write(tim, 0, &to, sizeof(to));
    rt_event_recv(event, (rt_uint32_t)events::timeout, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, RT_NULL);
}

Wtn6 wtn6;
Wtn6* Wtn6::self = &wtn6;

int init_wtn6() {
    wtn6.init();
    return RT_EOK;
}

INIT_APP_EXPORT(init_wtn6);
