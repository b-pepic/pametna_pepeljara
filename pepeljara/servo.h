/*
 * servo.h — SG90 na D9 (PB1 = OC1A).
 *
 * Timer1 radi u Fast PWM modu 14 (TOP = ICR1), predskalar 8:
 *   tick   = 8 / 16 MHz = 0.5 us
 *   ICR1   = 40000 tickova = 20 ms  -> 50 Hz, kako servo i traži
 *   OCR1A  = širina impulsa u tickovima (npr. 1500 us = 3000)
 *
 * detach() gasi izlaz kad se servo ne kreće
 */

#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

namespace Servo {

void init();                    /* postavlja Timer1, izlaz je isprva ugašen */
void attach();                  /* uključuje PWM izlaz na PB1 */
void detach();                  /* gasi PWM izlaz, pin ide u nulu */
bool attached();

void writeMicros(uint16_t us);  /* ograničeno na 500..2500 us */
void writeDegrees(uint8_t deg); /* 0..180 -> 500..2500 us */

} /* namespace Servo */

#endif /* SERVO_H */
