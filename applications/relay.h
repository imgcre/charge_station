/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-13     imgcr       the first version
 */
#ifndef APPLICATIONS_RELAY_H_
#define APPLICATIONS_RELAY_H_


enum class Relay {
    First,
    Second,
};

void relay_ctl(Relay relay, rt_base_t val);

#endif /* APPLICATIONS_RELAY_H_ */
