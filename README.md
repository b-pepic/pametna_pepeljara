[README.md](https://github.com/user-attachments/files/32078228/README.md)
# Pametna pepeljara 



| Podsustav | Kako je riješeno |
|---|---|
| `millis()` / `micros()` | Timer0, predskalar 64, prekid na prelijevanju |
| Servo PWM | Timer1, Fast PWM mod 14, hardverski izlaz OC1A (D9) |
| LCD 16x2 | vlastiti TWI (I2C) master + HD44780 driver preko PCF8574 |
| HX711 | bit-bang 25 impulsa, prekidi zabranjeni tijekom očitanja |
| HC-SR04 | okidni impuls + mjerenje odjeka preko `micros()`, medijan filtar |
| Serijska konzola | UART s prekidom na prijemu, tekstovi u flashu (`PSTR`) |
| Kalibracija | EEPROM (`avr/eeprom.h`) s magičnim brojem i CRC16 |



---

## Prevođenje i upload

```bash
sudo apt install gcc-avr avr-libc binutils-avr avrdude

make                              # prevedi + prikaži zauzeće
make flash PORT=/dev/ttyUSB0      # upiši na ploču
make monitor PORT=/dev/ttyUSB0    # serijski terminal, 9600
```



```bash
make flash PORT=/dev/ttyUSB0 BAUD=57600
```



---

## Pinovi 

| Komponenta | Pin | AVR |
|---|---|---|
| Gumb | D2 | PD2, interni pull-up, druga nožica na GND |
| HX711 DT | D3 | PD3 |
| HX711 SCK | D4 | PD4 |
| HC-SR04 Trig | D6 | PD6 |
| HC-SR04 Echo | D7 | PD7 |
| SG90 signal | D9 | PB1 / OC1A |
| LCD SDA | A4 | PC4 |
| LCD SCL | A5 | PC5 |


---

## Kako radi



**Vrata.** Kratki pritisak gumba pokreće servo iz zatvorenog u otvoreni
kut, po jedan stupanj svakih 12 ms. Ostaje otvoreno 1.2 s, pa se vraća i
servo se *otkvači* s PWM-a. Mjerenje pepela je zamrznuto dok se vrata miču
i još 1.5 s poslije, da senzor ne mjeri zaklopku ili oblak pepela.

**Plin.** HX711 mjeri masu upaljača. Postotak je linearna interpolacija
između mase praznog i punog upaljača. Ispod 4 g program smatra da upaljač
uopće nije na vagi i piše `(nema)`.

**LCD.**
```
Pepeo:  72% ███-
Plin:   45% 22.4g
```
Iznad 90% gornji red naizmjence trepće `!! ISPRAZNI !!`.

---

## Kalibracija (serijski terminal, 9600)



1. `t` — makni sve s vage, tariraj
2. `c 100` — stavi uteg poznate mase (npr. 100 g) i utipkaj njegovu masu
3. `e` — stavi **prazan** upaljač na vagu
4. `f` — stavi **pun** upaljač na vagu
5. `de` — prazan spremnik montiran
6. `df` — spremnik napunjen do razine koju smatramo punom
7. `sc 10` / `so 95` — podesi kutove zaklopke dok mehanika ne sjedne

Sve se odmah sprema u EEPROM i preživljava reset i odspajanje baterije.
`p` ispisuje trenutno stanje, `r` vraća tvorničke vrijednosti.
`i` skenira I2C — ako LCD ostane prazan, vjerojatno je na 0x3F umjesto
0x27, promijeni `LCD_I2C_ADDR` u `config.h`.

Dugi pritisak gumba (1.5 s) tarira vagu bez računala.

---



## Struktura projekta

```
src/config.h       svi pinovi i kalibracijske konstante
src/clock.*        Timer0 -> millis/micros
src/i2c.*          TWI master s timeoutima
src/lcd.*          HD44780 preko PCF8574
src/servo.*        Timer1 hardverski PWM
src/ultrasonic.*   HC-SR04 + medijan filtar
src/hx711.*        24-bitni ADC, neblokirajuće čitanje
src/uart.*         serijska konzola
src/storage.*      EEPROM kalibracija
src/main.cpp       logika, stanja vrata, prikaz, naredbe
Makefile
```
