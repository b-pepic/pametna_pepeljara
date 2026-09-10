/*
 * hx711.h — 24-bitni ADC za load ćeliju. DT na D3, SCK na D4.
 *
 * Protokol: kad DT padne u nulu, podatak je spreman. Šalje se 25 impulsa
 * na SCK — prva 24 izbacuju bitove (MSB prvi), a 25. bira kanal A s
 * pojačanjem 128 za sljedeće mjerenje.
 *
 * Bitno: SCK ne smije ostati visok dulje od ~60 us 
 *
 * Čitanje je neblokirajuće: ready() se ispituje u glavnoj petlji i
 * čita se samo kad podatak stvarno postoji (HX711 daje ~10 uzoraka/s).
 */

#ifndef HX711_H
#define HX711_H

#include <stdint.h>

class Hx711 {
public:
    void init();

    bool ready() const;          /* DT je u nuli -> podatak spreman */
    bool readRaw(int32_t& out);  /* jedan uzorak, bez čekanja */

    /* Poziva se u svakom prolazu petlje; ako ima novog uzorka, ubacuje
     * ga u eksponencijalni filtar. Vraća true kad se stanje promijenilo. */
    bool update();

    int32_t rawFiltered() const { return m_filtered; }
    bool    hasData()     const { return m_has; }

    /* Masa u gramima prema zadanoj nuli i faktoru */
    float grams(int32_t tareOffset, float calFactor) const {
        if (!m_has || calFactor == 0.0f) return 0.0f;
        return (float)(m_filtered - tareOffset) / calFactor;
    }

    /* Blokirajući prosjek — koristi se samo pri kalibraciji */
    bool averageBlocking(uint8_t samples, int32_t& out);

private:
    int32_t m_filtered = 0;
    bool    m_has      = false;
};

#endif /* HX711_H */
