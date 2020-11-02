/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-07     imgcr       the first version
 */
#ifndef APPLICATIONS_STATE_H_
#define APPLICATIONS_STATE_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>
#include <type_traits>
#include <functional>
#include <tuple>

#define DETECT_QUEUE_SIZE 10
#define PORTA_DETECT_PIN 16
#define PORTB_DETECT_PIN 17

void state_hw_config();

struct LodDetect {

    LodDetect(rt_base_t pin): pin(pin), tick(-2000), state(false) { }

    void init() {
        rt_pin_mode(pin, PIN_MODE_INPUT_PULLDOWN);

        rt_pin_attach_irq(pin, PIN_IRQ_MODE_RISING, [](auto p) {
            auto self = (LodDetect*)p;
            self->tick = rt_tick_get();

        }, this);
        rt_pin_irq_enable(pin, PIN_IRQ_ENABLE);
    }

    //以10Hz频率调用
    void update() {
        bool curState = (rt_tick_get() - tick) < 100;
        if(curState != state) {
            state = curState;
            if(onStateChangedCb) {
                onStateChangedCb(state);
            }
        }
    }

    void onStateChanged(std::function<void(bool state)> cb) {
        onStateChangedCb = cb;
    }

    bool isInserted() {
        return state;
    }

    std::function<void(bool state)> onStateChangedCb;
private:
    rt_base_t pin;
    rt_int64_t tick;
    bool state;
};

extern LodDetect lodDetectA, lodDetectB;

template <class T, int Addr, int Size=0, bool Write=true>
struct reg_def {
    static const int addr = Addr;
    static const int size = Size != 0 ? Size : sizeof(T);
    static const bool writable = Write;
    using reg = T;
};

struct reg_syscon {
    rt_int16_t pga_i_a: 3;
    rt_int16_t pga_u: 3;
    rt_int16_t pga_i_b: 3;
    rt_int16_t adc1_on: 1;
    rt_int16_t adc2_on: 1;
    rt_int16_t adc3_on: 1;
};

using syscon = reg_def<reg_syscon, 0x00>;

struct reg_emucon {
    rt_int16_t pa_run: 1;
    rt_int16_t pb_run: 1;
    rt_int16_t reserved1: 2;
    rt_int16_t hpf_u_off: 1;
    rt_int16_t hpf_i_a_off: 1;
    rt_int16_t hpf_i_b_off: 1;
    rt_int16_t zxd0: 1;
    rt_int16_t zxd1: 1;
    rt_int16_t dc_mode: 1;
    rt_int16_t pmode: 2;
    rt_int16_t comp_off: 1;
    rt_int16_t tensor_en: 1;
    rt_int16_t tensor_step: 2;
};

//TODO: 修改成0
using emucon = reg_def<reg_emucon, 0x01>;

struct DupSel {
    enum Value {
        f3_4Hz,
        f6_8Hz,
        f13_65Hz,
        f27_3Hz,
    };
};

struct reg_emucon2 {
    rt_int16_t vref_sel: 1;
    rt_int16_t peak_en: 1;
    rt_int16_t zx_en: 1;
    rt_int16_t over_en: 1;
    rt_int16_t sag_en: 1;
    rt_int16_t wave_en: 1;
    rt_int16_t p_factor_en: 1;
    rt_int16_t chs_ib: 1;
    rt_int16_t dup_sel: 2;
    rt_int16_t epa_cb: 1;
    rt_int16_t epb_cb: 1;
    rt_int16_t sdo_cmos: 1;
    rt_int16_t reserved2: 2;
    rt_int16_t dotp_sel: 1;
};

//TODO: 修改成0
//using emucon2 = reg_def<reg_emucon2, 0x13>;

struct emucon2: public reg_def<reg_emucon2, 0x13> {
    struct DupSel {
        enum Value {
            f3_4Hz,
            f6_8Hz,
            f13_65Hz,
            f27_3Hz,
        };
    };
};

struct reg_pin {
    rt_int16_t p1_sel: 4;
    rt_int16_t p2_sel: 4;
};

struct pin: public reg_def<reg_pin, 0x1d> {
    struct PSel {
        enum Value {
            PfaOut,
            PfbOut,
            LeakageIndicate,
            IRQ, //发生中断则低电平
            POverloadIndicate,
            ChAMinusPIndicate,
            ChBMinusPIndicate,
            InstantValUpdateIRQ,
            AverageValUpdateIRQ,
            VoltageZeroCrossing,
            IAZeroCrossing,
            IBZeroCrossing,
            OverVoltageIndicate,
            UnderVoltageIndicate,
            ChAOverCurrentIndicate,
            ChBOverCurrentIndicate,
        };
    };
};

struct reg_rms_i_a {
    rt_uint32_t data: 24;
};

using rms_i_a = reg_def<reg_rms_i_a, 0x24, 3>;


struct reg_rms_i_b {
    rt_uint32_t data: 24;
};

using rms_i_b = reg_def<reg_rms_i_b, 0x25, 3>;


struct reg_rms_u {
    rt_uint32_t data: 24;
};

using rms_u = reg_def<reg_rms_u, 0x26, 3>;


struct reg_peak_i_a {
    rt_uint32_t data: 24;
};

using peak_i_a = reg_def<reg_peak_i_a, 0x30, 3>;


struct reg_peak_u {
    rt_uint32_t data: 24;
};
using peak_u = reg_def<reg_peak_u, 0x32, 3>;

using rms_i_a_c = reg_def<uint16_t, 0x70>;
using rms_i_b_c = reg_def<uint16_t, 0x71>;
using rms_u_c = reg_def<uint16_t, 0x72>;

struct reg_ie {
    rt_int16_t dupd: 1; //<- 均值数据更新
    rt_int16_t pfa: 1;
    rt_int16_t pfb: 1;
    rt_int16_t peao: 1;
    rt_int16_t pebo: 1;
    rt_int16_t reserved: 1;
    rt_int16_t instan: 1;
    rt_int16_t oia: 1;
    rt_int16_t oib: 1;
    rt_int16_t ov: 1;
    rt_int16_t op: 1;
    rt_int16_t sag: 1;
    rt_int16_t zx_ia: 1;
    rt_int16_t zx_ib: 1;
    rt_int16_t zx_u: 1;
    rt_int16_t leakage: 1;
};
using ie = reg_def<reg_ie, 0x40>;

struct reg_if { //只读寄存器, 读清零
    rt_int16_t dupd: 1; //<- 均值数据更新
    rt_int16_t pfa: 1;
    rt_int16_t pfb: 1;
    rt_int16_t peao: 1;
    rt_int16_t pebo: 1;
    rt_int16_t reserved: 1;
    rt_int16_t instan: 1;
    rt_int16_t oia: 1;
    rt_int16_t oib: 1;
    rt_int16_t ov: 1;
    rt_int16_t op: 1;
    rt_int16_t sag: 1;
    rt_int16_t zx_ia: 1;
    rt_int16_t zx_ib: 1;
    rt_int16_t zx_u: 1;
    rt_int16_t leakage: 1;
};
using ifr = reg_def<reg_if, 0x41, 0, false>;

struct reg_sys_status { //只读寄存器
    rt_int8_t rst: 1;
    rt_int8_t reserved1: 3;
    rt_int8_t wren: 1; //写使能
    rt_int8_t reserved2: 1;
    rt_int8_t clk_sel: 1;
};
struct sys_status: public reg_def<reg_sys_status, 0x43, 0, false> {
    struct ClkSel {
        enum Value {
            External,
            Internal,
        };
    };
};


rt_err_t hlw_reg_read(int addr, void* data, int len);

template <class T>
auto hlw_reg_read(rt_err_t* err = RT_NULL) {
    typename T::reg reg;
    rt_err_t local_err = hlw_reg_read(T::addr, &reg, T::size);
    if(err) {
        *err = local_err;
    }
    return reg;
}

struct Hlw {

    enum Port {
        A = 0, B,
    };

    void config();

    template <Port P>
    float getI(rt_err_t* err = RT_NULL) {
        using rms_mt = std::tuple_element_t<P, std::tuple<std::pair<rms_i_a, rms_i_a_c>, std::pair<rms_i_b, rms_i_b_c>>>;
        auto val = hlw_reg_read<typename rms_mt::first_type>();
        auto rmsIC = hlw_reg_read<typename rms_mt::second_type>();
        float rmsI = 1.0 * val.data * rmsIC / (1 << 23) / 1.7;
        if(err) *err = RT_EOK;
        return rmsI;
    }
    float getU(rt_err_t* err = RT_NULL);
};

extern Hlw hlw;


#endif /* APPLICATIONS_STATE_H_ */
