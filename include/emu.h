#pragma once

#include <common.h>

typedef struct {
    bool paused;
    bool running;
    u64 ticks;
    bool die;
} emu_context;

int emu_run(int argc, char **argv);

emu_context* emu_get_context();
void emu_cycles(int cpu_cycles);
emu_context* emu_get_context();
