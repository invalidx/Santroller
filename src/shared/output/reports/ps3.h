#pragma once
#include "controller/controller.h"
#include <stdint.h>
// #include <avr/pgmspace.h>
#include "eeprom/eeprom.h"
#include "output/controller_structs.h"
// Bindings to go from controller to ps3
static uint8_t ps3ButtonBindings[] = {
    XBOX_Y,    XBOX_A,     XBOX_B,          XBOX_X,
    0xff,      0xff,       XBOX_LB,         XBOX_RB,
    XBOX_BACK, XBOX_START, XBOX_LEFT_STICK, XBOX_RIGHT_STICK,
    XBOX_HOME, XBOX_UNUSED};
static const uint8_t PROGMEM psGHButtonBindings[] = {
    XBOX_Y,     XBOX_A,          XBOX_B,
    XBOX_X,     XBOX_LB,         0xff,
    0xff,       XBOX_RB,         XBOX_BACK,
    XBOX_START, XBOX_LEFT_STICK, XBOX_RIGHT_STICK,
    XBOX_HOME,  XBOX_UNUSED};
static const uint8_t PROGMEM psRBButonBindings[] = {
    XBOX_X,     XBOX_A,          XBOX_B,
    XBOX_Y,     XBOX_LB,         0xff,
    0xff,       XBOX_RB,         XBOX_BACK,
    XBOX_START, XBOX_LEFT_STICK, XBOX_RIGHT_STICK,
    XBOX_HOME,  XBOX_UNUSED};

static const uint8_t PROGMEM ps3DrumButtonBindings[] = {XBOX_X,
                                                        XBOX_A,
                                                        XBOX_B,
                                                        XBOX_Y,
                                                        XBOX_LB,
                                                        XBOX_RB,
                                                        0xFF,
                                                        0xFF,
                                                        XBOX_BACK,
                                                        XBOX_START,
                                                        XBOX_LEFT_STICK,
                                                        XBOX_RIGHT_STICK,
                                                        XBOX_HOME,
                                                        XBOX_UNUSED};
static uint8_t ps3AxisBindings[] = {
    XBOX_DPAD_UP, XBOX_DPAD_RIGHT, XBOX_DPAD_DOWN, XBOX_DPAD_LEFT, 0xFF,
    0xFF,         XBOX_LB,         XBOX_RB,        XBOX_Y,         XBOX_B,
    XBOX_A,       XBOX_X};
static const uint8_t ghAxisBindings[] = {XBOX_DPAD_LEFT,  XBOX_DPAD_DOWN,
                                         XBOX_DPAD_RIGHT, XBOX_DPAD_UP,
                                         XBOX_X,          XBOX_B};
// Annoyingly, axis bindings for GH guitars overlap. This is the set of axis
// that overlap the previous list.
static const uint8_t PROGMEM ghAxisBindings2[] = {0xff, XBOX_LB, XBOX_Y,
                                                  XBOX_A};
static const uint8_t hat_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07,
                                       0x05, 0x08, 0x02, 0x01, 0x03};
uint8_t currentAxisBindingsLen = 0;
bool isPs3 = false;
void initPS3(void) {
  if (fullDeviceType > SWITCH_GAMEPAD) {
    if (fullDeviceType > PS3_GAMEPAD) {
      memcpy_P(ps3AxisBindings, ghAxisBindings, sizeof(ghAxisBindings));
      currentAxisBindingsLen = sizeof(ghAxisBindings);
    }
  }
  if (fullDeviceType == SWITCH_GAMEPAD) {
    ps3ButtonBindings[SWITCH_B] = XBOX_B;
    ps3ButtonBindings[SWITCH_A] = XBOX_A;
  }
  if (fullDeviceType == PS3_GUITAR_HERO_DRUMS ||
      fullDeviceType == PS3_ROCK_BAND_DRUMS) {
    memcpy_P(ps3ButtonBindings, ps3DrumButtonBindings,
             sizeof(ps3DrumButtonBindings));
  } else if (fullDeviceType == PS3_GUITAR_HERO_GUITAR) {
    memcpy_P(ps3ButtonBindings, psGHButtonBindings, sizeof(ps3ButtonBindings));
  } else if (fullDeviceType == PS3_ROCK_BAND_GUITAR ||
             fullDeviceType == WII_ROCK_BAND_GUITAR) {
    memcpy_P(ps3ButtonBindings, psRBButonBindings, sizeof(ps3ButtonBindings));
  }
}
void fillPS3Report(void *ReportData, uint8_t *const ReportSize,
                   Controller_t *controller) {
  *ReportSize = sizeof(USB_PS3Report_Data_t);
  USB_PS3Report_Data_t *JoystickReport = (USB_PS3Report_Data_t *)ReportData;
  JoystickReport->rid = REPORT_ID_GAMEPAD;
  uint8_t button;
  for (uint8_t i = 0; i < sizeof(ps3ButtonBindings); i++) {
    button = ps3ButtonBindings[i];
    if (button == 0xff) continue;
    bool bit_set = bit_check(controller->buttons, button);
    bit_write(bit_set, JoystickReport->buttons, i);
  }

  // Hat Switch
  button = controller->buttons & 0xF;
  JoystickReport->hat = button > 0x0a ? 0x08 : hat_bindings[button];

  // Tilt / whammy
  if (fullDeviceType == PS3_GUITAR_HERO_GUITAR ||
      fullDeviceType == PS3_LIVE_GUITAR) {
    JoystickReport->r_x = (controller->r_x >> 9) + 128 + 64;
    // GH PS3 guitars have a tilt axis
    // Since the PS3 tilt is based on an accelerometer, we need to subtract 40
    // as they start at a value of 1G, which works out to be around 40.
    JoystickReport->accel[0] = (-(controller->r_y >> 7) + 512) - 40;
    // r_y is tap, so lets disable it.
    JoystickReport->r_y = 0x7d;
  } else if (fullDeviceType == PS3_ROCK_BAND_GUITAR ||
             fullDeviceType == WII_ROCK_BAND_GUITAR) {
    JoystickReport->r_x = 128 + (controller->r_x >> 8);
    // RB PS3 guitars use R for a tilt bit
    bool tilt = controller->r_y == 32767;
    bit_write(tilt, JoystickReport->buttons, SWITCH_R);
    // r_y is the tone switch. Since lt isnt used, but r_y gets used by tilt, we
    // map rx to lt, and then fix it here
    JoystickReport->r_y = controller->lt;
  }
  if (fullDeviceType == PS3_GAMEPAD || fullDeviceType == SWITCH_GAMEPAD) {
    bit_write(controller->lt > 50, JoystickReport->buttons, SWITCH_L);
    bit_write(controller->rt > 50, JoystickReport->buttons, SWITCH_R);
    JoystickReport->axis[4] = controller->lt;
    JoystickReport->axis[5] = controller->rt;
    JoystickReport->l_x = (controller->l_x >> 8) + 128;
    JoystickReport->l_y = (controller->l_y >> 8) + 128;
    JoystickReport->r_x = (controller->r_x >> 8) + 128;
    JoystickReport->r_y = (controller->r_y >> 8) + 128;
  } else {
    // l_x and l_y are unused on guitars and drums. Center them.
    JoystickReport->l_y = 0x80;
    if (isPs3) {
      JoystickReport->l_x = 0x80;
    } else {
      // On a non ps3 device, also mirror tilt to l_x so it can be used in games without accel
      JoystickReport->l_x = 128 + (controller->r_y >> 8);
    }
  }
  if (fullDeviceType == PS3_TURNTABLE) {
    JoystickReport->r_x = (controller->l_x >> 8) + 128;
    JoystickReport->r_y = (controller->l_y >> 8) + 128;
    JoystickReport->accel[0] = controller->l_x;
    JoystickReport->accel[1] = controller->l_y;
  }
  if (fullDeviceType == SWITCH_GAMEPAD) {
    JoystickReport->l_y = 255 - JoystickReport->l_y;
    JoystickReport->r_y = 255 - JoystickReport->r_y;
  }
  if (fullDeviceType == PS3_GUITAR_HERO_DRUMS) {
    JoystickReport->axis[9] = drumVelocity[XBOX_Y - 8];
    JoystickReport->axis[10] = drumVelocity[XBOX_B - 8];
    JoystickReport->axis[11] = drumVelocity[XBOX_A - 8];
    JoystickReport->axis[12] = drumVelocity[XBOX_X - 8];
    JoystickReport->axis[13] = drumVelocity[XBOX_LB - 8];
    JoystickReport->axis[14] = drumVelocity[XBOX_RB - 8];
  }
}