#ifndef __SLEEP_H__
#define __SLEEP_H__

#include <Arduino.h>
#include <Time.h>
#include <DS1302RTC.h>
#include "Definitions.h"

class Sleep {
public:
	Sleep(DS1302RTC *rtc);
	~Sleep();
private:
	DS1302RTC *rtc;
	void setupSleep();
	void enableSleep();
	void sleepUntil(time_t t);
};

#endif
