/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-04     imgcr       the first version
 */
#include <rtthread.h>

#define LOG_TAG "conf"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

static int init_conf() {

    return RT_EOK;
}

INIT_APP_EXPORT(init_conf);
