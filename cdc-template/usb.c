#include <LUFA/Drivers/USB/USB.h>

#include "usb.h"

static FILE usb_stdio_stream;

static const USB_Descriptor_Device_t PROGMEM device_descriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification = VERSION_BCD(1, 1, 0),
    .Class = CDC_CSCP_CDCClass,
    .SubClass = CDC_CSCP_NoSpecificSubclass,
    .Protocol = CDC_CSCP_NoSpecificProtocol,

    .Endpoint0Size = FIXED_CONTROL_ENDPOINT_SIZE,

    .VendorID = 0x03EB,
    .ProductID = 0x2044,
    .ReleaseNumber = VERSION_BCD(0, 0, 1),

    .ManufacturerStrIndex = STRING_ID_Manufacturer,
    .ProductStrIndex = STRING_ID_Product,
    .SerialNumStrIndex = USE_INTERNAL_SERIAL,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS};

static const usb_descriptor_configuration_t PROGMEM configuration_descriptor = {
    .config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},

               .TotalConfigurationSize = sizeof(usb_descriptor_configuration_t),
               .TotalInterfaces = 2,

               .ConfigurationNumber = 1,
               .ConfigurationStrIndex = NO_DESCRIPTOR,

               .ConfigAttributes =
                   (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

               .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)},

    .cdc_cci_interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                     .Type = DTYPE_Interface},

                          .InterfaceNumber = INTERFACE_ID_CDC_CCI,
                          .AlternateSetting = 0,

                          .TotalEndpoints = 1,

                          .Class = CDC_CSCP_CDCClass,
                          .SubClass = CDC_CSCP_ACMSubclass,
                          .Protocol = CDC_CSCP_ATCommandProtocol,

                          .InterfaceStrIndex = NO_DESCRIPTOR},

    .cdc_functional_header =
        {
            .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t),
                       .Type = CDC_DTYPE_CSInterface},
            .Subtype = CDC_DSUBTYPE_CSInterface_Header,

            .CDCSpecification = VERSION_BCD(1, 1, 0),
        },

    .cdc_functional_acm =
        {
            .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t),
                       .Type = CDC_DTYPE_CSInterface},
            .Subtype = CDC_DSUBTYPE_CSInterface_ACM,

            .Capabilities = 0x06,
        },

    .cdc_functional_union =
        {
            .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t),
                       .Type = CDC_DTYPE_CSInterface},
            .Subtype = CDC_DSUBTYPE_CSInterface_Union,

            .MasterInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .SlaveInterfaceNumber = INTERFACE_ID_CDC_DCI,
        },

    .cdc_notification_endpoint =
        {.Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                    .Type = DTYPE_Endpoint},

         .EndpointAddress = CDC_NOTIFICATION_EPADDR,
         .Attributes =
             (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
         .EndpointSize = CDC_NOTIFICATION_EPSIZE,
         .PollingIntervalMS = 0xFF},

    .cdc_dci_interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                     .Type = DTYPE_Interface},

                          .InterfaceNumber = INTERFACE_ID_CDC_DCI,
                          .AlternateSetting = 0,

                          .TotalEndpoints = 2,

                          .Class = CDC_CSCP_CDCDataClass,
                          .SubClass = CDC_CSCP_NoDataSubclass,
                          .Protocol = CDC_CSCP_NoDataProtocol,

                          .InterfaceStrIndex = NO_DESCRIPTOR},

    .cdc_data_out_endpoint = {.Header = {.Size =
                                             sizeof(USB_Descriptor_Endpoint_t),
                                         .Type = DTYPE_Endpoint},

                              .EndpointAddress = CDC_RX_EPADDR,
                              .Attributes =
                                  (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC |
                                   ENDPOINT_USAGE_DATA),
                              .EndpointSize = CDC_TXRX_EPSIZE,
                              .PollingIntervalMS = 0x05},

    .cdc_data_in_endpoint = {
        .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t),
                   .Type = DTYPE_Endpoint},

        .EndpointAddress = CDC_TX_EPADDR,
        .Attributes =
            (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize = CDC_TXRX_EPSIZE,
        .PollingIntervalMS = 0x05}};

/** Language descriptor structure. This descriptor, located in FLASH memory, is
 * returned when the host requests the string descriptor with index 0 (the first
 * index). It is actually an array of 16-bit integers, which indicate via the
 * language ID table available at USB.org what languages the device supports for
 * its string descriptors.
 */
const USB_Descriptor_String_t PROGMEM language_string =
    USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

/** Manufacturer descriptor string. This is a Unicode string containing the
 * manufacturer's details in human readable form, and is read out upon request
 * by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
const USB_Descriptor_String_t PROGMEM manufacturer_string =
    USB_STRING_DESCRIPTOR(L"LUFA Library");

/** Product descriptor string. This is a Unicode string containing the product's
 * details in human readable form, and is read out upon request by the host when
 * the appropriate string ID is requested, listed in the Device Descriptor.
 */
const USB_Descriptor_String_t PROGMEM product_string =
    USB_STRING_DESCRIPTOR(L"LUFA CDC Application");

static USB_ClassInfo_CDC_Device_t cdc_interface = {
    .Config =
        {
            .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .DataINEndpoint =
                {
                    .Address = CDC_TX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .NotificationEndpoint =
                {
                    .Address = CDC_NOTIFICATION_EPADDR,
                    .Size = CDC_NOTIFICATION_EPSIZE,
                    .Banks = 1,
                },
        },
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t value, const uint16_t index,
                                    const void **const descriptor_address) {
  const uint8_t descriptor_type = (value >> 8);
  const uint8_t descriptor_number = (value & 0xFF);

  const void *address = NULL;
  uint16_t size = NO_DESCRIPTOR;

  switch (descriptor_type) {
  case DTYPE_Device:
    address = &device_descriptor;
    size = sizeof(USB_Descriptor_Device_t);
    break;
  case DTYPE_Configuration:
    address = &configuration_descriptor;
    size = sizeof(usb_descriptor_configuration_t);
    break;
  case DTYPE_String:
    switch (descriptor_number) {
    case STRING_ID_Language:
      address = &language_string;
      size = pgm_read_byte(&language_string.Header.Size);
      break;
    case STRING_ID_Manufacturer:
      address = &manufacturer_string;
      size = pgm_read_byte(&manufacturer_string.Header.Size);
      break;
    case STRING_ID_Product:
      address = &product_string;
      size = pgm_read_byte(&product_string.Header.Size);
      break;
    }

    break;
  }

  *descriptor_address = address;
  return size;
}

void usb_init(void) {
  USB_Init();
  CDC_Device_CreateStream(&cdc_interface, &usb_stdio_stream);
  stdout = &usb_stdio_stream;
  stdin = &usb_stdio_stream;
}

void usb_task(void) {
  CDC_Device_USBTask(&cdc_interface);
  USB_USBTask();
}

int16_t usb_read_byte(void) { return CDC_Device_ReceiveByte(&cdc_interface); }

uint8_t usb_write_byte(uint8_t byte) {
  return CDC_Device_SendByte(&cdc_interface, byte);
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void) {
  // do nothing
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void) {
  // do nothing
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
  CDC_Device_ConfigureEndpoints(&cdc_interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) {
  CDC_Device_ProcessControlRequest(&cdc_interface);
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const cdc_interface_info) {
  /* You can get changes to the virtual CDC lines in this callback; a common
     use-case is to use the Data Terminal Ready (DTR) flag to enable and
     disable CDC communications in your application when set to avoid the
     application blocking while waiting for a host to become ready and read
     in the pending data from the USB endpoints.
  */
  bool HostReady = (cdc_interface_info->State.ControlLineStates.HostToDevice &
                    CDC_CONTROL_LINE_OUT_DTR) != 0;

  (void)HostReady;
}
