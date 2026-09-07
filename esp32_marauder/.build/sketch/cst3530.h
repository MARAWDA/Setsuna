#line 1 "C:\\Users\\wisht\\New folder\\Setsuna\\esp32_marauder\\cst3530.h"
#pragma once
#ifndef cst3530_h
#define cst3530_h

#ifdef HAS_CAP_TOUCH

#include <Wire.h>

// Waveshare ESP32-C5-Touch-LCD-2.8: CST3530 capacitive touch controller.
// Touch/LCD reset and backlight are not wired to direct GPIOs on this board -
// they are all driven through the onboard CH32V003 I2C IO expander.
#define CST3530_ADDR             0x58
#define CST3530_DATA_REG         0xD0070000UL
#define CST3530_COORD_NEXT_REG   0xD0070900UL
#define CST3530_END_READ_REG     0xD00002ABUL

#define IO_EXPANDER_ADDR         0x24
#define IO_EXPANDER_MODE_REG     0x02
#define IO_EXPANDER_OUTPUT_REG   0x03
#define IO_EXPANDER_PWM_REG      0x05

#define IO_EXPANDER_PIN_TP_RST   0
#define IO_EXPANDER_PIN_LCD_RST  1
#define IO_EXPANDER_PIN_PA_CTRL  3

static uint8_t _io_expander_output = (1 << IO_EXPANDER_PIN_TP_RST) |
                                    (1 << IO_EXPANDER_PIN_LCD_RST) |
                                    (1 << IO_EXPANDER_PIN_PA_CTRL);

static bool _io_expander_write_u8(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(IO_EXPANDER_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static bool io_expander_set_output(uint8_t pin, bool level) {
    if (pin > 7) return false;
    if (level) _io_expander_output |= (1 << pin);
    else _io_expander_output &= ~(1 << pin);
    return _io_expander_write_u8(IO_EXPANDER_OUTPUT_REG, _io_expander_output);
}

static bool io_expander_set_backlight(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint8_t pwm = (uint8_t)(((uint16_t)percent * 255) / 100);
    return _io_expander_write_u8(IO_EXPANDER_PWM_REG, pwm);
}

static void io_expander_init() {
    // Match the Waveshare board reference: all EXIO pins are outputs, and the
    // panel/PA control lines start in the enabled state before the LCD reset pulse.
    _io_expander_write_u8(IO_EXPANDER_MODE_REG, 0xFF);
    _io_expander_write_u8(IO_EXPANDER_OUTPUT_REG, _io_expander_output);
    io_expander_set_output(IO_EXPANDER_PIN_TP_RST, true);
    io_expander_set_output(IO_EXPANDER_PIN_LCD_RST, true);
    io_expander_set_output(IO_EXPANDER_PIN_PA_CTRL, true);
}

static bool _cst3530_write(uint32_t reg, const uint8_t *data, uint8_t len) {
    Wire.beginTransmission(CST3530_ADDR);
    Wire.write((uint8_t)((reg >> 24) & 0xFF));
    Wire.write((uint8_t)((reg >> 16) & 0xFF));
    Wire.write((uint8_t)((reg >> 8) & 0xFF));
    Wire.write((uint8_t)(reg & 0xFF));
    for (uint8_t i = 0; i < len; i++) Wire.write(data[i]);
    return Wire.endTransmission() == 0;
}

static bool _cst3530_read(uint32_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(CST3530_ADDR);
    Wire.write((uint8_t)((reg >> 24) & 0xFF));
    Wire.write((uint8_t)((reg >> 16) & 0xFF));
    Wire.write((uint8_t)((reg >> 8) & 0xFF));
    Wire.write((uint8_t)(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom((int)CST3530_ADDR, (int)len);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = Wire.available() ? Wire.read() : 0;
    return true;
}

// Named ft6336_* to match the generic HAS_CAP_TOUCH call sites in Display.cpp
static void ft6336_init() {
    Wire.begin(CTP_SDA, CTP_SCL, 400000U);

    // Diagnostic: confirm the expander (0x24) and touch IC (0x58) are actually on the bus
    Serial.println("[Touch] I2C scan:");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[Touch]   found device at 0x%02X\n", addr);
        }
    }

    io_expander_init();

    // LCD panel reset lives on the same IO expander as the touch reset
    io_expander_set_output(IO_EXPANDER_PIN_LCD_RST, false);
    delay(50);
    io_expander_set_output(IO_EXPANDER_PIN_LCD_RST, true);
    delay(120);

    io_expander_set_output(IO_EXPANDER_PIN_TP_RST, false);
    delay(100);
    io_expander_set_output(IO_EXPANDER_PIN_TP_RST, true);
    delay(500);

    io_expander_set_output(IO_EXPANDER_PIN_PA_CTRL, true);
    io_expander_set_backlight(100);

    Serial.println("[Touch] CST3530 initialized via CH32V003 IO expander");
}

static uint8_t ft6336_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
    uint8_t buf[9];
    if (!_cst3530_read(CST3530_DATA_REG, buf, 9)) return 0;
    if ((buf[3] & 0x0F) == 0x00) {
        _cst3530_write(CST3530_END_READ_REG, NULL, 0);
        return 0;
    }
    *raw_x = (uint16_t)(((buf[7] & 0x0F) << 8) | buf[4]);
    *raw_y = (uint16_t)(((buf[7] & 0xF0) << 4) | buf[5]);
    _cst3530_write(CST3530_END_READ_REG, NULL, 0);
    return 1;
}

static uint8_t ft6336_update(uint16_t *x, uint16_t *y) {
    uint16_t raw_x, raw_y;
    if (!ft6336_read_raw(&raw_x, &raw_y)) return 0;
    *x = (raw_x < SCREEN_WIDTH)  ? raw_x : (uint16_t)(SCREEN_WIDTH  - 1);
    *y = (raw_y < SCREEN_HEIGHT) ? raw_y : (uint16_t)(SCREEN_HEIGHT - 1);
    return 1;
}

#endif // HAS_CAP_TOUCH
#endif // cst3530_h
