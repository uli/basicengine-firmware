// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include <joystick.h>
#include <stdio.h>

//#define DEBUG_USB_JOYSTICK
//#define DEBUG_USB_REPORT

#ifdef DEBUG_USB_JOYSTICK
#define dbg_joy(x...) printf(x)
#else
#define dbg_joy(x...)
#endif

#ifdef DEBUG_USB_REPORT
#define dbg_rep(x...) printf(x)
#else
#define dbg_rep(x...)
#endif

static int get_bit(uint8_t *data, int off)
{
    return !!(data[off/8] & (1 << off % 8));
}

static int get_bits(uint8_t *data, int off, int n)
{
    if (n == 8)
        return data[off/8];

    int ret = 0;
    int shift = 0;
    // XXX: better algorithm?
    while (n) {
        ret |= get_bit(data, off) << shift;
        off++;
        shift++;
        n--;
    }

    return ret;
}

void hook_usb_generic_report(int hcd, uint8_t dev_addr, hid_generic_report_t *rep)
{
    int pad_num;
    usb_pad *pad = joy.getPad(hcd, dev_addr, pad_num);
    if (!pad)
        return;

    uint8_t *data = rep->data;
    for (int i = 0; i < pad->report_len; ++i) {
        pad->report_data[i] = data[i];
    }

    if (pad->report_id != -1) {
        // check report ID, then skip over it
        if (data[0] != pad->report_id)
            return;
        data++;
    }

#ifdef DEBUG_USB_JOYSTICK
    for (int i = 0; i < pad->report_len + (pad->report_id != -1 ? 1 : 0); ++i) {
        dbg_joy("%02X ", rep->data[i]);
    }
    printf("\n");
#endif

    // Find out the X and Y axis. If we don't have any marked as such
    // we assume they are 0 and 1.
    int axis_x = 0;
    int axis_y = 1;
    int axis_hat = -1;

    for (int i = 0; i < pad->axes.length(); ++i) {
        if (pad->axes[i].usage == 0x30)		// Usage(X)
            axis_x = i;
        else if (pad->axes[i].usage == 0x31)	// Usage(Y)
            axis_y = i;
        else if (pad->axes[i].usage == 0x39)	// Hat Switch
            axis_hat = i;
    }

    if (pad->axes.length() >= 2) {
        if (get_bits(data, pad->axes[axis_x].bit_pos, pad->axes[axis_x].bit_width) < 0x40) joy.m_state |= EB_JOY_LEFT; else joy.m_state &= ~EB_JOY_LEFT;
        if (get_bits(data, pad->axes[axis_x].bit_pos, pad->axes[axis_x].bit_width) > 0xc0) joy.m_state |= EB_JOY_RIGHT; else joy.m_state &= ~EB_JOY_RIGHT;
        if (get_bits(data, pad->axes[axis_y].bit_pos, pad->axes[axis_y].bit_width) < 0x40) joy.m_state |= EB_JOY_UP; else joy.m_state &= ~EB_JOY_UP;
        if (get_bits(data, pad->axes[axis_y].bit_pos, pad->axes[axis_y].bit_width) > 0xc0) joy.m_state |= EB_JOY_DOWN; else joy.m_state &= ~EB_JOY_DOWN;
    }

    if (axis_hat != -1) {
        // Hat Switch is used for digital controls on analog controllers.
        // XXX: This could conceivably have different formats on different
        // controllers, but I have only encountered this one so far.
        switch (get_bits(data, pad->axes[axis_hat].bit_pos, pad->axes[axis_hat].bit_width)) {
            case 0: joy.m_state |= EB_JOY_UP; break;
            case 1: joy.m_state |= EB_JOY_UP | EB_JOY_RIGHT; break;
            case 2: joy.m_state |= EB_JOY_RIGHT; break;
            case 3: joy.m_state |= EB_JOY_RIGHT | EB_JOY_DOWN; break;
            case 4: joy.m_state |= EB_JOY_DOWN; break;
            case 5: joy.m_state |= EB_JOY_DOWN | EB_JOY_LEFT; break;
            case 6: joy.m_state |= EB_JOY_LEFT; break;
            case 7: joy.m_state |= EB_JOY_LEFT | EB_JOY_UP; break;
            default: break;
        }
    }

    for (int i = 0; i < pad->buttons.length(); ++i) {
        uint32_t bit = 1 << (pad->buttons[i].mapped_to + EB_JOY_FIRSTBUTTON_SHIFT);
        if (get_bit(data, pad->buttons[i].bit_pos))
            joy.m_state |= bit;
        else
            joy.m_state &= ~bit;
    }

    dbg_joy("pad state %08X\n", joy.m_state);
}

void hook_usb_generic_mounted(const usb_generic_device_t *dev)
{
    joy.addPad(dev);
}

void hook_usb_generic_unmounted(const usb_generic_device_t *dev)
{
    joy.removePad(dev->hcd, dev->dev_addr);
}

void Joystick::addPad(const usb_generic_device_t *dev)
{
    // Check if the pad doesn't exist already.
    for (int i = 0; i < m_pad_list.length(); ++i) {
        if (m_pad_list[i]->hcd == dev->hcd && m_pad_list[i]->dev_addr == dev->dev_addr)
            return;
    }

    // Might be a new pad. Create a new entry and parse the report descriptor,
    // then add it if the report descriptor parser says it's OK.

    usb_pad *pad = new usb_pad;
    if (!pad)			// OOM
        return;

    pad->hcd = dev->hcd;
    pad->dev_addr = dev->dev_addr;
    pad->report_desc_len = dev->report_desc_len;
    pad->report_desc = new uint8_t[dev->report_desc_len];
    pad->report_len = dev->report_len;
    pad->report_data = new uint8_t[dev->report_len];;
    if (!pad->report_desc || !pad->report_data)	// OOM
        return;

    memcpy(pad->report_desc, dev->report_desc, dev->report_desc_len);

    if (parseReportDesc(pad))
        m_pad_list.push_back(pad);
    else {
        // some other device; we ignore it
        delete pad->report_desc;
        delete pad->report_data;
        delete pad;
    }
}

void Joystick::removePad(int hcd, uint8_t dev_addr)
{
    for (int i = 0; i < m_pad_list.length(); ++i) {
        if (m_pad_list[i]->hcd == hcd && m_pad_list[i]->dev_addr == dev_addr) {
            usb_pad *pad = m_pad_list[i];
            m_pad_list.remove(pad);
            delete pad->report_desc;
            delete pad->report_data;
            delete pad;
            return;
        }
    }
}

usb_pad *Joystick::getPad(int hcd, uint8_t dev_addr, int &index)
{
    for (int i = 0; i < m_pad_list.length(); ++i) {
        if (m_pad_list[i]->hcd == hcd && m_pad_list[i]->dev_addr == dev_addr) {
            index = i;
            //printf("gotpad %p\n", &m_pad_list[i]);
            return m_pad_list[i];
        }
    }
    return NULL;
}

// USB HID joysticks do not normally have usage tags for buttons. This default
// mapping is for a DragonRise generic gamepad and kinda sorta works for other
// devices as well.
static const uint8_t default_button_map[] = {
    EB_JOY_TRIANGLE_SHIFT, EB_JOY_O_SHIFT, EB_JOY_X_SHIFT, EB_JOY_SQUARE_SHIFT, EB_JOY_L1_SHIFT, EB_JOY_R1_SHIFT,
    EB_JOY_L2_SHIFT, EB_JOY_R2_SHIFT, EB_JOY_START_SHIFT, EB_JOY_SELECT_SHIFT, EB_JOY_L3_SHIFT, EB_JOY_R3_SHIFT,
};

// This report descriptor parser deliberately ignores a lot of the complexity
// of USB HID report descriptors. This is on one hand to keep the code
// simple and understandable, and on the other hand because hardware
// manufacturers often can't get it right either, so taking the descriptor
// too seriously may actually be detrimental.

bool Joystick::parseReportDesc(usb_pad *pad)
{
    int i = 0;				// position in the report desciptor
    uint8_t *rd = pad->report_desc;	// report descriptor data
    int report_offset = 0;		// position in the report
    bool have_report_id = false;

    int rep_bits = 0;			// size in bits of the next input
    uint8_t rep_count = 0;		// repeat count for the next input
    int axes_count = 0;			// number of axes found
    int buttons_count = 0;		// number of buttons found
    int usage_page = 0;

    // check if this is a joystick
    // XXX: Does this really make sense? What about HID devices that don't
    // start with Generic Desktop Page?
    if (rd[0] == 0x05 && rd[1] == 0x01 &&			// Generic Desktop Page
        rd[2] == 0x09 && (rd[3] != 0x04 && rd[3] != 0x05)) {	// Usage ID != Joystick, Gamepad
        return false;
    }

    pad->report_id = -1;		// no report ID

    QList<int> usage_list;		// records usage tags to identify axes

    // parse the report descriptor
    while (i < pad->report_desc_len) {
        // assumption: the tag is short-form
        int arg_size = rd[i] & 0x3;
        int tag = rd[i] & 0xfc;
        uint32_t arg = 0;

        dbg_rep("report tag %02X @ %d len %d ", tag, i, arg_size);

        // parse variable-length argument
        int off = 0;
        for (int j = 0, off = 0; j < arg_size; ++j, off += 8) {
            arg |= rd[i + j + 1] << off;
        }
        dbg_rep("arg %lX\n", arg);

        // handle the tag
        if (tag == 0x74) {		// Report Size, bits
            rep_bits = arg;
        } else if (tag == 0x94) {	// Report Count
            rep_count = arg;
        } else if (tag == 0x80) {	// Input
            if ((arg & 0x03) == 0x02) {
                // Data, Variable. We ignore the various other bits (absolute/relative etc.).
                if (rep_bits > 1) {
                    // Assume everything larger than 1 bit is an axis.
                    for (int k = 0; k < rep_count; ++k) {
                        if (usage_page == 0x01) {
                            struct usb_pad_axis axis = {
                                .bit_pos = report_offset,
                                .bit_width = rep_bits,
                                .mapped_to = axes_count++,
                                .usage = -1,
                            };

                            if (usage_list.length() > 0) {
                                axis.usage = usage_list.front();
                                usage_list.pop_front();
                            }

                            dbg_rep("axis @ %d width %d usage %02X\n", report_offset, rep_bits, axis.usage);

                            pad->axes.push_back(axis);
                        }
                        report_offset += rep_bits;
                    }
                } else if (rep_bits == 1) {
                    // All single bit inputs are assumed to be buttons.
                    for (int k = 0; k < rep_count; ++k) {
                        if (usage_page == 0x09 && buttons_count < (int)sizeof(default_button_map)) {
                            struct usb_pad_button button = {
                                .bit_pos = report_offset,
                                // There are two reserved bits in our joystick bitmap
                                // that we need to skip over.
                                .mapped_to = buttons_count + 2,
                            };

                            button.mapped_to = default_button_map[buttons_count] - EB_JOY_FIRSTBUTTON_SHIFT;

                            dbg_rep("button @ %d mapped to %d\n", report_offset, button.mapped_to);

                            buttons_count++;

                            pad->buttons.push_back(button);
                        }

                        report_offset++;
                    }
                } else {
                    dbg_rep("WTF? @%d", i);
                }
            } else {
                // something that is not Data, Variable; probably Constant
                // or Array (like a keyboard report)
                // We ignore these, but have to account for their size.
                report_offset += rep_bits * rep_count;
            }
        } else if (tag == 0x84) {	// Report ID
            // XXX: if there is more than one Report ID, we need to treat this as two devices...
            if (pad->report_id == -1) {
                pad->report_id = arg;
            } else {
                printf("WARNING: extra report ID %u ignored\n", arg);
                break;
            }
        } else if (tag == 0xa0) {	// Collection
            usage_list.clear();
        } else if (tag == 0x08)	{	// Usage
            usage_list.push_back(arg);
        } else if (tag == 0x04) {	// Usage page
            usage_page = arg;
        }

        // skip over current tag and argument to next tag
        i += arg_size + 1;
    }

    return true;
}

Joystick joy;
