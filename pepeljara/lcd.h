/*
 * lcd.h — HD44780 16x2 u 4-bitnom modu, spojen preko PCF8574 I2C
 * ekspandera (tzv. "I2C backpack").
 *
 * Standardno ožičenje backpacka:
 *   P0 = RS, P1 = RW, P2 = EN, P3 = pozadinsko svjetlo, P4..P7 = D4..D7
 */

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

class Lcd {
public:
    explicit Lcd(uint8_t addr7);

    void init();
    void clear();
    void home();
    void setCursor(uint8_t col, uint8_t row);
    void print(const char* s);
    void print(char c);
    void printPadded(const char* s, uint8_t width);  /* dopuni razmacima */
    void backlight(bool on);

    /* Upisuje vlastiti znak (5x8 točaka) u CGRAM, indeksi 0..7 */
    void createChar(uint8_t index, const uint8_t data[8]);

private:
    void expanderWrite(uint8_t data);
    void pulseEnable(uint8_t data);
    void write4(uint8_t nibbleAndFlags);
    void send(uint8_t value, uint8_t rsFlag);

    void command(uint8_t c) { send(c, 0x00); }
    void data(uint8_t d)    { send(d, 0x01); }

    uint8_t m_addr;
    uint8_t m_backlight;
};

#endif /* LCD_H */
