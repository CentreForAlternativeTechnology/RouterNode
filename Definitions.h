#ifndef __DEFINITIONS_H__
#define __DEFINITIONS_H__

#include <Arduino.h>
/* uncomment to enable serial debug */
//#define DEBUG

/* EEPROM Addresses */
#define RESETEEPROM 0 /* if set to anything over than 0, resets EEPROM. */
#define RF24NODEIDEEPROM 1 /* RF24 NodeID, 0-255 */

#define EEPROM_ALARM_START 10

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

#endif
