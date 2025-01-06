#include <cpu.h>
#include <emu.h>
#include <common.h>
#include <bus.h>
#include <stack.h>

reg_type rt_lookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A,
};

reg_type decode_reg(u8 reg){
    if (reg > 0b111){
        return RT_NONE;
    }
    return rt_lookup[reg];    
}

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
static bool is_16_bit(reg_type rt){
    return rt >= RT_AF;
}
static void goto_addr(cpu_context* ctx, u16 addr, bool pushpc){
    if(check_cond(ctx)){
        if(pushpc){
            emu_cycles(2);
            stack_push16(ctx->regs.pc);
        }
        ctx->regs.pc = addr;
        emu_cycles(1);
    }
}

static void proc_nop(cpu_context* ctx){
    return;
}

static void proc_and(cpu_context* ctx){
    ctx->regs.a &= ctx->fetched_data;
    cpu_set_flags(ctx, (ctx->regs.a == 0), 0, 1, 0);
}

static void proc_xor(cpu_context* ctx){
    ctx->regs.a ^= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void proc_or(cpu_context* ctx){
    ctx->regs.a |= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void proc_cp(cpu_context* ctx){

    int n = (int)ctx->regs.a - (int)ctx->fetched_data;
    cpu_set_flags(ctx, n == 0, 1, ((int)ctx->regs.a & 0x0F) - ((int)ctx->fetched_data & 0x0F) < 0, n < 0);
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
    u16 addr = ctx->regs.pc + rel;
    goto_addr(ctx, addr, false);
}
static void proc_call(cpu_context* ctx){
    goto_addr(ctx, ctx->fetched_data, true);
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

static void  proc_pop(cpu_context* ctx){
    u16 lo = stack_pop();
    emu_cycles(1);
    u16 hi = stack_pop();
    emu_cycles(1);

    u16 stack_data = (hi << 8) | lo;
    if(ctx->cur_inst->reg_1 == RT_AF){
        cpu_set_reg(ctx->cur_inst->reg_1, stack_data & 0xFFF0);
    }else{
        cpu_set_reg(ctx->cur_inst->reg_1, stack_data);
    }
}

static void proc_push(cpu_context* ctx){
    u16 hi = (cpu_read_reg(ctx->cur_inst->reg_1) >> 8) & 0xFF;
    emu_cycles(1);
    stack_push(hi);
    u16 lo = (cpu_read_reg(ctx->cur_inst->reg_2)) & 0xFF;
    stack_push(lo);
    emu_cycles(1);

    emu_cycles(1);
}

static void proc_inc(cpu_context* ctx){
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) + 1;
    if(is_16_bit(ctx->cur_inst->reg_1)){
        emu_cycles(1);
    }
    
    if(ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR){
        val = bus_read(cpu_read_reg(RT_HL)) + 1;
        val &= 0xFF;
        bus_write(cpu_read_reg(RT_HL), val);
    }else{
        cpu_set_reg(ctx->cur_inst->reg_1, val);
    }

    if((ctx->cur_opcode & 0x03) == 0x03){
        return;
    } 

    cpu_set_flags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
    emu_cycles(1);
}

static void proc_dec(cpu_context* ctx){
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - 1;
    if(is_16_bit(ctx->cur_inst->reg_1)){
        emu_cycles(1);
    }
    
    if(ctx->cur_inst->reg_1 == RT_HL && ctx->cur_inst->mode == AM_MR){
        val = bus_read(cpu_read_reg(RT_HL)) - 1;
        val &= 0xFF;
        bus_write(cpu_read_reg(RT_HL), val);
    }else{
        cpu_set_reg(ctx->cur_inst->reg_1, val);
    }

    if((ctx->cur_opcode & 0x03) == 0x03){
        return;
    } 

    cpu_set_flags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
    emu_cycles(1);
}

static void proc_add(cpu_context *ctx) {
    u32 val = cpu_read_reg(ctx->cur_inst->reg_1) + ctx->fetched_data;

    bool is_16bit = is_16_bit(ctx->cur_inst->reg_1);

    if (is_16bit) {
        emu_cycles(1);
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {
        val = cpu_read_reg(ctx->cur_inst->reg_1) + (char)ctx->fetched_data;
    }

    int z = (val & 0xFF) == 0;
    int h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
    int c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;

    if (is_16bit) {
        z = -1;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xFFF) + (ctx->fetched_data & 0xFFF) >= 0x1000;
        u32 n = ((u32)cpu_read_reg(ctx->cur_inst->reg_1)) + ((u32)ctx->fetched_data);
        c = n >= 0x10000;
    }

    if (ctx->cur_inst->reg_1 == RT_SP) {
        z = 0;
        h = (cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
        c = (int)(cpu_read_reg(ctx->cur_inst->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) > 0x100;
    }

    cpu_set_reg(ctx->cur_inst->reg_1, val & 0xFFFF);
    cpu_set_flags(ctx, z, 0, h, c);
}

static void proc_adc(cpu_context *ctx) {
    u16 u = ctx->fetched_data;
    u16 a = ctx->regs.a;
    u16 c = CPU_FLAG_C;

    ctx->regs.a = (a + u + c) & 0xFF;

    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 
        (a & 0xF) + (u & 0xF) + c > 0xF,
        a + u + c > 0xFF);
}
static void proc_sbc(cpu_context *ctx) {
    u8 val = ctx->fetched_data + CPU_FLAG_C;

    int z = cpu_read_reg(ctx->cur_inst->reg_1) - val == 0;

    int h = ((int)cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)cpu_read_reg(ctx->cur_inst->reg_1)) - ((int)ctx->fetched_data) - ((int)CPU_FLAG_C) < 0;

    cpu_set_reg(ctx->cur_inst->reg_1, cpu_read_reg(ctx->cur_inst->reg_1) - val);
    cpu_set_flags(ctx, z, 1, h, c);
}

static void proc_sub(cpu_context *ctx) {
    u16 val = cpu_read_reg(ctx->cur_inst->reg_1) - ctx->fetched_data;

    int z = val == 0;
    int h = ((int)cpu_read_reg(ctx->cur_inst->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) < 0;
    int c = ((int)cpu_read_reg(ctx->cur_inst->reg_1)) - ((int)ctx->fetched_data) < 0;

    cpu_set_reg(ctx->cur_inst->reg_1, val);
    cpu_set_flags(ctx, z, 1, h, c);
}
static void proc_ret(cpu_context* ctx){
    if(ctx->cur_inst->cond != CT_NONE){
        emu_cycles(1);
    }

    if(check_cond(ctx)){
        u8 lo = stack_pop();
        emu_cycles(1);

        u8 hi = stack_pop();
        emu_cycles(1);

        u16 newaddr = (hi << 8) | lo;
        ctx->regs.pc = newaddr;
        emu_cycles(1);
    }
}

static void proc_reti(cpu_context* ctx){
    ctx->int_master_enabled = true;
    proc_ret(ctx);
}

static void proc_rst(cpu_context* ctx){
    goto_addr(ctx, ctx->cur_inst->param, true);
}

static void proc_cb(cpu_context* ctx){
    u8 op = ctx->fetched_data;
    reg_type reg = decode_reg(op & 0b111);

    u8 bit = (op >> 3) & 0b111;
    u8 bit_op = (op >> 6) & 0b11;

    emu_cycles(1);

    if(reg == RT_HL){
        emu_cycles(2);
    }

    switch (bit_op){
    case 1:
        //bit
        break;
    case 2:
        //bit
        break;
    default:
        break;
    }
}

IN_PROC processes[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_JP] = proc_jp,
    [IN_DI] = proc_di,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JR] = proc_jr,
    [IN_CALL] = proc_call,
    [IN_RST] = proc_rst,
    [IN_RET] = proc_ret,
    [IN_DEC] = proc_dec,
    [IN_INC] = proc_inc,
    [IN_RETI] = proc_reti,
    [IN_POP] = proc_pop,
    [IN_PUSH] = proc_push,
    [IN_ADD] = proc_add,
    [IN_ADC] = proc_adc,
    [IN_SUB] = proc_sub,
    [IN_SBC] = proc_sbc,
    [IN_AND] = proc_and,
    [IN_XOR] = proc_xor,
    [IN_OR] = proc_or,
    [IN_CP] = proc_cp,
    [IN_CB] = proc_cb,
};

IN_PROC get_proc_func(in_type type){
    return processes[type];
}
