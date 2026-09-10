#include "config.h"
#include "i2c.h"

#include <util/twi.h>

namespace {

/* Broj praznih iteracija prije nego odustanemo od čekanja na TWINT.
 * Pri 100 kHz jedan bajt traje ~90 us, pa je ovo velikodušan timeout. */
const uint16_t TWI_TIMEOUT = 40000;

bool waitFlag() {
    uint16_t guard = TWI_TIMEOUT;
    while (!(TWCR & (1 << TWINT))) {
        if (--guard == 0) {
            return false;
        }
    }
    return true;
}

bool start() {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    if (!waitFlag()) return false;

    uint8_t st = TW_STATUS;
    return (st == TW_START || st == TW_REP_START);
}

void stop() {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    /* TWSTO se sam čisti kad stop uvjet završi; ne čekamo ga da ne
     * blokiramo petlju ako je sabirnica u kvaru. */
}

bool sendByte(uint8_t b, uint8_t expected) {
    TWDR = b;
    TWCR = (1 << TWINT) | (1 << TWEN);
    if (!waitFlag()) return false;
    return TW_STATUS == expected;
}

} /* anonimni namespace */

namespace I2C {

void init() {
    /* SCL = F_CPU / (16 + 2 * TWBR * predskalar)
     * TWBR = 72, predskalar 1  ->  16e6 / (16 + 144) = 100 kHz */
    TWSR = 0;
    TWBR = 72;
    TWCR = (1 << TWEN);
}

bool writeByte(uint8_t addr7, uint8_t data) {
    if (!start()) { stop(); return false; }

    if (!sendByte((uint8_t)(addr7 << 1) | TW_WRITE, TW_MT_SLA_ACK)) {
        stop();
        return false;
    }
    if (!sendByte(data, TW_MT_DATA_ACK)) {
        stop();
        return false;
    }
    stop();
    return true;
}

bool probe(uint8_t addr7) {
    if (!start()) { stop(); return false; }

    bool ok = sendByte((uint8_t)(addr7 << 1) | TW_WRITE, TW_MT_SLA_ACK);
    stop();
    return ok;
}

} /* namespace I2C */
