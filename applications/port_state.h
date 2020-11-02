/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-14     imgcr       the first version
 */
#ifndef APPLICATIONS_PORT_STATE_H_
#define APPLICATIONS_PORT_STATE_H_

#include <rtthread.h>
extern "C" {
#include <at24cxx.h>
}

#include <functional>

extern at24cxx_device_t at24_dev;

struct PortState {
    enum Value {
        LoadNotInsert = 1,
        LoadInserted,
        Charging,
        ChargedButLoadNotRemove,
        Error,
    };

    PortState(int portNum): portNum(portNum) { }

    void init();

    Value get() {
        if(_loadInserted) {
            if(timerId > 0) {
                if(charging) {
                    if(leftSeconds > 0) {
                        return Charging;
                    } else {
                        return ChargedButLoadNotRemove;
                    }
                } else {
                    return ChargedButLoadNotRemove;
                }
            } else {
                return LoadInserted;
            }
        } else {
            return LoadNotInsert;
        }
    }

    void loadInserted() {
        _loadInserted = true;
        lastInsertTick = rt_tick_get();
    }

    void loadRemoved() {
        _loadInserted = false;
        timerId = 0;
    }

    bool isLoadInserted() {
        return _loadInserted;
    }

    rt_tick_t loadLastInsertTick() {
        return lastInsertTick;
    }

    void startCharging(int minutes, int timerId) {
        if(minutes < 1)
            minutes = 1;

        if(timerId < 1)
            timerId = 1;

        this->leftSeconds = minutes * 60;
        this->timerId = timerId;
        charging = true;
        save();
    }

    void stopCharging(int timerId) {
        this->timerId = timerId;
        //this->timerId = 0;
        charging = false;
        leftSeconds = 0;
        save();
    }


    int getTimerId() {
        return timerId > 0 ? timerId : 1;
    }

    int getPort() {
        return portNum;
    }

    int getLeftMinutes() {
        return leftSeconds > 0 ? (leftSeconds / 60) : 1;
    }

    bool isCharging() {
        return charging;
    }

    void error() {

    }

    void onInternalChargeOver(std::function<void()> cb) {
        onInternalChargeOverCb = cb;
    }

    struct Serialized {
        int timerId;
        int leftSeconds;
        bool charging;
    };

    void save();

    void resume();

    void onResumePortOpenRequired(std::function<bool()> cb) {
        onResumePortOpenRequiredCb = cb;
    }

private:
    int timerId = 0;
    int portNum;
    int leftSeconds = 0;
    bool _loadInserted = false;
    bool charging = false;
    int saveTickCnt = 0;
    rt_tick_t lastInsertTick = 0;
    std::function<void()> onInternalChargeOverCb;
    rt_timer_t timer;
    std::function<bool()> onResumePortOpenRequiredCb;
};



#endif /* APPLICATIONS_PORT_STATE_H_ */
