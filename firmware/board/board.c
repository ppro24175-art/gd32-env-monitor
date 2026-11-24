#include "board.h"
#include "device.h"

enum {
    DHT_PIN = 0, ESP_RESET_PIN = 0, SCL_PIN = 6, SDA_PIN = 7,
    INPUT_FLOATING = 0x4, OUTPUT_OPEN_DRAIN_2MHZ = 0x6,
    AF_PUSH_PULL_2MHZ = 0xa, AF_OPEN_DRAIN_2MHZ = 0xe,
    UART_RXNE = BIT(5), UART_TXE = BIT(7), UART_TC = BIT(6),
    I2C_ENABLE = BIT(0), I2C_START = BIT(8), I2C_STOP = BIT(9),
    I2C_ACK = BIT(10), I2C_POS = BIT(11),
    I2C_SB = BIT(0), I2C_ADDR = BIT(1), I2C_BTF = BIT(2),
    I2C_BUSY = BIT(1), I2C_MASTER = BIT(0), I2C_ERRORS = 0x0f00,
    I2C_TIMEOUT_MS = 20,
};

const char board_name[] = DEVICE_NAME;
static volatile uint32_t elapsed_ms;

void SysTick_Handler(void)
{
    ++elapsed_ms;
}

static uint16_t micros(void)
{
    return (uint16_t)REG32(MICRO_TIMER + TIMER_COUNT);
}

static uint32_t millis(void)
{
    return elapsed_ms;
}

static void delay_ms(uint32_t duration)
{
    const uint32_t start = millis();
    while ((uint32_t)(millis() - start) < duration) {
    }
}

static void delay_us(uint16_t duration)
{
    const uint16_t start = micros();
    while ((uint16_t)(micros() - start) < duration) {
    }
}

static void gpio_mode(uint32_t port, unsigned pin, uint32_t mode)
{
    const uint32_t config = port + (pin >= 8u ? 4u : 0u);
    const unsigned shift = (pin % 8u) * 4u;
    REG32(config) = (REG32(config) & ~(0x0fu << shift)) | (mode << shift);
}

static void dht_drive_low(bool low)
{
    REG32(GPIO_A + (low ? GPIO_CLEAR : GPIO_SET)) = BIT(DHT_PIN);
}

static bool dht_level(void)
{
    return (REG32(GPIO_A + GPIO_INPUT) & BIT(DHT_PIN)) != 0;
}

static uint32_t irq_pause(void)
{
    uint32_t mask;
    __asm__ volatile ("mrs %0, primask\n\tcpsid i" : "=r"(mask) :: "memory");
    return mask;
}

static void irq_restore(uint32_t mask)
{
    __asm__ volatile ("msr primask, %0" :: "r"(mask) : "memory");
}

static void i2c_init(void)
{
    REG32(CLOCK_APB1_RST) |= CLOCK_LIGHT_I2C;
    REG32(CLOCK_APB1_RST) &= ~CLOCK_LIGHT_I2C;
    gpio_mode(GPIO_B, SCL_PIN, AF_OPEN_DRAIN_2MHZ);
    gpio_mode(GPIO_B, SDA_PIN, AF_OPEN_DRAIN_2MHZ);
    REG32(LIGHT_I2C + I2C_CTL1) = INT_OSC_HZ / 1000000u;
    REG32(LIGHT_I2C + I2C_OWN_ADDRESS) = BIT(14); /* OAR1/SADDR0, fixed bit for 7-bit addressing */
    REG32(LIGHT_I2C + I2C_CLOCK) = INT_OSC_HZ / (2u * 100000u);
    REG32(LIGHT_I2C + I2C_RISE) = INT_OSC_HZ / 1000000u + 1u;
    REG32(LIGHT_I2C + I2C_CTL0) = I2C_ENABLE | I2C_ACK;
}

static bool i2c_wait(uint32_t address, uint32_t bit, bool set)
{
    const uint32_t start = millis();
    while (((REG32(address) & bit) != 0) != set) {
        if ((REG32(LIGHT_I2C + I2C_STAT0) & I2C_ERRORS) ||
            (uint32_t)(millis() - start) >= I2C_TIMEOUT_MS) {
            return false;
        }
    }
    return (REG32(LIGHT_I2C + I2C_STAT0) & I2C_ERRORS) == 0;
}

static bool i2c_abort(void)
{
    REG32(LIGHT_I2C + I2C_STAT0) &= ~I2C_ERRORS;
    if (REG32(LIGHT_I2C + I2C_STAT1) & I2C_MASTER) {
        REG32(LIGHT_I2C + I2C_CTL0) |= I2C_STOP;
        (void)i2c_wait(LIGHT_I2C + I2C_CTL0, I2C_STOP, false);
    }

    /* Up to nine clock pulses release a slave stranded in a read. */
    REG32(LIGHT_I2C + I2C_CTL0) = 0;
    REG32(GPIO_B + GPIO_SET) = BIT(SCL_PIN) | BIT(SDA_PIN);
    gpio_mode(GPIO_B, SCL_PIN, OUTPUT_OPEN_DRAIN_2MHZ);
    gpio_mode(GPIO_B, SDA_PIN, OUTPUT_OPEN_DRAIN_2MHZ);
    for (unsigned clock = 0; clock < 9 && !(REG32(GPIO_B + GPIO_INPUT) & BIT(SDA_PIN)); ++clock) {
        REG32(GPIO_B + GPIO_CLEAR) = BIT(SCL_PIN);
        delay_us(5);
        REG32(GPIO_B + GPIO_SET) = BIT(SCL_PIN);
        delay_us(10);
    }
    /* A STOP releases the bus regardless of how many pulses were needed. */
    REG32(GPIO_B + GPIO_CLEAR) = BIT(SCL_PIN) | BIT(SDA_PIN);
    delay_us(5);
    REG32(GPIO_B + GPIO_SET) = BIT(SCL_PIN);
    delay_us(10);
    REG32(GPIO_B + GPIO_SET) = BIT(SDA_PIN);
    delay_us(5);
    i2c_init();
    return false;
}

static bool i2c_address(uint8_t address, bool read)
{
    if (!i2c_wait(LIGHT_I2C + I2C_STAT1, I2C_BUSY, false)) {
        return false;
    }
    REG32(LIGHT_I2C + I2C_CTL0) |= I2C_START;
    if (!i2c_wait(LIGHT_I2C + I2C_STAT0, I2C_SB, true)) {
        return false;
    }
    REG32(LIGHT_I2C + I2C_DATA) = (uint32_t)address * 2u + read;
    return i2c_wait(LIGHT_I2C + I2C_STAT0, I2C_ADDR, true);
}

static void i2c_clear_address(void)
{
    (void)REG32(LIGHT_I2C + I2C_STAT0);
    (void)REG32(LIGHT_I2C + I2C_STAT1);
}

static bool i2c_write_byte(uint8_t address, uint8_t value)
{
    if (!i2c_address(address, false)) {
        return i2c_abort();
    }
    i2c_clear_address();
    REG32(LIGHT_I2C + I2C_DATA) = value;
    if (!i2c_wait(LIGHT_I2C + I2C_STAT0, I2C_BTF, true)) {
        return i2c_abort();
    }
    REG32(LIGHT_I2C + I2C_CTL0) |= I2C_STOP;
    return i2c_wait(LIGHT_I2C + I2C_CTL0, I2C_STOP, false) || i2c_abort();
}

static bool i2c_read_pair(uint8_t address, uint8_t data[2])
{
    if (!i2c_address(address, true)) {
        return i2c_abort();
    }

    /* Position the NACK on byte 2. Keep the ADDR/ACK and STOP/DR steps adjacent. */
    REG32(LIGHT_I2C + I2C_CTL0) |= I2C_POS;
    const uint32_t address_mask = irq_pause();
    i2c_clear_address();
    REG32(LIGHT_I2C + I2C_CTL0) &= ~I2C_ACK;
    irq_restore(address_mask);
    if (!i2c_wait(LIGHT_I2C + I2C_STAT0, I2C_BTF, true)) {
        return i2c_abort();
    }
    const uint32_t stop_mask = irq_pause();
    REG32(LIGHT_I2C + I2C_CTL0) |= I2C_STOP;
    data[0] = (uint8_t)REG32(LIGHT_I2C + I2C_DATA);
    irq_restore(stop_mask);
    data[1] = (uint8_t)REG32(LIGHT_I2C + I2C_DATA);
    if (!i2c_wait(LIGHT_I2C + I2C_CTL0, I2C_STOP, false)) {
        return i2c_abort();
    }
    REG32(LIGHT_I2C + I2C_CTL0) = I2C_ENABLE | I2C_ACK;
    return true;
}

static void uart_init(uint32_t uart)
{
    REG32(uart + UART_BAUD) = (INT_OSC_HZ + 57600u) / 115200u;
    REG32(uart + UART_CTL) = BIT(13) | BIT(3) | BIT(2); /* 8N1, TX and RX enabled */
}

static bool uart_wait(uint32_t uart, uint32_t bit)
{
    const uint32_t start = millis();
    while (!(REG32(uart + UART_STATUS) & bit)) {
        if ((uint32_t)(millis() - start) >= 20u) {
            return false;
        }
    }
    return true;
}

static bool uart_write(uint32_t uart, const char *data, size_t length)
{
    for (size_t index = 0; index < length; ++index) {
        if (!uart_wait(uart, UART_TXE)) {
            return false;
        }
        REG32(uart + UART_DATA) = (uint8_t)data[index];
    }
    return uart_wait(uart, UART_TC);
}

static bool esp_write(const char *data, size_t length)
{
    return uart_write(ESP_UART, data, length);
}

static int esp_read(void)
{
    const uint32_t status = REG32(ESP_UART + UART_STATUS);
    if (!(status & (UART_RXNE | 0x0fu))) {
        return -1;
    }
    const uint8_t byte = (uint8_t)REG32(ESP_UART + UART_DATA);
    return status & 0x0fu ? -1 : byte;
}

static void esp_reset(void)
{
    REG32(GPIO_B + GPIO_CLEAR) = BIT(ESP_RESET_PIN);
    delay_ms(15);
    REG32(GPIO_B + GPIO_SET) = BIT(ESP_RESET_PIN);
    delay_ms(1200);
}

static void log_line(const char *line)
{
    size_t length = 0;
    while (line[length]) {
        ++length;
    }
    (void)uart_write(LOG_UART, line, length);
    (void)uart_write(LOG_UART, "\r\n", 2);
}

const struct env_io board_io = {
    .micros = micros, .millis = millis, .delay_ms = delay_ms,
    .dht_drive_low = dht_drive_low, .dht_level = dht_level,
    .i2c_write_byte = i2c_write_byte, .i2c_read_pair = i2c_read_pair,
    .esp_reset = esp_reset, .esp_write = esp_write, .esp_read = esp_read, .log = log_line,
};

void board_init(void)
{
    /* Select each chip's reset oscillator and divide AHB/APB clocks by one. */
    REG32(CLOCK_CTL) |= INT_OSC_ENABLE;
    while (!(REG32(CLOCK_CTL) & INT_OSC_READY)) {
    }
    REG32(CLOCK_CFG) &= ~3u;
    while (REG32(CLOCK_CFG) & (3u << 2)) {
    }
    REG32(CLOCK_CFG) &= ~((15u << 4) | (7u << 8) | (7u << 11));
    REG32(CLOCK_APB2_EN) |= CLOCK_GPIO_A_B_AF | CLOCK_ESP_UART;
    REG32(CLOCK_APB1_EN) |= CLOCK_LOG_UART | CLOCK_MICRO_TIMER | CLOCK_LIGHT_I2C;
    (void)REG32(CLOCK_APB1_EN);

    REG32(MICRO_TIMER + TIMER_PSC) = INT_OSC_HZ / 1000000u - 1u;
    REG32(MICRO_TIMER + TIMER_PERIOD) = 65535;
    REG32(MICRO_TIMER + TIMER_UPDATE) = BIT(0);
    REG32(MICRO_TIMER + TIMER_CTL) = BIT(0);

    REG32(0xe000e014u) = INT_OSC_HZ / 1000u - 1u; /* SysTick reload */
    REG32(0xe000e018u) = 0;
    REG32(0xe000e010u) = BIT(2) | BIT(1) | BIT(0);

    REG32(GPIO_A + GPIO_SET) = BIT(DHT_PIN);
    REG32(GPIO_B + GPIO_SET) = BIT(ESP_RESET_PIN);
    gpio_mode(GPIO_A, DHT_PIN, OUTPUT_OPEN_DRAIN_2MHZ);
    gpio_mode(GPIO_B, ESP_RESET_PIN, OUTPUT_OPEN_DRAIN_2MHZ);
    gpio_mode(GPIO_A, 9, AF_PUSH_PULL_2MHZ);
    gpio_mode(GPIO_A, 10, INPUT_FLOATING);
    gpio_mode(GPIO_A, 2, AF_PUSH_PULL_2MHZ);
    gpio_mode(GPIO_A, 3, INPUT_FLOATING);
    uart_init(ESP_UART);
    uart_init(LOG_UART);
    i2c_init();
}
