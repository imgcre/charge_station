/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-06     imgcr       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <functional>

#define LOG_TAG "app.state"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#include <string.h>

#include <state.h>

using namespace std;

#define STATE_SERIAL "uart3"

static rt_device_t serial;
static rt_event_t event;
static volatile int rx_remain = 0;

enum state_event {
    state_event_serial_indicate = 1,
};

rt_timer_t lod_detect_timer;
LodDetect lodDetectA(PORTA_DETECT_PIN), lodDetectB(PORTB_DETECT_PIN);

static int init_state() {
    event = rt_event_create(LOG_TAG, RT_IPC_FLAG_FIFO);
    serial = rt_device_find(STATE_SERIAL);
    struct serial_configure conf = RT_SERIAL_CONFIG_DEFAULT;
    conf.data_bits = DATA_BITS_9;
    conf.baud_rate = BAUD_RATE_9600;
    conf.parity = PARITY_EVEN;
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &conf);
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(serial, [](auto dev, auto size) -> rt_err_t {
        rt_event_send(event, state_event_serial_indicate);
        rx_remain = size;
        return RT_EOK;
    });

    lodDetectA.init();
    lodDetectB.init();

    //创建定时器
    lod_detect_timer = rt_timer_create(LOG_TAG, [](auto p) {
        //50Hz的波形  //20ms的高电平
        lodDetectA.update();
        lodDetectB.update();
    }, RT_NULL, 10, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

    rt_timer_start(lod_detect_timer);

    return RT_EOK;
}

static rt_err_t serial_recv_wait(void* data, int len) {
    while(rx_remain < len) {
        if(rt_event_recv(event, state_event_serial_indicate, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 1000, RT_NULL) != RT_EOK) {
            return RT_ETIMEOUT;
        }
    }
    rt_base_t level = rt_hw_interrupt_disable();
    rx_remain -= len;
    rt_hw_interrupt_enable(level);
    rt_device_read(serial, 0, data, len);
    return RT_EOK;
}

void hlw_cmd(int cmd, void* data, int len) {
    char b = 0xa5, cs = 0;
   rt_device_write(serial, 0, &b, 1);
   cs += b;

   b = cmd;
   rt_device_write(serial, 0, &b, 1);
   cs += b;

   for(char* p = (char*)data + len - 1; p >= (char*)data; p--) {
       rt_device_write(serial, 0, p, 1);
       cs += *p;
   }

   cs = ~cs;
   rt_device_write(serial, 0, &cs, 1);
}

void hlw_spec_cmd(int cmd) {
    hlw_cmd(0xea, &cmd, 1);
}

void hlw_write_enable() { hlw_spec_cmd(0xe5); }
void hlw_write_disable() { hlw_spec_cmd(0xdc); }
void hlw_ch_a_sel() { hlw_spec_cmd(0x5a); }
void hlw_ch_b_sel() { hlw_spec_cmd(0xa5); }
void hlw_reset() { hlw_spec_cmd(0x96); }

void hlw_reg_write(int addr, void* data, int len) {
    hlw_cmd(addr | 0x80, data, len);
}

rt_err_t hlw_reg_read(int addr, void* data, int len) {
    char b = 0xa5, cs_expect = 0, cs;
    rt_device_write(serial, 0, &b, 1);
    cs_expect += b;

    b = addr;
    rt_device_write(serial, 0, &b, 1);
    cs_expect += b;

    for(char* p = (char*)data + len - 1; p >= (char*)data; p--) {
        if(serial_recv_wait(p, 1) != RT_EOK) {
            return RT_ETIMEOUT;
        }
        cs_expect += *p;
    }
    cs_expect = ~cs_expect;
    if(serial_recv_wait(&cs, 1) != RT_EOK) {
        return RT_ETIMEOUT;
    }
    if(cs != cs_expect) {
        LOG_E("exp: %x", cs_expect);
        return -RT_ERROR;
    }

    return RT_EOK;
}

template <class T>
rt_err_t hlw_reg_read(int addr, T& val) {
    auto err = hlw_reg_read(addr, &val, sizeof(T));
    return err;
}

template <class T>
void hlw_reg_write(int addr, T val) {
    hlw_reg_write(addr, &val, sizeof(val));
}


template <class T>
void hlw_reg_write(typename T::reg reg) {
    hlw_reg_write(T::addr, &reg, T::size);
}

template <class T>
struct hlw_session {
    hlw_session(): reg(hlw_reg_read<T>()) { }
    void commit() {
        if(T::writable) {
            hlw_reg_write<T>(reg);
        }
    };
    typename T::reg* operator->() { return &reg; }
    typename T::reg& operator*() { return reg; }
    ~hlw_session() { commit(); }

private:
    typename T::reg reg;
};


void state_hw_config() {
    hlw_reset();
    rt_thread_mdelay(1); //等待两个时钟周期
    hlw_write_enable();
    {
        hlw_session<emucon2> sess;
        sess->chs_ib = 1; //通道b选择测量电流
        sess->dup_sel = emucon2::DupSel::f3_4Hz; //设置均值更新频率
        sess->sdo_cmos = 0; //sdo脚cmos输出

        //sess->wave_en = 1; //TODO: delete
        //sess->peak_en = 1;
    }{
        hlw_session<emucon> sess;
        sess->hpf_i_a_off = 0; //关闭高通滤波器
        sess->hpf_i_b_off = 0;
        sess->hpf_u_off = 0;
        sess->comp_off = 1;
        //sess->dc_mode = 1;
    }  {
        hlw_session<pin> sess;
        sess->p1_sel = pin::PSel::IRQ; //TODO: 可能是均值更新中断输出
    } {//当中断产生时IRQ_N输出低电平
        hlw_session<ie> sess;
        sess->dupd = 1; //开启均值数据更新中断
    } {
        hlw_session<syscon> sess;
        sess->adc1_on = 1; //开启电流通道A
        sess->pga_i_a = 4;  //模拟增益=16
        sess->adc2_on = 1;
        sess->pga_i_b = 4;
        sess->adc3_on = 1;
        sess->pga_u = 0;
    }
    hlw_write_disable();
}

void Hlw::config() {
    state_hw_config();
}

float Hlw::getU(rt_err_t* err) {
    auto val = hlw_reg_read<rms_u>(err);
    if(*err != RT_EOK) return 0;
    auto rmsUC = hlw_reg_read<rms_u_c>(err);
    if(*err != RT_EOK) return 0;
    float rmsU = 1.0 * val.data * rmsUC / (1.0 * (1 << 22)) / 100; //单位10mV
    if(err) *err = RT_EOK;
    return rmsU;
}

Hlw hlw;

INIT_APP_EXPORT(init_state);
