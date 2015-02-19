#define DEBUG

#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>

#include "EMonCMS.h"

#ifdef DEBUG
#define LOG(x) Serial.print(x)
#else
#define LOG(x)
#endif

RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

unsigned char nodeID;

void setup() {
#ifdef DEBUG
	Serial.begin(115200);
#endif
	nodeID = EEPROM.read(0);
	LOG("Node id is "); LOG(nodeID); LOG("\n");
	mesh.setNodeID(nodeID);
	LOG("Connecting to mesh...\n");
	mesh.begin();
	/*
	registerRequest r;
	 	r.header.status = SUCCESS;
	 	r.header.dataCount = 0;
	 	*/
}

int networkReader(void *callbackData, int numBytes, void *buffer) {
	return network.read(*((RF24NetworkHeader*)callbackData), buffer, numBytes);
}

void loop() {
	mesh.update();
	if(network.available()) {
		RF24NetworkHeader header;
		network.peek(header);

		if(isEMonCMSPacket(header.type)) {
			HeaderInfo emonCMSHeader;
			if(network.read(header, &emonCMSHeader, 2) == 2) {
				emonCMSHeader.type = header.type;
				DataItem *items = NULL;
				parseEMonCMSPacket(&emonCMSHeader, &items, networkReader, &header);
			} else {
				LOG("Failed to read header bytes");
			}
		} else {
			network.read(header,0,0); 
			LOG("Unknown packet type, discarding\n");
		}
	}
}

