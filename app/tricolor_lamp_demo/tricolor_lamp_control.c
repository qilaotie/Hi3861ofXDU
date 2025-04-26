/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "pca9555.h"

#define DWHITE_LED 0x38+0x07
#define DRED_LED 0x08+0x01
#define DGREEN_LED 0x10+0x02
#define DBLUE_LED 0x20+0x04

static void TraColorLampControl(void)
{
    PCA9555Init();
    SetPCA9555GpioValue(PCA9555_PART1_IODIR, PCA9555_OUTPUT);
    SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, LED_OFF);

    /*
     * 控制左三色车灯跑马灯，红、绿、蓝、白：每隔一秒一次亮
     * 红灯：IO1_3 ==> 0000 1000 ==> 0x08
     * 绿灯：IO1_4 ==> 0001 0000 ==> 0x10
     * 蓝灯：IO1_5 ==>  0010 0000 ==> 0x20
     * 白灯：三灯全亮 ==>  0011 1000 ==> 0x38
     * Control the left tricolor running lights, green, blue, red and white: light up once every second
     * Green light: IO1_ 3 ==> 0000 1000 ==> 0x08
     * Blue light: IO1_4 ==> 0001 0000 ==> 0x10
     * Red light: IO1_5 ==>  0010 0000 ==> 0x20
     * White light: all three lights are on==>0011 1000==>0x38
     */
    /*
     * 控制右三色车灯跑马灯，红、绿、蓝、白
     * 红灯：IO1_0 ==> 0000 0001 ==> 0x01
     * 绿灯：IO1_1 ==> 0000 0010 ==> 0x02
     * 蓝灯：IO1_2 ==> 0000 0100 ==> 0x04
     * 白灯：三灯全亮 ==> 0000 0111 ==> 0x07
     * Control the right tricolor running lights, green, blue, red and white
     * Green light: IO1_ 0 ==> 0000 0001 ==> 0x01
     * Blue light: IO1_ 1 ==> 0000 0010 ==> 0x02
     * Red light: IO1_ 2 ==> 0000 0100 ==> 0x04
     * White light: all three lights are on==>0000 0111==>0x07
     */
    // 左右车灯同时按照白灯2秒、蓝灯1秒、红灯2秒、绿灯1秒
    while (1) {
        SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, DWHITE_LED);
        TaskMsleep(DELAY_MS);
        TaskMsleep(DELAY_MS);
        SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, DBLUE_LED);
        TaskMsleep(DELAY_MS);
        SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, DRED_LED);
        TaskMsleep(DELAY_MS);
        TaskMsleep(DELAY_MS);
        SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, DGREEN_LED);
        TaskMsleep(DELAY_MS);
    }
}

static void TraColorLampControlEntry(void)
{
    osThreadAttr_t attr;
    attr.name = "LedCntrolDemo";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024; /* 堆栈大小为1024 stack size 1024 */
    attr.priority = osPriorityNormal;
    if (osThreadNew((osThreadFunc_t)TraColorLampControl, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create LedTask!\n");
    }
}

APP_FEATURE_INIT(TraColorLampControlEntry);