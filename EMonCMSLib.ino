#define MAX_PACKET_SIZE 512
#define RESETEEPROM 0
#define RF24NODEIDEEPROM 1
#define EMONNODEIDEEPROM1 2
#define EMONNODEIDEEPROM2 3

#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>

#include "EMonCMS.h"
#include "Debug.h"

#define NODEIDREQUESTTIMEOUT 5000

RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

EMonCMS *emon = NULL;

unsigned char nodeID;
unsigned short eMonNodeID = 0;
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

void setup() {
#ifdef DEBUG
	Serial.begin(115200);
#endif
	if(EEPROM.read(RESETEEPROM)) {
		for(int i = 0; i < 10; i++) {
			EEPROM.write(i, 0);
		}
	}
	nodeID = EEPROM.read(RF24NODEIDEEPROM);
	eMonNodeID == ((EEPROM.read(EMONNODEIDEEPROM1) & 0xFF) << 8) | (EEPROM.read(EMONNODEIDEEPROM2) & 0xFF);
	LOG(F("Node id is ")); LOG(nodeID); LOG(F("\n"));
	mesh.setNodeID(nodeID);
	LOG(F("Connecting to mesh...\n"));
	mesh.begin();

	attrVal.attr.groupID = 10;
	attrVal.attr.attributeID = 20;
	attrVal.attr.attributeNumber = 40;
	attrVal.reader = timeAttributeReader;
	attrVal.registered = false;

	emon = new EMonCMS(&attrVal, 1, networkWriter, eMonNodeID);

}

void loop() {
	mesh.update();
	/* check to see whether we have a node id */
	if(emon->getNodeID() == 0 && (millis() - nodeIDRequestTime) > NODEIDREQUESTTIMEOUT) {
		if(emon->attrSender(NODE_REGISTER, NULL, 0) > 0) {
			LOG(F("Sent a request for node ID\n"));
		} else {
			LOG(F("Failed to send node ID request\n"));
		}
		nodeIDRequestTime = millis();
	} else if(emon->getNodeID() > 0 && (millis() - nodeIDRequestTime) > NODEIDREQUESTTIMEOUT) {
		DataItem regItems[4];
		emon->attrIdentAsDataItems(&(attrVal.attr), regItems);
		unsigned long def = 0;
		regItems[3].type = ULONG;
		regItems[3].item = &def;
		if(emon->attrSender(ATTR_REGISTER, regItems, 4) > 0) {
			LOG(F("Sent attribute register request for time\n"));
		} else {
			LOG(F("Error sending attribute registration request\n"));
		}
		nodeIDRequestTime = millis();
	}

	if(eMonNodeID != emon->getNodeID() && emon->getNodeID() != 0) {
		eMonNodeID = emon->getNodeID();
		EEPROM.write(EMONNODEIDEEPROM1, (eMonNodeID >> 8) & 0xFF);
		EEPROM.write(EMONNODEIDEEPROM2, (eMonNodeID & 0xFF));
	}
	
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

