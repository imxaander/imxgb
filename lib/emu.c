#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <ui.h>
#include <pthread.h>
#include <unistd.h>

/* 
  Emu components:

  |Cart|
  |CPU|
  |Address Bus|
  |PPU|
  |Timer|

*/

static emu_context ctx;

emu_context *emu_get_context() {
    return &ctx;
}


int emu_run(int argc, char **argv) {
    ctx.die = false;

    if (argc < 2) {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    if (!cart_load(argv[1])) {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("Cart loaded..\n");

    SDL_Init(SDL_INIT_VIDEO);
    printf("SDL INIT\n");
    TTF_Init();
    printf("TTF INIT\n");

    ui_init();

    pthread_t cpu_thread;
    if(pthread_create(&cpu_thread, NULL, cpu_run, NULL)){
        printf("cant create thread");
    }


    if(!ctx.die){
        sleep(1000);
        ui_handle_events();
    }
    return 0;
}

void* cpu_run(){
    cpu_init();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while(ctx.running) {


        if (ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }

        ctx.ticks++;
    }

    return 0;
}
void emu_cycles(int cpu_cycles){
    //printf("cpu cycling %d times...\n", cpu_cycles);
    // ctx.ticks += cpu_cycles;
};
