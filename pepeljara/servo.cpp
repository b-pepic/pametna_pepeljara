#include "config.h"
#include "servo.h"

namespace {

const uint16_t US_MIN   = 500;
const uint16_t US_MAX   = 2500;
const uint16_t TOP      = 40000;   /* 20 ms pri 0.5 us po ticku */

bool     g_attached = false;
uint16_t g_lastUs   = 1500;

inline uint16_t usToTicks(uint16_t us) {
    return (uint16_t)(us * 2);     /* 1 us = 2 ticka */
}

} /* anonimni namespace */

namespace Servo {

void init() {
    PIN_OUT(SERVO_DDR, SERVO_BIT);
    PIN_LOW(SERVO_PORT, SERVO_BIT);

    /* Fast PWM, TOP = ICR1 (mod 14), predskalar 8.
     * COM1A1 se NE postavlja ovdje — izlaz uključuje tek attach(). */
    TCCR1A = (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1   = TOP;
    OCR1A  = usToTicks(g_lastUs);
}

void attach() {
    OCR1A   = usToTicks(g_lastUs);
    TCCR1A |= (1 << COM1A1);       /* neinvertirajući PWM na OC1A */
    g_attached = true;
}

void detach() {
    TCCR1A &= ~(1 << COM1A1);      /* otkvači pin od timera */
    PIN_LOW(SERVO_PORT, SERVO_BIT);
    g_attached = false;
}

bool attached() {
    return g_attached;
}

void writeMicros(uint16_t us) {
    if (us < US_MIN) us = US_MIN;
    if (us > US_MAX) us = US_MAX;
    g_lastUs = us;
    OCR1A = usToTicks(us);
}

void writeDegrees(uint8_t deg) {
    if (deg > 180) deg = 180;
    /* 0 st. = 500 us, 180 st. = 2500 us  ->  ~11.11 us po stupnju */
    writeMicros((uint16_t)(US_MIN + ((uint32_t)deg * (US_MAX - US_MIN)) / 180));
}

} /* namespace Servo */
