#include <cpu.h>
#include <bus.h>
#include <emu.h>
#include <ram.h>
cpu_context ctx = {0};

void cpu_init() {
    ctx.regs.pc = 0x100;
}

static void fetch_instruction(){
    ctx.cur_opcode = bus_read(ctx.regs.pc++);
    ctx.cur_inst =  instruction_by_opcode(ctx.cur_opcode);
}

static void execute(){
    IN_PROC exec_process = get_proc_func(ctx.cur_inst->type);
    if(!exec_process){
        printf("Process for Instruction is ");
        NO_IMPL
    }
    exec_process(&ctx);
}

bool cpu_step() {
    if(!ctx.halted){
        u16 cur_pc = ctx.regs.pc;
        fetch_instruction();
        fetch_data();

        //bad way to do this.
        //printf("EXECUTING %s: \n\tOPCODE: %02X,\n\tPC: %04X,\n\tTYPE: %d,\n\tDATA: %04X - %02X - %02X\n", //inst_name(ctx.cur_inst->type), ctx.cur_opcode, cur_pc, ctx.cur_inst->type, ctx.fetched_data, bus_read(ctx.regs.pc + 1), bus_read(ctx.regs.pc + 2));

        //more organized way
        printf("%04X:%6s %02X %02X %02X (A: %02X, BC:%02X%02X, DE: %02X%02X, F:%02X, HL:%02X%02X, SP:%04X) \n", cur_pc, inst_name(ctx.cur_inst->type), ctx.cur_opcode, bus_read(cur_pc + 1), bus_read(cur_pc + 2), ctx.regs.a, ctx.regs.b, ctx.regs.c, ctx.regs.d, ctx.regs.e,ctx.regs.f, ctx.regs.h, ctx.regs.l, ctx.regs.sp);

        execute();

    }
    return true;
}

u8 get_cpu_ie_register(){
    return ctx.ie_register;
};

void set_cpu_ie_register(u8 val){
    ctx.ie_register = val;
};

cpu_registers* get_cpu_regs(){
    return &ctx.regs;
};

