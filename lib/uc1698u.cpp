#include "uc1698u.h"

#define BITSLICE(data, len, skip) (((data) >> (skip)) & ((1 << (len)) - 1))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define setPin(b, d) digitalWrite(b, d)
#define tstPin(b) digitalRead(b)

/* NOTES:
 *
 * [1]: for some commands the datasheet claims only 7 bits active, however 8 bits
 *      are required to write > 127 .. in practice all 8 bits are read and needed
 *      to configure e.g. 160 COM end
 *
*/

struct uc1698u_state uc1698u_default_state = {
	.col_addr = 0,
	.row_addr = 0,
	.display_sleep = UC1698U_DISPLAY_SLEEP,
		.display_mode = UC1698U_DISPLAY_MODE_32_SHADE,
		.green_enhance = UC1698U_DISPLAY_MODE_GREEN_ENHANCE_OFF,
	.pixel_inverse = UC1698U_INVERSE_DISABLE,
	.all_pixels = UC1698U_ALL_PIXEL_OFF,
	.mirror_x = UC1698U_LCD_MIRROR_X_DISABLE,
		.mirror_y = UC1698U_LCD_MIRROR_Y_DISABLE,
		.fixed_enable = UC1698U_LCD_PARTIAL_DISPLAY_DISABLE,
	.niv_type = UC1698U_NLINE_INV_31_LINES,
		.niv_xor = UC1698U_NLINE_INV_XOR_ON,
		.niv_enable = UC1698U_NLINE_INV_ENABLE,
	.rgb_filter = UC1698U_RGB_FILTER_ORDER_RGB_RGB,
	.color_mode = UC1698U_NORMAL_COLOR_MODE_64K,
	.temp_comp = UC1698U_TEMP_COMPENSATION_NEG_Op00_PERCENT,
	.fixed_top = 0,
		.fixed_bot = 0,
	.power_internal = UC1698U_POWER_CONTROL_INTERNAL,
		.lcd_cap = UC1698U_POWER_CONTROL_LOW_CAP,
	.line_rate = 0,
	.partial_disp_ctrl = UC1698U_PARTIAL_DISPLAY_CONTROL_DISABLE,
	.interlace_scan_func = UC1698U_INTERLACE_SCAN_LRM_SEQ_AEBCD_AEBCD,
		.frc_enable = UC1698U_FRC_DISABLE,
		.shade_option = UC1698U_PWM_ON_SEG_OUTPUT,
	.scroll_rate = 0,
	.vbias_pot = 0x40,
	.auto_wrap = UC1698U_AUTO_COL_ROW_WRAPAROUND_ENABLE,
		.auto_inc_order = UC1698U_AUTO_INCREMENT_COL_FIRST,
		.auto_inc_dir = UC1698U_ROW_ADDRESS_AUTO_INCREMENT_POS,
	.lcd_bias = UC1698U_LCD_BIAS_RATIO_12,
	.partial_disp_start = 0,
	.partial_disp_end = 159,
	.com_end = 159,
	.window_prog_start_col = 0,
	.window_prog_end_col = 127,
	.window_prog_start_row = 0,
	.window_prog_end_row = 159,
	.window_prog_mode = UC1698U_WINDOW_PROG_INSIDE_MODE
};

/* helper */

void
uc1698u_64k_decode(uint8_t b1, uint8_t b2, uint8_t *r, uint8_t *g, uint8_t *b)
{
	*r = BITSLICE(b1, 5, 3);
	*g = (BITSLICE(b1, 3, 0) << 3) | BITSLICE(b2, 3, 5);
	*b = BITSLICE(b2, 5, 0);
}

void
uc1698u_64k_encode(uint8_t *b1, uint8_t *b2, uint8_t r, uint8_t g, uint8_t b)
{
	*b1 = (BITSLICE(r, 5, 0) << 3) | BITSLICE(g, 3, 2);
	*b2 = (BITSLICE(g, 2, 0) << 6) | BITSLICE(b, 5, 0);
}

/* init & test */

void
uc1698u_init_pins(struct uc1698u_config *config)
{
	int i;

	pinMode(config->pin.CS, OUTPUT);
	setPin(config->pin.CS, HIGH);

	pinMode(config->pin.CD, OUTPUT);

	pinMode(config->pin.WR0, OUTPUT);
	setPin(config->pin.WR0, HIGH);

	pinMode(config->pin.WR1, OUTPUT);
	setPin(config->pin.WR1, HIGH);

	for (i = 0; i < 8; i++) {
		pinMode(config->pin.DX[i], OUTPUT);
		setPin(config->pin.DX[i], LOW);
	}
}

void
uc1698u_init_erc160160(struct uc1698u_config *config)
{
	delay(500);

	uc1698u_system_reset(config);
	delay(500);

	/* power control */
	uc1698u_set_lcd_bias_ratio(config, UC1698U_LCD_BIAS_RATIO_10);
	uc1698u_set_power_control(config, UC1698U_POWER_CONTROL_INTERNAL |
			UC1698U_POWER_CONTROL_MID_CAP);
	uc1698u_set_temp_compensation(config, UC1698U_TEMP_COMPENSATION_NEG_Op00_PERCENT);
	uc1698u_set_vbias_pot(config, 0xbf);

	/* display control */
	uc1698u_set_all_pixel(config, UC1698U_ALL_PIXEL_OFF);
	uc1698u_set_inverse_display(config, UC1698U_INVERSE_DISABLE);

	/* lcd control */
	uc1698u_set_lcd_mapping_control(config, UC1698U_LCD_PARTIAL_DISPLAY_DISABLE |
			UC1698U_LCD_MIRROR_X_DISABLE | UC1698U_LCD_MIRROR_Y_DISABLE);
	uc1698u_set_line_rate(config, UC1698U_32_SHADE_MODE_LINE_RATE_37p0_KILO_LINE_PER_SEC);
	uc1698u_set_color_pattern(config, UC1698U_RGB_FILTER_ORDER_RGB_RGB);
	uc1698u_set_color_mode(config, UC1698U_NORMAL_COLOR_MODE_64K);

	/* nline inversion */
	uc1698u_set_nline_inversion(config, UC1698U_NLINE_INV_37_LINES |
			UC1698U_NLINE_INV_XOR_OFF | UC1698U_NLINE_INV_DISABLE);

	/* com scan func */
	uc1698u_set_com_scan_function(config, UC1698U_INTERLACE_SCAN_LRM_SEQ_AEBCD_AEBCD |
			UC1698U_FRC_DISABLE | UC1698U_PWM_ON_SEG_OUTPUT);

	/* winow */
	uc1698u_set_window_prog_start_col_addr(config, 37);
	uc1698u_set_window_prog_start_row_addr(config, 0);
	uc1698u_set_window_prog_end_col_addr(config, 90);
	uc1698u_set_window_prog_end_row_addr(config, 159);
	uc1698u_set_window_prog_mode(config, UC1698U_WINDOW_PROG_INSIDE_MODE);

	/* ram addr wraparound */
	uc1698u_set_ram_address_control(config, UC1698U_AUTO_COL_ROW_WRAPAROUND_ENABLE |
			UC1698U_AUTO_INCREMENT_COL_FIRST | UC1698U_ROW_ADDRESS_AUTO_INCREMENT_POS);

	/* partial display */
	uc1698u_set_partial_display_control(config, UC1698U_PARTIAL_DISPLAY_CONTROL_DISABLE);
	uc1698u_set_com_end(config, 160 - 1);
	uc1698u_set_partial_display_start(config, 0);
	uc1698u_set_partial_display_end(config, 160 - 1);

	/* clear ram */
	uc1698u_fill_screen_64K(config, 0b00000);

	/* use default scroll settings (no scroll or partial display) */

	uc1698u_set_display_enable(config, UC1698U_DISPLAY_SLEEP | UC1698U_DISPLAY_MODE_32_SHADE |
			UC1698U_DISPLAY_MODE_GREEN_ENHANCE_OFF);
}

void
uc1698u_test_visual(struct uc1698u_config *config)
{
	int x, y;

	uc1698u_set_pixpos(config, 0, 0);
	for (y = 0; y < 160; y++) {
		for (x = 0; x < 54; x++) {
			uint8_t val = ((y + x) % 2 == 0) ? 0xFF : 0x00;
			uc1698u_write_tripix_64K(config, val, val, val);
		}
	}
}

void
uc1698u_wake_display(struct uc1698u_config *config)
{
	uc1698u_set_display_enable(config, (config->state.green_enhance << 2)
			| (config->state.display_mode << 1) | UC1698U_DISPLAY_AWAKE);
}

/* read & write */

void
uc1698u_write(struct uc1698u_config *config, int type, int argcount, ...)
{
	int k, i;
	uint8_t arg;
	va_list ap;

	setPin(config->pin.CS, LOW);
	setPin(config->pin.CD, BITSLICE(type, 1, 0));

	va_start(ap, argcount);
	for (k = 0; k < argcount; k++) {
		arg = va_arg(ap, int);

		for (i = 0; i < 8; i++)
			setPin(config->pin.DX[i], BITSLICE(arg, 1, i));

		setPin(config->pin.WR0, LOW);
		setPin(config->pin.WR0, HIGH);
	}
	va_end(ap);

	setPin(config->pin.CS, HIGH);
}

void
uc1698u_read(struct uc1698u_config *config, int argcount, ...)
{
	int i, k;
	uint8_t *arg;
	va_list ap;

	setPin(config->pin.CS, LOW);
	setPin(config->pin.CD, UC1698U_DATA);

	for (i = 0; i < 8; i++)
		pinMode(config->pin.DX[i], INPUT);

	va_start(ap, argcount);
	for (k = 0; k < argcount; k++) {
		arg = va_arg(ap, uint8_t*);

		setPin(config->pin.WR1, LOW);
		setPin(config->pin.WR1, HIGH);

		*arg = 0;
		for (i = 0; i < 8; i++)
			*arg |= tstPin(config->pin.DX[i]) << i;
	}
	va_end(ap);

	for (i = 0; i < 8; i++)
		pinMode(config->pin.DX[i], OUTPUT);

	setPin(config->pin.CS, HIGH);
}

/* graphics */

void
uc1698u_set_pixpos(struct uc1698u_config *config, uint16_t x, uint16_t y)
{
	uc1698u_set_col_address(config, config->state.window_prog_start_col + x / 3);
	uc1698u_set_row_address(config, config->state.window_prog_start_row + y);
}

void
uc1698u_write_tripix_64K(struct uc1698u_config *config, uint8_t a, uint8_t b, uint8_t c)
{
	uint8_t b1, b2;

	uc1698u_64k_encode(&b1, &b2, a, b, c);
	uc1698u_write(config, UC1698U_DATA, 2, b1, b2);
}

void
uc1698u_write_pixel_64K(struct uc1698u_config *config, uint8_t x, uint8_t y, uint8_t val)
{
	uint8_t dummy, b1 = 0, b2 = 0, triplet[3] = { 0x00, 0x00, 0x00 };

	uc1698u_set_col_address(config, config->state.window_prog_start_col + x / 3);
	uc1698u_set_row_address(config, config->state.window_prog_start_row + y);
	uc1698u_read(config, 3, &dummy, &b1, &b2);

	uc1698u_64k_decode(b1, b2, &triplet[0], &triplet[1], &triplet[2]);
	triplet[x % 3] = val;
	uc1698u_64k_encode(&b1, &b2, triplet[0], triplet[1], triplet[2]);

	uc1698u_set_row_address(config, config->state.window_prog_start_row + y);
	uc1698u_set_col_address(config, config->state.window_prog_start_col + x / 3);
	uc1698u_write_tripix_64K(config, triplet[0], triplet[1], triplet[2]);
}

void
uc1698u_fill_screen_64K(struct uc1698u_config *config, uint8_t fill)
{
	int x, y;

	uc1698u_set_pixpos(config, 0, 0);
	for (x = config->state.window_prog_start_col; x <= config->state.window_prog_end_col; x++) {
		for (y = config->state.window_prog_start_row; y <= config->state.window_prog_end_row; y++) {
			uc1698u_write_tripix_64K(config, fill, fill, fill);
		}
	}
}

void
uc1698u_write_image_64K(struct uc1698u_config *config, const uint8_t *data,
		uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
	uint32_t si;
	uint16_t x, y;

	for (y = 0; y < height; y++) {
		uc1698u_set_pixpos(config, sx, sy + y);
		si = y * width;
		for (x = 0; x < width; x += 3, si += 3) {
			uc1698u_write_tripix_64K(config,
					pgm_read_byte_near(data + si + 0),
					pgm_read_byte_near(data + si + 1),
					pgm_read_byte_near(data + si + 2));
		}
	}
}

/* commands */

void
uc1698u_set_col_address(struct uc1698u_config *config, uint8_t col)
{
	config->state.col_addr = BITSLICE(col, 7, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b00000000 | BITSLICE(col, 4, 0));
	uc1698u_write(config, UC1698U_CMD, 1, 0b00010000 | BITSLICE(col, 3, 4));
}

void
uc1698u_set_temp_compensation(struct uc1698u_config *config, int type)
{
	config->state.temp_comp = BITSLICE(type, 2, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b00100100 | BITSLICE(type, 2, 0));
}

void
uc1698u_set_power_control(struct uc1698u_config *config, int type)
{
	config->state.power_internal = BITSLICE(type, 1, 1);
	config->state.lcd_cap = BITSLICE(type, 1, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b00101000 | BITSLICE(type, 2, 0));
}

void
uc1698u_set_scroll_line(struct uc1698u_config *config, uint8_t val)
{
	config->state.scroll_rate = val;
	uc1698u_write(config, UC1698U_CMD, 1, 0b01000000 | BITSLICE(val, 4, 0));
	uc1698u_write(config, UC1698U_CMD, 1, 0b01010000 | BITSLICE(val, 4, 4));
}

void
uc1698u_set_row_address(struct uc1698u_config *config, uint8_t val)
{
	config->state.row_addr = val;
	uc1698u_write(config, UC1698U_CMD, 1, 0b01100000 | BITSLICE(val, 4, 0));
	uc1698u_write(config, UC1698U_CMD, 1, 0b01110000 | BITSLICE(val, 4, 4));
}

void
uc1698u_set_vbias_pot(struct uc1698u_config *config, uint8_t val)
{
	config->state.vbias_pot = val;
	uc1698u_write(config, UC1698U_CMD, 2, 0b10000001, val);
}

void
uc1698u_set_partial_display_control(struct uc1698u_config *config, int type)
{
	config->state.partial_disp_ctrl = BITSLICE(type, 1, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b10000100 | BITSLICE(type, 1, 0));
}

void
uc1698u_set_ram_address_control(struct uc1698u_config *config, int type)
{
	config->state.auto_wrap = BITSLICE(type, 1, 0);
	config->state.auto_inc_order = BITSLICE(type, 1, 1);
	config->state.auto_inc_dir = BITSLICE(type, 1, 2);
	uc1698u_write(config, UC1698U_CMD, 1, 0b10001000 | BITSLICE(type, 3, 0));
}

void
uc1698u_set_fixed_lines(struct uc1698u_config *config, uint8_t val)
{
	config->state.fixed_top = BITSLICE(val, 4, 4);
	config->state.fixed_bot = BITSLICE(val, 4, 0);
	uc1698u_write(config, UC1698U_CMD, 2, 0b10010000, val);
}

void
uc1698u_set_line_rate(struct uc1698u_config *config, int type)
{
	config->state.line_rate = BITSLICE(type, 2, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b10100000 | BITSLICE(type, 2, 0));
}

void
uc1698u_set_all_pixel(struct uc1698u_config *config, int type)
{
	config->state.all_pixels = BITSLICE(type, 1, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b10100100 | BITSLICE(type, 1, 0));
}

void
uc1698u_set_inverse_display(struct uc1698u_config *config, int type)
{
	config->state.pixel_inverse = BITSLICE(type, 1, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b10100110 | BITSLICE(type, 1, 0));
}

void
uc1698u_set_display_enable(struct uc1698u_config *config, int type)
{
	uc1698u_write(config, UC1698U_CMD, 1, 0b10101000 | BITSLICE(type, 3, 0));
	if (config->state.display_sleep == UC1698U_DISPLAY_SLEEP
			&& BITSLICE(type, 1, 0) == UC1698U_DISPLAY_AWAKE) {
		delay(100); /* wait on wake to avoid noise from inrush current pulse */
	}
	config->state.display_sleep = BITSLICE(type, 1, 0);
	config->state.display_mode = BITSLICE(type, 1, 1);
	config->state.green_enhance = BITSLICE(type, 1, 2);
}

void
uc1698u_set_lcd_mapping_control(struct uc1698u_config *config, int type)
{
	config->state.fixed_enable = BITSLICE(type, 1, 0);
	config->state.mirror_x = BITSLICE(type, 1, 1);
	config->state.mirror_y = BITSLICE(type, 1, 2);
	uc1698u_write(config, UC1698U_CMD, 1, 0b11000000 | BITSLICE(type, 3, 0));
}

void
uc1698u_set_nline_inversion(struct uc1698u_config *config, int type)
{
	config->state.niv_type = BITSLICE(type, 3, 0);
	config->state.niv_xor = BITSLICE(type, 1, 3);
	config->state.niv_enable = BITSLICE(type, 1, 4);
	uc1698u_write(config, UC1698U_CMD, 2, 0b11001000, BITSLICE(type, 5, 0));
}

void
uc1698u_set_color_pattern(struct uc1698u_config *config, int type)
{
	config->state.rgb_filter = BITSLICE(type, 1, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b11010000 | BITSLICE(type, 1, 0));
}

void
uc1698u_set_color_mode(struct uc1698u_config *config, int type)
{
	config->state.color_mode = BITSLICE(type, 2, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b11010100 | BITSLICE(type, 2, 0));
}

void
uc1698u_set_com_scan_function(struct uc1698u_config *config, int type)
{
	config->state.interlace_scan_func = BITSLICE(type, 1, 0);
	config->state.frc_enable = BITSLICE(type, 1, 1);
	config->state.shade_option = BITSLICE(type, 1, 2);
	uc1698u_write(config, UC1698U_CMD, 1, 0b11011000 | BITSLICE(type, 3, 0));
}

void
uc1698u_system_reset(struct uc1698u_config *config)
{
	config->state = uc1698u_default_state;
	uc1698u_write(config, UC1698U_CMD, 1, 0b11100010);
}

void
uc1698u_nop(struct uc1698u_config *config)
{
	uc1698u_write(config, UC1698U_CMD, 1, 0b11100011);
}

void
uc1698u_set_lcd_bias_ratio(struct uc1698u_config *config, int type)
{
	config->state.lcd_bias = BITSLICE(type, 2, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b11101000 | BITSLICE(type, 2, 0));
}

void
uc1698u_set_com_end(struct uc1698u_config *config, uint8_t val)
{
	/* datasheet command is wrong, see note [1] */
	config->state.com_end = val;
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110001, val);
}

void
uc1698u_set_partial_display_start(struct uc1698u_config *config, uint8_t val)
{
	/* datasheet command is wrong, see note [1] */
	config->state.partial_disp_start = val;
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110010, val);
}

void
uc1698u_set_partial_display_end(struct uc1698u_config *config, uint8_t val)
{
	/* datasheet command is wrong, see note [1] */
	config->state.partial_disp_end = val;
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110011, val);
}

void
uc1698u_set_window_prog_start_col_addr(struct uc1698u_config *config, uint8_t val)
{
	config->state.window_prog_start_col = BITSLICE(val, 7, 0);
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110100, BITSLICE(val, 7, 0));
}

void
uc1698u_set_window_prog_start_row_addr(struct uc1698u_config *config, uint8_t val)
{
	config->state.window_prog_start_row = val;
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110101, val);
}

void
uc1698u_set_window_prog_end_col_addr(struct uc1698u_config *config, uint8_t val)
{
	config->state.window_prog_end_col = BITSLICE(val, 7, 0);
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110110, BITSLICE(val, 7, 0));
}

void
uc1698u_set_window_prog_end_row_addr(struct uc1698u_config *config, uint8_t val)
{
	config->state.window_prog_end_row = val;
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110111, val);
}

void
uc1698u_set_window_prog_mode(struct uc1698u_config *config, int type)
{
	config->state.window_prog_mode = BITSLICE(type, 1, 0);
	uc1698u_write(config, UC1698U_CMD, 1, 0b11111000 | BITSLICE(type, 1, 0));
}

void
uc1698u_set_mtp_op_control(struct uc1698u_config *config, int type)
{
	uc1698u_write(config, UC1698U_CMD, 2, 0b10111000, BITSLICE(type, 5, 0));
}

void
uc1698u_set_mtp_write_mask(struct uc1698u_config *config, uint8_t mtpm, uint8_t mtpm1)
{
	uc1698u_write(config, UC1698U_CMD, 3, 0b10111001, BITSLICE(mtpm, 7, 0), BITSLICE(mtpm1, 2, 0));
}

void
uc1698u_set_vmtp1_pot(struct uc1698u_config *config, uint8_t val)
{
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110100, val);
}

void
uc1698u_set_vmtp2_pot(struct uc1698u_config *config, uint8_t val)
{
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110101, val);
}

void
uc1698u_set_mtp_write_timer(struct uc1698u_config *config, uint8_t val)
{
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110110, val);
}

void
uc1698u_set_mtp_read_timer(struct uc1698u_config *config, uint8_t val)
{
	uc1698u_write(config, UC1698U_CMD, 2, 0b11110111, val);
}
