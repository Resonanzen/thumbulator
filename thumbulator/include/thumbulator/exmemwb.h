#ifndef EXWBMEM_HEADER
#define EXWBMEM_HEADER

#include "sim_support.h"
#include "except.h"
#include "decode.h"

#define ESPR_T (1 << 24)

struct CPU {
  uint32_t gpr[16]; // General-purpose register plus FP, SP, LR, and PC
  uint32_t apsr;    // Application program status reg: flags
  uint32_t ipsr;    // Interrupt program status reg: except number
  uint32_t espr;    // Execution program status reg: not SW readable
  uint32_t primask;
  uint32_t control;
  uint32_t sp_main;
  uint32_t sp_process;
  uint32_t mode;
  uint32_t exceptmask; // Bit mask of pending exceptions
};

extern struct CPU cpu;

struct SYSTICK {
  uint32_t control;
  uint32_t reload;
  uint32_t value;
  uint32_t calib;
};

extern struct SYSTICK systick;

// Define bit fields of APSR
#define FLAG_N_INDEX 31
#define FLAG_Z_INDEX 30
#define FLAG_C_INDEX 29
#define FLAG_V_INDEX 28
#define FLAG_N_MASK (1 << FLAG_N_INDEX)
#define FLAG_Z_MASK (1 << FLAG_Z_INDEX)
#define FLAG_C_MASK (1 << FLAG_C_INDEX)
#define FLAG_V_MASK (1 << FLAG_V_INDEX)

#define cpu_get_gpr(x) cpu.gpr[x]
#define cpu_set_gpr(x, y) cpu.gpr[x] = y

// GPRs with special functions
#define GPR_SP 13
#define GPR_LR 14
#define GPR_PC 15
#define cpu_get_sp() cpu_get_gpr(GPR_SP)
#define cpu_set_sp(x) (cpu_set_gpr(GPR_SP, (x)))
#define cpu_get_lr() cpu_get_gpr(GPR_LR)
#define cpu_set_lr(x) cpu_set_gpr(GPR_LR, (x))
#define cpu_get_pc() cpu_get_gpr(GPR_PC)
#define cpu_set_pc(x) cpu_set_gpr(GPR_PC, (x))

// Get, set, and compute the CPU flags
#define cpu_get_flag_z() ((cpu.apsr & FLAG_Z_MASK) >> FLAG_Z_INDEX)
#define cpu_get_flag_n() ((cpu.apsr & FLAG_N_MASK) >> FLAG_N_INDEX)
#define cpu_get_flag_c() ((cpu.apsr & FLAG_C_MASK) >> FLAG_C_INDEX)
#define cpu_get_flag_v() ((cpu.apsr & FLAG_V_MASK) >> FLAG_V_INDEX)
#define cpu_set_flag_z(x) cpu.apsr = ((((x)&0x1) << FLAG_Z_INDEX) | (cpu.apsr & ~FLAG_Z_MASK))
#define cpu_set_flag_n(x) cpu.apsr = ((((x)&0x1) << FLAG_N_INDEX) | (cpu.apsr & ~FLAG_N_MASK))
#define cpu_set_flag_c(x) cpu.apsr = ((((x)&0x1) << FLAG_C_INDEX) | (cpu.apsr & ~FLAG_C_MASK))
#define cpu_set_flag_v(x) cpu.apsr = ((((x)&0x1) << FLAG_V_INDEX) | (cpu.apsr & ~FLAG_V_MASK))

#define do_zflag(x) cpu_set_flag_z(((x) == 0) ? 1 : 0)
#define do_nflag(x) cpu_set_flag_n((x) >> 31)
#define do_vflag(a, b, r) \
  cpu_set_flag_v(         \
      (((a) >> 31) & ((b) >> 31) & ~((r) >> 31)) | (~((a) >> 31) & ~((b) >> 31) & ((r) >> 31)))
void do_cflag(uint32_t a, uint32_t b, uint32_t carry);
#define cpu_get_apsr() (cpu.apsr)
#define cpu_set_apsr(x) cpu.apsr = (x)

// Other SPR
#define CPU_MODE_HANDLER 0
#define CPU_MODE_THREAD 1
#define cpu_mode_is_handler() (cpu.mode == 0x0)
#define cpu_mode_is_thread() (cpu.mode == 0x1)
#define cpu_mode_handler() cpu.mode = (0x0)
#define cpu_mode_thread() cpu.mode = (0x1)
#define cpu_get_ipsr() (cpu.ipsr)
#define cpu_set_ipsr(x) cpu.ipsr = (x & 0x1F)
#define CPU_STACK_MAIN 0
#define CPU_STACK_PROCESS 1
#define cpu_stack_is_main() ((cpu.control & 0x2) == 0x0)
#define cpu_stack_is_process() (~cpu_stack_is_main())
#define cpu_stack_use_main() cpu.control = (cpu.control & ~0x2)
#define cpu_stack_use_process() cpu.control = (cpu.control | 0x2)
#define cpu_get_except() (cpu.exceptmask)
#define cpu_set_except(x) cpu.exceptmask |= (1 << x)
#define cpu_clear_except(x) cpu.exceptmask &= ~(1 << x)

// Sign extension
#define zeroExtend32(x) (x)
#define signExtend32(x, n) \
  (((((x) >> ((n)-1)) & 0x1) != 0) ? (~((unsigned int)0) << (n)) | (x) : (x))

// Special write to PC
#define alu_write_pc(x)    \
  do {                     \
    takenBranch = 1;       \
    cpu_set_pc((x) | 0x1); \
  } while(0)

uint32_t exwbmem(uint16_t pInsn, decode_result decoded);

// Timing model
#define TIMING_BRANCH 2
#define TIMING_BRANCH_LINK 3
#define TIMING_PC_UPDATE 2
#define TIMING_MEM 2

#endif