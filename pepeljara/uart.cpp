#include "config.h"
#include "uart.h"

#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

namespace {

const uint8_t RX_SIZE = 32;

volatile char    g_rx[RX_SIZE];
volatile uint8_t g_head = 0;
volatile uint8_t g_tail = 0;

/* međuspremnik za sastavljanje linije */
char    g_line[RX_SIZE];
uint8_t g_lineLen = 0;

} /* anonimni namespace */

ISR(USART_RX_vect) {
    char c = (char)UDR0;
    uint8_t next = (uint8_t)((g_head + 1) % RX_SIZE);
    if (next != g_tail) {          /* ako je pun, znak se odbacuje */
        g_rx[g_head] = c;
        g_head = next;
    }
}

namespace Uart {

void init() {
    const uint16_t ubrr = (uint16_t)(F_CPU / 16 / UART_BAUD - 1);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8 bita, 1 stop, bez pariteta */
}

void putChar(char c) {
    while (!(UCSR0A & (1 << UDRE0))) { }
    UDR0 = (uint8_t)c;
}

void print(const char* s) {
    while (*s) putChar(*s++);
}

void println(const char* s) {
    print(s);
    putChar('\r');
    putChar('\n');
}

void printInt(int32_t v) {
    char buf[12];
    ltoa(v, buf, 10);
    print(buf);
}

void printFloat(float v, uint8_t decimals) {
    char buf[16];
    dtostrf((double)v, 1, decimals, buf);
    print(buf);
}

void print_P(const char* s) {
    char c;
    while ((c = (char)pgm_read_byte(s++)) != '\0') {
        putChar(c);
    }
}

void println_P(const char* s) {
    print_P(s);
    putChar('\r');
    putChar('\n');
}

bool available() {
    return g_head != g_tail;
}

char read() {
    if (g_head == g_tail) return 0;
    char c = g_rx[g_tail];
    g_tail = (uint8_t)((g_tail + 1) % RX_SIZE);
    return c;
}

bool readLine(char* buf, uint8_t size) {
    while (available()) {
        char c = read();

        if (c == '\r') continue;

        if (c == '\n') {
            g_line[g_lineLen] = '\0';
            uint8_t n = 0;
            while (n < size - 1 && g_line[n]) {
                buf[n] = g_line[n];
                n++;
            }
            buf[n] = '\0';
            g_lineLen = 0;
            return true;
        }

        if (g_lineLen < RX_SIZE - 1) {
            g_line[g_lineLen++] = c;
        }
    }
    return false;
}

} /* namespace Uart */
