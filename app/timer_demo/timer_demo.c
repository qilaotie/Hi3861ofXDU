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
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_watchdog.h"

#define STACK_SIZE          1024
#define COUNT_STOP          5
#define TIMER_COUNT_NUM     100
#define FEED_DELAY          10

static int count = 0;   // 定义一个count变量，用于控制投喂的次数
                        // Define a count variable to control the feeding times
static int FeedTime[6]={0,5,7,9,11,5}; // 定义一个数组，用于存储每次投喂持续的时间
                        // Define an array to store the time interval for each feed

struct Timer
{
    osTimerId_t id;
    osStatus_t status;
}; // 定义一个定时器结构体，用于存储定时器的ID和状态
        // Define a timer structure to store the timer ID and status

void FeedFoodEnd(struct Timer *timer)
{
    printf("[Timer_demo] FeedFoodEnd, stop feeding.\r\n");

    timer->status = osTimerDelete(timer->id);
    printf("[Timer_demo] Timer Delete, status :%d. \r\n", timer->status);

    free(timer);
}


void FeedSomeFood(void)
{
    struct Timer *timer = malloc(sizeof(struct Timer));
    if (timer == NULL) {
        printf("[Timer_demo] malloc failed!\r\n");
        return;
    }

    count++;
    printf("[Timer_demo] Start feeding count is %d \r\n", count);

    timer->id = osTimerNew((osTimerFunc_t)FeedFoodEnd, osTimerOnce, (void *)timer, NULL);
    if (timer->id == NULL) {
        printf("[Timer_demo] osTimerNew failed.\r\n");
        free(timer);
        return;
    }

    timer->status = osTimerStart(timer->id, FeedTime[count]*TIMER_COUNT_NUM);
    if (timer->status != osOK) {
        printf("[Timer_demo] osTimerStart failed.\r\n");
        osTimerDelete(timer->id);
        free(timer);
        return;
    }

}


void TimerThread(const char *arg)
{
    (void)arg;
    osTimerId_t id;
    osStatus_t status;

    // 创建一个周期性的定时器,回调函数是FeedSomeFood，用于宠物喂食机的投喂
    // Create a periodic timer. The callback function is FeedSomeFood, which is used for feeding the pet feeder
    id = osTimerNew((osTimerFunc_t)FeedSomeFood, osTimerPeriodic, NULL, NULL);
    if (id == NULL) {
        printf("[Timer_demo] osTimerNew failed.\r\n");
    } else {
        printf("[Timer_demo] osTimerNew success.\r\n");
    }
    
    // 开始计时2000个时钟周期,一个时钟周期是10ms，2000个时钟周期就是20s
    // Start timing 2000 clock cycles, one clock cycle is 10ms, 2000 clock cycles is 20s
    status = osTimerStart(id, TIMER_COUNT_NUM*20);
    if (status != osOK) {
        printf("[Timer_demo] osTimerStart failed.\r\n");
    } else {
        printf("[Timer_demo] osTimerStart success.\r\n");
    }

    // 投喂5次后，停止投喂
    // Stop feeding after feeding for 5 times
    while (count < COUNT_STOP) {
        osDelay(FEED_DELAY*20);
    }

    // 停止定时器
    // Stop Timer
    status = osTimerStop(id);
    printf("[Timer_demo] Timer Stop, status :%d. \r\n", status);

    // 删除定时器
    // Delete Timer
    status = osTimerDelete(id);
    printf("[Timer_demo] Timer Delete, status :%d. \r\n", status);
}

void TimerExampleEntry(void)
{
    osThreadAttr_t attr;

    IoTWatchDogDisable();

    attr.name = "TimerThread";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = STACK_SIZE;
    attr.priority = osPriorityNormal;

    // 创建一个线程，并注册一个回调函数 TimerThread，控制红色LED灯每隔1秒钟闪烁一次
    // Create a thread, register a callback function TimerThread, and control the red LED to flash once every 1 second
    if (osThreadNew((osThreadFunc_t)TimerThread, NULL, &attr) == NULL) {
        printf("[Timer_demo] osThreadNew Falied to create TimerThread!\n");
    }
}


APP_FEATURE_INIT(TimerExampleEntry);
