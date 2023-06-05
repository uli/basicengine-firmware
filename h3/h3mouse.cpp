// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifndef JAILHOUSE

#include <Arduino.h>
#include <mouse.h>
#include <usb.h>

Mouse mouse;

#define MOUSE_BUF_SIZE 128
static hid_mouse_report_t reps[MOUSE_BUF_SIZE];
static int repidx_r = 0, repidx_w = 0;

// queue report; we can't process it here because we can't use the FPU in an
// interrupt handler
void hook_usb_mouse_report(int hcd, uint8_t dev_addr, hid_mouse_report_t *r)
{
  reps[repidx_w] = *r;
  repidx_w = (repidx_w + 1) % MOUSE_BUF_SIZE;
}

void mouse_task(void)
{
  while (repidx_r != repidx_w) {
    hid_mouse_report_t *r = &reps[repidx_r];
    mouse.setButtons(r->buttons);
    mouse.move(r->x * eb_psize_width() / 600.0, r->y * eb_psize_width() / 600.0);
    mouse.updateWheel(r->wheel);
    repidx_r = (repidx_r + 1) % MOUSE_BUF_SIZE;
  }
}

#endif	// !JAILHOUSE
