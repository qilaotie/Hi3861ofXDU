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
#include <stdint.h>
#include <string.h>
#include "hi_timer.h"

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "iot_i2c.h"
#include "iot_uart.h"
#include "hi_uart.h"
#include "iot_watchdog.h"
#include "iot_errno.h"
#include "hi_errno.h"
#include "hi_i2c.h"
#include "oled_ssd1306.h"
#include "pca9555.h"

#define SIDE_WOLK_DELAY         5   // 人行道持续工作5秒
// The sidewalk works continuously for 5 seconds
#define MOTOR_WAY_DELAY         10  // 机动车道持续工作10秒
                                    // The motorway works continuously for 10 seconds
#define NUM                     1
#define STACK_SIZE              1024

static volatile int g_buttonState = 0;

osSemaphoreId_t traffic_light;      // 交通灯信号量
                                    // Traffic light signal volume

void OnFuncKeyPressed(char *arg)
{
    (void) arg;
    g_buttonState = 1;
}

void FuncKeyInit(void)
{
    /*
     * 使能GPIO11的中断功能, OnFuncKeyPressed 为中断的回调函数
     * Enable the interrupt function of GPIO11. OnFuncKeyPressed is the interrupt callback function
     */
    IoTGpioRegisterIsrFunc(IOT_IO_NAME_GPIO_11, IOT_INT_TYPE_EDGE,
                           IOT_GPIO_EDGE_FALL_LEVEL_LOW, OnFuncKeyPressed, NULL);
    /*
     * S3:IO0_2,S4:IO0_3,S5:IO0_4 0001 1100 => 0x1c 将IO0_2,IO0_3,IO0_4方向设置为输入，1为输入，0位输出
     * S3:IO0_ 2,S4:IO0_ 3,S5:IO0_ 4 0001 1100=>0x1c Change IO0_ 2,IO0_ 3,IO0_ 4 direction is set as
     * input, 1 is input, and 0 bit is output
     */
    SetPCA9555GpioValue(PCA9555_PART0_IODIR, 0x1c);
}

void LED_GreenOn(void)
{
    // 设置GPIO10输出高电平点亮绿色交通灯LED3
    // Set GPIO10 output high level to turn on green traffic light LED 3
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_10, IOT_GPIO_VALUE1);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_9, IOT_GPIO_VALUE0);
}

void LED_RedOn(void)
{
    // 设置GPIO9输出高电平点亮红色交通灯LED3
    // Set GPIO9 output high level to turn on red traffic light LED 3
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_9, IOT_GPIO_VALUE1);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_10, IOT_GPIO_VALUE0);
}

// 人行道线程
// Sidewalk thread
void SideWalkThread(const char *arg)
{
    unsigned int cout;
    (void)arg;

    /*
     * IO扩展芯片初始化
     * IO expansion chip initialization
     */
    PCA9555Init();
    IoTGpioInit(IOT_IO_NAME_GPIO_9);
    IoTGpioInit(IOT_IO_NAME_GPIO_10);
    IoTGpioInit(IOT_IO_NAME_GPIO_5);
    IoSetFunc(IOT_IO_NAME_GPIO_9, IOT_IO_FUNC_GPIO_9_GPIO);
    IoSetFunc(IOT_IO_NAME_GPIO_10, IOT_IO_FUNC_GPIO_10_GPIO);
    IoSetFunc(IOT_IO_NAME_GPIO_5, IOT_IO_FUNC_GPIO_5_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_9, IOT_GPIO_DIR_OUT);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_10, IOT_GPIO_DIR_OUT);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_5, IOT_GPIO_DIR_OUT);
    /*
     * 按键中断初始化
     * Key interrupt initialization
     */
    FuncKeyInit();
    uint8_t ext_io_state = 0;
    uint8_t ext_io_state_d = 0x3C;
    uint8_t status;
    int space = 0;
    static char line[32] = {0};
    OledInit();
    OledFillScreen(0);
    while (NUM) {
        space=osSemaphoreGetCount(traffic_light);
        if (g_buttonState == 1) {
            uint8_t diff;
            status = PCA9555I2CReadByte(&ext_io_state);
            if (status != IOT_SUCCESS) {
                printf("i2c error!\r\n");
                ext_io_state = 0;
                ext_io_state_d = 0x3C;
                g_buttonState = 0;
                continue;
            }
            diff = ext_io_state ^ ext_io_state_d;
            // printf("diff = %0X\r\n", diff);
            // printf("ext_io_state = %0X\r\n", ext_io_state);
            // printf("ext_io_state_d = %0X\r\n", ext_io_state_d);
            if (diff == 0) {
                printf("diff = 0! state:%0X, %0X\r\n", ext_io_state, ext_io_state_d);
            }
            if ((diff & 0x04) && ((ext_io_state & 0x04) == 0)) {
                printf("button3 pressed,\r\n");
                while (osSemaphoreGetCount(traffic_light) < 5) {
                    // 等待信号量的令牌
                    // Wait for the semaphore token
                    printf("%d\r\n",osSemaphoreRelease(traffic_light));
                    LED_GreenOn();
                    space++;
                    OledFillScreen(0);
                }
            } else if ((diff & 0x08) && ((ext_io_state & 0x08) == 0)) {
                printf("button2 pressed \r\n");
                LED_GreenOn();
                OledFillScreen(0);
                if(space < 5)
                // 释放信号量
                // Release traffic light signal
                printf("%d\r\n",osSemaphoreRelease(traffic_light));
            } else if ((diff & 0x10) && ((ext_io_state & 0x10) == 0)) {
                printf("button1 pressed \r\n");
                printf("%d",space);
                if(space > 0) {
                    // 信号量令牌
                    // Obtain traffic light semaphore token
                    osSemaphoreAcquire(traffic_light, osWaitForever);
                    LED_GreenOn();
                } else {
                    LED_RedOn();
                    printf("No parking space\r\n");
                    OledShowString(1, 3, "No parking space", 1);
                }
            }
            status = PCA9555I2CReadByte(&ext_io_state);
            ext_io_state_d = ext_io_state;
            g_buttonState = 0;
        }
        snprintf(line, sizeof(line), "init: %d pre: %d", 5, space);
        OledShowString(1, 1, line, 1);
        osDelay(2);
    }
}

static void SemaphoreDemoEntry(void)
{
    osThreadAttr_t attr;

    attr.name = "SideWalkThread";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = STACK_SIZE;
    attr.priority = osPriorityNormal;

    // 创建一个线程，回调函数是SideWalkThread，用来执行人行道的工作
    // Create a thread. The callback function is SideWalkThread, which is used to execute the work of the sidewalk
    if (osThreadNew((osThreadFunc_t)SideWalkThread, NULL, &attr) == NULL) {
        printf("[SideWalkThread] Failed to create SideWalkThread!\n");
    }

    // 创建一个交通灯的信号量，可用令牌的最大数量为5，可用令牌的初始化数量为0
    // Create a traffic light semaphore. The maximum number of available tokens is 1,
    // and the initialization number of available tokens is 0
    traffic_light = osSemaphoreNew(5, 0, NULL);
    if (traffic_light == NULL) {
        printf("Falied to create Semaphore!\n");
    }

    // 释放交通灯信号量
    // Release traffic light signal
    osSemaphoreRelease(traffic_light);
    osSemaphoreRelease(traffic_light);
    osSemaphoreRelease(traffic_light);
    osSemaphoreRelease(traffic_light);
    osSemaphoreRelease(traffic_light);
}

APP_FEATURE_INIT(SemaphoreDemoEntry);