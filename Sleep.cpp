#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "Sleep.h"

Sleep::Sleep(DS1302RTC *rtc, RF24 *radio) {
	this->rtc = rtc;
	this->radio = radio;
}

Sleep::~Sleep() {
	/* Nothing to destroy */
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
	radio->powerDown();

	this->setupSleep();

	do {
		this->enableSleep();
		digitalWrite(RTC_EN, HIGH);
		currentTime = rtc->get();
		digitalWrite(RTC_EN, LOW);
	} while(untilTime > currentTime);

	digitalWrite(EN_PIN1, HIGH);

	radio->powerUp();
}

ISR(WDT_vect) {
	/* We need to register an ISR for it to wake rather than reset */
}
