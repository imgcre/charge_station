/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-14     imgcr       the first version
 */
#ifndef APPLICATIONS_LIGHT_H_
#define APPLICATIONS_LIGHT_H_

#include <rtthread.h>
#include <rtdevice.h>

#define LIGHT1_R_PIN 15
#define LIGHT1_G_PIN 12
#define LIGHT1_B_PIN 11

#define LIGHT2_R_PIN 21
#define LIGHT2_G_PIN 20
#define LIGHT2_B_PIN 19

struct Light {

    Light(rt_base_t rPin, rt_base_t gPin, rt_base_t bPin): rPin(rPin), gPin(gPin), bPin(bPin) { }

    enum class State {
        LoadButNotPay,
        LoadAndPaid,
        Charged,
        LoadNotReady,
        Error,
    };

    void init() {
        rt_pin_mode(rPin, PIN_MODE_OUTPUT);
        rt_pin_mode(bPin, PIN_MODE_OUTPUT);
        rt_pin_mode(gPin, PIN_MODE_OUTPUT);

        timer = rt_timer_create("LT", [](auto p){
            auto self = (Light*)p;
            self->period++;
            switch(self->state) {
                case State::LoadButNotPay:
                    rt_pin_write(self->gPin, PIN_LOW);
                    rt_pin_write(self->bPin, PIN_LOW);

                    if(self->period % 2) {
                        rt_pin_write(self->rPin, PIN_HIGH);
                    } else {
                        rt_pin_write(self->rPin, PIN_LOW);
                    }
                    break;
                case State::LoadAndPaid:
                    rt_pin_write(self->gPin, PIN_LOW);
                    rt_pin_write(self->bPin, PIN_LOW);
                    rt_pin_write(self->bPin, PIN_HIGH);
                    break;
                case State::Charged:
                    rt_pin_write(self->rPin, PIN_LOW);
                    rt_pin_write(self->bPin, PIN_LOW);

                    if(self->period % 2) {
                        rt_pin_write(self->gPin, PIN_HIGH);
                    } else {
                        rt_pin_write(self->gPin, PIN_LOW);
                    }
                    break;
                case State::LoadNotReady:
                    rt_pin_write(self->rPin, PIN_LOW);
                    rt_pin_write(self->bPin, PIN_LOW);
                    rt_pin_write(self->gPin, PIN_HIGH);
                    break;
                case State::Error:
                    rt_pin_write(self->bPin, PIN_LOW);
                    if(self->period % 2) {
                        rt_pin_write(self->rPin, PIN_HIGH);
                        rt_pin_write(self->gPin, PIN_LOW);
                    } else {
                        rt_pin_write(self->rPin, PIN_LOW);
                        rt_pin_write(self->gPin, PIN_HIGH);
                    }
                    break;
            }

        }, this, 100, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
        rt_timer_start(timer);
    }

    void setState(State state) {
        this->state = state;
    }

    State state = State::LoadNotReady;
    rt_timer_t timer;
    int period = 0;
    rt_base_t rPin, gPin, bPin;
};

extern Light light1, light2;


#endif /* APPLICATIONS_LIGHT_H_ */
