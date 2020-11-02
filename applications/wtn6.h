/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-13     imgcr       the first version
 */
#ifndef APPLICATIONS_WTN6_H_
#define APPLICATIONS_WTN6_H_

enum class VoiceFrg: rt_uint8_t {
    Slience, //20ms静音
    PortAPluged, //一号插座已插入
    PortBPluged, //二号插座已插入
    NoPay, //请扫码或刷卡充电
    CardDetected, //刷卡成功
    StartCharing, //开始充电
    NotAvailable, //当前设备不可用，请更换设备尝试
    PortAUnpluged, //一号插座已拔出
    PortBUnpluged,
    PlugNotReady, //请先插入充电器
    BalanceNotEnough, //当前卡余额不足，请充值
    ChargeCompleted, //充电已完成
    Plugout, //请拔出插头
    Reserved1,
    Reserved2,
    Reserved3,
};

struct Wtn6 {
    void init();

    void write(uint8_t data);

    void operator << (uint8_t data);

    void operator << (VoiceFrg voice);

    bool isBuzy();

private:
    static void writeEntry(void* p);

    void writeBit(bool bit);

    void timeout_us(int timeout);

    enum class events {
        timeout = 1,
    };

    rt_device_t tim;
    rt_event_t event;
    static Wtn6* self;
    rt_thread_t writeThread;
    rt_mailbox_t writeMailbox;
};

extern Wtn6 wtn6;


#endif /* APPLICATIONS_WTN6_H_ */
