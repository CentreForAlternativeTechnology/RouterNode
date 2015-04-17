#ifndef __SLEEP_H__
#define __SLEEP_H__

#include <Arduino.h>
#include <DS1302.h>
#include "Definitions.h"

class Sleep {
public:
	Sleep(DS1302 *rtc);
	~Sleep();
private:
	DS1302 *rtc;
	void setupSleep();
	void enableSleep();
	void sleepUntil(Time *t);
};

#endif