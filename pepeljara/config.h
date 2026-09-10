/*
 * config.h — centralna konfiguracija projekta "Pametna pepeljara"
 *
 * Mapiranje Arduino Nano oznaka -> ATmega328P registri:
 *   D2 = PD2   D3 = PD3   D4 = PD4   D6 = PD6
 *   D7 = PD7   D9 = PB1 (OC1A)       A4 = PC4 (SDA)   A5 = PC5 (SCL)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>
#include <stdint.h>


#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* ------------------------------------------------------------------ */
/*  Pinovi (prema shemi spajanja)                                      */
/* ------------------------------------------------------------------ */

/* Gumb — D2, spojen na GND, koristi interni pull-up */
#define BTN_DDR   DDRD
#define BTN_PORT  PORTD
#define BTN_PINR  PIND
#define BTN_BIT   PD2

/* HX711 (vaga za upaljač) — DT na D3, SCK na D4 */
#define HX_DT_DDR   DDRD
#define HX_DT_PORT  PORTD
#define HX_DT_PINR  PIND
#define HX_DT_BIT   PD3

#define HX_SCK_DDR  DDRD
#define HX_SCK_PORT PORTD
#define HX_SCK_BIT  PD4

/* HC-SR04 (razina pepela) — Trig na D6, Echo na D7 */
#define US_TRIG_DDR  DDRD
#define US_TRIG_PORT PORTD
#define US_TRIG_BIT  PD6

#define US_ECHO_DDR  DDRD
#define US_ECHO_PORT PORTD
#define US_ECHO_PINR PIND
#define US_ECHO_BIT  PD7

/* SG90 servo — D9 = PB1 = OC1A (hardverski PWM izlaz Timera1) */
#define SERVO_DDR  DDRB
#define SERVO_PORT PORTB
#define SERVO_BIT  PB1

#define LCD_I2C_ADDR 0x27


/* Kutovi u stupnjevima. Podesi prema stvarnoj montaži poluge.
 * Mogu se mijenjati u letu naredbama 'sc' / 'so' i spremiti u EEPROM. */
#define SERVO_CLOSED_DEG_DEF 10
#define SERVO_OPEN_DEG_DEF   95

/* Koliko dugo vrata ostaju otvorena (ms) */
#define DOOR_OPEN_HOLD_MS 1200

/* Brzina gibanja: 1 stupanj svakih SERVO_STEP_MS ms.
 * Namjerno sporo — TPS613222A je niskostrujni boost pretvarač pa
 * nagli trzaj servo motora može srušiti 5V sabirnicu. */
#define SERVO_STEP_MS 12

/* Koliko dugo nakon zatvaranja čekamo da se pepeo slegne prije
 * nego opet vjerujemo mjerenju udaljenosti (ms) */
#define ASH_SETTLE_MS 1500


/* Udaljenost senzora (strop unutrašnjosti) do dna PRAZNOG spremnika.
 * Izvedeno iz tvojih STL-ova: baza_za_pepeo 92 mm + top_bottom 47 mm.
 * OBAVEZNO izmjeri stvarnu vrijednost naredbom 'de' nakon montaže. */
#define DIST_EMPTY_CM_DEF 13.5f

/* Udaljenost do vrha hrpe pepela kad spremnik smatramo PUNIM. */
#define DIST_FULL_CM_DEF 4.5f


#define CAL_FACTOR_DEF 420.0f

/* Masa upaljača kad je prazan / pun (grami).
 * Postavlja se naredbama 'e' i 'f' sa stvarnim upaljačima na vagi. */
#define LIGHTER_EMPTY_G_DEF 16.0f
#define LIGHTER_FULL_G_DEF  27.0f

/* Ispod ove mase smatramo da upaljač uopće nije na vagi */
#define LIGHTER_PRESENT_G 4.0f

/* Postotak iznad kojeg LCD upozorava da treba isprazniti pepeljaru */
#define ASH_WARN_PCT 90


#define UART_BAUD     9600UL
#define BTN_DEBOUNCE_MS   40
#define BTN_LONGPRESS_MS 1500   /* dugi pritisak = tariranje vage */

#define LCD_REFRESH_MS   350
#define US_PERIOD_MS     120    /* jedno mjerenje udaljenosti svakih 120 ms */

/* Mali pomoćni makroi za rad s pinovima */
#define PIN_HIGH(port, bit)  ((port) |=  (1 << (bit)))
#define PIN_LOW(port, bit)   ((port) &= ~(1 << (bit)))
#define PIN_READ(pinr, bit)  (((pinr) & (1 << (bit))) != 0)
#define PIN_OUT(ddr, bit)    ((ddr)  |=  (1 << (bit)))
#define PIN_IN(ddr, bit)     ((ddr)  &= ~(1 << (bit)))

#endif /* CONFIG_H */
