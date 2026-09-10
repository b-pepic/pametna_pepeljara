/*
 * storage.h — trajna pohrana kalibracije u EEPROM (avr-libc, bez Arduina).
 *
 * Magični broj na početku strukture razlikuje "prazan/nov" EEPROM od
 * valjanih podataka. Ako se ne poklapa, učitavaju se tvorničke vrijednosti
 * iz config.h.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

struct Config {
    uint16_t magic;

    /* vaga */
    int32_t  tareOffset;      /* sirovo očitanje prazne vage */
    float    calFactor;       /* sirovih brojeva po gramu */
    float    lighterEmptyG;   /* masa praznog upaljača */
    float    lighterFullG;    /* masa punog upaljača */

    /* pepeo */
    float    distEmptyCm;     /* udaljenost do dna praznog spremnika */
    float    distFullCm;      /* udaljenost kad je spremnik pun */

    /* servo */
    uint8_t  servoClosedDeg;
    uint8_t  servoOpenDeg;

    uint16_t crc;             /* jednostavna provjera ispravnosti */
};

namespace Storage {

void load(Config& cfg);       /* učitava iz EEPROM-a ili postavlja defaulte */
void save(const Config& cfg);
void defaults(Config& cfg);

} /* namespace Storage */

#endif /* STORAGE_H */
