// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 Ulrich Hecht

// USB joystick driver.

#ifdef JAILHOUSE

#include <stdint.h>
#include <vector>

#define PLATFORM_MAX_PADS 16

class Joystick {
public:
  void begin();
  int read(int num);
  int count();

  void addInstance(uint32_t id);
  void delInstance(uint32_t id);
  void buttonDown(uint32_t id, uint8_t button);
  void buttonUp(uint32_t id, uint8_t button);
  void axis(uint32_t id, uint8_t axis, int16_t value);

private:
  void indexFromInstance(int id);

  std::vector<int> m_index;
  int m_state[PLATFORM_MAX_PADS];

  int m_num_pads;
};

#else	// JAILHOUSE

#include <stdint.h>
#include <QList.h>
#include <usb.h>

struct usb_pad_axis {
  int bit_pos;
  int bit_width;
  int mapped_to;
  int usage;
};

struct usb_pad_button {
  int bit_pos;
  int mapped_to;
};

struct usb_pad {
  int hcd;
  uint8_t dev_addr;

  uint8_t *report_desc;
  int report_desc_len;
  uint8_t *report_data;
  int report_len;
  int report_id;

  QList<struct usb_pad_axis> axes;
  QList<struct usb_pad_button> buttons;
};

class Joystick {
friend void ::hook_usb_generic_report(int hcd, uint8_t dev_addr, hid_generic_report_t *data);
friend void ::hook_usb_generic_mounted(const usb_generic_device_t *dev);
public:
  void begin() {
  }
  int read() {
    return m_state;
  }

  void addPad(const usb_generic_device_t *dev);
  void removePad(int hcd, uint8_t dev_addr);
  struct usb_pad *getPad(int hcd, uint8_t dev_addr, int &index);

  bool parseReportDesc(usb_pad *pad);

private:
  int m_state;
  QList<usb_pad *> m_pad_list;
};

#endif	// JAILHOUSE
