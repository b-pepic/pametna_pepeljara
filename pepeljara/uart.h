/*
 * uart.h — serijska konzola (9600 8N1) preko USB-serijskog čipa Nano ploče.
 * Služi za kalibraciju i dijagnostiku. Slanje je blokirajuće (kratko),
 * primanje ide preko prekida u kružni međuspremnik.
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

namespace Uart {

void init();

void putChar(char c);
void print(const char* s);
void println(const char* s);
void printInt(int32_t v);
void printFloat(float v, uint8_t decimals);

/* Varijante koje čitaju tekst izravno iz flasha umjesto iz RAM-a.
 */
void print_P(const char* progmemStr);
void println_P(const char* progmemStr);

bool available();
char read();

/* Skuplja znakove do '\n'. Vraća true kad je cijela linija spremna
 * u 'buf' (bez znaka za novi red). */
bool readLine(char* buf, uint8_t size);

} /* namespace Uart */

/* Kratice: UPRINT("tekst") umjesto Uart::print("tekst") — tekst ostaje
 * u flashu, PSTR() se brine za smještaj. */
#include <avr/pgmspace.h>
#define UPRINT(s)   Uart::print_P(PSTR(s))
#define UPRINTLN(s) Uart::println_P(PSTR(s))

#endif /* UART_H */
