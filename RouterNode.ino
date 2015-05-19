#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Time.h>
#include <DS1302RTC.h>
#include "Definitions.h"
#include "SerialEventHandler.h"
#include "Sleep.h"

#define DEBUG
#include "Debug.h"

/* Radio and communication related definitions */
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

/* Real-time clock */
DS1302RTC rtc(RTC_CLK, RTC_DATA, RTC_RST);

/* Sleep controller */
Sleep sleep(&rtc, &radio, EEPROM_ALARM_START);

/* called to enter programming mode */
void programmingMode() {
	/* enable serial */
	Serial.begin(115200);
	Serial.println("Programming Mode");
	SerialEventHandler serialEvent(&rtc);
	/* power up peripherals */
	digitalWrite(RTC_EN, HIGH);
	radio.begin();
	/* attempt to read from serial and parse */
	while(true) {
		serialEvent.parseSerial();
	}
}

void setup() {
	/* use the 1.1V internal analog reference voltage, for battery level */
	analogReference(INTERNAL);

	/* enable pullup on the pin that controls programming mode */
	pinMode(PROG_MODE_PIN, INPUT_PULLUP);

	/* Enable the RTC */
	pinMode(RTC_EN, OUTPUT);
	digitalWrite(RTC_EN, HIGH);

	/* seed the random nubmer table with the current timestamp */
	randomSeed(rtc.get());

	/* if the EEPROM is anything but 0 then reset all fields */
	if(EEPROM.read(RESETEEPROM)) {
		for(int i = 0; i < 1024; i++) {
			EEPROM.write(i, 0);
		}
	}

	/* if the RF24 Node ID is unset, assign a random one */
	if(EEPROM.read(RF24NODEIDEEPROM) == 0) {
		/* random to no more than 250 because for some reason it
		 *  doesn't register with the gateway correctly otherwise.
		 */
		EEPROM.write(RF24NODEIDEEPROM, random(220, 248));
	}

	/* if the progrmaming mode pin is pulled down, enter programming mode */
	if(!digitalRead(PROG_MODE_PIN)) {
		programmingMode();
	} else {
		/* initialise debug */
		DEBUG_INIT;
	}

	/* read the rf24 node ID and set it */
	LOG(F("Node id is ")); LOG(EEPROM.read(RF24NODEIDEEPROM)); LOG(F("\r\n"));
	mesh.setNodeID(EEPROM.read(RF24NODEIDEEPROM));
	radio.begin();
	radio.setPALevel(RF24_PA_HIGH);
	
	LOG(F("Connecting to mesh...\r\n"));
	mesh.begin(MESH_DEFAULT_CHANNEL, RF24_1MBPS);
	LOG(F("Connected to mesh\r\n"));
}

void loop() {
	/* See if it's sleeping time */
	sleep.checkSleep();

	// If a write fails, check connectivity to the mesh network
	if( ! mesh.checkConnection() ){
		//refresh the network address
		Serial.println("Renewing Address");
		mesh.renewAddress(); 
	}else{
		Serial.println("Send fail, Test OK"); 
	}
	
	mesh.update();
}
