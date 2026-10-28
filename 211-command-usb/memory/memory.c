#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"
#include "command.h"
#include "device.h"
#include "memory.h"
#include "led.h"

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

uint32_t data_variable = 100;
uint32_t bss_variable;

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

int main(void);

void fw_info(void)
{
       // считаем вызов: data_variable и bss_variable на единицу больше
       data_variable++;
       bss_variable++;
    // адреса функций со сброшенным признаком Thumb
    // локальная переменная и блок из кучи

    // шапка: объект, адрес, значение
       printf("object          address     value\n");
    // main, fw_info  — адрес с признаком Thumb и два байта по сброшенному адресу
       uint16_t *main_code = (uint16_t *)((uintptr_t)main & ~1u);
       printf("main            0x%08x  0x%04x\n", (uintptr_t)main, *main_code);
       uint16_t *fw_code = (uint16_t *)((uintptr_t)fw_info & ~1u);
       printf("fw_info         0x%08x  0x%04x\n", (uintptr_t)fw_info, *fw_code);
       uint16_t *commands_code = (uint16_t *)((uintptr_t)commands & ~1u);
       printf("commands        0x%08x\n", (uintptr_t)commands_code);
    // commands       — адрес массива
       for (int i = 0; i < command_count; ++i)
       {
              printf("%-13s 0x%08x\n", commands[i].name, (uintptr_t)commands[i].handler);
       }

    // обработчики    — имя команды и адрес обработчика, строкой на команду
       printf("DEVICE_PROJECT  0x%08x %s\n", (uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT);
    // константы      — адрес и значение строк паспорта из device.h
       printf("DEVICE_BOARD    0x%08x %s\n", (uintptr_t)DEVICE_BOARD, DEVICE_BOARD);
    // data_variable  — адрес и значение, секция .data
       printf("data_variable   0x%08x %u\n", (uintptr_t)&data_variable, data_variable);
    // bss_variable   — адрес и значение, секция .bss
       printf("bss_variable    0x%08x %u\n", (uintptr_t)&bss_variable, bss_variable);
    // stack_variable — адрес и значение
       uint32_t stack_variable = 1946;
       uint32_t *heap_variable = malloc(sizeof(uint32_t));

       if (heap_variable != NULL)
       {
              *heap_variable = 1951;
       }
       
       printf("stack_variable  0x%08x %u\n", (uintptr_t)&stack_variable, stack_variable);
    // heap_variable  — адрес и значение
       printf("heap_variable   0x%08x %u\n", (uintptr_t)heap_variable, *heap_variable);
    // возвращаем блок кучи
       free(heap_variable);

}

#define VECTOR_TABLE 0x10000100

void boot_info(void)
{
       // указатель на таблицу векторов и два первых слова из неё
       const uint32_t *vectors = (const uint32_t *)VECTOR_TABLE;

       uint32_t stack_top = vectors[0];
       uint32_t reset_handler = vectors[1];
    // указатель на регистр GPIO_IN и разряд вывода светодиода

    // vector table   — адрес таблицы
       printf("vector table   0x%08x\n",(uint32_t)VECTOR_TABLE);
    //   stack top    — первое слово
       printf("  stack top    0x%08x\n", stack_top);
    //   reset        — второе слово
       printf("  reset        0x%08x\n", reset_handler);
    //   reset (even) — оно же со сброшенным признаком Thumb
       printf("  reset (even) 0x%08x\n", (reset_handler & ~1u));
    // gpio in        — адрес регистра
       volatile uint32_t *gpio_in = (uint32_t*)0xd0000004;
       uint32_t level = (*gpio_in >> led_pin()) & 1u;

       printf("gpio in        0x%08x\n", (uintptr_t)gpio_in);
    //   led bit      — разряд из регистра
       printf("  led bit      %u\n", level);
    //   gpio_get     — то же значение через SDK
       printf("  gpio get     %u\n", gpio_get(led_pin()));

    /*vector table   0x10000100
  stack top    0x20042000
  reset        0x100001f7
  reset (even) 0x100001f6
gpio in        0xd0000004
  led bit      0
  gpio_get     0*/
}