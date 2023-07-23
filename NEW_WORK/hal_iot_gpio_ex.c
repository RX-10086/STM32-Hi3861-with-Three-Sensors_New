/* Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED */

#include "iot_errno.h"
#include "iot_gpio_ex.h"
#include "hi_gpio.h"
#include "hi_io.h"
#include "hi_task.h"
#include "hi_types_base.h"

unsigned int IoSetPull(unsigned int id, IotIoPull val)
{
    if (id >= HI_GPIO_IDX_MAX)
    {
        return IOT_FAILURE;
    }
    return hi_io_set_pull((hi_io_name)id, (hi_io_pull)val);
}

unsigned int IoSetFunc(unsigned int id, unsigned char val)
{
    if (id >= HI_GPIO_IDX_MAX)
    {
        return IOT_FAILURE;
    }
    return hi_io_set_func((hi_io_name)id, val);
}

unsigned int TaskMsleep(unsigned int ms)
{
    if (ms <= 0)
    {
        return IOT_FAILURE;
    }
    return hi_sleep((hi_u32)ms);
}