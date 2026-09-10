#include "config.h"
#include "ultrasonic.h"
#include "clock.h"

#include <util/delay.h>

namespace {

/* Zvuk putuje do prepreke i natrag: 58 us po centimetru. */
const float    US_PER_CM   = 58.0f;
const uint32_t ECHO_TIMEOUT_US = 20000;   /* ~3.4 m */

} /* anonimni namespace */

void Ultrasonic::init() {
    PIN_OUT(US_TRIG_DDR, US_TRIG_BIT);
    PIN_LOW(US_TRIG_PORT, US_TRIG_BIT);

    PIN_IN(US_ECHO_DDR, US_ECHO_BIT);
    PIN_LOW(US_ECHO_PORT, US_ECHO_BIT);   /* bez pull-upa */
}

bool Ultrasonic::measure(float& cm) {
    /* Okidni impuls: 10 us visoko */
    PIN_LOW(US_TRIG_PORT, US_TRIG_BIT);
    _delay_us(4);
    PIN_HIGH(US_TRIG_PORT, US_TRIG_BIT);
    _delay_us(10);
    PIN_LOW(US_TRIG_PORT, US_TRIG_BIT);

    /* Čekaj početak odjeka */
    uint32_t t0 = Clock::micros();
    while (!PIN_READ(US_ECHO_PINR, US_ECHO_BIT)) {
        if (Clock::micros() - t0 > ECHO_TIMEOUT_US) {
            return false;
        }
    }

    /* Mjeri trajanje visokog stanja */
    uint32_t start = Clock::micros();
    while (PIN_READ(US_ECHO_PINR, US_ECHO_BIT)) {
        if (Clock::micros() - start > ECHO_TIMEOUT_US) {
            return false;
        }
    }
    uint32_t width = Clock::micros() - start;

    float d = (float)width / US_PER_CM;

    /* HC-SR04 ne mjeri pouzdano ispod ~2 cm */
    if (d < 2.0f || d > 350.0f) {
        return false;
    }
    cm = d;
    return true;
}

bool Ultrasonic::update() {
    float d;
    if (!measure(d)) {
        return m_valid;   /* zadrži staru vrijednost, ne ruši prikaz */
    }

    m_buf[m_idx] = d;
    m_idx = (uint8_t)((m_idx + 1) % N);
    if (m_count < N) m_count++;

    /* Medijan: kopiraj i sortiraj (N je 5, insertion sort je ovdje
     * brži i manji od bilo čega pametnijeg). */
    float tmp[N];
    for (uint8_t i = 0; i < m_count; i++) tmp[i] = m_buf[i];

    for (uint8_t i = 1; i < m_count; i++) {
        float key = tmp[i];
        int8_t j = (int8_t)i - 1;
        while (j >= 0 && tmp[j] > key) {
            tmp[j + 1] = tmp[j];
            j--;
        }
        tmp[j + 1] = key;
    }

    m_filtered = tmp[m_count / 2];
    m_valid = true;
    return true;
}
