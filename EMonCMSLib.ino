#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "EMonCMS.h"
#include "Debug.h"
#include "PTMNRS485.h"

#define MAX_PACKET_SIZE 64
#define RESETEEPROM 0
#define RF24NODEIDEEPROM 1
#define EMONNODEIDEEPROM1 2
#define EMONNODEIDEEPROM2 3
#define ATTR_REGISTERED_START 4

#define RX_PIN 10
#define TX_PIN 9
#define EN_TX_PIN 6

RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

EMonCMS *emon = NULL;

PTMNRS485 sensor(RX_PIN, TX_PIN, EN_TX_PIN);

enum ATTRS {
	ATTR_TIME,
	ATTR_PRESSURE,
	NUM_ATTR
};

unsigned char nodeID;
uint64_t timeData = 0;
AttributeValue attrVal[NUM_ATTR];
unsigned char incoming_buffer[MAX_PACKET_SIZE];

int timeAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	LOG(F("timeAttributeReader: enter\r\n"));
	timeData = millis();
	item->type = ULONG;
	item->item = &timeData;
	LOG(F("timeAttributeReader: done\r\n"));
	return 0;
}

int pressureAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	LOG(F("pressureAttributeReader: enter\r\n"));
	/*
	if(!sensor.blockingRead()) {
		LOG(F("pressureAttributeReader: Sensor read failed\r\n"));
		return 1;
	} else {
		LOG(F("pressureAttributeReader: sensor read successfully\r\n"));
	}
	*/
	item->type = USHORT;
	item->item = sensor.getReadingPtr();
	LOG(F("pressureAttributeReader: exit\r\n"));
	return 0;
}

int networkWriter(unsigned char type, unsigned char *buffer, int length) {
	if(!mesh.write(buffer, type, length)){
      // If a write fails, check connectivity to the mesh network
      if( mesh.checkConnection() ){
        //refresh the network address
        mesh.renewAddress(); 
        if(!mesh.write(buffer, type, length)){
        	LOG(F("networkWriter: failed\r\n"));
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

void setup() {
#ifdef DEBUG
	Serial.begin(115200);
	EEPROM.write(RESETEEPROM, 1);
#endif
	/* if the EEPROM is anything but 0 then reset all fields */
	if(EEPROM.read(RESETEEPROM)) {
		for(int i = 0; i < 10; i++) {
			EEPROM.write(i, 0);
		}
	}

	/* load RF24 node id from EEPROM */
	nodeID = EEPROM.read(RF24NODEIDEEPROM);
	/* if it's unset, assign a random one */
	if(nodeID == 0) {
		nodeID = random(220, 255);
		EEPROM.write(RF24NODEIDEEPROM, nodeID);
	}
	
	LOG(F("Node id is ")); LOG(nodeID); LOG(F("\r\n"));
	mesh.setNodeID(nodeID);
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


