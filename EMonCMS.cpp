#include <RF24.h>
#include <RF24Network.h>
#include "EMonCMS.h"
#include "Debug.h"

EMonCMS::EMonCMS() {
	this->nodeID = 0;
}

EMonCMS::~EMonCMS() {
	
}

bool EMonCMS::isEMonCMSPacket(unsigned char type) {
	switch(type) {
		case 'r':
			/* Acknowledged register */
		case 'a':
			/* Acknowledged attribute registration */
		case 'p': 
			/* Acknowledged post */
		case 'P':
			/* Request for attribute */
			return true;
		default:
			return false;
	}
}

int EMonCMS::getTypeSize(unsigned char type) {
	switch(type) {
		case STRING: case CHAR: case UCHAR:
			return 1;
		case SHORT: case USHORT:
			return 2;
		case INT: case UINT: case FLOAT:
			return 4;
		case LONG: case ULONG:
			return 8;
		default:
			return 0;
	}
}

bool EMonCMS::checkHeader(HeaderInfo *header, unsigned char size) {
	return header->dataCount == size;
}

bool EMonCMS::parseEMonCMSPacket(HeaderInfo *header, unsigned char *buffer, DataItem items[]) {
	if(!isEMonCMSPacket(header->type)) {
		return false;
	}

	if(header->dataCount > 0) {
		int index = 0;
		/* For each of the data items in the buffer set them up
		 *  in data items.
		 */
		for(int i = 0; i < header->dataCount; i++) {
			items[i].type = buffer[index];
			index++;
			items[i].item = &(buffer[index]);
			index += getTypeSize(items[i].type);
		}
	}

	switch(header->type) {
		case 'r':
			if(!checkHeader(header, 1)) {
				break;
			}
			this->nodeID = T_USHORT(items[0].item);
			LOG("emonCMSNodeID = "); LOG(this->nodeID); LOG("\n");
			break;
			
	}

	return true;
}
