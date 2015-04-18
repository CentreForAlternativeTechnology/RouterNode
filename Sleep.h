#ifndef __SLEEP_H__
#define __SLEEP_H__

#include <Arduino.h>
#include <Time.h>
#include <DS1302RTC.h>
#include <RF24.h>
#include "Definitions.h"

typedef struct {
	uint8_t hour;
	uint8_t minute;
} WakeTime;

class Sleep {
public:
	Sleep(DS1302RTC *rtc, RF24 *radio, int wakeTimesAddress);
	~Sleep();
	void sleepUntil(time_t t);
	time_t getNextWakeTime();
	void checkSleep();
private:
	void setupSleep();
	void enableSleep();
	DS1302RTC *rtc;
	RF24 *radio;
	WakeTime *wakeTimes;
	uint8_t numWakeTimes;
	uint8_t wakeLength;
};

#endif
