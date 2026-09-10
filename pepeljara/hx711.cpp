#include "config.h"
#include "hx711.h"
#include "clock.h"

#include <util/delay.h>
#include <util/atomic.h>

void Hx711::init() {
    PIN_IN(HX_DT_DDR, HX_DT_BIT);
    PIN_HIGH(HX_DT_PORT, HX_DT_BIT);      /* blagi pull-up */

    PIN_OUT(HX_SCK_DDR, HX_SCK_BIT);
    PIN_LOW(HX_SCK_PORT, HX_SCK_BIT);     /* SCK nisko = čip radi */
}

bool Hx711::ready() const {
    return !PIN_READ(HX_DT_PINR, HX_DT_BIT);
}

bool Hx711::readRaw(int32_t& out) {
    if (!ready()) {
        return false;
    }

    uint32_t value = 0;

    /* Cijelo očitanje traje ~100 us; prekidi su zabranjeni da nam ISR
     * ne razvuče impuls na SCK preko dopuštenih 60 us. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (uint8_t i = 0; i < 24; i++) {
            PIN_HIGH(HX_SCK_PORT, HX_SCK_BIT);
            _delay_us(1);
            value = (value << 1) | (PIN_READ(HX_DT_PINR, HX_DT_BIT) ? 1UL : 0UL);
            PIN_LOW(HX_SCK_PORT, HX_SCK_BIT);
            _delay_us(1);
        }

        /* 25. impuls: kanal A, pojačanje 128 */
        PIN_HIGH(HX_SCK_PORT, HX_SCK_BIT);
        _delay_us(1);
        PIN_LOW(HX_SCK_PORT, HX_SCK_BIT);
        _delay_us(1);
    }

    /* Predznak: 24-bitni dvojni komplement -> 32 bita */
    if (value & 0x800000UL) {
        value |= 0xFF000000UL;
    }
    out = (int32_t)value;
    return true;
}

bool Hx711::update() {
    int32_t raw;
    if (!readRaw(raw)) {
        return false;
    }

    if (!m_has) {
        m_filtered = raw;
        m_has = true;
    } else {
        /* Eksponencijalni filtar s faktorom 1/4 — dovoljno miran prikaz
         * bez cjelobrojnog dijeljenja s ostatkom koji bi se gubio. */
        m_filtered = m_filtered + (raw - m_filtered) / 4;
    }
    return true;
}

bool Hx711::averageBlocking(uint8_t samples, int32_t& out) {
    if (samples == 0) return false;

    int64_t  sum = 0;
    uint8_t  got = 0;
    uint32_t t0  = Clock::millis();

    while (got < samples) {
        int32_t raw;
        if (readRaw(raw)) {
            sum += raw;
            got++;
        }
        /* HX711 daje ~10 uzoraka/s; 200 ms po uzorku je dovoljno */
        if (Clock::elapsed(t0, (uint32_t)samples * 200UL + 500UL)) {
            break;
        }
    }

    if (got == 0) return false;
    out = (int32_t)(sum / got);
    return true;
}
