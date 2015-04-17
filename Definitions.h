#ifndef __DEFINITIONS_H__
#define __DEFINITIONS_H__

#include "Arduino.h"
/* uncomment to enable serial debug */
//#define DEBUG

#define MAX_PACKET_SIZE 128 /* Maximum size of EMon packet */

/* EEPROM Addresses */
#define RESETEEPROM 0 /* if set to anything over than 0, resets EEPROM. */
#define RF24NODEIDEEPROM 1 /* RF24 NodeID, 0-255 */
#define EMONNODEIDEEPROM1 2 /* unsigned short emon cms node id, LSB */
#define EMONNODEIDEEPROM2 3 /* unsigned short emon cms node id, MSB */
#define ATTR_REGISTERED_START 100 /* start of storage for attributes registered */

#define EEPROM_CALIB_GRAD 4
#define EEPROM_CALIB_CONST 8
#define EEPROM_CALIB_BASE 12

#define EEPROM_ENCRYPT_ENABLE 14
#define EEPROM_ENCRYPT_KEY 15

/* Enable pins on communication connector */
#define EN_PIN1 10
#define EN_PIN2 9

/* Pin when dragged low goes into config mode */
#define PROG_MODE_PIN  A1

/* Connected to 3.7v battery via resitive divider [Vin - 12k -[SENSE]- 3.3k - GND] */
#define BATTERY_PIN A2

#define RADIO_CE_PIN 7
#define RADIO_CSN_PIN 8

/* Real-time clock pins */
#define RTC_CLK 4
#define RTC_DATA 5
#define RTC_RST 6
#define RTC_EN 3

/* Time between posting attributes */
#define ATTR_POST_WAIT 2000

/* Pin for collecting random data */
#define RAND_PIN A7

union floatBytes {
	uint8_t bytes[4];
	float value;
};

union intBytes {
	uint8_t bytes[2];
	int value;
};

#endif
