/*
 * clock.h — sistemsko vrijeme bez Arduino cora.
 *
 * Timer0 radi u normalnom modu s predskalarom 64:
 *   tick   = 64 / 16 MHz = 4 us
 *   prekid = 256 tickova = 1024 us
 *
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

namespace Clock {

void     init();          /* pokreće Timer0 i globalne prekide */
uint32_t millis();        /* ms od pokretanja */
uint32_t micros();        /* us od pokretanja, rezolucija 4 us */

/* Je li od trenutka 'since' proteklo barem 'ms' milisekundi.
 * Otporno na prelijevanje 32-bitnog brojača. */
inline bool elapsed(uint32_t since, uint32_t ms) {
    return (uint32_t)(millis() - since) >= ms;
}

} /* namespace Clock */

#endif /* CLOCK_H */
