/*
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the DFU class bootloader. This file contains the
 * complete bootloader logic.
 */

#define INCLUDE_FROM_BOOTLOADER_C
#include "BootloaderDFU.h"

/** Flag to indicate if the bootloader is currently running in secure mode,
 * disallowing memory operations other than erase. This is initially set to the
 * value set by SECURE_MODE, and cleared by the bootloader once a memory erase
 * has completed in a bootloader session.
 */
static bool IsSecure = SECURE_MODE;

/** Flag to indicate if the bootloader should be running, or should exit and
 * allow the application code to run via a soft reset. When cleared, the
 * bootloader will abort, the USB interface will shut down and the application
 *  jumped to via an indirect jump to location 0x0000 (or other location
 * specified by the host).
 */
static bool RunBootloader = true;

/** Flag to indicate if the bootloader is waiting to exit. When the host
 * requests the bootloader to exit and jump to the application address it
 * specifies, it sends two sequential commands which must be properly
 *  acknowledged. Upon reception of the first the RunBootloader flag is cleared
 * and the WaitForExit flag is set, causing the bootloader to wait for the final
 * exit command before shutting down.
 */
static bool WaitForExit = false;

/** Current DFU state machine state, one of the values in the DFU_State_t enum.
 */
static uint8_t DFU_State = dfuIDLE;

/** Status code of the last executed DFU command. This is set to one of the
 * values in the DFU_Status_t enum after each operation, and returned to the
 * host when a Get Status DFU request is issued.
 */
static uint8_t DFU_Status = OK;

/** Data containing the DFU command sent from the host. */
static DFU_Command_t SentCommand;

/** Response to the last issued Read Data DFU command. Unlike other DFU
 * commands, the read command requires a single byte response from the
 * bootloader containing the read data when the next DFU_UPLOAD command is
 * issued by the host.
 */
static uint8_t ResponseByte;

/** Pointer to the start of the user application. By default this is 0x0000 (the
 * reset vector), however the host may specify an alternate address when issuing
 * the application soft-start command.
 */
static AppPtr_t AppStartPtr = (AppPtr_t)0x0000;

/** 64-bit flash page number. This is concatenated with the current 16-bit
 * address on USB AVRs containing more than 64KB of flash memory.
 */
static uint8_t Flash64KBPage = 0;

/** Memory start address, indicating the current address in the memory being
 * addressed (either FLASH or EEPROM depending on the issued command from the
 * host).
 */
static uint16_t StartAddr = 0x0000;

/** Memory end address, indicating the end address to read from/write to in the
 * memory being addressed (either FLASH of EEPROM depending on the issued
 * command from the host).
 */
static uint16_t EndAddr = 0x0000;

/** Magic lock for forced application start. If the HWBE fuse is programmed and
 * BOOTRST is unprogrammed, the bootloader will start if the /HWB line of the
 * AVR is held low and the system is reset. However, if the /HWB line is still
 * held low when the application attempts to start via a watchdog reset, the
 * bootloader will re-start. If set to the value \ref MAGIC_BOOT_KEY the special
 * init function \ref Application_Jump_Check() will force the application to
 * start.
 */
uint16_t MagicBootKey ATTR_NO_INIT;

/** Main program entry point. This routine configures the hardware required by
 * the bootloader, then continuously runs the bootloader processing routine
 * until instructed to soft-exit, or hard-reset via the watchdog to start the
 * loaded application code.
 */
int main(void) {
  /* Configure hardware required by the bootloader */
  SetupHardware();

  /* Enable global interrupts so that the USB stack can function */
  GlobalInterruptEnable();

  /* Run the USB management task while the bootloader is supposed to be running
   */
  // while (RunBootloader || WaitForExit)
  while (1)
    USB_USBTask();
}

/** Configures all hardware required for the bootloader. */
static void SetupHardware(void) {
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  /* Disable clock division */
  clock_prescale_set(clock_div_1);

  /* Relocate the interrupt vector table to the bootloader section */
  MCUCR = (1 << IVCE);
  MCUCR = (1 << IVSEL);

  /* Initialize the USB and other board hardware drivers */
  USB_Init();
}

/** Event handler for the USB_ControlRequest event. This is used to catch and
 * process control requests sent to the device from the USB host before passing
 * along unhandled control requests to the library for processing internally.
 */
void EVENT_USB_Device_ControlRequest(void) {
  /* Ignore any requests that aren't directed to the DFU interface */
  if ((USB_ControlRequest.bmRequestType &
       (CONTROL_REQTYPE_TYPE | CONTROL_REQTYPE_RECIPIENT)) !=
      (REQTYPE_CLASS | REQREC_INTERFACE)) {
    return;
  }

  /* Get the size of the command and data from the wLength value */
  SentCommand.DataSize = USB_ControlRequest.wLength;

  switch (USB_ControlRequest.bRequest) {
  case DFU_REQ_DNLOAD:
    Endpoint_ClearSETUP();

    /* Check if bootloader is waiting to terminate */
    if (WaitForExit) {
      /* Bootloader is terminating - process last received command */
      ProcessBootloaderCommand();

      /* Indicate that the last command has now been processed - free to exit
       * bootloader */
      WaitForExit = false;
    }

    /* If the request has a data stage, load it into the command struct */
    if (SentCommand.DataSize) {
      while (!(Endpoint_IsOUTReceived())) {
        if (USB_DeviceState == DEVICE_STATE_Unattached)
          return;
      }

      /* First byte of the data stage is the DNLOAD request's command */
      SentCommand.Command = Endpoint_Read_8();

      /* One byte of the data stage is the command, so subtract it from the
       * total data bytes */
      SentCommand.DataSize--;

      /* Load in the rest of the data stage as command parameters */
      for (uint8_t DataByte = 0;
           (DataByte < sizeof(SentCommand.Data)) && Endpoint_BytesInEndpoint();
           DataByte++) {
        SentCommand.Data[DataByte] = Endpoint_Read_8();
        SentCommand.DataSize--;
      }

      /* Process the command */
      ProcessBootloaderCommand();
    }

    /* Check if currently downloading firmware */
    if (DFU_State == dfuDNLOAD_IDLE) {
      if (!(SentCommand.DataSize)) {
        DFU_State = dfuIDLE;
      } else {
        /* Throw away the filler bytes before the start of the firmware */
        DiscardFillerBytes(DFU_FILLER_BYTES_SIZE);

        /* Throw away the packet alignment filler bytes before the start of the
         * firmware */
        DiscardFillerBytes(StartAddr % FIXED_CONTROL_ENDPOINT_SIZE);

        /* Calculate the number of bytes remaining to be written */
        uint16_t BytesRemaining = ((EndAddr - StartAddr) + 1);

        if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x00)) // Write flash
        {
          /* Calculate the number of words to be written from the number of
           * bytes to be written */
          uint16_t WordsRemaining = (BytesRemaining >> 1);

          union {
            uint16_t Words[2];
            uint32_t Long;
          } CurrFlashAddress = {.Words = {StartAddr, Flash64KBPage}};

          uint32_t CurrFlashPageStartAddress = CurrFlashAddress.Long;
          uint8_t WordsInFlashPage = 0;

          while (WordsRemaining--) {
            /* Check if endpoint is empty - if so clear it and wait until ready
             * for next packet */
            if (!(Endpoint_BytesInEndpoint())) {
              Endpoint_ClearOUT();

              while (!(Endpoint_IsOUTReceived())) {
                if (USB_DeviceState == DEVICE_STATE_Unattached)
                  return;
              }
            }

            /* Write the next word into the current flash page */
            BootloaderAPI_FillWord(CurrFlashAddress.Long,
                                   Endpoint_Read_16_LE());

            /* Adjust counters */
            WordsInFlashPage += 1;
            CurrFlashAddress.Long += 2;

            /* See if an entire page has been written to the flash page buffer
             */
            if ((WordsInFlashPage == (SPM_PAGESIZE >> 1)) ||
                !(WordsRemaining)) {
              /* Commit the flash page to memory */
              BootloaderAPI_WritePage(CurrFlashPageStartAddress);

              /* Check if programming incomplete */
              if (WordsRemaining) {
                CurrFlashPageStartAddress = CurrFlashAddress.Long;
                WordsInFlashPage = 0;

                /* Erase next page's temp buffer */
                BootloaderAPI_ErasePage(CurrFlashAddress.Long);
              }
            }
          }

          /* Once programming complete, start address equals the end address */
          StartAddr = EndAddr;
        } else // Write EEPROM
        {
          while (BytesRemaining--) {
            /* Check if endpoint is empty - if so clear it and wait until ready
             * for next packet */
            if (!(Endpoint_BytesInEndpoint())) {
              Endpoint_ClearOUT();

              while (!(Endpoint_IsOUTReceived())) {
                if (USB_DeviceState == DEVICE_STATE_Unattached)
                  return;
              }
            }

            /* Read the byte from the USB interface and write to to the EEPROM
             */
            eeprom_update_byte((uint8_t *)StartAddr, Endpoint_Read_8());

            /* Adjust counters */
            StartAddr++;
          }
        }

        /* Throw away the currently unused DFU file suffix */
        DiscardFillerBytes(DFU_FILE_SUFFIX_SIZE);
      }
    }

    Endpoint_ClearOUT();

    Endpoint_ClearStatusStage();

    break;
  case DFU_REQ_UPLOAD:
    Endpoint_ClearSETUP();

    while (!(Endpoint_IsINReady())) {
      if (USB_DeviceState == DEVICE_STATE_Unattached)
        return;
    }

    if (DFU_State != dfuUPLOAD_IDLE) {
      if ((DFU_State == dfuERROR) &&
          IS_ONEBYTE_COMMAND(SentCommand.Data, 0x01)) // Blank Check
      {
        /* Blank checking is performed in the DFU_DNLOAD request - if we get
           here we've told the host that the memory isn't blank, and the host is
           requesting the first non-blank address */
        Endpoint_Write_16_LE(StartAddr);
      } else {
        /* Idle state upload - send response to last issued command */
        Endpoint_Write_8(ResponseByte);
      }
    } else {
      /* Determine the number of bytes remaining in the current block */
      uint16_t BytesRemaining = ((EndAddr - StartAddr) + 1);

      if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x00)) // Read FLASH
      {
        /* Calculate the number of words to be written from the number of bytes
         * to be written */
        uint16_t WordsRemaining = (BytesRemaining >> 1);

        union {
          uint16_t Words[2];
          uint32_t Long;
        } CurrFlashAddress = {.Words = {StartAddr, Flash64KBPage}};

        while (WordsRemaining--) {
          /* Check if endpoint is full - if so clear it and wait until ready for
           * next packet */
          if (Endpoint_BytesInEndpoint() == FIXED_CONTROL_ENDPOINT_SIZE) {
            Endpoint_ClearIN();

            while (!(Endpoint_IsINReady())) {
              if (USB_DeviceState == DEVICE_STATE_Unattached)
                return;
            }
          }

/* Read the flash word and send it via USB to the host */
#if (FLASHEND > 0xFFFF)
          Endpoint_Write_16_LE(pgm_read_word_far(CurrFlashAddress.Long));
#else
          Endpoint_Write_16_LE(pgm_read_word(CurrFlashAddress.Long));
#endif

          /* Adjust counters */
          CurrFlashAddress.Long += 2;
        }

        /* Once reading is complete, start address equals the end address */
        StartAddr = EndAddr;
      } else if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x02)) // Read EEPROM
      {
        while (BytesRemaining--) {
          /* Check if endpoint is full - if so clear it and wait until ready for
           * next packet */
          if (Endpoint_BytesInEndpoint() == FIXED_CONTROL_ENDPOINT_SIZE) {
            Endpoint_ClearIN();

            while (!(Endpoint_IsINReady())) {
              if (USB_DeviceState == DEVICE_STATE_Unattached)
                return;
            }
          }

          /* Read the EEPROM byte and send it via USB to the host */
          Endpoint_Write_8(eeprom_read_byte((uint8_t *)StartAddr));

          /* Adjust counters */
          StartAddr++;
        }
      }

      /* Return to idle state */
      DFU_State = dfuIDLE;
    }

    Endpoint_ClearIN();

    Endpoint_ClearStatusStage();
    break;
  case DFU_REQ_GETSTATUS:
    Endpoint_ClearSETUP();

    while (!(Endpoint_IsINReady())) {
      if (USB_DeviceState == DEVICE_STATE_Unattached)
        return;
    }

    /* Write 8-bit status value */
    Endpoint_Write_8(DFU_Status);

    /* Write 24-bit poll timeout value */
    Endpoint_Write_8(0);
    Endpoint_Write_16_LE(0);

    /* Write 8-bit state value */
    Endpoint_Write_8(DFU_State);

    /* Write 8-bit state string ID number */
    Endpoint_Write_8(0);

    Endpoint_ClearIN();

    Endpoint_ClearStatusStage();
    break;
  case DFU_REQ_CLRSTATUS:
    Endpoint_ClearSETUP();

    /* Reset the status value variable to the default OK status */
    DFU_Status = OK;

    Endpoint_ClearStatusStage();
    break;
  case DFU_REQ_GETSTATE:
    Endpoint_ClearSETUP();

    while (!(Endpoint_IsINReady())) {
      if (USB_DeviceState == DEVICE_STATE_Unattached)
        return;
    }

    /* Write the current device state to the endpoint */
    Endpoint_Write_8(DFU_State);

    Endpoint_ClearIN();

    Endpoint_ClearStatusStage();
    break;
  case DFU_REQ_ABORT:
    Endpoint_ClearSETUP();

    /* Reset the current state variable to the default idle state */
    DFU_State = dfuIDLE;

    Endpoint_ClearStatusStage();
    break;
  }
}

/** Routine to discard the specified number of bytes from the control endpoint
 * stream. This is used to discard unused bytes in the stream from the host,
 * including the memory program block suffix.
 *
 *  \param[in] NumberOfBytes  Number of bytes to discard from the host from the
 * control endpoint
 */
static void DiscardFillerBytes(uint8_t NumberOfBytes) {
  while (NumberOfBytes--) {
    if (!(Endpoint_BytesInEndpoint())) {
      Endpoint_ClearOUT();

      /* Wait until next data packet received */
      while (!(Endpoint_IsOUTReceived())) {
        if (USB_DeviceState == DEVICE_STATE_Unattached)
          return;
      }
    } else {
      Endpoint_Discard_8();
    }
  }
}

/** Routine to process an issued command from the host, via a DFU_DNLOAD request
 * wrapper. This routine ensures that the command is allowed based on the
 * current secure mode flag value, and passes the command off to the appropriate
 * handler function.
 */
static void ProcessBootloaderCommand(void) {
  /* Check if device is in secure mode */
  if (IsSecure) {
    /* Don't process command unless it is a READ or chip erase command */
    if (!(((SentCommand.Command == COMMAND_WRITE) &&
           IS_TWOBYTE_COMMAND(SentCommand.Data, 0x00, 0xFF)) ||
          (SentCommand.Command == COMMAND_READ))) {
      /* Set the state and status variables to indicate the error */
      DFU_State = dfuERROR;
      DFU_Status = errWRITE;

      /* Stall command */
      Endpoint_StallTransaction();

      /* Don't process the command */
      return;
    }
  }

  /* Dispatch the required command processing routine based on the command type
   */
  switch (SentCommand.Command) {
  case COMMAND_PROG_START:
    ProcessMemProgCommand();
    break;
  case COMMAND_DISP_DATA:
    ProcessMemReadCommand();
    break;
  case COMMAND_WRITE:
    ProcessWriteCommand();
    break;
  case COMMAND_READ:
    ProcessReadCommand();
    break;
  case COMMAND_CHANGE_BASE_ADDR:
    if (IS_TWOBYTE_COMMAND(SentCommand.Data, 0x03,
                           0x00)) // Set 64KB flash page command
      Flash64KBPage = SentCommand.Data[2];

    break;
  }
}

/** Routine to concatenate the given pair of 16-bit memory start and end
 * addresses from the host, and store them in the StartAddr and EndAddr global
 * variables.
 */
static void LoadStartEndAddresses(void) {
  union {
    uint8_t Bytes[2];
    uint16_t Word;
  } Address[2] = {{.Bytes = {SentCommand.Data[2], SentCommand.Data[1]}},
                  {.Bytes = {SentCommand.Data[4], SentCommand.Data[3]}}};

  /* Load in the start and ending read addresses from the sent data packet */
  StartAddr = Address[0].Word;
  EndAddr = Address[1].Word;
}

/** Handler for a Memory Program command issued by the host. This routine
 * handles the preparations needed to write subsequent data from the host into
 * the specified memory.
 */
static void ProcessMemProgCommand(void) {
  if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x00) || // Write FLASH command
      IS_ONEBYTE_COMMAND(SentCommand.Data, 0x01))   // Write EEPROM command
  {
    /* Load in the start and ending read addresses */
    LoadStartEndAddresses();

    /* If FLASH is being written to, we need to pre-erase the first page to
     * write to */
    if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x00)) {
      union {
        uint16_t Words[2];
        uint32_t Long;
      } CurrFlashAddress = {.Words = {StartAddr, Flash64KBPage}};

      /* Erase the current page's temp buffer */
      BootloaderAPI_ErasePage(CurrFlashAddress.Long);
    }

    /* Set the state so that the next DNLOAD requests reads in the firmware */
    DFU_State = dfuDNLOAD_IDLE;
  }
}

/** Handler for a Memory Read command issued by the host. This routine handles
 * the preparations needed to read subsequent data from the specified memory out
 * to the host, as well as implementing the memory blank check command.
 */
static void ProcessMemReadCommand(void) {
  if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x00) || // Read FLASH command
      IS_ONEBYTE_COMMAND(SentCommand.Data, 0x02))   // Read EEPROM command
  {
    /* Load in the start and ending read addresses */
    LoadStartEndAddresses();

    /* Set the state so that the next UPLOAD requests read out the firmware */
    DFU_State = dfuUPLOAD_IDLE;
  } else if (IS_ONEBYTE_COMMAND(SentCommand.Data,
                                0x01)) // Blank check FLASH command
  {
    uint32_t CurrFlashAddress = 0;

    while (CurrFlashAddress < (uint32_t)BOOT_START_ADDR) {
/* Check if the current byte is not blank */
#if (FLASHEND > 0xFFFF)
      if (pgm_read_byte_far(CurrFlashAddress) != 0xFF)
#else
      if (pgm_read_byte(CurrFlashAddress) != 0xFF)
#endif
      {
        /* Save the location of the first non-blank byte for response back to
         * the host */
        Flash64KBPage = (CurrFlashAddress >> 16);
        StartAddr = CurrFlashAddress;

        /* Set state and status variables to the appropriate error values */
        DFU_State = dfuERROR;
        DFU_Status = errCHECK_ERASED;

        break;
      }

      CurrFlashAddress++;
    }
  }
}

/** Handler for a Data Write command issued by the host. This routine handles
 * non-programming commands such as bootloader exit (both via software jumps and
 * hardware watchdog resets) and flash memory erasure.
 */
static void ProcessWriteCommand(void) {
  if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x03)) // Start application
  {
    /* Indicate that the bootloader is terminating */
    WaitForExit = true;

    /* Check if data supplied for the Start Program command - no data executes
     * the program */
    if (SentCommand.DataSize) {
      if (SentCommand.Data[1] == 0x01) // Start via jump
      {
        union {
          uint8_t Bytes[2];
          AppPtr_t FuncPtr;
        } Address = {.Bytes = {SentCommand.Data[4], SentCommand.Data[3]}};

        /* Load in the jump address into the application start address pointer
         */
        AppStartPtr = Address.FuncPtr;
      }
    } else {
      if (SentCommand.Data[1] == 0x00) // Start via watchdog
      {
        /* Unlock the forced application start mode of the bootloader if it is
         * restarted */
        MagicBootKey = MAGIC_BOOT_KEY;

        /* Start the watchdog to reset the AVR once the communications are
         * finalized */
        wdt_enable(WDTO_250MS);
      } else // Start via jump
      {
        /* Set the flag to terminate the bootloader at next opportunity if a
         * valid application has been loaded */
        if (pgm_read_word_near(0) == 0xFFFF)
          RunBootloader = false;
      }
    }
  } else if (IS_TWOBYTE_COMMAND(SentCommand.Data, 0x00, 0xFF)) // Erase flash
  {
    /* Clear the application section of flash */
    for (uint32_t CurrFlashAddress = 0;
         CurrFlashAddress < (uint32_t)BOOT_START_ADDR;
         CurrFlashAddress += SPM_PAGESIZE)
      BootloaderAPI_ErasePage(CurrFlashAddress);

    /* Memory has been erased, reset the security bit so that
     * programming/reading is allowed */
    IsSecure = false;
  }
}

/** Handler for a Data Read command issued by the host. This routine handles
 * bootloader information retrieval commands such as device signature and
 * bootloader version retrieval.
 */
static void ProcessReadCommand(void) {
  const uint8_t BootloaderInfo[3] = {BOOTLOADER_VERSION, BOOTLOADER_ID_BYTE1,
                                     BOOTLOADER_ID_BYTE2};
  const uint8_t SignatureInfo[4] = {0x58, AVR_SIGNATURE_1, AVR_SIGNATURE_2,
                                    AVR_SIGNATURE_3};

  uint8_t DataIndexToRead = SentCommand.Data[1];
  bool ReadAddressInvalid = false;

  if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x00)) // Read bootloader info
  {
    if (DataIndexToRead < 3)
      ResponseByte = BootloaderInfo[DataIndexToRead];
    else
      ReadAddressInvalid = true;
  } else if (IS_ONEBYTE_COMMAND(SentCommand.Data, 0x01)) // Read signature byte
  {
    switch (DataIndexToRead) {
    case 0x30:
      ResponseByte = SignatureInfo[0];
      break;
    case 0x31:
      ResponseByte = SignatureInfo[1];
      break;
    case 0x60:
      ResponseByte = SignatureInfo[2];
      break;
    case 0x61:
      ResponseByte = SignatureInfo[3];
      break;
    default:
      ReadAddressInvalid = true;
      break;
    }
  }

  if (ReadAddressInvalid) {
    /* Set the state and status variables to indicate the error */
    DFU_State = dfuERROR;
    DFU_Status = errADDRESS;
  }
}
