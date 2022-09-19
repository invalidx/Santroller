#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// === MODIFIED ===
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "i2c/i2c.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "timer/timer.h"
#include "util/util.h"

#ifndef NDEBUG
#  include <stdio.h>
#endif

// === MODIFIED ===
static uint16_t TIMEOUT = 1000;

/*
 * Function twi_init
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void twi_init(bool fivetar) {
  i2c_init(i2c1, fivetar ? TWI_FREQ_5TAR : TWI_FREQ);
  gpio_set_function(PIN_WIRE_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_WIRE_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_WIRE_SDA);
  gpio_pull_up(PIN_WIRE_SCL);
}

/*
 * Function twi_disable
 * Desc     disables twi pins
 * Input    none
 * Output   none
 */
void twi_disable(void) { i2c_deinit(i2c1); }

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
bool twi_readFrom(uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop) {
  int ret = i2c_read_timeout_per_char_us(i2c1, address, data, length, !sendStop, 10);
  return ret > 0 ? ret : 0;
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
bool twi_writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop) {
  int ret = i2c_write_timeout_per_char_us(i2c1, address, data, length, !sendStop, 10);
  if (ret < 0) ret = i2c_write_timeout_per_char_us(i2c1, address, data, length, !sendStop, 10);
  return ret > 0;
}