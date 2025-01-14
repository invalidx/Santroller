#include "control_requests.h"
#include "../config/defines.h"
#include "controller_structs.h"
#include "descriptors.h"
#include "output/serial_handler.h"
// Dumps from a real guitar

AVR_CONST uint8_t capabilities1[] = {0x00, 0x08, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00};
AVR_CONST uint8_t capabilities2[] = {0x00, 0x14, 0x3f, 0xf7, 0xff, 0xff, 0x00,
                                     0x00, 0x00, 0x00, 0xc0, 0xff, 0xc0, 0xff,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

AVR_CONST USB_OSExtendedCompatibleIDDescriptor_t ExtendedIDs = {
  TotalLength : sizeof(USB_OSExtendedCompatibleIDDescriptor_t),
  Version : 0x0100,
  Index : EXTENDED_PROPERTIES_DESCRIPTOR,
  TotalSections : 1,
  SectionSize : 132,
  ExtendedID : {
    PropertyDataType : 1,
    PropertyNameLength : 40,
    PropertyName : {'D', 'e', 'v', 'i', 'c', 'e', 'I', 'n', 't', 'e',
                    'r', 'f', 'a', 'c', 'e', 'G', 'U', 'I', 'D', '\0'},
    PropertyDataLength : 78,
    PropertyData :
        {'{', 'D', 'F', '5', '9', '0', '3', '7', 'D', '-', '7', 'C', '9',
         '2', '-', '4', '1', '5', '5', '-', 'A', 'C', '1', '2', '-', '7',
         'D', '7', '0', '0', 'A', '3', '1', '3', 'D', '7', '8', '}', '\0'}
  }
};
AVR_CONST CompatibleDescriptorType DevCompatIDs = {
  TotalLength : sizeof(CompatibleDescriptorType),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
  TotalSections : 2,
  Reserved : {0},
  CompatID : {
    FirstInterfaceNumber : INTERFACE_ID_XInput,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  },
  CompatID2 : {
    FirstInterfaceNumber : INTERFACE_ID_Config,
    Reserved : 0x04,
    CompatibleID : "WINUSB",
    SubCompatibleID : {0},
    Reserved2 : {0}
  }
};
