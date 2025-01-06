#pragma once

#include <common.h>
#include <instructions.h>

typedef struct{
    u8 a;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 f;
    u8 h;
    u8 l;
    u16 sp;
    u16 pc;
} cpu_registers;

typedef struct{
    //registers
    cpu_registers regs;

    // current fetch data
    u16 fetched_data;
    // current mem destination
    u16 mem_dest;

    //current opcode of the cpu
    u8 cur_opcode;

    //current instruction pointer
    instruction *cur_inst;
    bool stepping;
    bool halted;

    bool dest_is_mem;

    //interrupts flag
    bool int_master_enabled;

    //interrupt register
    u8 ie_register;
} cpu_context;


void cpu_init();
bool cpu_step();

typedef void (*IN_PROC)(cpu_context*);
IN_PROC get_proc_func(in_type type);

#define CPU_FLAG_C BIT(ctx->regs.f, 4)
#define CPU_FLAG_Z BIT(ctx->regs.f, 7)

u16 cpu_read_reg(reg_type rt);
void cpu_set_reg(reg_type rt, u16 val);

u8 cpu_read_reg8(reg_type rt);
void cpu_set_reg8(reg_type rt, u8 val);

void fetch_data();

u8 get_cpu_ie_register();
void set_cpu_ie_register(u8 val);

cpu_registers* get_cpu_regs();




//https://youtu.be/GU2I_zHd4wU?list=PLVxiWMqQvhg_yk4qy2cSC3457wZJga_e5&t=1045
//https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html