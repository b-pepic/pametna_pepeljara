/*
 * ultrasonic.h — HC-SR04, Trig na D6, Echo na D7.
 *
 * Jedno mjerenje je blokirajuće, ali traje najviše ~20 ms 
 */

#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>

class Ultrasonic {
public:
    void init();

    /* Jedno sirovo mjerenje. Vraća false ako nema odjeka. */
    bool measure(float& cm);

    /* Mjeri i ubacuje u filtar. Vraća true ako filtar ima valjanu
     * vrijednost koja se može pročitati preko distance(). */
    bool update();

    float distance() const { return m_filtered; }
    bool  valid()    const { return m_valid; }

private:
    static const uint8_t N = 5;

    float   m_buf[N] = {0};
    uint8_t m_count  = 0;
    uint8_t m_idx    = 0;
    float   m_filtered = 0.0f;
    bool    m_valid    = false;
};

#endif /* ULTRASONIC_H */
