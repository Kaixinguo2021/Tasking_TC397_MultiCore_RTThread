/**********************************************************************************************************************
 * \file Cpu0_Main.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of 
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and 
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all 
 * derivative works of the Software, unless such copies or derivative works are solely in the form of 
 * machine-executable object code generated by a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *********************************************************************************************************************/
 /*\title Multicore LED control
 * \abstract One LED is controlled by using three different cores.
 * \description Core 0 is switching on an LED. When the LED flag is set, Core 1 is switching off the LED.
 *              Core 2 is controlling the state of the LED flag.
 *
 * \name Multicore_1_KIT_TC397_TFT
 * \version V1.0.1
 * \board APPLICATION KIT TC3X7 V2.0, KIT_A2G_TC397_5V_TFT, TC39xXX_B-Step
 * \keywords multicore, LED, AURIX, Multicore_1
 * \documents https://www.infineon.com/aurix-expert-training/Infineon-AURIX_Multicore_1_KIT_TC397_TFT-TR-v01_00_01-EN.pdf
 * \documents https://www.infineon.com/aurix-expert-training/TC39B_iLLD_UM_1_0_1_12_1.chm
 * \lastUpdated 2020-12-18
 *********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"
#include "Multicore.h"
#include "rtthread.h"
#include "cpuport.h"
#include "UART_Logging.h"

IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent = 0;

void core0_main(void)
{
    IfxCpu_enableInterrupts();
    
    /* !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdogs and service them periodically if it is required
     */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());
	get_clk();
    
	rtthread_startup();
    sendUARTMessage("core0_main\r\n",sizeof("core0_main")+2);
    while(1)
    {

    }
}

rt_uint32_t Led_CNT = 0x00;
static void led_thread_entry(void *parameter)
{
    while(1)
    {
        sendUARTMessage("led_thread_entry\r\n",sizeof("led_thread_entry")+2);
        Led_CNT++; 
        rt_thread_mdelay(30);
    }
}

//add by song
static rt_uint8_t led_thread_stack[512];
static struct rt_thread led_thread_thread;

uint32 Main_CNT = 0x00;
extern struct rt_thread main_thread;
extern rt_uint8_t Flag_10ms;
int main(void)
{
    #define EVENT_FLAG3 (1 << 3)
    #define EVENT_FLAG5 (1 << 5)

    /* Wait for CPU sync event: wait until all CPUs are in CpuX_Main to avoid variables' initialization problems */
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    rt_thread_init(&led_thread_thread, "led", led_thread_entry, RT_NULL,\
    &led_thread_stack[0], sizeof(led_thread_stack), RT_MAIN_THREAD_PRIORITY+1, 20);
    
    rt_thread_startup(&led_thread_thread);

    for(;;)
    {
        sendUARTMessage("int main(void)\r\n",sizeof("int main(void)")+2);
        Main_CNT++;
        rt_thread_mdelay(50);
    }
}