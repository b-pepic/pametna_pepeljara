#include "config.h"
#include "clock.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

namespace {

volatile uint32_t g_ms       = 0;   /* pune milisekunde */
volatile uint16_t g_us_rest  = 0;   /* neiskorišteni ostatak u us (< 1000) */
volatile uint32_t g_ovf      = 0;   /* broj prelijevanja Timera0 */

} /* anonimni namespace */

ISR(TIMER0_OVF_vect) {
    g_ovf++;

    /* svako prelijevanje = 256 * 4 us = 1024 us */
    g_us_rest += 1024;
    while (g_us_rest >= 1000) {
        g_us_rest -= 1000;
        g_ms++;
    }
}

namespace Clock {

void init() {
    TCCR0A = 0;                             /* normalan mod */
    TCCR0B = (1 << CS01) | (1 << CS00);     /* predskalar 64 -> 4 us/tick */
    TCNT0  = 0;
    TIMSK0 = (1 << TOIE0);                  /* prekid na prelijevanje */
    sei();
}

uint32_t millis() {
    uint32_t v;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        v = g_ms;
    }
    return v;
}

uint32_t micros() {
    uint32_t ovf;
    uint8_t  cnt;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ovf = g_ovf;
        cnt = TCNT0;
        /* Ako je prelijevanje čekalo dok su prekidi bili zabranjeni,
         * ISR ga još nije prebrojao — dodaj ga ručno. */
        if ((TIFR0 & (1 << TOV0)) && cnt < 255) {
            ovf++;
        }
    }
    return ((ovf << 8) + cnt) * 4UL;
}

} /* namespace Clock */
