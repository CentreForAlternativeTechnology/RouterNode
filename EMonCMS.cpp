#include <RF24.h>
#include <RF24Network.h>
#include "EMonCMS.h"

bool isEMonCMSPacket(unsigned char type) {
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

int getTypeSize(unsigned char type) {
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

bool parseEMonCMSPacket(HeaderInfo *header, unsigned char *buffer, DataItem items[]) {
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

	return true;
}
