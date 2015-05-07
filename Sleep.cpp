#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <EEPROM.h>

#include "Sleep.h"

Sleep::Sleep(DS1302RTC *rtc, RF24 *radio, int wakeTimesAddress) {
	this->rtc = rtc;
	this->radio = radio;
	this->nextWake = 0;
	this->nextSleep = 0;
	/* Load the alarms into RAM for a quick go over */
	this->wakeLength = EEPROM.read(wakeTimesAddress);
	if(this->wakeLength == 0) {
		this->wakeLength = 0xFF;
	}
	this->numWakeTimes = EEPROM.read(wakeTimesAddress + 1);
	if(numWakeTimes > 0) {
		this->wakeTimes = (WakeTime *)malloc(sizeof(WakeTime) * numWakeTimes);
	} else {
		this->wakeTimes = NULL;
	}
	for(int i = 0; i < this->numWakeTimes; i++) {
		this->wakeTimes[i].hour = EEPROM.read((i * 2) + 2 + wakeTimesAddress);
		this->wakeTimes[i].minute = EEPROM.read((i * 2) + 3 + wakeTimesAddress);
	}
}

Sleep::~Sleep() {
	free(this->wakeTimes);
	this->wakeTimes = NULL;
}

void Sleep::enableSleep() {
	/* Set sleep mode */
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	/* Set sleep enable bit */
	sleep_enable();

	/* Actually sleep */
	sleep_mode();

	/* Resume sleep from here */
	sleep_disable();

	/* Enable all things */
	power_all_enable();
}

void Sleep::setupSleep() {
	/*** Setup the WDT ***/

	/* Clear the reset flag. */
	MCUSR &= ~(1<<WDRF);

	/* In order to change WDE or the prescaler, we need to
	 * set WDCE (This will allow updates for 4 clock cycles).
	 */
	WDTCSR |= (1<<WDCE) | (1<<WDE);

	/* set new watchdog timeout prescaler value */
	WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */

	/* Enable the WD interrupt (note no reset). */
	WDTCSR |= _BV(WDIE);
}

void Sleep::sleepUntil(time_t untilTime) {
	time_t currentTime;
	/* Sensor is powered up when needed */
	digitalWrite(EN_PIN1, LOW);
	digitalWrite(EN_PIN2, LOW);
	digitalWrite(RTC_EN, LOW);
	radio->powerDown();

	this->setupSleep();

	do {
		this->enableSleep();
		digitalWrite(RTC_EN, HIGH);
		currentTime = rtc->get();
		digitalWrite(RTC_EN, LOW);
	} while(untilTime > currentTime);

	digitalWrite(RTC_EN, HIGH);
	digitalWrite(EN_PIN1, HIGH);

	radio->powerUp();
}

time_t Sleep::getNextWakeTime() {
	if(this->numWakeTimes == 0) {
		return rtc->get();
	}
	tmElements_t tm;
	rtc->read(tm);
	uint8_t smallestIndex;
	int smallestValue = 2880; /* 2 days from now */
	for(int i = 0; i < this->numWakeTimes; i++) {
		uint8_t hour = this->wakeTimes[i].hour;
		/* If the hour is 0xFF then that is every hour.
		 *  If this is the case see if it has already happened this hour.
		 *  If it has not then report for this hour else if not the next hour.
		 */
		if(hour == 0xFF) {
			hour = (this->wakeTimes[i].minute > tm.Minute) ? tm.Hour : (tm.Hour + 1);
		}
		int value = (hour - tm.Hour) * 60 + this->wakeTimes[i].minute - tm.Minute;
		if(value < 0) {
			value += 24 * 60;
		}
		if(value < smallestValue) {
			smallestValue = value;
			smallestIndex = i;
		}
	}
	
	uint8_t hour = this->wakeTimes[smallestIndex].hour;
	/* If the hour is 0xFF then that is every hour.
	 *  If this is the case see if it has already happened this hour.
	 *  If it has not then report for this hour else if not the next hour.
	 */
	if(hour == 0xFF) {
		hour = (this->wakeTimes[smallestIndex].minute > tm.Minute) ? tm.Hour : (tm.Hour + 1);
	}
	tm.Hour = hour;
	tm.Minute = this->wakeTimes[smallestIndex].minute;
	return makeTime(tm);
}

void printTime(char *str, time_t t) {
	char buffer[40];
	sprintf(buffer, "%s %d:%d\r\n", str, hour(t), minute(t));
	Serial.print(buffer);
}

bool Sleep::shouldBeAwake() {
	if(this->numWakeTimes == 0) {
		return true;
	}
	tmElements_t start, end;
	rtc->read(start);
	rtc->read(end);
	for(int i = 0; i < this->numWakeTimes; i++) {
		uint8_t hour = this->wakeTimes[i].hour;
		/* If the hour is 0xFF then that is every hour.
		 *  Since we're checking whether it should be awake, the hour will be the current.
		 */
		if(hour == 0xFF) {
			hour = start.Hour;
		}
		start.Hour = hour;
		start.Minute = this->wakeTimes[i].minute;
		end.Hour = hour + (this->wakeTimes[i].minute + this->wakeLength) / 60;
		end.Minute = (this->wakeTimes[i].minute + this->wakeLength) % 60;
		if(rtc->get() > makeTime(start) && rtc->get() < makeTime(end)) {
			return true;
		}
	}
	return false;
}

void Sleep::checkSleep() {
	if(!(rtc->get() >= this->nextWake && rtc->get() < this->nextSleep)) {
		if(!this->shouldBeAwake()) {
			nextWake = getNextWakeTime();
			nextSleep = nextWake + (wakeLength * 60);
			sleepUntil(nextWake);
		}
	}
}

ISR(WDT_vect) {
	/* We need to register an ISR for it to wake rather than reset */
}
