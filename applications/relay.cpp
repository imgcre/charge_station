/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-13     imgcr       the first version
 */

#include <rtthread.h>
#include <relay.h>
#include <rtdevice.h>

#define PIN_RELAY_1 18
#define PIN_RELAY_2 23


int relay_init() {
    rt_pin_mode(PIN_RELAY_1, PIN_MODE_OUTPUT);
    rt_pin_mode(PIN_RELAY_2, PIN_MODE_OUTPUT);

    rt_pin_write(PIN_RELAY_1, PIN_LOW);
    rt_pin_write(PIN_RELAY_2, PIN_LOW);
}

void relay_ctl(Relay relay, rt_base_t val) {
    switch(relay) {
        case Relay::First:
            rt_pin_write(PIN_RELAY_1, val);
            break;
        case Relay::Second:
            rt_pin_write(PIN_RELAY_2, val);
            break;
    }
}

INIT_BOARD_EXPORT(relay_init);

