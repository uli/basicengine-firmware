#include "basic.h"
#include "eb_io.h"

#include <Wire.h>

void SMALL basic_init_io() {
#ifdef ESP8266
  Wire.begin(2, 0);
  // ESP8266 Wire code assumes that SCL and SDA pins are set low, instead
  // of taking care of that itself. WTF?!?
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);
#endif
}

/***bc io GPOUT
Sets the state of a general-purpose I/O pin.
\usage
GPOUT pin, value (ESP8266)

GPOUT port, pin, value (H3)
\args
@port	port number [`0` to `7`]
@pin	pin number [`0` to `15` (ESP8266) or `31` (H3)]
@value	pin state [`0` for "low", anything else for "high"]
\note
* This command is only implemented on the ESP8266 and H3 platforms.
* On the ESP8266 platform, `GPOUT` allows access to pins on the I2C I/O
  extender only.
\ref GPIN() GPMODE
***/
void Basic::idwrite() {
  uint32_t portno = 0;
  uint32_t pinno, data;

  if (
#ifndef ESP8266
      getParam(portno, I_COMMA) ||
#endif
      getParam(pinno, I_COMMA) ||
      getParam(data, I_NONE))
    return;

  eb_gpio_set_pin(portno, pinno, !!data);
}

/***bc io GPMODE
Sets the mode of a general-purpose I/O pin.
\usage GPMODE port, pin, value
\args
@port	port number [`0` to `7`]
@pin	pin number [`0` to `31`]
@mode	pin mode [`0` to `7`]
\note
* This command is only implemented on the H3 platform.
* A `mode` value of `0` or `1` configures the pin as a general-purpose input
  or output, respectively. A value of `7` disables the pin.
* For the functions of other modes, consult the Allwinner H3 datasheet,
  chapter 3.2 ("GPIO Multiplexing Functions").
* WARNING: Improper use of this command can cause permanent damage to the
  system and/or attached devices.
\ref GPIN() GPOUT
***/
void Basic::igpmode() {
#ifdef H3
  uint32_t portno = 0;
  uint32_t pinno, mode;

  if (getParam(portno, I_COMMA) ||
      getParam(pinno, I_COMMA) ||
      getParam(mode, I_NONE))
    return;

  eb_gpio_set_pin_mode(portno, pinno, mode);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bf io I2CW
Sends data to an I2C device.
\usage res = I2CW(i2c_addr, out_data)
\args
@i2c_addr	I2C address [`0` to `$7F`]
@out_data	data to be transmitted
\ret
Status code of the transmission.
\ref I2CR()
***/
num_t BASIC_INT Basic::ni2cw() {
  int32_t i2cAdr;
  BString out;

  if (checkOpen())
    return 0;
  if (getParam(i2cAdr, 0, 0x7f, I_COMMA))
    return 0;
  out = istrexp();
  if (checkClose())
    return 0;

  int ret = eb_i2c_write(i2cAdr, out.c_str(), out.length());
  // XXX: should we process that a little?
  return ret;
}

/***bf io I2CR
Request data from I2C device.
\usage in$ = I2CR(i2c_addr, out_data, read_length)
\args
@i2c_addr	I2C address [`0` to `$7F`]
@out_data	data to be transmitted
@read_length	number of bytes to be received
\ret
Returns the received data as the value of the function call.

Also returns the status code of the outward transmission in `RET(0)`.
\note
If `out_data` is an empty string, no data is sent before the read request.
\ref I2CW()
***/
BString Basic::si2cr() {
  int32_t i2cAdr, rdlen;
  BString in, out;
  char *in_buf;

  if (checkOpen())
    goto out;
  if (getParam(i2cAdr, 0, 0x7f, I_COMMA))
    goto out;
  out = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }
  if (getParam(rdlen, 0, INT32_MAX, I_CLOSE))
    goto out;

  if (out.length() && eb_i2c_write(i2cAdr, out.c_str(), out.length()) != 0)
    goto out;

  in_buf = new char[rdlen];
  if (eb_i2c_read(i2cAdr, in_buf, rdlen) == 0) {
    // XXX: need a BString ctor for binary arrays
    for (int i = 0; i < rdlen; ++i) {
      in += in_buf[i];
    }
  }
  delete[] in_buf;

out:
  return in;
}

/***bc io SWRITE
Writes a byte to the serial port.

WARNING: This command may be renamed in the future to reduce namespace
pollution.
\usage SWRITE c
\args
@c	byte to be written
\ref SMODE
***/
void Basic::iswrite() {
  int32_t c;
  if (getParam(c, I_NONE))
    return;
  Serial.write(c);
}

/***bc io SMODE
Changes the serial port configuration.

WARNING: This command is likely to be renamed in the future to reduce
namespace pollution.
\usage SMODE baud[, flags]
\args
@baud	baud rate [`0` to `921600`]
@flags	serial port flags
\bugs
The meaning of `flags` is determined by `enum` values in the Arduino core
and cannot be relied on to remain stable.
\ref SWRITE
***/
void SMALL Basic::ismode() {
  int32_t baud, flags = SERIAL_8N1;
  if (getParam(baud, 0, 921600, I_NONE))
    return;
  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(flags, 0, 0x3f, I_NONE))
      return;
  }
  Serial.begin(baud,
    (SerialConfig)
    flags);
}

/***bf io GPIN
Reads the state of a general-purpose I/O pin.
\usage s = GPIN(pin)
\args
@portno	port number [`0` to `7`]
@pin	pin number [`0` to `15`]
\ret State of pin: `0` for low, `1` for high.
\note
* This function is only implemented on the ESP8266 and H3 platforms.
* On the ESP8266 platform, `GPIN()` allows access to pins on the I2C I/O
  extender only.
\ref GPOUT GPMODE
***/
num_t BASIC_INT Basic::ngpin() {
  uint32_t portno = 0;
  uint32_t pinno;
  if (checkOpen())
    return 0;
#ifndef ESP8266
  if (getParam(portno, I_COMMA)) return 0;
#endif
  if (getParam(pinno, 0, 15, I_NONE)) return 0;
  if (checkClose()) return 0;

  return eb_gpio_get_pin(portno, pinno);
}

/***bf io ANA
Reads value from the analog input pin.
\usage v = ANA()
\ret Analog value read.
***/
num_t BASIC_FP Basic::nana() {
#if !(defined(ESP8266) || defined(ESP32)) || defined(ESP8266_NOWIFI)
  err = ERR_NOT_SUPPORTED;
  return 0;
#else
  if (checkOpen())
    return 0;
  if (checkClose())
    return 0;
  return analogRead(A0);  // 入力値取得
#endif
}

/***bf io SREAD
Reads bytes from the serial port.

WARNING: This function is currently useless because the serial port
receive pin is used for sound output in the BASIC Engine. It may be
removed in the future if it turns out that there is no viable way
to support serial input.
\usage b = SREAD
\ret Byte of data read, or `-1` if there was no data available.
\note
The alternative syntax `SREAD()` is supported for backwards compatibility
with earlier versions of Engine BASIC.
\ref SREADY
***/
num_t BASIC_INT Basic::nsread() {
  // backwards compatibility with SREAD() syntax
  if (*cip == I_OPEN && cip[1] == I_CLOSE)
    cip += 2;

  return Serial.read();
}

/***bf io SREADY
Checks for available bytes on the serial port.

WARNING: This function is currently useless because the serial port
receive pin is used for sound output in the BASIC Engine. It may be
removed in the future if it turns out that there is no viable way
to support serial input.
\usage n = SREADY
\ret Number of bytes available to read.
\note
The alternative syntax `SREADY()` is supported for backwards compatibility
with earlier versions of Engine BASIC.
\ref SREAD
***/
num_t BASIC_INT Basic::nsready() {
  // backwards compatibility with SREADY() syntax
  if (*cip == I_OPEN && cip[1] == I_CLOSE)
    cip += 2;

  return Serial.available();
}

void Basic::isprint() {
  iprint(1);
}

/***bc io SPIW
Sends data to an SPI device.
\usage SPIW out$
\args
@out$	data to be transmitted
\ref SPIRW$() SPICONFIG
***/
void Basic::ispiw() {
  BString data = istrexp();
  eb_spi_write(data.c_str(), data.length());
}

/***bc io SPICONFIG
Configures the SPI controller.
\usage SPICONFIG bit_order, freq, mode
\args
@bit_order	data bit order [`0` = LSB first, `1` = MSB first]
@freq		transfer frequency in Hz
@mode		SPI mode [`0` to `3`]
\ref SPIW SPIRW$()
***/
void Basic::ispiconfig() {
  int32_t bit_order, freq, mode;

  if (getParam(bit_order, I_COMMA) ||
      getParam(freq, I_COMMA) ||
      getParam(mode, I_NONE))
    return;

  eb_spi_set_bit_order(bit_order);
  eb_spi_set_freq(freq);
  eb_spi_set_mode(mode);
}

/***bf io SPIRW$
Send and receive data to and from an SPI device.
\usage in$ = SPIRW$(out$)
\args
@out$		data to be transmitted
\ret
Returns a string containing the received data that is of the same length
as out$.
\ref SPIW SPICONFIG
***/
BString Basic::sspirw() {
  BString in_data;

  if (checkOpen())
    return in_data;

  BString out_data = istrexp();
  if (checkClose())
    return in_data;

  char in_buf[out_data.length()];

  eb_spi_transfer(out_data.c_str(), in_buf, out_data.length());

  // XXX: need a BString ctor for binary arrays
  for (unsigned int i = 0; i < out_data.length(); ++i) {
    in_data += in_buf[i];
  }

  return in_data;
}
