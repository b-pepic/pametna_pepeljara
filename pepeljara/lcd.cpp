#include "config.h"
#include "lcd.h"
#include "i2c.h"

#include <util/delay.h>

namespace {

const uint8_t BIT_RS = 0x01;
const uint8_t BIT_EN = 0x04;
const uint8_t BIT_BL = 0x08;   /* pozadinsko svjetlo */

} /* anonimni namespace */

Lcd::Lcd(uint8_t addr7)
    : m_addr(addr7), m_backlight(BIT_BL) {}

void Lcd::expanderWrite(uint8_t d) {
    I2C::writeByte(m_addr, d | m_backlight);
}

void Lcd::pulseEnable(uint8_t d) {
    expanderWrite(d | BIT_EN);
    _delay_us(1);
    expanderWrite(d & ~BIT_EN);
    _delay_us(50);
}

void Lcd::write4(uint8_t nibbleAndFlags) {
    expanderWrite(nibbleAndFlags);
    pulseEnable(nibbleAndFlags);
}

void Lcd::send(uint8_t value, uint8_t rsFlag) {
    write4((uint8_t)((value & 0xF0) | rsFlag));
    write4((uint8_t)(((value << 4) & 0xF0) | rsFlag));
}

void Lcd::init() {
    _delay_ms(50);              /* čekaj da se napajanje LCD-a stabilizira */
    expanderWrite(0x00);
    _delay_ms(10);

    /* Propisana sekvenca za ulazak u 4-bitni mod (HD44780 datasheet) */
    write4(0x30); _delay_ms(5);
    write4(0x30); _delay_us(150);
    write4(0x30); _delay_us(150);
    write4(0x20); _delay_us(150);

    command(0x28);   /* 4 bita, 2 retka, font 5x8 */
    command(0x08);   /* ekran ugašen */
    command(0x01);   /* brisanje */
    _delay_ms(2);
    command(0x06);   /* kursor ide udesno, ekran se ne pomiče */
    command(0x0C);   /* ekran upaljen, bez kursora i treptanja */
    _delay_ms(2);
}

void Lcd::clear() {
    command(0x01);
    _delay_ms(2);
}

void Lcd::home() {
    command(0x02);
    _delay_ms(2);
}

void Lcd::setCursor(uint8_t col, uint8_t row) {
    static const uint8_t rowOffset[2] = { 0x00, 0x40 };
    if (row > 1) row = 1;
    command((uint8_t)(0x80 | (col + rowOffset[row])));
}

void Lcd::print(char c) {
    data((uint8_t)c);
}

void Lcd::print(const char* s) {
    while (*s) {
        data((uint8_t)*s++);
    }
}

void Lcd::printPadded(const char* s, uint8_t width) {
    uint8_t n = 0;
    while (*s && n < width) {
        data((uint8_t)*s++);
        n++;
    }
    while (n < width) {
        data(' ');
        n++;
    }
}

void Lcd::backlight(bool on) {
    m_backlight = on ? BIT_BL : 0x00;
    expanderWrite(0x00);
}

void Lcd::createChar(uint8_t index, const uint8_t d[8]) {
    index &= 0x07;
    command((uint8_t)(0x40 | (index << 3)));
    for (uint8_t i = 0; i < 8; i++) {
        data(d[i]);
    }
    setCursor(0, 0);
}
