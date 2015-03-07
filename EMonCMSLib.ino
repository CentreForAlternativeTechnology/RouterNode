#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>
#include "EMonCMS.h"
#include "Debug.h"

#define MAX_PACKET_SIZE 512
#define RESETEEPROM 0
#define RF24NODEIDEEPROM 1
#define EMONNODEIDEEPROM1 2
#define EMONNODEIDEEPROM2 3
#define EMONATTRREGISTERDEEPROM 4

RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

EMonCMS *emon = NULL;

unsigned char nodeID;
unsigned long timeData = 0;
unsigned long nodeIDRequestTime = 0;
AttributeValue attrVal;

int timeAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	timeData = millis();
	item->type = ULONG;
	item->item = &timeData;
	return 0;
}

int networkWriter(unsigned char type, unsigned char *buffer, int length) {
	RF24NetworkHeader header(0, type);
	return network.write(header, buffer, length) ? length : 0;
}

void nodeIDRegistered(unsigned short emonNodeID) {
	/* save the node id into EEPROM */
	EEPROM.write(EMONNODEIDEEPROM1, (emonNodeID >> 8) & 0xFF);
	EEPROM.write(EMONNODEIDEEPROM2, (emonNodeID & 0xFF));
}

void attributeRegistered(AttributeIdentifier *attr) {
	/* save that the single attribute has been registered */
	EEPROM.write(EMONATTRREGISTERDEEPROM, 1);
}

void setup() {
#ifdef DEBUG
	Serial.begin(115200);
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
	
	LOG(F("Node id is ")); LOG(nodeID); LOG(F("\n"));
	mesh.setNodeID(nodeID);
	LOG(F("Connecting to mesh...\n"));
	mesh.begin();

	/* setup the time reading attribute */
	attrVal.attr.groupID = 10;
	attrVal.attr.attributeID = 20;
	attrVal.attr.attributeNumber = 40;
	attrVal.reader = timeAttributeReader;
	attrVal.registered = EEPROM.read(EMONATTRREGISTERDEEPROM);

	/* read the emoncms node id and init emonCMS */
	unsigned short eMonNodeID = ((EEPROM.read(EMONNODEIDEEPROM1) & 0xFF) << 8) | (EEPROM.read(EMONNODEIDEEPROM2) & 0xFF);
	emon = new EMonCMS(&attrVal, 1, networkWriter, attributeRegistered, nodeIDRegistered, eMonNodeID);
}

void loop() {
	mesh.update();
	/* check to see whether we have a node id */
	emon->registerNode();
	
	if(network.available()) {
		RF24NetworkHeader header;
		network.peek(header);

		if(emon->isEMonCMSPacket(header.type)) {
			HeaderInfo emonCMSHeader;
			/* Setup an EMonCMS packet */
			if(network.read(header, &emonCMSHeader, 4) == 4) {
				if(emonCMSHeader.dataSize < MAX_PACKET_SIZE) {
					/* Setup buffers for storing the read data and parsing
					 *  it to a reable format.
					 */
					unsigned char buffer[emonCMSHeader.dataSize];
					DataItem items[emonCMSHeader.dataCount];
					if(network.read(header, buffer, emonCMSHeader.dataSize) != emonCMSHeader.dataSize) {
						LOG(F("Failed to read entire EMonCMS data packet\n"));
					} else {
						if(!emon->parseEMonCMSPacket(&emonCMSHeader, header.type, buffer, items)) {
							LOG(F("Failed to parse EMonCMS packet\n"));
						}
					}
				} else {
					LOG(F("Received packet too large, discarding\n"));
					network.read(header,0,0); 
				}
			} else {
				LOG(F("Failed to read header bytes"));
			}
		} else {
			network.read(header,0,0); 
			LOG(F("Unknown packet type, discarding\n"));
		}
	}
}

