#define MAX_PACKET_SIZE 512

#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>

#include "EMonCMS.h"
#include "Debug.h"

RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

EMonCMS emonCMS;

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

		if(emonCMS.isEMonCMSPacket(header.type)) {
			HeaderInfo emonCMSHeader;
			/* Setup an EMonCMS packet */
			if(network.read(header, &emonCMSHeader, 4) == 4) {
				emonCMSHeader.type = header.type;
				if(emonCMSHeader.dataSize < MAX_PACKET_SIZE) {
					/* Setup buffers for storing the read data and parsing
					 *  it to a reable format.
					 */
					unsigned char buffer[emonCMSHeader.dataSize];
					DataItem items[emonCMSHeader.dataCount];
					if(network.read(header, buffer, emonCMSHeader.dataSize) != emonCMSHeader.dataSize) {
						LOG("Failed to read entire EMonCMS data packet\n");
					} else {
						if(!emonCMS.parseEMonCMSPacket(&emonCMSHeader, buffer, items)) {
							LOG("Failed to parse EMonCMS packet\n");
						}
					}
				} else {
					LOG("Received packet too large, discarding\n");
					network.read(header,0,0); 
				}
			} else {
				LOG("Failed to read header bytes");
			}
		} else {
			network.read(header,0,0); 
			LOG("Unknown packet type, discarding\n");
		}
	}
}

