// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Ulrich Hecht

#ifdef __linux__

#include <vector>
#include <string>
#include <errno.h>

#include "eb_api.h"
#include "eb_io.h"

#include <gpiod.h>
#include <libsoc_i2c.h>
#include <libsoc_spi.h>

static std::vector<struct gpiod_chip *> chips;

static struct gpiod_chip *get_chip(int no)
{
    std::string chip_name = "gpiochip" + std::to_string(no);
    for (auto gc : chips) {
        if (!strcmp(chip_name.c_str(), gpiod_chip_name(gc)))
            return gc;
    }

    struct gpiod_chip *gc = gpiod_chip_open_by_name(chip_name.c_str());

    if (gc)
        chips.push_back(gc);

    return gc;
}

static void set_error(const char *msg)
{
    static char errmsg[80];
    err = ERR_IO;
    sprintf(errmsg, "%s: %s", msg, strerror(errno));
    err_expected = errmsg;
}

static struct gpiod_line *get_line(int portno, int pinno)
{
    struct gpiod_chip *gc = get_chip(portno);
    if (!gc) {
        set_error(_("failed to acquire GPIO chip"));
        return NULL;
    }

    struct gpiod_line *line = gpiod_chip_get_line(gc, pinno);
    if (!line) {
        set_error(_("failed to acquire GPIO line"));
        return NULL;
    }

    return line;
}

EBAPI int eb_gpio_set_pin(int portno, int pinno, int data)
{
    struct gpiod_line *line = get_line(portno, pinno);
    if (!line)
        return -1;

    int ret;
    if ((ret = gpiod_line_set_value(line, data)) < 0)
        set_error(_("failed to set GPIO state"));

    gpiod_line_release(line);

    return ret;
}

EBAPI int eb_gpio_get_pin(int portno, int pinno)
{
    struct gpiod_line *line = get_line(portno, pinno);
    if (!line)
        return -1;

    int ret;
    if ((ret = gpiod_line_get_value(line)) < 0)
        set_error(_("failed to get GPIO state"));

    gpiod_line_release(line);

    return ret;
}

EBAPI int eb_gpio_set_pin_mode(int portno, int pinno, int mode)
{
    struct gpiod_line *line = get_line(portno, pinno);
    if (!line)
        return -1;

    struct gpiod_line_request_config config = {
        "basic",
        mode == 1 ? GPIOD_LINE_REQUEST_DIRECTION_OUTPUT : GPIOD_LINE_REQUEST_DIRECTION_INPUT,
        0
    };

    int ret;

    if ((ret = gpiod_line_request(line, &config, 0)) < 0)
        set_error(_("failed to configure GPIO line"));

    gpiod_line_release(line);

    return ret;
}

static int i2c_bus_no = 0;

EBAPI int eb_i2c_select_bus(unsigned char bus)
{
    i2c_bus_no = bus;
    return 0;
}

EBAPI int eb_i2c_write(unsigned char addr, const char *data, int count)
{
    i2c *dev = libsoc_i2c_init(i2c_bus_no, addr);
    if (!dev) {
        err = ERR_IO;
        return -1;
    }

    int ret = 0;
    if (libsoc_i2c_write(dev, (uint8_t *)data, count) == EXIT_FAILURE) {
        err = ERR_IO;
        ret = -1;
    }

    libsoc_i2c_free(dev);

    return ret;
}

EBAPI int eb_i2c_read(unsigned char addr, char *data, int count)
{
    i2c *dev = libsoc_i2c_init(i2c_bus_no, addr);
    if (!dev) {
        err = ERR_IO;
        return -1;
    }

    int ret = 0;
    if (libsoc_i2c_read(dev, (uint8_t *)data, count) == EXIT_FAILURE) {
        err = ERR_IO;
        ret = -1;
    }

    libsoc_i2c_free(dev);

    return ret;
}

spi *spi_dev = NULL;

EBAPI int eb_spi_select_device(unsigned short major, unsigned char minor)
{
    if (spi_dev)
        libsoc_spi_free(spi_dev);

    spi_dev = libsoc_spi_init(major, minor);
    if (!spi_dev) {
        err = ERR_IO;
        return -1;
    }
    return 0;
}

EBAPI int eb_spi_write(const char *out_data, unsigned int count)
{
    if (!spi_dev) {
        err = ERR_IO;
        err_expected = _("no SPI device selected");
    }

    return libsoc_spi_write(spi_dev, (uint8_t *)out_data, count) == EXIT_FAILURE;
}

EBAPI int eb_spi_transfer(const char *out_data, char *in_data, unsigned int count)
{
    if (!spi_dev) {
        err = ERR_IO;
        err_expected = _("no SPI device selected");
    }

    return libsoc_spi_rw(spi_dev, (uint8_t *)out_data, (uint8_t *)in_data, count) == EXIT_FAILURE;
}

// SPI bit order is not in libsoc...
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
EBAPI void eb_spi_set_bit_order(int bit_order)
{
    if (!spi_dev) {
        err = ERR_IO;
        err_expected = _("no SPI device selected");
    }

    uint8_t lsb_first = bit_order == 0;

    if (ioctl(spi_dev->fd, SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0)
        set_error(_("failed to set bit order"));
}

EBAPI int eb_spi_set_freq(int freq)
{
    if (!spi_dev) {
        err = ERR_IO;
        err_expected = _("no SPI device selected");
    }

    return libsoc_spi_set_speed(spi_dev, freq) == EXIT_FAILURE;
}

EBAPI int eb_spi_set_mode(int mode)
{
    if (!spi_dev) {
        err = ERR_IO;
        err_expected = _("no SPI device selected");
    }

    return libsoc_spi_set_mode(spi_dev, (spi_mode)mode) == EXIT_FAILURE;
}

#endif // __linux__
