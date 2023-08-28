#ifndef USER_IFX_CFG_TRAP_H_
#define USER_IFX_CFG_TRAP_H_

#include "Ifx_Types.h"
#include "IfxCpu_Intrinsics.h"
#include "cpuport.h"

// IFX_INLINE void tricore_trap_yield_for_task(IfxCpu_Trap trapInfo)
// {
//   switch(trapInfo.tId)
//   {
//     case 0:
//         trigger_scheduling();
//       break;

//     default:
//       /* Unimplemented trap called. */
//       /* TODO*/
//       break;
//   }
// }

// #define IFX_CFG_CPU_TRAP_SYSCALL_CPU0_HOOK(trapInfo)  tricore_trap_yield_for_task(trapInfo)

#endif /* USER_IFX_CFG_TRAP_H_ */
