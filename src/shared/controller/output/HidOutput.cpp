#include "HidOutput.h"

#if OUTPUT_TYPE == KEYBOARD || OUTPUT_TYPE == GAMEPAD

/** Buffer to hold the previously generated HID report, for comparison purposes
 * inside the HID class driver. */
static uint8_t PrevHIDReport[HID_REPORTSIZE];
USB_ClassInfo_HID_Device_t HID_Interface = {
    .Config =
        {
            .InterfaceNumber = INTERFACE_ID_HID,
            .ReportINEndpoint =
                {
                    .Address = HID_EPADDR,
                    .Size = HID_EPSIZE,
                    .Banks = 1,
                },
            .PrevReportINBuffer = PrevHIDReport,
            .PrevReportINBufferSize = PrevHIDReport_size,
        },
};
/** Configuration descriptor structure. This descriptor, located in FLASH
 * memory, describes the usage of the device in one of its supported
 * configurations, including information about any device interfaces and
 * endpoints. The descriptor is read out by the USB host during the enumeration
 * process when selecting a configuration so that the host may correctly
 * communicate with the USB device.
 */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},

               .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
               .TotalInterfaces = 1,

               .ConfigurationNumber = 1,
               .ConfigurationStrIndex = NO_DESCRIPTOR,

               .ConfigAttributes =
                   (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

               .MaxPowerConsumption = USB_CONFIG_POWER_MA(500)},

    .HID_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                 .Type = DTYPE_Interface},

                      .InterfaceNumber = INTERFACE_ID_HID,
                      .AlternateSetting = 0x00,

                      .TotalEndpoints = 1,

                      .Class = HID_CSCP_HIDClass,
                      .SubClass = HID_CSCP_NonBootSubclass,
                      .Protocol = HID_CSCP_NonBootProtocol,

                      .InterfaceStrIndex = NO_DESCRIPTOR},

    .HID_GamepadHID = {.Header = {.Size = sizeof(USB_HID_Descriptor_HID_t),
                                  .Type = HID_DTYPE_HID},

                       .HIDSpec = VERSION_BCD(1, 1, 1),
                       .CountryCode = 0x00,
                       .TotalReportDescriptors = 1,
                       .HIDReportType = HID_DTYPE_Report,
                       .HIDReportLength = HID_REPORTSIZE},

    .HID_ReportINEndpoint = {
        .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                   .Type = DTYPE_Endpoint},

        .EndpointAddress = HID_EPADDR,
        .Attributes =
            (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize = HID_EPSIZE,
        .PollingIntervalMS = POLL_RATE}};

Output::Output() {}

bool Output::ready() { return true; }

void Output::init() {
  wdt_enable(WDTO_2S);
  USB_Init();
  sei();
}

void Output::usb_connect() {}

void Output::usb_disconnect() {}

void Output::usb_configuration_changed() {
  HID_Device_ConfigureEndpoints(&HID_Interface);
  USB_Device_EnableSOFEvents();
}

void Output::usb_control_request() {
  if (USB_ControlRequest.bRequest == 0x30) {
    bootloader();
  }
  HID_Device_ProcessControlRequest(&HID_Interface);
}

void Output::usb_start_of_frame() {
  HID_Device_MillisecondElapsed(&HID_Interface);
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {}

uint16_t Output::get_descriptor(const uint8_t DescriptorType,
                                const uint8_t DescriptorNumber,
                                const void **const DescriptorAddress) {
  uint16_t Size = NO_DESCRIPTOR;
  const void *Address = NULL;
  switch (DescriptorType) {
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = sizeof(ConfigurationDescriptor);
    break;
  case HID_DTYPE_HID:
    Address = &ConfigurationDescriptor.HID_GamepadHID;
    Size = sizeof(USB_HID_Descriptor_HID_t);
    break;
  case HID_DTYPE_Report:
    Address = &HIDReport_Datatype;
    Size = HID_REPORTSIZE;
    break;
  }
  *DescriptorAddress = Address;
  return Size;
}
#endif