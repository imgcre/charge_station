/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-14     imgcr       the first version
 */

#include "light.h"
#include <stm32f1xx_hal.h>

Light light1(LIGHT1_R_PIN, LIGHT1_G_PIN, LIGHT1_B_PIN), light2(LIGHT2_R_PIN, LIGHT2_G_PIN, LIGHT2_B_PIN);

//JTAG模式设置,用于设置JTAG的模式
//mode:jtag,swd模式设置;00,全使能;01,使能SWD;10,全关闭;
//CHECK OK
//100818
void JTAG_Set(rt_uint8_t mode)
{
    rt_uint32_t temp;
    temp=mode;
    temp<<=25;
    RCC->APB2ENR|=1<<0;     //开启辅助时钟
    AFIO->MAPR&=0XF8FFFFFF; //清除MAPR的[26:24]
    AFIO->MAPR|=temp;       //设置jtag模式
}


int init_light() {
    JTAG_Set(0b01);
    light1.init();
    light2.init();
    return RT_EOK;
}

INIT_APP_EXPORT(init_light);
