#ifndef UC1698U_8080_H
#define UC1698U_8080_H

/* Library for controlling UC1698U LCD Driver
 *
 * Initial tests based on C51 demo code from:
 * https://www.buydisplay.com/3-inch-graphic-160x160
 * -lcd-display-controller-uc1698-module-black-on-white
 *
 * Used for reference:
 * https://github.com/henningmz/uc1698/blob/master/uc1698.cpp
 *
 * Interesting pages of UC1698U datasheet:
 * - P.11 for control registers
 * - P.14-30 for extensive command reference
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "Arduino.h"

struct uc1698u_pins {
	uint8_t CS,       /* chip select (LOW ENABLE)*/
			CD,       /* cmd (L) / data (H) select */
			WR0, WR1, /* WRITE (WR0) vs READ (WR1) flow control for data bus (RISING EDGE) */
			DX[8];    /* 8-bit data bus */
};

struct uc1698u_state {
	uint8_t col_addr,                                      /* uc1698u_set_col_address */
			row_addr,                                      /* uc1698u_set_row_address */
			display_sleep, display_mode, green_enhance,    /* uc1698u_set_display_enable */
			pixel_inverse,                                 /* uc1698u_set_inverse_display */
			all_pixels,                                    /* uc1698u_set_all_pixel */
			mirror_x, mirror_y, fixed_enable,              /* uc1698u_set_lcd_mapping_control */
			niv_type, niv_xor, niv_enable,                 /* uc1698u_set_nline_inversion */
			rgb_filter,                                    /* uc1698u_set_color_pattern */
			color_mode,                                    /* uc1698u_set_color_mode */
			temp_comp,                                     /* uc1698u_set_temp_compensation */
			fixed_top, fixed_bot,                          /* uc1698u_set_fixed_lines */
			power_internal, lcd_cap,                       /* uc1698u_set_power_control */
			line_rate,                                     /* uc1698u_set_line_rate */
			partial_disp_ctrl,                             /* uc1698u_set_partial_display_control */
			interlace_scan_func, frc_enable, shade_option, /* uc1698u_set_com_scan_function */
			scroll_rate,                                   /* uc1698u_set_scroll_line */
			vbias_pot,                                     /* uc1698u_set_vbias_pot */
			auto_wrap, auto_inc_order, auto_inc_dir,       /* uc1698u_set_ram_address_control */
			lcd_bias,                                      /* uc1698u_set_lcd_bias_ratio */
			partial_disp_start,                            /* uc1698u_set_partial_display_start */
			partial_disp_end,                              /* uc1698u_set_partial_display_end */
			com_end,                                       /* uc1698u_set_com_end */
			window_prog_start_col,                         /* uc1698u_set_window_prog_start_col_addr */
			window_prog_end_col,                           /* uc1698u_set_window_prog_end_col_addr */
			window_prog_start_row,                         /* uc1698u_set_window_prog_start_row_addr */
			window_prog_end_row,                           /* uc1698u_set_window_prog_end_row_addr */
			window_prog_mode;                              /* uc1698u_set_window_prog_mode */
};

extern struct uc1698u_state uc1698u_default_state;

struct uc1698u_config {
	struct uc1698u_pins pin;
	struct uc1698u_state state;
};

/* helper */

void uc1698u_64k_decode(uint8_t b1, uint8_t b2, uint8_t *r, uint8_t *g, uint8_t *b);
void uc1698u_64k_encode(uint8_t *b1, uint8_t *b2, uint8_t r, uint8_t g, uint8_t b);

/* init & test */

void uc1698u_init_pins(struct uc1698u_config *config);
void uc1698u_init_erc160160(struct uc1698u_config *config);
void uc1698u_test_visual(struct uc1698u_config *config);
void uc1698u_wake_display(struct uc1698u_config *config);

/* read & write */

enum {
	UC1698U_CMD = 0,
	UC1698U_DATA = 1
};

void uc1698u_write(struct uc1698u_config *config, int type, int argcount, ...);
void uc1698u_read(struct uc1698u_config *config, int argcount, ...);

/* graphics */

void uc1698u_set_pixpos(struct uc1698u_config *config, uint16_t x, uint16_t y);

/* 64K colormode */
void uc1698u_write_pixel_64K(struct uc1698u_config *config, uint8_t x, uint8_t y, uint8_t val);
void uc1698u_fill_screen_64K(struct uc1698u_config *config, uint8_t fill);
void uc1698u_write_tripix_64K(struct uc1698u_config *config, uint8_t a, uint8_t b, uint8_t c);
void uc1698u_write_image_64K(struct uc1698u_config *config, const uint8_t *data,
		uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);

/* commands */

/* DEFAULT: 0x00 */
void uc1698u_set_col_address(struct uc1698u_config *config, uint8_t col);

/* DEFAULT: UC1698U_TEMP_COMPENSATION_NEG_Op00_PERCENT */
enum {
	UC1698U_TEMP_COMPENSATION_NEG_Op00_PERCENT,
	UC1698U_TEMP_COMPENSATION_NEG_Op05_PERCENT,
	UC1698U_TEMP_COMPENSATION_NEG_Op15_PERCENT,
	UC1698U_TEMP_COMPENSATION_NEG_Op25_PERCENT
};
void uc1698u_set_temp_compensation(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_POWER_CONTROL_LOW_CAP | UC1698U_POWER_CONTROL_INTERNAL */
/* (!) This is a bitmask */
enum {
	UC1698U_POWER_CONTROL_LOW_CAP = 0,  /* 0nF <= LCD <= 13nF */
	UC1698U_POWER_CONTROL_MID_CAP = 1,  /* 13nF < LCD <= 22nF */
	UC1698U_POWER_CONTROL_EXTERNAL = 0,
	UC1698U_POWER_CONTROL_INTERNAL = 2,
};
void uc1698u_set_power_control(struct uc1698u_config *config, int type);

/* 'Set Adv. Program Control' not implemented since use condemmed by datasheet */

/* DEFAULT: 0x00 */
void uc1698u_set_scroll_line(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 0x00 */
void uc1698u_set_row_address(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 0x40 */
/* controls contrast */
void uc1698u_set_vbias_pot(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: UC1698U_PARTIAL_DISPLAY_CONTROL_DISABLE */
enum {
	UC1698U_PARTIAL_DISPLAY_CONTROL_DISABLE = 0,
	UC1698U_PARTIAL_DISPLAY_CONTROL_ENABLE = 1,
};
void uc1698u_set_partial_display_control(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_AUTO_COL_ROW_WRAPAROUND_ENABLE
 *        | UC1698U_AUTO_INCREMENT_COL_FIRST
 *        | UC1698U_ROW_ADDRESS_AUTO_INCREMENT_POS */
/* (!) This is a bitmask */
enum {
	UC1698U_AUTO_COL_ROW_WRAPAROUND_DISABLE = 0b000,
	UC1698U_AUTO_COL_ROW_WRAPAROUND_ENABLE = 0b001,
	UC1698U_AUTO_INCREMENT_COL_FIRST = 0b000,
	UC1698U_AUTO_INCREMENT_ROW_FIRST = 0b010,
	UC1698U_ROW_ADDRESS_AUTO_INCREMENT_POS = 0b000,
	UC1698U_ROW_ADDRESS_AUTO_INCREMENT_NEG = 0b100,
};
void uc1698u_set_ram_address_control(struct uc1698u_config *config, int type);

/* DEFAULT: 0b0000 << 4 | 0b0000 */
/*            ^^^^ FLT      ^^^^ FLB */
/* FLT: fixed lines on top (x2), FLB: fixed lines on bottom (x2) */
void uc1698u_set_fixed_lines(struct uc1698u_config *config, uint8_t data);

/* DEFAULT: UC1698U_32_SHADE_MODE_LINE_RATE_37p0_KILO_LINE_PER_SEC
 *       or UC1698U_ON_OFF_MODE_LINE_RATE_12p6_KILO_LINE_PER_SEC */
enum {
	UC1698U_32_SHADE_MODE_LINE_RATE_25p2_KILO_LINE_PER_SEC = 0b00,
	UC1698U_32_SHADE_MODE_LINE_RATE_30p5_KILO_LINE_PER_SEC = 0b01,
	UC1698U_32_SHADE_MODE_LINE_RATE_37p0_KILO_LINE_PER_SEC = 0b10,
	UC1698U_32_SHADE_MODE_LINE_RATE_44p8_KILO_LINE_PER_SEC = 0b11,
	UC1698U_ON_OFF_MODE_LINE_RATE_8p2_KILO_LINE_PER_SEC = 0b00,
	UC1698U_ON_OFF_MODE_LINE_RATE_10p4_KILO_LINE_PER_SEC = 0b01,
	UC1698U_ON_OFF_MODE_LINE_RATE_12p6_KILO_LINE_PER_SEC = 0b10,
	UC1698U_ON_OFF_MODE_LINE_RATE_15p2_KILO_LINE_PER_SEC = 0b11,
};
void uc1698u_set_line_rate(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_ALL_PIXEL_OFF */
enum {
	UC1698U_ALL_PIXEL_OFF,
	UC1698U_ALL_PIXEL_ON
};
void uc1698u_set_all_pixel(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_INVERSE_DISABLE */
enum {
	UC1698U_INVERSE_DISABLE,
	UC1698U_INVERSE_ENABLE
};
void uc1698u_set_inverse_display(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_DISPLAY_OFF | UC1698U_DISPLAY_MODE_32_SHADE
 *          | UC1698U_DISPLAY_GREEN_ENHANCE_OFF */
/* (!) This is a bitmask */
enum {
	UC1698U_DISPLAY_SLEEP = 0,
	UC1698U_DISPLAY_AWAKE = 1,
	UC1698U_DISPLAY_MODE_ON_OFF = 0,
	UC1698U_DISPLAY_MODE_32_SHADE = 2,
	UC1698U_DISPLAY_MODE_GREEN_ENHANCE_ON = 0,
	UC1698U_DISPLAY_MODE_GREEN_ENHANCE_OFF = 4,
};
void uc1698u_set_display_enable(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_LCD_PARTIAL_DISPLAY_DISABLE | UC1698U_LCD_MIRROR_X_DISABLE
 *          | UC1698U_LCD_MIRROR_Y_DISABLE */
/* (!) This is a bitmask */
enum {
	UC1698U_LCD_PARTIAL_DISPLAY_DISABLE = 0,
	UC1698U_LCD_PARTIAL_DISPLAY_ENABLE = 1,
	UC1698U_LCD_MIRROR_X_DISABLE = 0,
	UC1698U_LCD_MIRROR_X_ENABLE = 2,
	UC1698U_LCD_MIRROR_Y_DISABLE = 0,
	UC1698U_LCD_MIRROR_Y_ENABLE = 4,
};
void uc1698u_set_lcd_mapping_control(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_NLINE_INV_ENABLE | UC1698U_NLINE_INV_XOR_ON
 *          | UC1698U_NLINE_INV_31_LINES */
/* (!) This is a (partial) bitmask */
enum {
	UC1698U_NLINE_INV_11_LINES = 0b000,
	UC1698U_NLINE_INV_19_LINES = 0b001,
	UC1698U_NLINE_INV_21_LINES = 0b010,
	UC1698U_NLINE_INV_25_LINES = 0b011,
	UC1698U_NLINE_INV_29_LINES = 0b100,
	UC1698U_NLINE_INV_31_LINES = 0b101,
	UC1698U_NLINE_INV_37_LINES = 0b110,
	UC1698U_NLINE_INV_43_LINES = 0b111,
	UC1698U_NLINE_INV_XOR_OFF = 0b0000,
	UC1698U_NLINE_INV_XOR_ON = 0b1000,
	UC1698U_NLINE_INV_DISABLE = 0b00000,
	UC1698U_NLINE_INV_ENABLE = 0b10000,
};
void uc1698u_set_nline_inversion(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_RGB_FILTER_ORDER_BGR_BGR */
enum {
	UC1698U_RGB_FILTER_ORDER_BGR_BGR = 0,
	UC1698U_RGB_FILTER_ORDER_RGB_RGB = 1,
};
void uc1698u_set_color_pattern(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_GREEN_ENHANCE_COLOR_MODE_64K */
/* (!) This is not a bitmask, choose one */
enum {
	/* NOTE: see uc1698u_set_display_enable for green enhance mode */
	UC1698U_GREEN_ENHANCE_COLOR_MODE_4K = 0b00,  /* 4R-4G-4B */
	UC1698U_GREEN_ENHANCE_COLOR_MODE_64K = 0b10, /* 5R-6G-5B */
	UC1698U_NORMAL_COLOR_MODE_4K = 0b01,         /* 4R-5G-3B */
	UC1698U_NORMAL_COLOR_MODE_64K = 0b10,        /* 5R-6G-5B */
};
void uc1698u_set_color_mode(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_INTERLACE_SCAN_LRM_SEQ_AEBCD_AEBCD */
/* (!) This is a bitmask */
enum {
	UC1698U_INTERLACE_SCAN_LRM_SEQ_AEBCD_AEBCD = 0b000,
	UC1698U_INTERLACE_SCAN_LRM_SEQ_AEBCD_EBCDA = 0b001,
	UC1698U_FRC_DISABLE = 0b000,
	UC1698U_FRC_ENABLE = 0b010,
	UC1698U_DITHER_ON_INPUT = 0b000,
	UC1698U_PWM_ON_SEG_OUTPUT = 0b100,
};
void uc1698u_set_com_scan_function(struct uc1698u_config *config, int type);

void uc1698u_system_reset(struct uc1698u_config *config);

void uc1698u_nop(struct uc1698u_config *config);

/* DEFAULT: UC1698U_LCD_BIAS_RATIO_12 */
enum {
	UC1698U_LCD_BIAS_RATIO_5,
	UC1698U_LCD_BIAS_RATIO_10,
	UC1698U_LCD_BIAS_RATIO_11,
	UC1698U_LCD_BIAS_RATIO_12,
};
void uc1698u_set_lcd_bias_ratio(struct uc1698u_config *config, int type);

/* DEFAULT: 159 */
void uc1698u_set_com_end(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 0 */
void uc1698u_set_partial_display_start(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 159 */
void uc1698u_set_partial_display_end(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 0 */
void uc1698u_set_window_prog_start_col_addr(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 0 */
void uc1698u_set_window_prog_start_row_addr(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 127 */
void uc1698u_set_window_prog_end_col_addr(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: 159 */
void uc1698u_set_window_prog_end_row_addr(struct uc1698u_config *config, uint8_t val);

/* DEFAULT: UC1698U_WINDOW_PROG_INSIDE_MODE */
enum {
	UC1698U_WINDOW_PROG_INSIDE_MODE, /* write to sram within window (WPC0, WPP0) to (WPC1, WPP1) */
	UC1698U_WINDOW_PROG_OUTSIDE_MODE /* write to sram but skip window (WPC0, WPP0) to (WPC1, WPP1) */
};
void uc1698u_set_window_prog_mode(struct uc1698u_config *config, int type);

/* DEFAULT: UC1698U_MTP_CMD_IDLE | UC1698U_MTP_USE */
enum {
	UC1698U_MTP_CMD_IDLE = 0b000,
	UC1698U_MTP_CMD_READ = 0b001,
	UC1698U_MTP_CMD_ERASE = 0b010,
	UC1698U_MTP_CMD_PROGRAM = 0b011,
	/* 0b1xx for UltraChip debug only */
	UC1698U_MTP_DISABLE = 0b000,
	UC1698U_MTP_ENABLE = 0b100, /* cleared after each mtp command */
	UC1698U_MTP_IGNORE = 0b0000,
	UC1698U_MTP_USE = 0b1000,
};
void uc1698u_set_mtp_op_control(struct uc1698u_config *config, int type);

void uc1698u_set_mtp_write_mask(struct uc1698u_config *config, uint8_t mtpm, uint8_t mptm1);

void uc1698u_set_vmtp1_pot(struct uc1698u_config *config, uint8_t val);

void uc1698u_set_vmtp2_pot(struct uc1698u_config *config, uint8_t val);

void uc1698u_set_mtp_write_timer(struct uc1698u_config *config, uint8_t val);

void uc1698u_set_mtp_read_timer(struct uc1698u_config *config, uint8_t val);

#endif // UC1698U_8080_H
