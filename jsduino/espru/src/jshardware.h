/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Hardware interface Layer
 * NOTE: The definitions of these functions are inside:
 *                                         targets/{target}/jshardware.c
 * ----------------------------------------------------------------------------
 */

#ifndef JSHARDWARE_H_
#define JSHARDWARE_H_

#include "jsutils.h"
#include "jsvar.h"
#include "jsdevices.h"
#ifndef LINUX
#include "jshardware_pininfo.h"
#else
#include <inttypes.h>
#endif

void jshInit();
void jshKill();
void jshIdle(); // stuff to do on idle

/// Get this IC's serial number. Passed max # of chars and a pointer to write to. Returns # of chars
int jshGetSerialNumber(unsigned char *data, int maxChars);

bool jshIsUSBSERIALConnected(); // is the serial device connected?

/// Get the system time (in ticks)
JsSysTime jshGetSystemTime();
/// Convert a time in Milliseconds to one in ticks
JsSysTime jshGetTimeFromMilliseconds(JsVarFloat ms);
/// Convert ticks to a time in Milliseconds
JsVarFloat jshGetMillisecondsFromTime(JsSysTime time);

/// Given a string, convert it to a pin ID (or -1 if it doesn't exist)
Pin jshGetPinFromString(const char *s);
/** Write the pin name to a string. String must have at least 8 characters (to be safe) */
void jshGetPinString(char *result, Pin pin);
/// Given a var, convert it to a pin ID (or -1 if it doesn't exist). safe for undefined!
static inline Pin jshGetPinFromVar(JsVar *pinv) {
  Pin pin=-1;
  if (jsvIsString(pinv) && pinv->varData.str[5]==0/*should never be more than 4 chars!*/) {
    pin = jshGetPinFromString(&pinv->varData.str[0]);
  } else if (jsvIsInt(pinv) /* This also tests for the Pin datatype */) {
    pin = (Pin)jsvGetInteger(pinv);
  }
  return pin;
}

static inline Pin jshGetPinFromVarAndUnLock(JsVar *pinv) {
  Pin pin = jshGetPinFromVar(pinv);
  jsvUnLock(pinv);
  return pin;
}

// software IO functions...
void jshInterruptOff();
void jshInterruptOn();
void jshDelayMicroseconds(int microsec);
void jshPinSetValue(Pin pin, bool value);
bool jshPinGetValue(Pin pin);
bool jshIsPinValid(Pin pin); // is the specific pin actually valid?
// ------

typedef enum {
  JSHPINSTATE_UNDEFINED,
  JSHPINSTATE_GPIO_OUT,
  JSHPINSTATE_GPIO_OUT_OPENDRAIN,
  JSHPINSTATE_GPIO_IN,
  JSHPINSTATE_GPIO_IN_PULLUP,
  JSHPINSTATE_GPIO_IN_PULLDOWN,
  JSHPINSTATE_ADC_IN,
  JSHPINSTATE_AF_OUT,
  JSHPINSTATE_USART_IN,
  JSHPINSTATE_USART_OUT,
  JSHPINSTATE_DAC_OUT,
  JSHPINSTATE_I2C,
} JshPinState;

#define JSHPINSTATE_IS_OUTPUT(state) ( \
             state==JSHPINSTATE_GPIO_OUT ||               \
             state==JSHPINSTATE_GPIO_OUT_OPENDRAIN ||     \
             state==JSHPINSTATE_AF_OUT ||                 \
             state==JSHPINSTATE_USART_OUT ||              \
             state==JSHPINSTATE_DAC_OUT ||                \
             state==JSHPINSTATE_I2C                       \
)

bool jshGetPinStateIsManual(Pin pin);
void jshSetPinStateIsManual(Pin pin, bool manual);
void jshPinSetState(Pin pin, JshPinState state);


bool jshPinInput(Pin pin);
JsVarFloat jshPinAnalog(Pin pin);
void jshPinOutput(Pin pin, bool value);
void jshPinAnalogOutput(Pin pin, JsVarFloat value, JsVarFloat freq); // if freq<=0, the default is used
void jshPinPulse(Pin pin, bool value, JsVarFloat time);
void jshPinWatch(Pin pin, bool shouldWatch);
/// returns false if timer queue was full...
bool jshPinOutputAtTime(JsSysTime time, Pin pin, bool value);

/** Check the pin associated with this EXTI - return true if it is a 1 */
bool jshGetWatchedPinState(IOEventFlags device);

bool jshIsEventForPin(IOEvent *event, Pin pin);

/** Is the given device initialised? */
bool jshIsDeviceInitialised(IOEventFlags device);


#define DEFAULT_BAUD_RATE               9600
#define DEFAULT_BYTESIZE                8
#define DEFAULT_PARITY                  0
#define DEFAULT_STOPBITS                1

typedef struct {
  int baudRate; // FIXME uint32_t ???
  Pin pinRX;
  Pin pinTX;
  unsigned char bytesize;
  unsigned char parity;
  unsigned char stopbits;
} JshUSARTInfo;

static inline void jshUSARTInitInfo(JshUSARTInfo *inf) {
  inf->baudRate = DEFAULT_BAUD_RATE;
  inf->pinRX    = -1;
  inf->pinTX    = -1;
  inf->bytesize = DEFAULT_BYTESIZE;
  inf->parity   = DEFAULT_PARITY; // PARITY_NONE = 0, PARITY_ODD = 1, PARITY_EVEN = 2 FIXME: enum?
  inf->stopbits = DEFAULT_STOPBITS;
}

/** Set up a UART, if pins are -1 they will be guessed */
void jshUSARTSetup(IOEventFlags device, JshUSARTInfo *inf);
/** Kick a device into action (if required). For instance we may need
 * to set up interrupts */
void jshUSARTKick(IOEventFlags device);

typedef enum {
  SPIF_CPHA = 1,
  SPIF_CPOL = 2,
  SPIF_SPI_MODE_0 = 0,
  SPIF_SPI_MODE_1 = SPIF_CPHA,
  SPIF_SPI_MODE_2 = SPIF_CPOL,
  SPIF_SPI_MODE_3 = SPIF_CPHA | SPIF_CPOL,
  /* Mode   CPOL  CPHA
        0   0     0
        1   0     1
        2   1     0
        3   1     1
    */

} JshSPIFlags;

typedef struct {
  int baudRate;
  Pin pinSCK;
  Pin pinMISO;
  Pin pinMOSI;
  unsigned char spiMode;

} JshSPIInfo;
static inline void jshSPIInitInfo(JshSPIInfo *inf) {
  inf->baudRate = 1000000;
  inf->pinSCK = -1;
  inf->pinMISO = -1;
  inf->pinMOSI = -1;
  inf->spiMode = SPIF_SPI_MODE_0;
}

/** Set up SPI, if pins are -1 they will be guessed */
void jshSPISetup(IOEventFlags device, JshSPIInfo *inf);
/** Send data through the given SPI device (if data>=0), and return the result
 * of the previous send (or -1). If data<0, no data is sent and the function
 * waits for data to be returned */
int jshSPISend(IOEventFlags device, int data);
/** Send 16 bit data through the given SPI device. */
void jshSPISend16(IOEventFlags device, int data);
/** Set whether to send 16 bits or 8 over SPI */
void jshSPISet16(IOEventFlags device, bool is16);

typedef struct {
  Pin pinSCL;
  Pin pinSDA;
  char slaveAddr; // or -1 if it is master!
  // speed? 100khz std
  // timeout?
} JshI2CInfo;
static inline void jshI2CInitInfo(JshI2CInfo *inf) {
  inf->pinSCL = -1;
  inf->pinSDA = -1;
  inf->slaveAddr = (char)-1; // master
}
/** Set up I2C, if pins are -1 they will be guessed */
void jshI2CSetup(IOEventFlags device, JshI2CInfo *inf);
/** Addresses are 7 bit - that is, between 0 and 0x7F */
void jshI2CWrite(IOEventFlags device, unsigned char address, int nBytes, const unsigned char *data);
void jshI2CRead(IOEventFlags device, unsigned char address, int nBytes, unsigned char *data);


/// Save contents of JsVars into Flash
void jshSaveToFlash();
/// Load contents of JsVars from Flash
void jshLoadFromFlash();
/// Returns true if flash contains something useful
bool jshFlashContainsCode();

/// Enter simple sleep mode (can be woken up by interrupts)
void jshSleep();

/// Crazy one-wire bitbang code
void jshBitBang(Pin pin, JsVarFloat t0h, JsVarFloat t0l, JsVarFloat t1h, JsVarFloat t1l, JsVar *str);

// ---------------------------------------------- LOW LEVEL
#ifdef ARM
// ----------------------------------------------------------------------------
//                                                                      SYSTICK
// On SYSTick interrupt, call this
void jshDoSysTick();
#ifdef USB
// Kick the USB SysTick watchdog - we need this to see if we have disconnected or not
void jshKickUSBWatchdog();
#endif

#endif // ARM

#endif /* JSHARDWARE_H_ */
