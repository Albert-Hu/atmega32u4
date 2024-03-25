#ifndef _USB_H_
#define _USB_H_

#include <stdint.h>

#include <LUFA/Drivers/USB/Class/Device/CDCClassDevice.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

#define CDC_NOTIFICATION_EPADDR (ENDPOINT_DIR_IN | 2)
#define CDC_TX_EPADDR (ENDPOINT_DIR_IN | 3)
#define CDC_RX_EPADDR (ENDPOINT_DIR_OUT | 4)
#define CDC_NOTIFICATION_EPSIZE 8
#define CDC_TXRX_EPSIZE 16

typedef struct {
  USB_Descriptor_Configuration_Header_t config;

  // CDC Control Interface
  USB_Descriptor_Interface_t cdc_cci_interface;
  USB_CDC_Descriptor_FunctionalHeader_t cdc_functional_header;
  USB_CDC_Descriptor_FunctionalACM_t cdc_functional_acm;
  USB_CDC_Descriptor_FunctionalUnion_t cdc_functional_union;
  USB_Descriptor_Endpoint_t cdc_notification_endpoint;

  // CDC Data Interface
  USB_Descriptor_Interface_t cdc_dci_interface;
  USB_Descriptor_Endpoint_t cdc_data_out_endpoint;
  USB_Descriptor_Endpoint_t cdc_data_in_endpoint;
} usb_descriptor_configuration_t;

enum InterfaceDescriptors_t {
  INTERFACE_ID_CDC_CCI = 0, /**< CDC CCI interface descriptor ID */
  INTERFACE_ID_CDC_DCI = 1, /**< CDC DCI interface descriptor ID */
};

enum StringDescriptors_t {
  STRING_ID_Language =
      0, /**< Supported Languages string descriptor ID (must be zero) */
  STRING_ID_Manufacturer = 1, /**< Manufacturer string ID */
  STRING_ID_Product = 2,      /**< Product string ID */
};

/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void **const DescriptorAddress)
    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

void usb_init(void);
void usb_task(void);
int16_t usb_read_byte(void);
uint8_t usb_write_byte(uint8_t byte);

#endif // _USB_H_