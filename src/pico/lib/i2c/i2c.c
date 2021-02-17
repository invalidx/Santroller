/*
  twi.c - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified 2019 by IanSC to return after a timeout period
*/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// === MODIFIED ===
#include "hardware/i2c.h"
#include "i2c/i2c.h"
#include "util/util.h"
#include "timer/timer.h"

// === MODIFIED ===
static uint16_t TIMEOUT = 100;

/*
 * Function twi_init
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void twi_init(void) { i2c_init(i2c0, TWI_FREQ); }

/*
 * Function twi_disable
 * Desc     disables twi pins
 * Input    none
 * Output   none
 */
void twi_disable(void) { i2c_deinit(i2c0); }

/*
 * Function twi_readFrom
 * Desc     attempts to become twi bus master and read a
 *          series of bytes from a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */
// === MODIFIED ===
uint8_t twi_readFrom(uint8_t address, uint8_t *data, uint8_t length,
                     uint8_t sendStop) {

  return i2c_read_timeout_us(i2c0, address, data, length, !sendStop, TIMEOUT);
}

/*
 * Function twi_writeTo
 * Desc     attempts to become twi bus master and write a
 *          series of bytes to a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 *          sendStop: boolean indicating whether or not to send a stop at the
 * end Output   0 .. success 1 .. length too long for buffer 2 .. address send,
 * NACK received 3 .. data send, NACK received 4 .. other twi error (lost bus
 * arbitration, bus error, ..)
 */
uint8_t twi_writeTo(uint8_t address, uint8_t *data, uint8_t length,
                    uint8_t wait, uint8_t sendStop) {

  return i2c_write_timeout_us(i2c0, address, data, length, !sendStop, TIMEOUT);
}

bool twi_readFromPointerSlow(uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data) {
  uint8_t ret = !twi_writeTo(address, &pointer, 1, true, true);
  if (!ret) return ret;
  _delay_us(175);
  return twi_readFrom(address, data, length, true);
}
bool twi_readFromPointer(uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data) {
  return !twi_writeTo(address, &pointer, 1, true, true) &&
         twi_readFrom(address, data, length, true);
}
bool twi_writeSingleToPointer(uint8_t address, uint8_t pointer, uint8_t data) {
  return twi_writeToPointer(address, pointer, 1, &data);
}
bool twi_writeToPointer(uint8_t address, uint8_t pointer, uint8_t length,
                        uint8_t *data) {
  uint8_t data2[length + 1];
  data2[0] = pointer;
  memcpy(data2 + 1, data, length);

  return !twi_writeTo(address, data2, length + 1, true, true);
}

// === ADDED/MODIFIED ===
void twi_setTimeout(uint16_t timeout) { TIMEOUT = timeout; }