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
#include "hi_timer.h"

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "iot_errno.h"
#include "hi_errno.h"
#include "pca9555.h"

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

#define LRED_LED 0x08
#define LGREEN_LED 0x10
#define LBLUE_LED 0x20
#define LWHITE_LED 0x38
#define RRED_LED 0x01
#define RGREEN_LED 0x02
#define RBLUE_LED 0x04
#define RWHITE_LED 0x07
#define DWHITE_LED 0x38+0x07
#define DRED_LED 0x08+0x01
#define DGREEN_LED 0x10+0x02
#define DBLUE_LED 0x20+0x04
#define DELAY_US 20
static volatile int g_buttonState = 0;

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

void GetFunKeyState(void)
{
    uint8_t ext_io_state = 0;
    uint8_t ext_io_state_d = 0x3C;
    uint8_t status;

    while (1) {
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
                printf("button1 pressed,\r\n");
                SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, RED_LED);
            } else if ((diff & 0x08) && ((ext_io_state & 0x08) == 0)) {
                printf("button2 pressed \r\n");
                SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, LGREEN_LED+RBLUE_LED);
            } else if ((diff & 0x10) && ((ext_io_state & 0x10) == 0)) {
                printf("button3 pressed \r\n");
                SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, LBLUE_LED+RGREEN_LED);
            }
            status = PCA9555I2CReadByte(&ext_io_state);
            ext_io_state_d = ext_io_state;
            g_buttonState = 0;
        }
        usleep(DELAY_US);
    }
}

static void ButtonControl(void)
{
    printf("ButtonControl\r\n");
    /*
     * IO扩展芯片初始化
     * IO expansion chip initialization
     */
    PCA9555Init();
    /*
     * 配置IO扩展芯片的part1的所有管脚为输出
     * Configure all pins of part1 of IO expansion chip as output
     */
    SetPCA9555GpioValue(PCA9555_PART1_IODIR, PCA9555_OUTPUT);
    /*
     * 配置左右三色车灯全灭
     * Configured with left and right tricolor lights all off
     */
    SetPCA9555GpioValue(PCA9555_PART1_OUTPUT, LED_OFF);
    /*
     * 按键中断初始化
     * Key interrupt initialization
     */
    FuncKeyInit();
    /*
     * 获取实时的按键状态
     * Get real-time key status
     */
    GetFunKeyState();
}

static void ButtonControlEntry(void)
{
    osThreadAttr_t attr;
    attr.name = "LedCntrolDemo";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024; // 堆栈大小为1024,Stack size is 1024
    attr.priority = osPriorityNormal;
    if (osThreadNew((osThreadFunc_t)ButtonControl, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create LedTask!\n");
    }
}

APP_FEATURE_INIT(ButtonControlEntry);