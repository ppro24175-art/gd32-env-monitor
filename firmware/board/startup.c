#include <stdint.h>

extern uint32_t __stack_top, __data_load, __data_start, __data_end, __bss_start, __bss_end;
extern void SysTick_Handler(void);
extern int main(void);

static void default_handler(void)
{
    for (;;) {
    }
}

void reset_handler(void)
{
    const uint32_t *src = &__data_load;
    for (volatile uint32_t *dest = &__data_start; dest < &__data_end; ++dest) {
        *dest = *src++;
    }
    for (volatile uint32_t *dest = &__bss_start; dest < &__bss_end; ++dest) {
        *dest = 0;
    }
    *(volatile uint32_t *)0xe000ed08u = 0x08000000u; /* SCB->VTOR */
    __asm__ volatile ("dsb\n\tisb" ::: "memory");
    (void)main();
    default_handler();
}

__attribute__((section(".vectors"), used))
const uintptr_t vectors[76] = {
    [0] = (uintptr_t)&__stack_top,
    [1] = (uintptr_t)reset_handler,
    [2 ... 6] = (uintptr_t)default_handler,
    [11] = (uintptr_t)default_handler,
    [12] = (uintptr_t)default_handler,
    [14] = (uintptr_t)default_handler,
    [15] = (uintptr_t)SysTick_Handler,
    [16 ... 75] = (uintptr_t)default_handler,
};
