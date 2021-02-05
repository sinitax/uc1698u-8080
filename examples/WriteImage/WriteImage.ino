/* Sample setup for an arduino pro mini running at 3.3V and 8 MHz (schematic in /docs) */

#include "Arduino.h"
#include <uc1698u.h>
#include <avr/pgmspace.h>

#include "img.h"

struct uc1698u_config config = {
	.pin = {
		.CS = 10,
		.CD = 11,
		.WR0 = 13,
		.WR1 = 12,
		.DX = {9, 8, 7, 6, 5, 4, A0, A1 } /* not using pins 2, 3 because of interrupts */
	},
	.state = uc1698u_default_state
};

void
setup()
{
	Serial.begin(115200);
	while (!Serial) {}

	uc1698u_init_pins(&config);
	Serial.write("Init pins done!\n\r");

	uc1698u_init_erc160160(&config);
	Serial.write("Init lcd done!\n\r");

	uc1698u_write_image_64K(&config, img, 0, 0, 160, 160);
	Serial.write("Writing image done!\n\r");

	uc1698u_wake_display(&config);
	Serial.write("Display wake done!\n\r");
}

void
loop()
{
}
