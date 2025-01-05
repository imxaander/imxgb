#include <stack.h>
#include <bus.h>
#include <cpu.h>

void stack_push(u8 data){
    get_cpu_regs()->sp--;
    bus_write(get_cpu_regs()->sp, data);
};

void stack_push16(u16 data){
    stack_push((data >> 8) & 0xFF);
    stack_push(data & 0xFF);
};

u8 stack_pop(){
    return bus_read(get_cpu_regs()->sp++);
};

u16 stack_pop16(){
    u8 lo = stack_pop();
    u8 hi = stack_pop();

    return (hi << 8) | lo;
};