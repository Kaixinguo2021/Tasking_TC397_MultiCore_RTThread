/*
************************************************************************************************************************
* File    : cpu_port.h
* By      : xyou
* Version : V1.00.00
************************************************************************************************************************
*/



#ifndef _CPU_PORT_H_
#define _CPU_PORT_H_

#include "rtdef.h"
#include "IfxCpu.h"
#include "IfxCpu_reg.h"
#include "Bsp.h"
#include "Stm/Timer/IfxStm_Timer.h"
#include <rthw.h>
#include <rtthread.h>
/*
*********************************************************************************************************
*                                             CPU INTERRUPT PRIORITY
*********************************************************************************************************
*/
#define CPU_INTERRUPT_YIELD         0x01 // should be set to the lowest priority.
#define CPU_INTERRUPT_TICK          0x00



/*
*********************************************************************************************************
*                                             FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void rt_hw_board_init(void);
#ifdef RT_USING_SMP
#else
rt_base_t rt_hw_interrupt_disable(void);
void rt_hw_interrupt_enable(rt_base_t level);
#endif
rt_uint8_t *rt_hw_stack_init(void *tentry,void *parameter,rt_uint8_t *stack_addr,void *texit);
void rt_hw_systick_init(void);
#endif /* _CPU_PORT_H_ */
