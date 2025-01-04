#include <cpu.h>
#include <emu.h>
#include <common.h>
#include <bus.h>
#include <stack.h>

static bool check_cond(cpu_context* ctx){
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch(ctx->cur_inst->cond){
        case CT_NONE: return true;
        case CT_NZ: return !z;
        case CT_Z: return z;
        case CT_C: return c;
        case CT_NC: return !c;
    }

    return false;
}

void cpu_set_flags(cpu_context* ctx, char z, char n, char h, char c){
    if(z != -1){
        BIT_SET(ctx->regs.f, 7, z);
    }

    if(n != -1){
        BIT_SET(ctx->regs.f, 6, n);
    }

    if(h != -1){
        BIT_SET(ctx->regs.f, 5, h);
    }
    if(c != -1){
        BIT_SET(ctx->regs.f, 4, c);
    }
}
static void proc_nop(cpu_context* ctx){
    return;
}

static void proc_di(cpu_context* ctx){
    ctx->int_master_enabled = false;
}

static void proc_none(cpu_context* ctx){
    printf("invalid instruction\n");
    exit(-7);
}

static void proc_jp(cpu_context* ctx){
    goto_addr(ctx, ctx->fetched_data, false);
}
static void proc_jr(cpu_context* ctx){
    char rel = (char)(ctx->fetched_data & 0xFF);
    u16 addr = ctx->fetched_data + rel;
    goto_addr(ctx, addr, false);
}
static void proc_call(cpu_context* ctx){
    goto_addr(ctx, ctx->fetched_data, true);
}

static void proc_xor(cpu_context* ctx){
    ctx->regs.a ^= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a, 0, 0, 0);
}
static void proc_ld(cpu_context* ctx){
    if(ctx->dest_is_mem){

        if(ctx->cur_inst->reg_2 >= RT_AF){
            emu_cycles(1);
            bus_write16(ctx->mem_dest, ctx->fetched_data);
        }else{
            emu_cycles(1);
            bus_write(ctx->mem_dest, ctx->fetched_data);
        }

        return;
    }

    if(ctx->cur_inst->mode == AM_HL_SPR){
        u8 hflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;

        u8 cflag = (cpu_read_reg(ctx->cur_inst->reg_2) & 0xFF) + (ctx->fetched_data & 0xFF) >= 0x100;

        cpu_set_flags(ctx, 0, 0, hflag, cflag);
        cpu_set_reg(ctx->cur_inst->reg_2,
        cpu_read_reg(ctx->cur_inst->reg_2) + (char)ctx->fetched_data);

        return;
    }
    
    cpu_set_reg(ctx->cur_inst->reg_1, ctx->fetched_data);
}

static void proc_ldh(cpu_context* ctx){
    if(ctx->cur_inst->reg_1 == RT_A){
        cpu_set_reg(ctx->cur_inst->reg_1, bus_read(0xFF00 | ctx->fetched_data));
    }else{
        bus_write(0xFF00 | ctx->fetched_data, ctx->regs.a);
    }

    emu_cycles(1);
}

static void proc_dec(cpu_context* ctx){

}
static void goto_addr(cpu_context* ctx, u16 addr, bool pushpc){
    if(check_cond(ctx)){
        if(pushpc){
            emu_cycles(2);
            stack_push16(ctx->regs.pc);
        }
        ctx->regs.pc = ctx->fetched_data;
        emu_cycles(1);
    }
}
static void  proc_pop(cpu_context* ctx){
    u16 lo = stack_pop();
    emu_cycles(1);
    u16 hi = stack_pop();
    emu_cycles(1);

    u16 stack_data = (hi << 8) | lo;

    if(ctx->cur_inst->reg_1 = RT_AF){
        cpu_set_reg(ctx->cur_inst->reg_1, stack_data & 0xFFF0);
    }else{
        cpu_set_reg(ctx->cur_inst->reg_1, stack_data);
    }
}

static void proc_push(cpu_context* ctx){
    u16 hi = (cpu_read_reg(ctx->cur_inst->reg_1) >> 8) & 0xFF;
    emu_cycles(1);
    stack_push(hi);
    u16 lo = (cpu_read_reg(ctx->cur_inst->reg_2) >> 8) & 0xFF;
    stack_push(lo);
    emu_cycles(1);

    emu_cycles(1);
}

IN_PROC processes[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_JP] = proc_jp,
    [IN_DI] = proc_di,
    [IN_XOR] = proc_xor,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JR] = proc_jr,
    [IN_CALL] = proc_call,
    [IN_POP] = proc_pop,
    [IN_PUSH] = proc_push,
    
};

IN_PROC get_proc_func(in_type type){
    return processes[type];
}
