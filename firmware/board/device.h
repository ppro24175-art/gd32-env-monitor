#ifndef ENV_DEVICE_H
#define ENV_DEVICE_H

#include <stdint.h>

#define BIT(n) (1u << (n))
#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

enum {
    CLOCK_CTL = 0x40021000u, CLOCK_CFG = 0x40021004u,
    CLOCK_APB1_RST = 0x40021010u, CLOCK_APB2_EN = 0x40021018u,
    CLOCK_APB1_EN = 0x4002101cu,
    GPIO_A = 0x40010800u, GPIO_B = 0x40010c00u,
};

/* Both maps use these offsets. The vendor names and instance indices differ. */
enum {
    GPIO_INPUT = 0x08, GPIO_SET = 0x10, GPIO_CLEAR = 0x14,
    TIMER_CTL = 0x00, TIMER_UPDATE = 0x14, TIMER_COUNT = 0x24,
    TIMER_PSC = 0x28, TIMER_PERIOD = 0x2c,
    UART_STATUS = 0x00, UART_DATA = 0x04, UART_BAUD = 0x08, UART_CTL = 0x0c,
    I2C_CTL0 = 0x00, I2C_CTL1 = 0x04, I2C_OWN_ADDRESS = 0x08, I2C_DATA = 0x10,
    I2C_STAT0 = 0x14, I2C_STAT1 = 0x18, I2C_CLOCK = 0x1c, I2C_RISE = 0x20,
};

enum {
    CLOCK_GPIO_A_B_AF = BIT(0) | BIT(2) | BIT(3),
    CLOCK_ESP_UART = BIT(14), CLOCK_LOG_UART = BIT(17),
    CLOCK_MICRO_TIMER = BIT(0), CLOCK_LIGHT_I2C = BIT(21),
};

#if defined(TARGET_STM32F103) && !defined(TARGET_GD32F103)
/* RCC HSI, TIM2, I2C1, USART1, USART2. */
#define DEVICE_NAME "STM32F103C8T6"
#define INT_OSC_HZ 8000000u
#define INT_OSC_ENABLE BIT(0)
#define INT_OSC_READY BIT(1)
#define MICRO_TIMER 0x40000000u
#define LIGHT_I2C 0x40005400u
#define ESP_UART 0x40013800u
#define LOG_UART 0x40004400u
#elif defined(TARGET_GD32F103) && !defined(TARGET_STM32F103)
/* RCU IRC8M, TIMER1, I2C0, USART0, USART1. */
#define DEVICE_NAME "GD32F103C8T6"
#define INT_OSC_HZ 8000000u
#define INT_OSC_ENABLE BIT(0)
#define INT_OSC_READY BIT(1)
#define MICRO_TIMER 0x40000000u
#define LIGHT_I2C 0x40005400u
#define ESP_UART 0x40013800u
#define LOG_UART 0x40004400u
#else
#error Select exactly one supported MCU
#endif

#endif
