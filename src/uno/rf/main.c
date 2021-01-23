
#define ARDUINO_MAIN
#include "../shared/device_comms.h"
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/rf.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "pins_arduino.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
Controller_t controller;
Controller_t previousController;
Configuration_t newConfig;
long lastPoll = 0;
volatile bool send_message = false;
__attribute__((section(".rfrecv"))) uint32_t rftxID = 0x8581f888;
__attribute__((section(".rfrecv"))) uint32_t rfrxID = 0xc2292dde;
// TODO: we can use millis here to work out if a controller has been out of use
// for several minutes, and to then go into a sleep mode
int main(void) {
  loadConfig();
  sei();
  setupMicrosTimer();
  Serial_Init(115200, true);
  config.main.inputType = DIRECT;
  config.pins.a = 19;
  initInputs();
  initReports();
  // id = generate_crc32();
  initRF(0, pgm_read_dword(&rftxID), pgm_read_dword(&rfrxID));
  while (true) {
    if (millis() - lastPoll > config.main.pollRate && rf_interrupt) {
      tickInputs(&controller);
      tickLEDs(&controller);
      controller.l_x = rand();
      if (memcmp(&controller, &previousController, sizeof(Controller_t)) != 0) {
        lastPoll = millis();
        // We got an ack of some sort, send one last packet and then jump to
        // config mode
        if (tickRFTX(&controller)) {
          nrf24_flush_rx();
          initRF(false, pgm_read_dword(&rftxID), pgm_read_dword(&rfrxID));
          _delay_us(500);
          break;
        }
        memcpy(&previousController, &controller, sizeof(Controller_t));
      }
    }
  }
  Serial_SendByte('A');
  Serial_SendByte(nrf24_getStatus());
  uint8_t data[sizeof(XInput_Data_t)] = {};
  while (true) {
    if (rf_interrupt) {
      Serial_SendByte(nrf24_getStatus());
      if (tickRFInput((Controller_t *)data)) {
        for (int i = 0; i < sizeof(data); i++) { Serial_SendByte(data[i]); }
      }
    }
  }
}