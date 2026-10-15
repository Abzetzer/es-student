#include <stdio.h>
#include <stdint.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

// memory.c

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

#define SRAM_SIZE (256u * 1024u)
#define ROM_SIZE (16u * 1024u)

void mem_info(void)
{
    // шапка таблицы: область, начало, конец, размер
    printf("area       start      end          size\n");
    // flash — XIP_BASE и PICO_FLASH_SIZE_BYTES
    // sram — базовый адрес из SDK, размер из datasheet
    // rom — базовый адрес из SDK, размер из datasheet
    // image — от __flash_binary_start до __flash_binary_end
    // free  — от __flash_binary_end до конца флеш-памяти
    // boot2 — от __boot2_start__ до __boot2_end__
    // text  — от __boot2_end__ до __etext: код и константы

    // data flash — хранение .data, от __etext, длиной с .data
    // data ram   — работа .data, от __data_start__ до __data_end__
    // bss        — от __bss_start__ до __bss_end__
    // heap       — от __bss_end__ до __HeapLimit
    // stack      — от __StackBottom до __StackTop

    // итог: образ во флеш и из чего он сложился
    // итог: свободно во флеш-памяти из всего её объёма
    // итог: занято в ОЗУ — .data и .bss
    // итог: свободно в ОЗУ — под кучу и под стек
    uintptr_t flash_start = XIP_BASE;
    uintptr_t flash_end = XIP_BASE + PICO_FLASH_SIZE_BYTES;
    uintptr_t sram_start  = SRAM_BASE;
    uintptr_t sram_end    = SRAM_BASE + SRAM_SIZE;
    uintptr_t rom_start   = ROM_BASE;
    uintptr_t rom_end     = ROM_BASE + ROM_SIZE;

    uintptr_t dat_sz  = (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__;
    uintptr_t b2_sz = (uintptr_t)&__boot2_end__ - (uintptr_t)&__boot2_start__;
    uintptr_t txt_sz = (uintptr_t)&__etext - (uintptr_t)&__boot2_end__;
    uintptr_t bss_sz = (uintptr_t)&__bss_end__ - (uintptr_t)&__bss_start__;
    uintptr_t heap_sz = (uintptr_t)&__HeapLimit - (uintptr_t)&__bss_end__;
    uintptr_t stk_sz = (uintptr_t)&__StackTop - (uintptr_t)&__StackBottom;

    uintptr_t data_flash_start = (uintptr_t)&__etext;
    uintptr_t data_flash_end = data_flash_start + dat_sz;

    row("flash", flash_start, flash_end);
    row("sram", sram_start, sram_end);
    row("rom", rom_start, rom_end);

    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    row("free", (uintptr_t)&__flash_binary_end, flash_end);
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

    row("data flash", (uintptr_t)&__etext, data_flash_end);
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
    row("stack", (uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    printf("\ntotal\n");
    printf("  flash image  %7u = boot2 %u + text %u + data %u\n",
           (unsigned)(b2_sz + txt_sz + dat_sz),
           (unsigned)b2_sz, (unsigned)txt_sz, (unsigned)dat_sz);
    printf("  flash free   %7u of %u\n",
           (unsigned)(flash_end - (uintptr_t)&__flash_binary_end),
           (unsigned)(flash_end - flash_start));
    printf("  ram used     %7u = data %u + bss %u\n",
           (unsigned)(dat_sz + bss_sz),
           (unsigned)dat_sz, (unsigned)bss_sz);
    printf("  ram free     %7u for heap and %u for stack\n",
           (unsigned)heap_sz, (unsigned)stk_sz);
}