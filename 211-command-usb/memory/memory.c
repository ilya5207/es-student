#include "device.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"
#include "memory.h"

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

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void)
{
    printf("area       start        end        size\n");

    row("flash", XIP_BASE, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("sram", SRAM_BASE, SRAM_END);
    row("rom", ROM_BASE, ROM_BASE + 16 * 1024);

    row("image", (uintptr_t)&__flash_binary_start, (uintptr_t)&__flash_binary_end);
    row("free", (uintptr_t)&__flash_binary_end, XIP_BASE + PICO_FLASH_SIZE_BYTES);
    row("boot2", (uintptr_t)&__boot2_start__, (uintptr_t)&__boot2_end__);
    row("text", (uintptr_t)&__boot2_end__, (uintptr_t)&__etext);

    row("data flash", (uintptr_t)&__etext, (uintptr_t)&__etext + (&__data_end__ - &__data_start__));
    row("data ram", (uintptr_t)&__data_start__, (uintptr_t)&__data_end__);
    row("bss", (uintptr_t)&__bss_start__, (uintptr_t)&__bss_end__);
    row("heap", (uintptr_t)&__bss_end__, (uintptr_t)&__HeapLimit);
    row("stack",(uintptr_t)&__StackBottom, (uintptr_t)&__StackTop);

    uint32_t boot2_size = (unsigned)(&__boot2_end__ - &__boot2_start__);
    uint32_t text_size = (unsigned)(&__etext - &__boot2_end__);
    uint32_t data_flash_size = (unsigned)(&__data_end__ - &__data_start__);
    uint32_t flash_image_size = boot2_size + text_size + data_flash_size;
    uint32_t flash_free_size = PICO_FLASH_SIZE_BYTES - flash_image_size;
    uint32_t data_ram_size = (unsigned)(&__data_end__ - &__data_start__);
    uint32_t bss_size = (unsigned)(&__bss_end__ - &__bss_start__);
    uint32_t ram_used_size = data_ram_size + bss_size;
    uint32_t heap_size = (unsigned)(&__HeapLimit - &__bss_end__);
    uint32_t stack_size = (unsigned)(&__StackTop - &__StackBottom);

    printf("\ntotal\n");
    printf("  flash image %8d = boot2 %d + text %d + data %d\n", flash_image_size, boot2_size, text_size, data_flash_size);
    printf("  flash free  %8d of %d\n", flash_free_size, PICO_FLASH_SIZE_BYTES);
    printf("  ram used    %8d = data %d + bss %d\n", ram_used_size, data_ram_size, bss_size);
    printf("  ram free    %8d for heap and %d for stack\n", heap_size, stack_size);
}
