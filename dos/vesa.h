#ifndef __VESA20_H_INCLUDED
#define __VESA20_H_INCLUDED

#include <stdint.h>

struct VBEInfo {
    char            vbe_signature[4];     // 'VESA' 4 byte signature
    short           vbe_version;          // VBE version number
    char           *oem_string_ptr;       // Pointer to OEM string
    unsigned long   capabilities;         // Capabilities of video card
    unsigned short *video_mode_ptr;       // Pointer to supported modes
    short           total_memory;         // Number of 64kb memory blocks
    // added for VBE 2.0
    short           oem_software_rev;     // OEM Software revision number
    char           *oem_vendor_name_ptr;  // Pointer to Vendor Name string
    char           *oem_product_name_ptr; // Pointer to Product Name string
    char           *oem_product_rev_ptr;  // Pointer to Product Revision str
    char            reserved[222];        // Pad to 256 byte block size
    char            oem_data[256];        // Scratch pad for OEM data

} __attribute__ ((packed));

typedef struct VBEInfo VBEINFO;

struct VBEModeInfo {
    // Mandatory information for all VBE revisions:
    unsigned short  mode_attributes;      // mode attributes
    unsigned char   win_a_attributes;     // window A attributes
    unsigned char   win_b_attributes;     // window B attributes
    unsigned short  win_granularity;      // window granularity
    unsigned short  win_size;             // window size
    unsigned short  win_a_segment;        // window A start segment
    unsigned short  win_b_segment;        // window B start segment
    unsigned long   win_func_ptr;         // pointer to window function
    unsigned short  bytes_per_scan_line;  // bytes per scan line

    // Mandatory information for VBE 1.2 and above:
    unsigned short  x_resolution;          // horizontal resolution in pixels or chars
    unsigned short  y_resolution;          // vertical resolution in pixels or chars
    unsigned char   x_char_size;           // character cell width in pixels
    unsigned char   y_char_size;           // character cell height in pixels
    unsigned char   number_of_planes;      // number of memory planes
    unsigned char   bits_per_pixel;        // bits per pixel
    unsigned char   number_of_banks;       // number of banks
    unsigned char   memory_model;          // memory model type
    unsigned char   bank_size;             // bank size in KB
    unsigned char   number_of_image_pages; // number of images
    unsigned char   reserved;              // reserved for page function

    // Direct Color fields (required for direct/6 and YUV/7 memory models)
    unsigned char   red_mask_size;          // size of direct color red mask in bits
    unsigned char   red_field_position;     // bit position of lsb of red mask
    unsigned char   green_mask_size;        // size of direct color green mask in bits
    unsigned char   green_field_position;   // bit position of lsb of green mask
    unsigned char   blue_mask_size;         // size of direct color blue mask in bits
    unsigned char   blue_field_position;    // bit position of lsb of blue mask
    unsigned char   rsvd_mask_size;         // size of direct color reserved mask in bits
    unsigned char   rsvd_field_position;    // bit position of lsb of reserved mask
    unsigned char   direct_color_mode_info; // direct color mode attributes

    // Mandatory information for VBE 2.0 and above:
    unsigned long   phys_base_ptr;          // physical address for flat frame buffer
    unsigned long   off_screen_mem_offset;  // pointer to start of off screen memory
    unsigned short  off_screen_mem_size;    // amount of off screen memory in 1k units
    char            reserved_buf[206];

} __attribute__ ((packed));

typedef struct VBEModeInfo MODEINFO;

struct PModeInterface {
    short set_window;
    short set_display_start;
    short set_palette;
    char  io_info;
} __attribute__ ((packed));

typedef struct PModeInterface PMINTERFACE;

struct VBESurface {
    unsigned long   lfb_linear_address;
    unsigned short  lfb_selector;
    void           *lfb_near_ptr;
    unsigned long   lfb_mem_size;
    unsigned short  x_resolution;
    unsigned short  y_resolution;
    unsigned long   bits_per_pixel;
    unsigned short  virtual_x_resolution;
    unsigned long   bytes_per_pixel;
    unsigned long   screen_bytes;
    unsigned long   screen_dwords;
    unsigned long   center_x;
    unsigned long   center_y;
    unsigned short  number_of_offscreens;
    void*           offscreen_ptr;
    unsigned char   vbe_boolean;
    unsigned char   vbe_init_boolean;
    signed   long   io_segment;
    unsigned long   io_linear;

};

typedef struct VBESurface VBESURFACE;

//********************************Functions*********************************//

VBEINFO    *VBEgetInfo(void);
MODEINFO   *VBEgetModeInfo(uint16_t vbe_mode_nr);
void       *VBEgetPmodeInterface(void);
uint16_t      VBEselectModeNr(uint16_t xres, uint16_t yres, uint8_t bpp);
VBESURFACE *VBEsetMode(uint16_t vbe_mode_nr);
VBESURFACE *VBEmodeInit(uint16_t xres, uint16_t yres, uint8_t bpp);
uint16_t      VBEsetScanlineLength(uint16_t pixel_length);
void        VBEsetDisplayStart(uint32_t pixel, uint32_t scanline);
VBESURFACE *VBEgetVbeSurfacePtr(void);
MODEINFO   *VBEgetModeInfoPtr(void);
VBESURFACE *VBEinfoInit(uint16_t xres, uint16_t yres, uint8_t bpp, uint16_t delay);
VBESURFACE *VBEinit(uint16_t xres, uint16_t yres, uint8_t bpp);
int         VBEshutdown(void);

void VGAcreatePalette(void);
void VGAwaitVrt(void);

extern inline void putPixel8(long x, long y, uint32_t color);
extern inline void putPixel16(long x, long y, uint32_t color);
extern inline void putPixel32(long x, long y, uint32_t color);
extern inline void flipScreen(void);

extern void (*putPixel)(long x, long y, uint32_t color);

#endif
