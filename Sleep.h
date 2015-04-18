#ifndef __SLEEP_H__
#define __SLEEP_H__

#include <Arduino.h>
#include <Time.h>
#include <DS1302RTC.h>
#include <RF24.h>
#include "Definitions.h"

class Sleep {
public:
	Sleep(DS1302RTC *rtc, RF24 *radio);
	~Sleep();
	void sleepUntil(time_t t);
private:
	DS1302RTC *rtc;
	RF24 *radio;
	void setupSleep();
	void enableSleep();
};

#endif
