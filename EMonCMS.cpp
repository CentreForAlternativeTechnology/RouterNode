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

bool parseEMonCMSPacket(HeaderInfo *header, DataItem *items[], networkRead read, void *callbackData) {
	if(!isEMonCMSPacket(header->type)) {
		return false;
	}

	if(header->dataCount > 0) {
		(*items) = (DataItem *)malloc(sizeof(DataItem) * header->dataCount);
		for(int i=0; i<header->dataCount; i++) {
			(*items)[i].item = NULL;
		}
		for(int i=0; i<header->dataCount; i++) {
			/* Read the item type */
			if(read(callbackData, 1, &(*items)[i].type) != 1) {
				/* If it failed to read a byte then return false after freeing 
				 *  memory allocated because of this function
				 */
				for(int j=0; j<=i; j++) {
					if((*items)[i].item != NULL) {
						free((*items)[i].item);
					}
				}
				free(*items);
				items = NULL;
				return false;
			} else {
				/* If the type was successfully read then read the rest
				 *  of the bytes for that type.
				 */
				int size = getTypeSize((*items)[i].type);
				(*items)[i].item = malloc(size);
				if(read(callbackData, size, (*items)[i].item) != size) {
					/* If it failed to read the bytes for this type then free
					 *  the memory allocated by this function and return false.
					 */
					for(int j=0; j<=i; j++) {
						if((*items)[i].item != NULL) {
							free((*items)[i].item);
						}
					}
					free(*items);
					items = NULL;
					return false;
				}
			}
		}
	}

	return true;
}
