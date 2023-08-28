/**********************************************************************************************************************
 * \file Multicore.c
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

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Multicore.h"
#include "Ifx_Types.h"
#include "IfxPort.h"
#include "Bsp.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define LED         &MODULE_P13, 0  /* LED D107                                                                     */
#define WAIT_TIME   1000            /* Number of milliseconds to wait between each duty cycle change                */

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
uint16 g_turnLEDon = FALSE;     /* Variable for the LED, CPU0 and CPU1 are toggling the LED depending on its state  */
Ifx_TickTime g_ticksFor1s;      /* Variable to store the number of ticks to wait for 1 second delay                 */

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* Function to initialize the LED port pin and the time constants */
void initLEDAndTime(void)
{
    /* Turn off the LED (LED is low-level active) */
    IfxPort_setPinHigh(LED);
    /* Set the port pin mode to output push-pull */
    IfxPort_setPinMode(LED, IfxPort_Mode_outputPushPullGeneral);

    /* Initialize the time variable */
    g_ticksFor1s = IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, WAIT_TIME);
}

/* Function to turn on the LED accordingly to the state of g_turnLEDon */
void turnLEDon(void)
{
    if(g_turnLEDon == TRUE)
    {
        IfxPort_setPinLow(LED);     /* Turn on the LED (LED is low-level active)    */
    }
}

/* Function to turn off the LED accordingly to the state of g_turnLEDon */
void turnLEDoff(void)
{
    if(g_turnLEDon == FALSE)
    {
        IfxPort_setPinHigh(LED);    /* Turn off the LED (LED is low-level active)   */
    }
}

/* Function to toggle the state of g_turnLEDon */
void controlLEDflag(void)
{
    g_turnLEDon = !g_turnLEDon;     /* Toggle the state of the global variable      */
    wait(g_ticksFor1s);             /* Wait for approximately 1 second              */
}