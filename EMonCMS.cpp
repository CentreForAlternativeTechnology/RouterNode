#include "EMonCMS.h"
#include "Debug.h"

EMonCMS::EMonCMS(AttributeValue values[],
			int length, 
			NetworkSender sender,
			AttributeRegistered attrRegistered,
			NodeIDRegistered nodeRegistered,
			unsigned short nodeID
			)
{
	this->attrValues = values;
	this->attrValuesLength = length;
	this->networkSender = sender;
	this->nodeID = nodeID;
	this->attrRegistered = attrRegistered;
	this->nodeRegistered = nodeRegistered;
	this->lastRegisterRequest = 0;
	#ifdef LINUX
	this->start_time = time(0);
	#endif
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

void EMonCMS::registerNode() {
	if((millis() - this->lastRegisterRequest) > REGISTERREQUESTTIMEOUT) {
		LOG(F("registerNode: enter\r\n"));
		if(this->nodeID == 0) {
			LOG(F("registerNode: registering node\r\n"));
			if(this->attrSender(NODE_REGISTER, NULL, 0) > 0) {
				LOG(F("Sent a request for node ID\r\n"));
			} else {
				LOG(F("Failed to send node ID request\r\n"));
			}
			LOG(F("registerNode: request sent\r\n"));
			this->lastRegisterRequest = millis();
		} else {
			for(int i = 0; i < attrValuesLength; i++) {
				LOG(F("registerNode: attr: ")); LOG(i); LOG(F("\r\n"));
				if(!this->attrValues[i].registered) {
					DataItem regItems[4];
					this->attrIdentAsDataItems(&(this->attrValues[i].attr), regItems);
					if(this->attrValues[i].reader(&(this->attrValues[i].attr), &(regItems[4]))) {
						LOG(F("registerNode: Failed to register attribute\r\n"));
					} else {
						LOG(F("registerNode: registering attribute ")); LOG(i); LOG(F("\r\n"));
						if(this->attrSender(ATTR_REGISTER, regItems, 4) > 0) {
							LOG(F("registerNode: Sent attribute register request\r\n"));
						} else {
							LOG(F("registerNode: Error sending attribute registration request\r\n"));
						}
					}
					LOG(F("registerNode: attribute registered\r\n"));
				}
			}
			LOG(F("registerNode: setting last time\r\n"));
			this->lastRegisterRequest = millis();
		}
		LOG(F("registerNode: done\r\n"));
	}	
}

AttributeValue *EMonCMS::getAttribute(AttributeIdentifier *attr) {
	for(int i = 0; i < this->attrValuesLength; i++) {
		if(this->attrValues[i].attr.groupID == attr->groupID &&
			this->attrValues[i].attr.attributeID == attr->attributeID &&
			this->attrValues[i].attr.attributeNumber == attr->attributeNumber)
		{
			return &(this->attrValues[i]);
		}
	}
	return NULL;
}

bool EMonCMS::requestAttribute(HeaderInfo *header, DataItem items[]) {
	/* extract the attribute identifying information */
	AttributeIdentifier ident;
	ident.groupID = *(unsigned short *)(items[1].item);
	ident.attributeID = *(unsigned short *)(items[2].item);
	ident.attributeNumber = *(unsigned short *)(items[3].item);
	
	Status status = SUCCESS;

	/* first do we have attribute, if not send back packet with error */
	AttributeValue *attrVal = this->getAttribute(&ident);
	DataItem item;
	if(attrVal == NULL) {
		status = UNSUPPORTED_ATTRIBUTE;
	} else if(!attrVal->reader(&ident, &item)) {
		status = INVALID_VALUE;
	}

	if(status != SUCCESS) {
		int size = attrSize(ATTR_FAILURE, &(items[1]), 3);
		unsigned char failureBuffer[size];
		if(attrBuilder(ATTR_FAILURE, &(items[1]), 3, failureBuffer) != size) {
			LOG(F("Error: could not build response request failure"));
			return false;
		} else {
			((HeaderInfo *)failureBuffer)->status = status;
			if(!this->networkSender('p', failureBuffer, size)) {
				LOG(F("Error sending error response to attribute request\r\n"));
			}
		}
	} else {
		DataItem responseItems[4];
		memcpy(responseItems, items, sizeof(DataItem) * 3);
		responseItems[3].type = item.type;
		responseItems[3].item = item.item;
		int size = attrSize(ATTR_POST, responseItems, 4);
		unsigned char responseBuffer[size];
		if(attrBuilder(ATTR_POST, responseItems, 4, responseBuffer) != size) {
			LOG(F("Error: could not build response request failure"));
			return false;
		} else {
			if(!this->networkSender('p', responseBuffer, size)) {
				LOG(F("Error sending success response to attribute request\r\n"));
			}
		}
				
	}
	
	return true;
}

bool EMonCMS::parseEMonCMSPacket(HeaderInfo *header, unsigned char type, unsigned char *buffer, DataItem items[]) {
	LOG(F("parseEmonCMSPacket: enter\r\n"));
	if(!isEMonCMSPacket(type)) {
		LOG(F("parseEmonCMSPacket: not emon cms packet\r\n"));
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
				LOG(F("Server did not return valid status code (node register)\r\n"));
			} else {
				this->nodeID = T_USHORT(items[0].item);
				LOG(F("emonCMSNodeID = ")); LOG(this->nodeID); LOG(F("\r\n"));
				if(this->nodeRegistered != NULL) {
					this->nodeRegistered(this->nodeID);
				}
			}
			break;
		case 'P':
			if(!requestAttribute(header, items)) {
				LOG(F("Error responding to attribute request\r\n"));
				return false;
			}
			break;
		case 'a':
			if(header->status != SUCCESS) {
				LOG(F("Server did not return success for attribute register\r\n"));
			} else {
				AttributeIdentifier ident;
				ident.groupID = *(unsigned short *)(items[1].item);
				ident.attributeID = *(unsigned short *)(items[2].item);
				ident.attributeNumber = *(unsigned short *)(items[3].item);
				LOG(F("Attribute registration response success\r\n"));
				if(getAttribute(&ident) != NULL) {
					getAttribute(&ident)->registered = 1;
					if(this->attrRegistered != NULL) {
						this->attrRegistered(&ident);
					}
				} else {
					LOG(F("Attribute not found in list\r\n"));
				}
			}
			break;
		default:
			LOG(F("Unknown header type ")); LOG(type); LOG(F("\r\n"));
			return false;
	}
	
	LOG(F("parseEmonCMSPacket: exit\r\n"));

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
		case ATTR_FAILURE:
			/* fallthrough */
		case ATTR_POST:
			size += (sizeof(nodeID) + 1);
			break;
		case NODE_REGISTER:
			/* nothing */
			break;
		default:
			LOG(F("Requested size of unknown\r\n"));
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

int EMonCMS::attrSender(RequestType type, DataItem *items, int length) {
		LOG(F("attrSender: enter\r\n"));
		int size = this->attrSize(type, items, length);
		if(size == 0) {
			LOG(F("attrSender: attrSize 0\r\n"));
			return size;
		}
		unsigned char buffer[size];
		if(this->attrBuilder(type, items, length, buffer) != size) {
			LOG(F("attrSender: attrBuilder size mismatch\r\n"));
			return 0;
		}
		LOG(F("attrSender: exit\r\n"));
		return this->networkSender(type, buffer, size);
}

int EMonCMS::attrBuilder(RequestType type, DataItem *items, int length, unsigned char *buffer) {
	LOG(F("attrBuilder: enter\r\n"));
	/* Setup the header and input neccessary data */
	HeaderInfo *header = (HeaderInfo *)buffer;
	header->dataSize = 0;
	for(int i = 0; i < length; i++) {
		header->dataSize += sizeof(items[i].type) + this->getTypeSize(items[i].type);
	}
	
	DataItem nid;

	int itemIndex = sizeof(HeaderInfo);
	switch(type) {
		case ATTR_REGISTER:
		case ATTR_POST:
			if(length != 4) {
				LOG(F("Wrong number of items passed to builder for post/register\r\n"));
				return 0;
			}
			if(this->nodeID == 0) {
				LOG(F("Cannot register/post attribute, no node iD\r\n"));
				return 0;
			}
			header->dataCount = 5; /* NID, GID, AID, ATTRNUM, ATTRVAL/ATTRDEFAULT */
			header->dataSize += (sizeof(nodeID) + 1);
			header->status = SUCCESS;
			/* Add Node ID to packet */
			nid.type = USHORT;
			nid.item = &(this->nodeID);
			itemIndex += dataItemToBuffer(&nid, &(buffer[itemIndex]));
			break;
		case ATTR_FAILURE:
			if(length != 3) {
				LOG(F("Wrong number of items passed to builder for failure\r\n"));
				return 0;
			}
			if(this->nodeID == 0) {
				LOG(F("Cannot post attribute failure, no node iD\r\n"));
				return 0;
			}
			header->dataCount = 4; /* NID, GID, AID, ATTRNUM */
			header->dataSize += (sizeof(nodeID) + 1);
			header->status = FAILURE; /* set custom error code later */
			/* Add Node ID to packet */
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
			LOG(F("Requested build of unknown\r\n"));
			return 0;
	}

	for(int i = 0; i < length; i++) {
		itemIndex += dataItemToBuffer(&(items[i]), &(buffer[itemIndex]));
	}
	LOG(F("attrBuilder: exit\r\n"));
	return itemIndex;
}

#ifdef LINUX
unsigned long EMonCMS::millis() {
	return (unsigned long)difftime(time(0), start_time);
}
#endif
