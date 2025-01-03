#include <common.h>

#include <ram.h>

typedef struct {
    u8 wram[0x2000];
    u8 hram[0x80];
} ram_context;

static ram_context ctx;

u8 wram_read(u16 address){
    address -= 0xC000;

    if(address >= 0x2000){
        printf("INVALID WRAM ADDRESS!\n");
        exit(-7);
    }

    return ctx.wram[address];
}

void wram_write(u16 address, u8 value){
    address -= 0xC000;

    ctx.wram[address] = value;
}

u8 hram_read(u16 address){
    address -= 0xFF80;

    if(address >= 0xFF80){
        printf("INVALID WRAM ADDRESS!\n");
        exit(-7);
    }

    return ctx.wram[address];
}

void hram_write(u16 address, u8 value){
    address -= 0xFF80;

    ctx.wram[address] = value;
}

