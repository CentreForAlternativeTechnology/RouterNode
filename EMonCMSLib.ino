#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include "EMonCMS.h"
#include "Debug.h"
#include "ARandom.h"

#define MAX_PACKET_SIZE 64 /* Maximum size of EMon packet */

/* EEPROM Addresses */
#define RESETEEPROM 0 /* if set to anything over than 0, resets EEPROM. */
#define RF24NODEIDEEPROM 1 /* RF24 NodeID, 0-255 */
#define EMONNODEIDEEPROM1 2 /* unsigned short emon cms node id, LSB */
#define EMONNODEIDEEPROM2 3 /* unsigned short emon cms node id, MSB */
#define ATTR_REGISTERED_START 4 /* start of storage for attributes registered */

/* Enable pins on communication connector */
#define EN_PIN1 10
#define EN_PIN2 9

/* Pin when dragged low goes into config mode */
#define PROG_MODE_PIN  A1

/* Connected to 3.7v battery via resitive divider [Vin - 12k -[SENSE]- 3.3k - GND] */
#define BATTERY_PIN A2

#define RADIO_CE_PIN 7
#define RADIO_CSN_PIN 8

/* Time between posting attributes */
#define ATTR_POST_WAIT 2000

/* Radio and communication related definitions */
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

unsigned char incoming_buffer[MAX_PACKET_SIZE];

EMonCMS *emon = NULL;

/* Attribute definitions */
enum ATTRS {
	ATTR_TIME,
	ATTR_PRESSURE,
	NUM_ATTR
};

AttributeValue attrVal[NUM_ATTR];

/* Data to store attribute readings */
short sensorReading = 0;
uint64_t timeData = 0;

/* Time in millis of last post sent time */
unsigned long lastAttributePostTime = 0;

int timeAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	LOG("timeAttributeReader: enter\r\n");
	timeData = millis();
	item->type = ULONG;
	item->item = &timeData;
	LOG("timeAttributeReader: done\r\n");
	return true;
}

int pressureAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	Wire.requestFrom(4, 2);
	if(Wire.available()) {
		uint8_t buffer[2];
		for(int i = 0; i < 2; i++) {
			buffer[i] = Wire.read();
		}
		
		sensorReading = (buffer[1] << 8) | buffer[0];
  		item->item = &sensorReading;
		item->type = SHORT;
		LOG(F("pressureAttributeReader: value read as ")); LOG(sensorReading); LOG(F("\r\n"));
  		return true;
	} else {
		LOG("pressureAttributeReader: Sensor read failed\r\n");
		return false;
	}
}

int networkWriter(unsigned char type, unsigned char *buffer, int length) {
	if(!mesh.write(buffer, type, length)){
      // If a write fails, check connectivity to the mesh network
      if( mesh.checkConnection() ){
        //refresh the network address
        mesh.renewAddress(); 
        if(!mesh.write(buffer, type, length)){
        	LOG("networkWriter: failed\r\n");
        	return 0;
        }
      }
    }
#ifdef DEBUG
	char sbuff[7];
	for(int i = 0; i < length; i++) {
		sprintf(sbuff, "0x%x, ", buffer[i]);
		LOG(sbuff);
	}
	LOG(F("\r\n"));
#endif
	
	return length;
}

void nodeIDRegistered(unsigned short emonNodeID) {
	/* save the node id into EEPROM */
	EEPROM.write(EMONNODEIDEEPROM1, (emonNodeID >> 8) & 0xFF);
	EEPROM.write(EMONNODEIDEEPROM2, (emonNodeID & 0xFF));
}

void attributeRegistered(AttributeIdentifier *attr) {
	for(int i = 0; i < NUM_ATTR; i++) {
		if(emon->compareAttribute(attr, &(attrVal[i].attr))) {
			EEPROM.write(ATTR_REGISTERED_START + i, 1);
		}
	}
}

void programmingLoop() {
	unsigned char buffer[3];
	if(Serial.available()) {
		for(int i = 0; i < 2; i++) {
			buffer[i] = buffer[i + 1];
		}
		buffer[2] = Serial.read();
	}
	
}

void setup() {
	analogReference(INTERNAL);
	pinMode(PROG_MODE_PIN, INPUT_PULLUP);
	pinMode(EN_PIN1, OUTPUT);
	/* Boot with peripheral disabled */
	digitalWrite(EN_PIN1, LOW);

	DEBUG_INIT;

	randomSeed(arandom());

	if(!digitalRead(PROG_MODE_PIN)) {
#ifdef DEBUG
		EEPROM.write(RESETEEPROM, 1);
#endif
		Serial.begin(115200);
		Serial.println("Programming Mode");
		while(true) {
			programmingLoop();
		}
	}

	LOG(F("Initialising sensor boards..."));
	/* Initialise I2C and enable secondary board */
	Wire.begin();
	digitalWrite(EN_PIN1, HIGH);
	LOG(F("done\r\n"));

	/* if the EEPROM is anything but 0 then reset all fields */
	if(EEPROM.read(RESETEEPROM)) {
		for(int i = 0; i < 10; i++) {
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
	
	LOG(F("Node id is ")); LOG(EEPROM.read(RF24NODEIDEEPROM)); LOG(F("\r\n"));
	mesh.setNodeID(EEPROM.read(RF24NODEIDEEPROM));
	radio.begin();
	radio.setPALevel(RF24_PA_HIGH);
	LOG(F("Connecting to mesh...\r\n"));
	mesh.begin();

	/* setup the time reading attribute */
	attrVal[ATTR_TIME].attr.groupID = 10;
	attrVal[ATTR_TIME].attr.attributeID = 20;
	attrVal[ATTR_TIME].attr.attributeNumber = 40;
	attrVal[ATTR_TIME].reader = timeAttributeReader;
	
	attrVal[ATTR_PRESSURE].attr.groupID = 0x0403;
	attrVal[ATTR_PRESSURE].attr.attributeID = 0x1010;
	attrVal[ATTR_PRESSURE].attr.attributeNumber = 0x0;
	attrVal[ATTR_PRESSURE].reader = pressureAttributeReader;
	
	for(int i = 0; i < NUM_ATTR; i++) {
		attrVal[i].registered = EEPROM.read(ATTR_REGISTERED_START + i);
	}

	/* read the emoncms node id and init emonCMS */
	LOG(F("Setting up emon lib... "));
	unsigned short eMonNodeID = ((EEPROM.read(EMONNODEIDEEPROM1) & 0xFF) << 8) | (EEPROM.read(EMONNODEIDEEPROM2) & 0xFF);
	emon = new EMonCMS(attrVal, NUM_ATTR, networkWriter, attributeRegistered, nodeIDRegistered, eMonNodeID);
	LOG(F("done\r\n"));
}

void loop() {
	mesh.update();
	/* check to see whether we have a node id */
	emon->registerNode();

	if(millis() - lastAttributePostTime > ATTR_POST_WAIT && attrVal[ATTR_PRESSURE].registered) {
		if(emon->postAttribute(&(attrVal[ATTR_PRESSURE].attr)) > 0) {
			LOG(F("Sent attribute post\r\n"));
			lastAttributePostTime = millis();
		} else {
			LOG(F("Failed to post attribute\r\n"));
		}
	}
	
	if(network.available()) {
		RF24NetworkHeader header;
		network.peek(header);

		if(emon->isEMonCMSPacket(header.type)) {
			//HeaderInfo emonCMSHeader;
			/* Setup an EMonCMS packet */
			int read = 0;
			if((read = network.read(header, incoming_buffer, MAX_PACKET_SIZE)) > sizeof(HeaderInfo)) {
				if(((HeaderInfo *)incoming_buffer)->dataSize < (MAX_PACKET_SIZE - sizeof(HeaderInfo))) {
					/* Setup buffers for storing the read data and parsing
					 *  it to a reable format.
					 */
					//unsigned char buffer[emonCMSHeader.dataSize];
					DataItem items[((HeaderInfo *)incoming_buffer)->dataCount];
					//if((read = network.read(header, buffer, emonCMSHeader.dataSize)) == emonCMSHeader.dataSize) {
					//	LOG(F("Failed to read entire EMonCMS data packet\r\n"));
					//	LOG(F("Received ")); LOG(read); LOG(F(" bytes\r\n"));
					//} else {
					if(((HeaderInfo *)incoming_buffer)->dataSize > (read - sizeof(HeaderInfo))) {
						LOG(F("Size mismatch for incoming packet\r\n"));
						LOG(F("Received ")); LOG(read - sizeof(HeaderInfo));
						LOG(F(" expected ")); LOG(((HeaderInfo *)incoming_buffer)->dataSize);
						LOG(F("\r\n"));
					} else {
						LOG(F("Parsing incoming packet...\r\n"));
						if(!emon->parseEMonCMSPacket(((HeaderInfo *)incoming_buffer),
							header.type,
							&(incoming_buffer[sizeof(HeaderInfo)]),
							items))
						{
							LOG(F("Failed to parse EMonCMS packet\r\n"));
						}
					}
					//}
				} else {
					LOG(F("Received packet too large, discarding\r\n"));
					network.read(header,0,0); 
				}
			} else {
				LOG(F("Failed to read header bytes\r\n"));
				LOG(F("Received ")); LOG(read); LOG(F(" bytes\r\n"));
			}
		} else {
			network.read(header,0,0); 
			LOG(F("Unknown packet type, discarding\r\n"));
		}
	}
}


