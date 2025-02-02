#include "config/defines.h"
#include "eeprom/eeprom.h"
#include "output/control_requests.h"
#include "output/descriptors.h"
#include "output/serial_handler.h"
#include "timer/timer.h"
void deviceControlRequest(void) {
  if (!(Endpoint_IsSETUPReceived())) return;
  const void *buffer = NULL;
  uint8_t len = 0;
  if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
      USB_ControlRequest.bmRequestType ==
          (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
    processHIDReadFeatureReport(USB_ControlRequest.wValue, 0, NULL);
  } else if (USB_ControlRequest.bRequest == HID_REQ_SetReport &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    processHIDWriteFeatureReportControl(USB_ControlRequest.wValue,
                                        USB_ControlRequest.wLength);
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             USB_ControlRequest.wIndex == INTERFACE_ID_XInput &&
             USB_ControlRequest.wValue == 0x0000) {
    len = sizeof(capabilities1);
    buffer = &capabilities1;
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             USB_ControlRequest.wIndex == EXTENDED_PROPERTIES_DESCRIPTOR &&
             USB_ControlRequest.wValue == INTERFACE_ID_Config) {
    len = ExtendedIDs.TotalLength;
    buffer = &ExtendedIDs;
  } else if (USB_ControlRequest.bRequest == REQ_GetOSFeatureDescriptor &&
             USB_ControlRequest.bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE) &&
             USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    len = DevCompatIDs.TotalLength;
    buffer = &DevCompatIDs;
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE)) &&
             USB_ControlRequest.wIndex == 0x00 &&
             USB_ControlRequest.wValue == 0x0000) {
    union {
      uint32_t Value;
      uint8_t Bytes[4];
    } Data;
    Data.Value = micros();
    Endpoint_ClearSETUP();
    while (!Endpoint_IsINReady()) {

    }
    Endpoint_Write_8(Data.Bytes[0]);
    Endpoint_Write_8(Data.Bytes[1]);
    Endpoint_Write_8(Data.Bytes[2]);
    Endpoint_Write_8(Data.Bytes[3]);
    Endpoint_ClearIN();
    Endpoint_ClearStatusStage();
  } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport &&
             (USB_ControlRequest.bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             USB_ControlRequest.wIndex == INTERFACE_ID_XInput &&
             USB_ControlRequest.wValue == 0x0100) {
    len = sizeof(capabilities2);
    buffer = &capabilities2;
  } else if ((USB_ControlRequest.bmRequestType &
              (CONTROL_REQTYPE_RECIPIENT | CONTROL_REQTYPE_TYPE)) ==
                 (REQTYPE_CLASS | REQREC_INTERFACE) &&
             USB_ControlRequest.bRequest != HID_REQ_SetIdle) {
    Endpoint_ClearSETUP();
    Endpoint_ClearStatusStage();
  }
  if (buffer) {
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_PStream_LE(buffer, len);
    Endpoint_ClearStatusStage();
  }
}
void write_endpoint_mods(const void *const Buffer, uint16_t Length,
                         uint8_t *mods, uint8_t modCount) {
  Endpoint_ClearSETUP();
  bool LastPacketFull = false;
  uint8_t current = 0;

  if (Length > USB_ControlRequest.wLength)
    Length = USB_ControlRequest.wLength;

  while (true) {
    if (Endpoint_IsSETUPReceived())
      return;
    else if (Endpoint_IsOUTReceived())
      break;
    if (Length || LastPacketFull) {
      if (Endpoint_IsINReady()) {
        uint16_t BytesInEndpoint = Endpoint_BytesInEndpoint();

        while (Length && (BytesInEndpoint < USB_Device_ControlEndpointSize)) {
          uint8_t bytes = 1;
          for (uint8_t i = 0; i < modCount; i += 3) {
            if (current == mods[i]) {
              bytes = 2;
              Endpoint_Write_8(mods[i + 1]);
              Endpoint_Write_8(mods[i + 2]);
            }
          }
          if (bytes == 1) { Endpoint_Write_8(pgm_read_byte(Buffer + current)); }
          // We need to skip over 2 bytes if we find a block to modify, as each
          // mod block overwrites two bytes
          Length -= bytes;
          BytesInEndpoint += bytes;
          current += bytes;
        }

        LastPacketFull = (BytesInEndpoint == USB_Device_ControlEndpointSize);
        Endpoint_ClearIN();
      }
    }
  }
  Endpoint_ClearStatusStage();
}
/** This function is called by the library when in device mode, and must be
 * overridden (see library "USB Descriptors" documentation) by the application
 * code so that the address and size of a requested descriptor can be given to
 * the USB library. When the device receives a Get Descriptor request on the
 * control endpoint, this function is called so that the descriptor details can
 * be passed back and the appropriate descriptor sent back to the USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const descriptorAddress) {
  const uint8_t descriptorType = (wValue >> 8);
  const uint8_t descriptorNumber = (wValue & 0xFF);
  uint16_t size = NO_DESCRIPTOR;
  const void *address = NULL;
  uint8_t mods[6] = {};
  switch (descriptorType) {
  case DTYPE_Device:
    address = &deviceDescriptor;
    size = deviceDescriptor.Header.Size;
    uint8_t modCount = 0;
    if (deviceType >= SWITCH_GAMEPAD && deviceType < MOUSE) {
      uint8_t offs = (deviceType - SWITCH_GAMEPAD) * 2;
      mods[0] = offsetof(USB_Descriptor_Device_t, VendorID);
      mods[1] = pgm_read_byte(((uint8_t *)vid) + offs);
      mods[2] = pgm_read_byte(((uint8_t *)vid) + offs + 1);
      mods[3] = offsetof(USB_Descriptor_Device_t, ProductID);
      mods[4] = pgm_read_byte(((uint8_t *)pid) + offs);
      mods[5] = pgm_read_byte(((uint8_t *)pid) + offs + 1);
      modCount = sizeof(mods);
    }
    write_endpoint_mods(address, size, mods, modCount);
    return NO_DESCRIPTOR;
  case DTYPE_Configuration:
    address = &ConfigurationDescriptor;
    size = ConfigurationDescriptor.Config.TotalConfigurationSize;
    mods[0] = offsetof(USB_Descriptor_Configuration_t, XInputReserved.subtype);
    mods[1] = deviceType;
    mods[2] = 0x25;
    write_endpoint_mods(address, size, mods, 3);
    return NO_DESCRIPTOR;
    break;
  case HID_DTYPE_Report:
    if (deviceType <= KEYBOARD_ROCK_BAND_DRUMS) {
      address = kbd_report_descriptor;
    } else {
      address = ps3_report_descriptor;
    }
    size = sizeof(kbd_report_descriptor);
    break;
  case DTYPE_String:
    if (descriptorNumber <= 3) {
      address = (void *)pgm_read_word(descriptorStrings + descriptorNumber);
    } else if (descriptorNumber == 0xEE) {
      address = &OSDescriptorString;
    } else {
      break;
    }
    size =
        pgm_read_byte(address + offsetof(USB_StdDescriptor_String_t, bLength));
    break;
  }
  *descriptorAddress = address;

  return size;
}