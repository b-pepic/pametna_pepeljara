#include "config.h"
#include "storage.h"

#include <avr/eeprom.h>
#include <string.h>

namespace {

const uint16_t MAGIC = 0xA5C3;

/* Rezervirano mjesto u EEPROM-u (adresa 0) */
Config EEMEM g_eeConfig;

uint16_t crc16(const uint8_t* data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 1) ? (uint16_t)((crc >> 1) ^ 0xA001) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

uint16_t computeCrc(const Config& c) {
    /* CRC pokriva sve osim samog polja crc na kraju */
    return crc16((const uint8_t*)&c, (uint16_t)(sizeof(Config) - sizeof(uint16_t)));
}

} /* anonimni namespace */

namespace Storage {

void defaults(Config& cfg) {
    cfg.magic          = MAGIC;
    cfg.tareOffset     = 0;
    cfg.calFactor      = CAL_FACTOR_DEF;
    cfg.lighterEmptyG  = LIGHTER_EMPTY_G_DEF;
    cfg.lighterFullG   = LIGHTER_FULL_G_DEF;
    cfg.distEmptyCm    = DIST_EMPTY_CM_DEF;
    cfg.distFullCm     = DIST_FULL_CM_DEF;
    cfg.servoClosedDeg = SERVO_CLOSED_DEG_DEF;
    cfg.servoOpenDeg   = SERVO_OPEN_DEG_DEF;
    cfg.crc            = computeCrc(cfg);
}

void load(Config& cfg) {
    eeprom_read_block(&cfg, &g_eeConfig, sizeof(Config));

    if (cfg.magic != MAGIC || cfg.crc != computeCrc(cfg)) {
        defaults(cfg);
    }
}

void save(const Config& cfg) {
    Config tmp = cfg;
    tmp.magic = MAGIC;
    tmp.crc   = computeCrc(tmp);
    /* update_block piše samo bajtove koji se stvarno mijenjaju —
     * čuva ograničeni broj ciklusa upisa EEPROM-a */
    eeprom_update_block(&tmp, &g_eeConfig, sizeof(Config));
}

} /* namespace Storage */
