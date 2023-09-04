/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *rt_interrupt_from_thread_core0
 * SPDX-License-Identifier: GPL-2.0 License
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021/02/01     BalanceTWK   The unify TriCore porting code.
 */

#include <rthw.h>
#include <rtthread.h>
#include "cpuport.h"
#include "IfxCpu_Trap.h"
#include "UART_Logging.h"
#include "IfxSrc_cfg.h"
#include "rtdef.h"

static IfxStm_Timer tricore_timers[2];
static volatile Ifx_STM *const STMs[2] = {&MODULE_STM0, &MODULE_STM1};
static volatile Ifx_SRC_SRCR *const GPSR[2] = {&SRC_GPSR_GPSR0_SR0, &SRC_GPSR_GPSR1_SR0};

static volatile rt_ubase_t rt_interrupt_from_thread_core0 = RT_NULL;
static volatile rt_ubase_t rt_interrupt_to_thread_core0 = RT_NULL;
static volatile rt_ubase_t rt_interrupt_from_thread_core1 = RT_NULL;
static volatile rt_ubase_t rt_interrupt_to_thread_core1 = RT_NULL;
static volatile rt_ubase_t rt_thread_switch_interrupt_flag_core0 = RT_NULL;
static volatile rt_ubase_t rt_thread_switch_interrupt_flag_core1 = RT_NULL;

typedef struct UpperCtxInfo{
    unsigned long  _PCXI;
    unsigned long  _PSW;
    unsigned long*  _A10;
    unsigned long*  _A11;
    unsigned long  _D8;
    unsigned long  _D9;
    unsigned long  _D10;
    unsigned long  _D11;
    unsigned long*  _A12;
    unsigned long*  _A13;
    unsigned long*  _A14;
    unsigned long*  _A15;
    unsigned long  _D12;
    unsigned long  _D13;
    unsigned long  _D14;
    unsigned long  _D15;
}UpperCtx_T,*UpperCtx_Ptr;

typedef struct LowCtxInfo{
    unsigned long  _PCXI;
    unsigned long*  _PC;
    unsigned long*  _A2;
    unsigned long*  _A3;
    unsigned long  _D0;
    unsigned long  _D1;
    unsigned long  _D2;
    unsigned long  _D3;
    unsigned long*  _A4;
    unsigned long*  _A5;
    unsigned long*  _A6;
    unsigned long*  _A7;
    unsigned long  _D4;
    unsigned long  _D5;
    unsigned long  _D6;
    unsigned long  _D7;
}LowCtx_T,*LowCtx_Ptr;

App_Cpu g_AppCpu0; //频率信息变量

void rt_hw_systick_init( void )
{
  IfxStm_Timer_Config timer_config;
  IfxStm_Timer_initConfig(&timer_config, STMs[TRICORE_CPU_ID]);

  timer_config.base.frequency = RT_TICK_PER_SECOND;
  timer_config.base.isrPriority = 2;
  IfxStm_Timer_init(&tricore_timers[TRICORE_CPU_ID], &timer_config);
  IfxStm_Timer_run(&tricore_timers[TRICORE_CPU_ID]);
}

void rt_hw_usart_init(void)
{
    initUART();
}

void core0_trigger_scheduling(void)
{
    
    UpperCtx_Ptr ptUpperCtx = NULL;
#ifdef RT_USING_SMP
    rt_base_t level;
    level = rt_hw_local_irq_disable();
#endif

    /* 判断是否需要切换到 to 线程 */
    if(rt_interrupt_to_thread_core0)
    {
        /* To ensure memory coherency, a DSYNC instruction must be executed prior to 
           any access to an active CSA memory location. The DSYNC instruction forces 
           all internally buffered CSA register state to be written to memory.     */
        __dsync();
        /* 获取当前线程的 CSA 上下文地址 */
        ptUpperCtx = LINKWORD_TO_ADDRESS( __mfcr( CPU_PCXI ) );

        if(rt_interrupt_from_thread_core0)
        {
            /* 保存当前线程的 CSA 上下文 */
            *( (unsigned long *)rt_interrupt_from_thread_core0 ) = ptUpperCtx->_PCXI;
        }
        /* 将to线程的 CSA linkword赋给当前线程的上层 上下文的 LinkWord ，用于 TriCore 自动切换线程。*/
        ptUpperCtx->_PCXI = *( (unsigned long *)rt_interrupt_to_thread_core0 );
        __isync();
    }
#ifdef RT_USING_SMP
    rt_hw_local_irq_enable(level);
#endif
}

void core1_trigger_scheduling(void)
{
    
    UpperCtx_Ptr ptUpperCtx = NULL;
#ifdef RT_USING_SMP
    rt_base_t level;
    level = rt_hw_local_irq_disable();
#endif

    if(rt_thread_switch_interrupt_flag_core1 == 1)
        rt_thread_switch_interrupt_flag_core1 = 0;

    /* 判断是否需要切换到 to 线程 */
    if(rt_interrupt_to_thread_core1)
    {
        /* To ensure memory coherency, a DSYNC instruction must be executed prior to 
           any access to an active CSA memory location. The DSYNC instruction forces 
           all internally buffered CSA register state to be written to memory.     */
        __dsync();
        /* 获取当前线程的 CSA 上下文地址 */
        ptUpperCtx = LINKWORD_TO_ADDRESS( __mfcr( CPU_PCXI ) );

        if(rt_interrupt_from_thread_core1)
        {
            /* 保存当前线程的 CSA 上下文 */
            *( (unsigned long *)rt_interrupt_from_thread_core1 ) = ptUpperCtx->_PCXI;
        }
        /* 将to线程的 CSA linkword赋给当前线程的上层 上下文的 LinkWord ，用于 TriCore 自动切换线程。*/
        ptUpperCtx->_PCXI = *( (unsigned long *)rt_interrupt_to_thread_core1 );
        __isync();
    }

#ifdef RT_USING_SMP
    rt_hw_local_irq_enable(level);
#endif
}


IFX_INTERRUPT(Core0_INTERRUPT, 0, 2)
{
#ifdef RT_USING_SMP
    struct rt_cpu    *pcpu;
#endif
    rt_thread_switch_interrupt_flag_core0 = 1;
    rt_interrupt_enter();
    IfxStm_Timer_acknowledgeTimerIrq(&tricore_timers[TRICORE_CPU_ID]);
    rt_tick_increase();
    rt_interrupt_leave();
    rt_thread_switch_interrupt_flag_core0 = 0;
    
#ifdef RT_USING_SMP
    pcpu   = rt_cpu_index(0);
    /* whether do switch in interrupt */
    if (pcpu->irq_switch_flag)
    {
        rt_scheduler_do_irq_switch();
    }
#endif
}


IFX_INTERRUPT(Core1_INTERRUPT, 1, 2)
{
#ifdef RT_USING_SMP
    struct rt_cpu    *pcpu;
#endif
    rt_thread_switch_interrupt_flag_core1 = 1;
    rt_interrupt_enter();
    IfxStm_Timer_acknowledgeTimerIrq(&tricore_timers[TRICORE_CPU_ID]);
    rt_tick_increase();
    rt_interrupt_leave();
    rt_thread_switch_interrupt_flag_core1 = 0;

#ifdef RT_USING_SMP
    pcpu   = rt_cpu_index(1);
    /* whether do switch in interrupt */
    if (pcpu->irq_switch_flag)
    {
        rt_scheduler_do_irq_switch();
    }
#endif
}

IFX_INTERRUPT(Core0_YIELD, 0, 1)
{
    rt_interrupt_enter();
    /* clear the SRR bit */
    GPSR[TRICORE_CPU_ID]->B.CLRR = 1;

    core0_trigger_scheduling();
    rt_interrupt_leave();
}

IFX_INTERRUPT(Core1_YIELD, 1, 1)
{
    /* clear the SRR bit */
    GPSR[TRICORE_CPU_ID]->B.CLRR = 1;

    core1_trigger_scheduling();
}

/**
 * This function will initial board.
 */
#ifdef RT_USING_HEAP
extern unsigned int __HEAP[];
extern unsigned int __HEAP_END[];
#endif
void rt_hw_board_init()
{
    IfxStm_setSuspendMode(STMs[TRICORE_CPU_ID], IfxStm_SuspendMode_hard);
#ifdef RT_USING_HEAP
    /* initialize heap */
    rt_system_heap_init(__HEAP, __HEAP_END);
#endif

    /* Set-up the timer interrupt. */
    rt_hw_systick_init();
    /* USART driver initialization is open by default */
#ifdef RT_USING_SERIAL
    rt_hw_usart_init();
#endif

    /* Set the shell console output device */
#ifdef RT_USING_CONSOLE
    // rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

    /* Board underlying hardware initialization */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

    IfxSrc_init(GPSR[TRICORE_CPU_ID], (IfxSrc_Tos)TRICORE_CPU_ID, 1);
    IfxSrc_enable(GPSR[TRICORE_CPU_ID]);
}

#ifdef RT_USING_SMP

void rt_hw_context_switch_to(rt_ubase_t to, struct rt_thread *to_thread)
{
    rt_base_t levelCore0,levelCore1;
    rt_base_t level;

    switch (__mfcr(CPU_CORE_ID))
    {
    case 0:
        levelCore0 = rt_hw_local_irq_disable();
        rt_interrupt_to_thread_core0 = (*((unsigned long *)to));
        rt_cpu_self()->current_thread = to_thread;
        rt_hw_local_irq_enable(levelCore0);
        __syscall( 0 );
        break;
    case 1:
        levelCore1 = rt_hw_local_irq_disable();
        rt_interrupt_to_thread_core1 = (*((unsigned long *)to));
        rt_cpu_self()->current_thread = to_thread;
        rt_hw_local_irq_enable(levelCore1);
        __syscall( 1 );
        break;
    
    default:
        break;
    }
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread)
{
    rt_base_t levelCore0,levelCore1;

    switch (__mfcr(CPU_CORE_ID))
    {
    case 0:
        levelCore0 = rt_hw_local_irq_disable();
        rt_interrupt_from_thread_core0 = (*( (unsigned long *)from ));
        rt_interrupt_to_thread_core0 = (*((unsigned long *)to));
        rt_cpu_self()->current_thread = to_thread;
        rt_hw_local_irq_enable(levelCore0);
        break;
    case 1:
        levelCore1 = rt_hw_local_irq_disable();
        rt_interrupt_from_thread_core1 = (*( (unsigned long *)from ));
        rt_interrupt_to_thread_core1 = (*((unsigned long *)to));
        rt_cpu_self()->current_thread = to_thread;
        rt_hw_local_irq_enable(levelCore1);
        break;
    default:
        break;
    }
    /* 在保存完线程 CSA LinkWord 后,会给 SETR 位置 1，这样会触发一个异常陷阱函数。在这个陷阱函数里才会去真正的切换线程。*/
    GPSR[TRICORE_CPU_ID]->B.SETR = 1;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread)
{
    rt_base_t levelCore0,levelCore1;

    switch (__mfcr(CPU_CORE_ID))
    {
    case 0:
        levelCore0 = rt_hw_local_irq_disable();
        rt_interrupt_from_thread_core0 = (*( (unsigned long *)from ));
        rt_interrupt_to_thread_core0 = (*((unsigned long *)to));
        to = RT_NULL;
        rt_cpu_self()->current_thread = to_thread;
        rt_hw_local_irq_enable(levelCore0);
        /* 在保存完线程 CSA LinkWord 后，判断这次线程切换是否是 systick 触发的线程切换。*/
        if(rt_thread_switch_interrupt_flag_core0 == 0)
        {
            extern rt_hw_spinlock_t _cpus_lock;
            if(_cpus_lock.slock)
            {
                rt_hw_spin_unlock(&_cpus_lock);
            }
            // rt_interrupt_from_thread_core0 = (*( (unsigned long *)from ));
            /* 如果不是 systick 引发的线程切换，那么就触发一个线程切换的异常函数，这个异常函数里会真正做线程切换的事情。*/
            __syscall( 0 );
        }
        break;
    case 1:
        levelCore1 = rt_hw_local_irq_disable();
        rt_interrupt_from_thread_core1 = (*( (unsigned long *)from ));
        rt_interrupt_to_thread_core1 = (*((unsigned long *)to));
        to = RT_NULL;
        rt_cpu_self()->current_thread = to_thread;
        rt_hw_local_irq_enable(levelCore1);
        /* 在保存完线程 CSA LinkWord 后，判断这次线程切换是否是 systick 触发的线程切换。*/
        if(rt_thread_switch_interrupt_flag_core1 == 0)
        {
            extern rt_hw_spinlock_t _cpus_lock;
            if(_cpus_lock.slock)
            {
                rt_hw_spin_unlock(&_cpus_lock);
            }
            // rt_interrupt_from_thread_core0 = (*( (unsigned long *)from ));
            /* 如果不是 systick 引发的线程切换，那么就触发一个线程切换的异常函数，这个异常函数里会真正做线程切换的事情。*/
            __syscall( 1 );
        }
        break;
    default:
        break;
    }
}

void rt_hw_secondary_cpu_up(void)
{

}


void Core1_init(void)
{
    #define TRICORE_CPU1_ID     1

    IfxStm_Timer_Config timer_config;
    IfxStm_Timer_initConfig(&timer_config, STMs[TRICORE_CPU1_ID]);

    timer_config.base.frequency = RT_TICK_PER_SECOND;
    timer_config.base.isrPriority = 2;
    IfxStm_Timer_init(&tricore_timers[TRICORE_CPU1_ID], &timer_config);
    IfxStm_Timer_run(&tricore_timers[TRICORE_CPU1_ID]);

    IfxSrc_init(GPSR[TRICORE_CPU1_ID], (IfxSrc_Tos)IfxSrc_Tos_cpu1, 1);
    IfxSrc_enable(GPSR[TRICORE_CPU1_ID]);
}

void rt_hw_ipi_send(int ipi_vector, unsigned int cpu_mask)
{
    
}

void rt_hw_spin_lock_init(rt_hw_spinlock_t *lock)
{
    lock->slock = 0;
}

/* 获取spinlock，忙等待直到获取成功 */
void rt_hw_spin_lock(rt_hw_spinlock_t *lock)
{
    boolean         retVal = FALSE;
    volatile unsigned int spinLockVal;

    do
    {
        /** \brief This function is a implementation of a binary semaphore using compare and swap instruction
         * \param address address of resource.
         * \param value This variable is updated with status of address
         * \param condition if the value of address matches with the value of condition, then swap of value & address occurs.
         *  __cmpswapw((address), ((unsigned long)value), (condition) )
         */
        spinLockVal = 1UL;
        spinLockVal =
            (unsigned int)__cmpAndSwap(((unsigned int *)(&(lock->slock))), spinLockVal, 0);

        /* Check if the SpinLock WAS set before the attempt to acquire spinlock */
        if (spinLockVal == 0)
        {
            retVal = TRUE;
        }
    } while (retVal == FALSE);
}

void rt_hw_spin_unlock(rt_hw_spinlock_t *lock)
{
    boolean         retVal = FALSE;
    volatile unsigned int spinLockVal;

    do
    {
        /** \brief This function is a implementation of a binary semaphore using compare and swap instruction
         * \param address address of resource.
         * \param value This variable is updated with status of address
         * \param condition if the value of address matches with the value of condition, then swap of value & address occurs.
         *  __cmpswapw((address), ((unsigned long)value), (condition) )
         */
        spinLockVal = 0UL;
        spinLockVal =
            (unsigned int)__cmpAndSwap(((unsigned int *)(&(lock->slock))), spinLockVal, 1);

        /* Check if the SpinLock WAS set before the attempt to acquire spinlock */
        if (spinLockVal == 1)
        {
            retVal = TRUE;
        }
    } while (retVal == FALSE);
}

int rt_hw_cpu_id(void)
{
    return ((int)IfxCpu_getCoreId());
}

void rt_hw_local_irq_enable(rt_base_t level)
{
    restoreInterrupts((boolean)level);
}

void rt_hw_secondary_cpu_idle_exec(void)
{
     while(1)
     {

     }
}

rt_base_t rt_hw_local_irq_disable()
{
    rt_base_t level;
    level = IfxCpu_disableInterrupts();
    return level;
}
#else

rt_base_t rt_hw_interrupt_disable(void)
{
    rt_base_t level;
    level = IfxCpu_disableInterrupts();
    return level;
}

void rt_hw_interrupt_enable(rt_base_t level)
{
    restoreInterrupts((boolean)level);
}

void rt_hw_context_switch_to(rt_ubase_t to)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_interrupt_to_thread_core0 = (*((unsigned long *)to));
    __syscall( 0 );
    rt_hw_interrupt_enable(level);
   /* Will not get here. */
}

void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_interrupt_from_thread_core0 = (*( (unsigned long *)from ));
    rt_interrupt_to_thread_core0 = (*((unsigned long *)to));
    rt_hw_interrupt_enable(level);
    /* 在保存完线程 CSA LinkWord 后,会给 SETR 位置 1，这样会触发一个异常陷阱函数。在这个陷阱函数里才会去真正的切换线程。*/
    GPSR[TRICORE_CPU_ID]->B.SETR = 1;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_interrupt_from_thread_core0 = (*( (unsigned long *)from ));
    rt_interrupt_to_thread_core0 = (*((unsigned long *)to));
    to = RT_NULL;
    rt_hw_interrupt_enable(level);
    /* 在保存完线程 CSA LinkWord 后，判断这次线程切换是否是 systick 触发的线程切换。*/
    if(rt_thread_switch_interrupt_flag_core0 == 0)
    {
         /* 如果不是 systick 引发的线程切换，那么就触发一个线程切换的异常函数，这个异常函数里会真正做线程切换的事情。*/
        __syscall( 0 );
    }
}

#endif

/**
 * This function will initialize thread stack
 *
 * @param tentry the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 * @param texit the function will be called when thread exit
 *
 * @return stack address
 */
rt_uint8_t *rt_hw_stack_init(void       *tentry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *texit)
{
    rt_base_t level;
    UpperCtx_Ptr upperCtxPtr = NULL;
    LowCtx_Ptr   lowCtxPtr = NULL;
#ifdef RT_USING_SMP
    level = rt_hw_local_irq_disable();
#else
    level = rt_hw_interrupt_disable();
#endif
    {
        /* DSync to ensure that buffering is not a problem. */
        __dsync();

        /* Consume two free CSAs. */
        upperCtxPtr = LINKWORD_TO_ADDRESS(__mfcr(CPU_FCX));

        if( NULL != upperCtxPtr )
        {
            /* The Lower Links to the Upper. */
            lowCtxPtr = LINKWORD_TO_ADDRESS( upperCtxPtr->_PCXI );
        }

        /* Check that we have successfully reserved two CSAs. */
        if( ( NULL != lowCtxPtr ) && ( NULL != upperCtxPtr ))
        {
            /* Remove the two consumed CSAs from the free CSA list. */
            __dsync();
            __mtcr( CPU_FCX, lowCtxPtr->_PCXI );
            /* MTCR is an exception and must be followed by an ISYNC instruction. */
            __isync();
        }
        else
        {
            /* Simply trigger a context list depletion trap. */
            __svlcx();
        }
    }
#ifdef RT_USING_SMP
    rt_hw_local_irq_enable(level);
#else
    rt_hw_interrupt_enable(level);
#endif

    memset( upperCtxPtr, 0, TRICORE_NUM_WORDS_IN_CSA * sizeof( unsigned long ) );
    upperCtxPtr->_A11 = ( unsigned long* )texit;
    /* 对应 A10 寄存器; 这个寄存器用于保存栈指针 */
    upperCtxPtr->_A10 = ( unsigned long* )stack_addr;
    /* 对应 PSW 寄存器；这个寄存器用于保存当前线程的初始状态*/
    upperCtxPtr->_PSW = TRICORE_SYSTEM_PROGRAM_STATUS_WORD;

    /* Clear the lower CSA. */
    memset( lowCtxPtr, 0, TRICORE_NUM_WORDS_IN_CSA * sizeof( unsigned long ) );
    /* 对应 A4 寄存器; 用于保存函数的入参 */
    lowCtxPtr->_A4 = ( unsigned long *) parameter;
    /* 对应 A11 寄存器; 用于保存 PC 指针。*/
    lowCtxPtr->_PC = ( unsigned long *) tentry;
    /* PCXI pointing to the Upper context. */
    lowCtxPtr->_PCXI = ( TRICORE_INITIAL_PCXI_UPPER_CONTEXT_WORD | ( unsigned long ) ADDRESS_TO_LINKWORD( upperCtxPtr ) );
    /* Save the link to the CSA in the top of stack. */
    *((unsigned long * )stack_addr) = (unsigned long) ADDRESS_TO_LINKWORD( lowCtxPtr );
    /* DSync to ensure that buffering is not a problem. */
    __dsync();

    return stack_addr;
}

void get_clk(void)
{
	disableInterrupts();
	//禁用看门狗
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    //获取时钟频率，便于查看当前系统运行频率
    g_AppCpu0.info.pllFreq = IfxScuCcu_getPllFrequency();
    g_AppCpu0.info.cpuFreq = IfxScuCcu_getCpuFrequency(IfxCpu_getCoreIndex());
    g_AppCpu0.info.sysFreq = IfxScuCcu_getSpbFrequency();
    g_AppCpu0.info.stmFreq = IfxStm_getFrequency(&MODULE_STM0);
}

void tricore0_trap_yield_for_task( int iTrapIdentification )
{
  switch( iTrapIdentification )
  {
    case 0:
        core0_trigger_scheduling();
      break;

    default:
      /* Unimplemented trap called. */
      /* TODO */
      break;
  }
}


void tricore1_trap_yield_for_task( int iTrapIdentification )
{
  switch( iTrapIdentification )
  {
    case 1:
        core1_trigger_scheduling();
      break;

    default:
      /* Unimplemented trap called. */
      /* TODO */
      break;
  }
}


