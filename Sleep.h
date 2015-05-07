#ifndef __SLEEP_H__
#define __SLEEP_H__

#include <Arduino.h>
#include <Time.h>
#include <DS1302RTC.h>
#include <RF24.h>
#include "Definitions.h"

/**
 * Structure defining the repeating alarms in EEPROM
 **/
typedef struct {
	uint8_t hour;
	uint8_t minute;
} WakeTime;

class Sleep {
public:
	/**
	 * @param rtc Pointer to DS1302 RTC
	 * @param radio Pointer to RF24 radio
	 * @param wakeTimesAddress the start address of the alarm storage in EEPROM
	 **/
	Sleep(DS1302RTC *rtc, RF24 *radio, int wakeTimesAddress);
	~Sleep();
	/**
	 * Puts the device into sleep until a specified time
	 **/
	void sleepUntil(time_t t);
	/**
	 * Gets the start time of the next wake
	 **/
	time_t getNextWakeTime();
	/**
	 * Checks whether the device should be asleep and sleeps if it should
	 **/
	void checkSleep();
	/**
	 * Iterates thrgough the alarms to see if any of them are valid
	 * @return true if the device should be awake
	 **/
	bool shouldBeAwake();
private:
	/**
	 * Sets up the chip for sleeping
	 **/
	void setupSleep();
	/**
	 * Puts the chip and peripherals into sleep mode
	 **/
	void enableSleep();
	/**
	 * Pointer to the RTC object
	 **/
	DS1302RTC *rtc;
	/**
	 * Pointer to the radio object
	 **/
	RF24 *radio;
	/**
	 * The time the device next has to go to sleep
	 **/
	time_t nextSleep;
	/**
	 * The time the device next has to wake
	 **/
	time_t nextWake;
	/**
	 * The alarms copied into RAM on startup
	 **/
	WakeTime *wakeTimes;
	/**
	 * The number of alarms
	 **/
	uint8_t numWakeTimes;
	/**
	 * The length of time to stay awake for
	 **/
	uint8_t wakeLength;
};

#endif
