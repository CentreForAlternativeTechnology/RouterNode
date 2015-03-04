#include "EMonCMS.h"
#include "Debug.h"

EMonCMS::EMonCMS(AttributeValue values[], int length, NetworkSender sender) {
	this->nodeID = 0;
	this->attrValues = values;
	this->attrValuesLength = length;
	this->networkSender = sender;
}

EMonCMS::~EMonCMS() {
	/* do nothing */
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
		case CHAR: case UCHAR:
			return sizeof(char);
		case SHORT: case USHORT:
			return sizeof(short);
		case INT: case UINT:
			return sizeof(int);
		case FLOAT:
			return sizeof(float);
		case LONG: case ULONG:
			return sizeof(long);
		default:
			return 0;
	}
}

bool EMonCMS::checkHeader(HeaderInfo *header, unsigned char size) {
	return header->dataCount == size;
}

bool EMonCMS::hasAttribute(AttributeIdentifier attr) {
	for(int i = 0; i < this->attrValuesLength; i++) {
		if(this->attrValues[i].attr.groupID == attr.groupID &&
			this->attrValues[i].attr.groupID == attr.attributeID &&
			this->attrValues[i].attr.groupID == attr.attributeNumber)
		{
			return true;
		}
	}
	return false;
}

bool EMonCMS::parseEMonCMSPacket(HeaderInfo *header, unsigned char type, unsigned char *buffer, DataItem items[]) {
	if(!isEMonCMSPacket(type)) {
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

	switch(type) {
		case 'r':
			if(!checkHeader(header, 1)) {
				break;
			}
			if(header->status != SUCCESS) {
				LOG(F("Server did not return valid status code\n"));
			} else {
				this->nodeID = T_USHORT(items[0].item);
				LOG(F("emonCMSNodeID = ")); LOG(this->nodeID); LOG(F("\n"));
			}
			break;
		case 'P':
			/* extract the attribute identifying information */
			AttributeIdentifier ident;
			ident.groupID = *(unsigned short *)(items[1].item);
			ident.attributeID = *(unsigned short *)(items[2].item);
			ident.attributeNumber = *(unsigned short *)(items[3].item);
			
			/* first do we have attribute, if not send back packet with error */
			if(!this->hasAttribute(ident)) {
				/* the buffer just contains the data items and we want to send the same
				 * items back but with a different header with an error status */
				 LOG("Error: Unknown attribute requested, sending error response\n");
				 unsigned char sendBuffer[header->dataSize + sizeof(HeaderInfo)];
				 memcpy(&(sendBuffer[sizeof(HeaderInfo)]), buffer, header->dataSize);
				 ((HeaderInfo *)sendBuffer)->dataSize = header->dataSize;
				 ((HeaderInfo *)sendBuffer)->dataCount = header->dataCount;
				 ((HeaderInfo *)sendBuffer)->status = UNSUPPORTED_ATTRIBUTE;
				 if(!this->networkSender('p', sendBuffer, header->dataSize + sizeof(HeaderInfo))) {
					 LOG("Error: sending error response to attribute request\n");
				 }
			}
			break;

		default:
			LOG(F("Unknown header type ")); LOG(type); LOG(F("\n"));
			return false;
	}

	return true;
}

unsigned short EMonCMS::getNodeID() {
	return this->nodeID;
}

int EMonCMS::attrSize(RequestType type, DataItem *item, int length) {
	int size = sizeof(HeaderInfo);
	for(int i = 0; i < length; i++) {
		size += this->getTypeSize(item[i].type) + 1; /* Actual data size plus type */
	}
	switch(type) {
		case ATTR_REGISTER:
			/* fallthrough */
		case ATTR_POST:
			size += (sizeof(nodeID) + 1);
			break;
		case NODE_REGISTER:
			/* nothing */
			break;
		default:
			LOG(F("Requested size of unknown\n"));
			break;
	}
	return size;
}

int EMonCMS::dataItemToBuffer(DataItem *item, unsigned char *buffer) {
	buffer[0] = item->type;
	unsigned char *k = (unsigned char *)(item->item);
	for(int i = 0; i < this->getTypeSize(item->type); i++) {
		buffer[1 + i] = k[i];
	}
	return sizeof(item->type) + this->getTypeSize(item->type);
}

void EMonCMS::attrIdentAsDataItems(AttributeIdentifier *ident, DataItem *attrItems) {
	/* Attribute identifier to data items */
	attrItems[0].type = USHORT;
	attrItems[0].item = &(ident->groupID);
	attrItems[1].type = USHORT;
	attrItems[1].item = &(ident->attributeID);
	attrItems[2].type = USHORT;
	attrItems[2].item = &(ident->attributeNumber);
}

int EMonCMS::attrBuilder(RequestType type, DataItem *items, int length, unsigned char *buffer) {
	/* Setup the header and input neccessary data */
	HeaderInfo *header = (HeaderInfo *)buffer;
	header->dataSize = 0;
	for(int i = 0; i < length; i++) {
		header->dataSize += sizeof(items[i].type) + this->getTypeSize(items[i].type);
	}

	int itemIndex = sizeof(HeaderInfo);
	switch(type) {
		case ATTR_REGISTER:
		case ATTR_POST:
			if(length != 4) {
				LOG(F("Wrong number of items passed to builder for post/register\n"));
				return 0;
			}
			header->dataCount = 5; /* NID, GID, AID, ATTRNUM, ATTRVAL/ATTRDEFAULT */
			header->dataSize += (sizeof(nodeID) + 1);
			header->status = SUCCESS;
			/* Add Node ID to packet */
			DataItem nid;
			nid.type = USHORT;
			nid.item = &(this->nodeID);
			itemIndex += dataItemToBuffer(&nid, &(buffer[itemIndex]));
			break;
		case NODE_REGISTER:
			header->status = SUCCESS;
			header->dataCount = 0;
			header->dataSize = 0;
			break;
		default:
			LOG(F("Requested build of unknown\n"));
			return 0;
	}

	for(int i = 0; i < length; i++) {
		itemIndex += dataItemToBuffer(&(items[i]), &(buffer[itemIndex]));
	}
	return itemIndex;
}
