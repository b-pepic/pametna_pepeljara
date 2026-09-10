/*
 * main.cpp — Pametna pepeljara
 *
 * Funkcije:
 *   - HC-SR04 sa stropa unutrašnjosti mjeri udaljenost do hrpe pepela i
 *     iz toga računa popunjenost spremnika u postocima
 *   - HX711 + load ćelija mjere masu upaljača i iz nje procjenjuju
 *     koliko je plina ostalo
 *   - SG90 servo otvara "trap door" na pritisak gumba pa pepeo propada
 *     u izvlačivi spremnik
 *   - LCD 16x2 prikazuje obje brojke
 *   - serijska konzola (9600) služi za kalibraciju, sve se pamti u EEPROM
 *
 * Glavna petlja nigdje ne stoji na delay-u; sve ide preko Clock::millis().
 */

#include "config.h"
#include "clock.h"
#include "i2c.h"
#include "lcd.h"
#include "servo.h"
#include "ultrasonic.h"
#include "hx711.h"
#include "uart.h"
#include "storage.h"

#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Globalni objekti                                                    */
/* ------------------------------------------------------------------ */

static Lcd        lcd(LCD_I2C_ADDR);
static Ultrasonic sonar;
static Hx711      scale;
static Config     cfg;

/* ------------------------------------------------------------------ */
/*  Stanje vrata (trap door)                                            */
/* ------------------------------------------------------------------ */

enum class DoorState : uint8_t { Closed, Opening, Holding, Closing };

static DoorState g_door        = DoorState::Closed;
static uint8_t   g_servoDeg    = SERVO_CLOSED_DEG_DEF;
static uint32_t  g_doorTimer   = 0;
static uint32_t  g_settleUntil = 0;

/* ------------------------------------------------------------------ */
/*  Izmjerene vrijednosti                                               */
/* ------------------------------------------------------------------ */

static int8_t g_ashPct  = -1;    /* -1 = još nema valjanog mjerenja */
static int8_t g_gasPct  = -1;
static float  g_weightG = 0.0f;
static bool   g_lighterPresent = false;

/* ------------------------------------------------------------------ */
/*  Pomoćne funkcije                                                    */
/* ------------------------------------------------------------------ */

static int8_t clampPct(float v) {
    if (v < 0.0f)   return 0;
    if (v > 100.0f) return 100;
    return (int8_t)(v + 0.5f);
}

static void startDoor() {
    if (g_door != DoorState::Closed) return;   /* već je u pokretu */
    Servo::attach();
    g_door = DoorState::Opening;
    g_doorTimer = Clock::millis();
    UPRINTLN("Vrata: otvaram");
}

/* Pomiče servo po jedan stupanj prema cilju. Vraća true kad je stigao. */
static bool stepServoToward(uint8_t target) {
    if (!Clock::elapsed(g_doorTimer, SERVO_STEP_MS)) {
        return g_servoDeg == target;
    }
    g_doorTimer = Clock::millis();

    if (g_servoDeg < target)      g_servoDeg++;
    else if (g_servoDeg > target) g_servoDeg--;

    Servo::writeDegrees(g_servoDeg);
    return g_servoDeg == target;
}

static void updateDoor() {
    switch (g_door) {

    case DoorState::Closed:
        break;

    case DoorState::Opening:
        if (stepServoToward(cfg.servoOpenDeg)) {
            g_door = DoorState::Holding;
            g_doorTimer = Clock::millis();
        }
        break;

    case DoorState::Holding:
        if (Clock::elapsed(g_doorTimer, DOOR_OPEN_HOLD_MS)) {
            g_door = DoorState::Closing;
            g_doorTimer = Clock::millis();
        }
        break;

    case DoorState::Closing:
        if (stepServoToward(cfg.servoClosedDeg)) {
            g_door = DoorState::Closed;
            Servo::detach();               /* prestani trošiti struju */
            g_settleUntil = Clock::millis() + ASH_SETTLE_MS;
            UPRINTLN("Vrata: zatvorena");
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Gumb na D2                                                          */
/* ------------------------------------------------------------------ */

static bool     g_btnStable   = false;   /* true = pritisnut */
static bool     g_btnLast     = false;
static uint32_t g_btnChanged  = 0;
static bool     g_longHandled = false;

static void tareScale();   /* deklaracija unaprijed */

static void updateButton() {
    /* pull-up: pritisnuto = logička nula */
    bool raw = !PIN_READ(BTN_PINR, BTN_BIT);

    if (raw != g_btnLast) {
        g_btnLast    = raw;
        g_btnChanged = Clock::millis();
    }

    if (Clock::elapsed(g_btnChanged, BTN_DEBOUNCE_MS) && raw != g_btnStable) {
        g_btnStable = raw;

        if (g_btnStable) {
            g_longHandled = false;         /* početak pritiska */
        } else if (!g_longHandled) {
            startDoor();                   /* kratki pritisak */
        }
    }

    /* dugi pritisak = tariranje vage */
    if (g_btnStable && !g_longHandled &&
        Clock::elapsed(g_btnChanged, BTN_LONGPRESS_MS)) {
        g_longHandled = true;
        tareScale();
    }
}

/* ------------------------------------------------------------------ */
/*  Mjerenja                                                            */
/* ------------------------------------------------------------------ */

static void updateAsh() {
    /* Ne mjeri dok se vrata miču ni dok se pepeo sliježe —
     * senzor bi gledao u zaklopku umjesto u hrpu. */
    if (g_door != DoorState::Closed) return;
    if ((int32_t)(Clock::millis() - g_settleUntil) < 0) return;

    static uint32_t last = 0;
    if (!Clock::elapsed(last, US_PERIOD_MS)) return;
    last = Clock::millis();

    if (!sonar.update() || !sonar.valid()) return;

    float span = cfg.distEmptyCm - cfg.distFullCm;
    if (span <= 0.1f) return;              /* neispravna kalibracija */

    g_ashPct = clampPct((cfg.distEmptyCm - sonar.distance()) / span * 100.0f);
}

static void updateScale() {
    if (!scale.update()) return;

    g_weightG = scale.grams(cfg.tareOffset, cfg.calFactor);
    g_lighterPresent = (g_weightG > LIGHTER_PRESENT_G);

    if (!g_lighterPresent) {
        g_gasPct = -1;
        return;
    }

    float span = cfg.lighterFullG - cfg.lighterEmptyG;
    if (span <= 0.1f) return;

    g_gasPct = clampPct((g_weightG - cfg.lighterEmptyG) / span * 100.0f);
}

/* ------------------------------------------------------------------ */
/*  Prikaz na LCD-u                                                     */
/* ------------------------------------------------------------------ */

static void drawBar(int8_t pct, uint8_t cells) {
    if (pct < 0) pct = 0;
    uint8_t filled = (uint8_t)(((uint16_t)pct * cells + 50) / 100);
    for (uint8_t i = 0; i < cells; i++) {
        lcd.print(i < filled ? (char)0xFF : '-');
    }
}

static void updateDisplay() {
    static uint32_t last = 0;
    static bool     blink = false;

    if (!Clock::elapsed(last, LCD_REFRESH_MS)) return;
    last = Clock::millis();
    blink = !blink;

    char line[17];

    /* --- gornji red: pepeo --- */
    lcd.setCursor(0, 0);

    if (g_ashPct >= ASH_WARN_PCT && blink) {
        lcd.printPadded("!! ISPRAZNI !!", 16);
    } else if (g_ashPct < 0) {
        lcd.printPadded("Pepeo:  ---", 16);
    } else {
        snprintf(line, sizeof(line), "Pepeo:%4d%% ", (int)g_ashPct);
        lcd.print(line);
        drawBar(g_ashPct, 4);
    }

    /* --- donji red: plin u upaljaču --- */
    lcd.setCursor(0, 1);

    if (!g_lighterPresent) {
        lcd.printPadded("Plin:  -- (nema)", 16);
    } else {
        char w[8];
        dtostrf((double)g_weightG, 4, 1, w);
        snprintf(line, sizeof(line), "Plin:%4d%% %sg",
                 (int)(g_gasPct < 0 ? 0 : g_gasPct), w);
        lcd.printPadded(line, 16);
    }
}

/* ------------------------------------------------------------------ */
/*  Kalibracija i serijske naredbe                                      */
/* ------------------------------------------------------------------ */

static void tareScale() {
    UPRINTLN("Tariranje... makni sve s vage");
    int32_t avg;
    if (scale.averageBlocking(10, avg)) {
        cfg.tareOffset = avg;
        Storage::save(cfg);
        UPRINT("Nova nula: ");
        Uart::printInt(cfg.tareOffset);
        UPRINTLN("");
    } else {
        UPRINTLN("GRESKA: HX711 se ne javlja");
    }
}

static void printHelp() {
    UPRINTLN("");
    UPRINTLN("=== Pametna pepeljara — naredbe ===");
    UPRINTLN("p        ispis trenutnog stanja");
    UPRINTLN("t        tariraj vagu (prazna vaga)");
    UPRINTLN("c <g>    kalibriraj utegom poznate mase, npr. 'c 100'");
    UPRINTLN("e        zapamti masu na vagi kao PRAZAN upaljac");
    UPRINTLN("f        zapamti masu na vagi kao PUN upaljac");
    UPRINTLN("de       zapamti udaljenost kao PRAZAN spremnik");
    UPRINTLN("df       zapamti udaljenost kao PUN spremnik");
    UPRINTLN("sc <st>  kut zatvorenih vrata");
    UPRINTLN("so <st>  kut otvorenih vrata");
    UPRINTLN("o        otvori vrata");
    UPRINTLN("i        skeniraj I2C sabirnicu");
    UPRINTLN("r        vrati tvornicke postavke");
    UPRINTLN("?        ovaj popis");
    UPRINTLN("");
}

static void printStatus() {
    UPRINT("Pepeo: ");
    if (g_ashPct < 0) UPRINT("---");
    else              Uart::printInt(g_ashPct);
    UPRINT("%  udaljenost=");
    Uart::printFloat(sonar.distance(), 1);
    UPRINT("cm  [prazno=");
    Uart::printFloat(cfg.distEmptyCm, 1);
    UPRINT(" puno=");
    Uart::printFloat(cfg.distFullCm, 1);
    UPRINTLN("]");

    UPRINT("Vaga: ");
    Uart::printFloat(g_weightG, 2);
    UPRINT("g  sirovo=");
    Uart::printInt(scale.rawFiltered());
    UPRINT("  nula=");
    Uart::printInt(cfg.tareOffset);
    UPRINT("  faktor=");
    Uart::printFloat(cfg.calFactor, 2);
    UPRINTLN("");

    UPRINT("Plin: ");
    if (g_gasPct < 0) UPRINT("---");
    else              Uart::printInt(g_gasPct);
    UPRINT("%  [prazan=");
    Uart::printFloat(cfg.lighterEmptyG, 1);
    UPRINT("g pun=");
    Uart::printFloat(cfg.lighterFullG, 1);
    UPRINTLN("g]");

    UPRINT("Servo: zatvoreno=");
    Uart::printInt(cfg.servoClosedDeg);
    UPRINT(" otvoreno=");
    Uart::printInt(cfg.servoOpenDeg);
    UPRINTLN("");
}

static void scanI2C() {
    UPRINTLN("Skeniram I2C...");
    uint8_t found = 0;
    for (uint8_t a = 1; a < 127; a++) {
        if (I2C::probe(a)) {
            UPRINT("  uredjaj na 0x");
            char b[4];
            snprintf(b, sizeof(b), "%02X", a);
            Uart::println(b);
            found++;
        }
    }
    if (!found) UPRINTLN("  nista pronadjeno — provjeri SDA/SCL i napajanje");
}

static void handleCommand(const char* cmd) {
    if (cmd[0] == '\0') return;

    /* dvoslovne naredbe prvo */
    if (strncmp(cmd, "de", 2) == 0) {
        if (sonar.valid()) {
            cfg.distEmptyCm = sonar.distance();
            Storage::save(cfg);
            UPRINT("Prazan spremnik = ");
            Uart::printFloat(cfg.distEmptyCm, 1);
            UPRINTLN(" cm");
        } else {
            UPRINTLN("GRESKA: nema valjanog mjerenja");
        }
        return;
    }
    if (strncmp(cmd, "df", 2) == 0) {
        if (sonar.valid()) {
            cfg.distFullCm = sonar.distance();
            Storage::save(cfg);
            UPRINT("Pun spremnik = ");
            Uart::printFloat(cfg.distFullCm, 1);
            UPRINTLN(" cm");
        } else {
            UPRINTLN("GRESKA: nema valjanog mjerenja");
        }
        return;
    }
    if (strncmp(cmd, "sc", 2) == 0) {
        cfg.servoClosedDeg = (uint8_t)atoi(cmd + 2);
        Storage::save(cfg);
        Servo::attach();
        Servo::writeDegrees(cfg.servoClosedDeg);
        g_servoDeg = cfg.servoClosedDeg;
        UPRINTLN("Kut zatvoreno postavljen");
        return;
    }
    if (strncmp(cmd, "so", 2) == 0) {
        cfg.servoOpenDeg = (uint8_t)atoi(cmd + 2);
        Storage::save(cfg);
        UPRINTLN("Kut otvoreno postavljen");
        return;
    }

    switch (cmd[0]) {

    case 'p': printStatus(); break;
    case '?': printHelp();   break;
    case 'i': scanI2C();     break;
    case 'o': startDoor();   break;
    case 't': tareScale();   break;

    case 'c': {
        float known = (float)atof(cmd + 1);
        if (known <= 0.0f) {
            UPRINTLN("Upotreba: c <masa u gramima>, npr. 'c 100'");
            break;
        }
        UPRINTLN("Drzi uteg na vagi...");
        int32_t avg;
        if (scale.averageBlocking(10, avg)) {
            cfg.calFactor = (float)(avg - cfg.tareOffset) / known;
            Storage::save(cfg);
            UPRINT("Novi faktor: ");
            Uart::printFloat(cfg.calFactor, 3);
            UPRINTLN(" brojeva po gramu");
        } else {
            UPRINTLN("GRESKA: HX711 se ne javlja");
        }
        break;
    }

    case 'e':
        cfg.lighterEmptyG = g_weightG;
        Storage::save(cfg);
        UPRINT("Prazan upaljac = ");
        Uart::printFloat(cfg.lighterEmptyG, 2);
        UPRINTLN(" g");
        break;

    case 'f':
        cfg.lighterFullG = g_weightG;
        Storage::save(cfg);
        UPRINT("Pun upaljac = ");
        Uart::printFloat(cfg.lighterFullG, 2);
        UPRINTLN(" g");
        break;

    case 'r':
        Storage::defaults(cfg);
        Storage::save(cfg);
        UPRINTLN("Vracene tvornicke postavke");
        break;

    default:
        UPRINTLN("Nepoznata naredba — utipkaj '?'");
        break;
    }
}

static void updateSerial() {
    char buf[32];
    if (Uart::readLine(buf, sizeof(buf))) {
        handleCommand(buf);
    }
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main() {
    /* Gumb: ulaz s internim pull-upom (shema: druga nožica na GND) */
    PIN_IN(BTN_DDR, BTN_BIT);
    PIN_HIGH(BTN_PORT, BTN_BIT);

    Clock::init();
    Uart::init();
    I2C::init();
    sonar.init();
    scale.init();
    Servo::init();

    Storage::load(cfg);
    g_servoDeg = cfg.servoClosedDeg;

    lcd.init();
    lcd.backlight(true);
    lcd.setCursor(0, 0);
    lcd.printPadded("Pametna", 16);
    lcd.setCursor(0, 1);
    lcd.printPadded("pepeljara", 16);

    /* Servo jednom postavi u zatvoreni položaj pa ga otkvači */
    Servo::attach();
    Servo::writeDegrees(g_servoDeg);
    _delay_ms(600);
    Servo::detach();

    UPRINTLN("");
    UPRINTLN("Pametna pepeljara — spremna. '?' za popis naredbi.");

    _delay_ms(800);
    lcd.clear();

    for (;;) {
        updateButton();
        updateDoor();
        updateAsh();
        updateScale();
        updateDisplay();
        updateSerial();
    }
}
