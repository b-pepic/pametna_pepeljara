/*
 * i2c.h — minimalni I2C master preko hardverske TWI jedinice (PC4/PC5).
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

namespace I2C {

void init();                                  /* 100 kHz */
bool writeByte(uint8_t addr7, uint8_t data);  /* true = ACK primljen */
bool probe(uint8_t addr7);                    /* postoji li uređaj na adresi */

} /* namespace I2C */

#endif /* I2C_H */
