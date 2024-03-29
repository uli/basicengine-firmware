// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

int eb_gpio_set_pin(int portno, int pinno, int data);
int eb_gpio_get_pin(int portno, int pinno);
int eb_gpio_set_pin_mode(int portno, int pinno, int mode);
int eb_i2c_write(unsigned char addr, const char *data, int count);
int eb_i2c_read(unsigned char addr, char *data, int count);
int eb_spi_write(const char *out_data, unsigned int count);
int eb_spi_transfer(const char *out_data, char *in_data, unsigned int count);
void eb_spi_set_bit_order(int bit_order);
int eb_spi_set_freq(int freq);
int eb_spi_set_mode(int mode);

int eb_i2c_select_bus(unsigned char bus);
int eb_spi_select_device(unsigned short major, unsigned char minor);

#define EB_SPI_BIT_ORDER_LSB 0
#define EB_SPI_BIT_ORDER_MSB 1

#ifdef __cplusplus
}
#endif
