#ifdef LINUX

#include "Debug.h"
#include "EMonCMS.h"

#include <iostream>
#include <fstream>
#include <cstring>

int main(int argc, char *args[]) {
	EMonCMS emon;
	int failure = 0;
	int passCount = 0;
	int regSize = 0;
	if((regSize = emon.attrSize(NODE_REGISTER, NULL, 0)) != 4) {
		failure++;
		std::cout << "ERR: Size of register should be 4, is " << regSize << "\n";
	} else {
		passCount++;
	}
	unsigned char c[] = { 0x00, 0x00, 0x00, 0x00 };
	unsigned char buffer[regSize];
	emon.attrBuilder(NODE_REGISTER, NULL, 0, buffer);
	if(memcmp(c, buffer, 4) != 0) {
		failure = 1;
		std::cout << "ERR: wrong packet generated for node register\n";
	} else {
		passCount++;
	}

	DataItem regItems[4];
	AttributeIdentifier regIdent;
	regIdent.groupID = 0x6453;
	regIdent.attributeID = 0x2321;
	regIdent.attributeNumber = 0x0321;
	emon.attrIdentAsDataItems(&regIdent, regItems);

	unsigned long fakeReading = 200200;
	regItems[3].type = ULONG;
	regItems[3].item = &fakeReading;

	regSize = 0;
	if((regSize = emon.attrSize(ATTR_REGISTER, regItems, 4)) != 25) {
		failure++;
		std::cout << "ERR: wrong packet size for attribute register " << regSize << "\n";
	} else {
		passCount++;
	}
	unsigned char regBuffer[regSize];
	emon.attrBuilder(ATTR_REGISTER, regItems, 4, regBuffer);
	
	std::cout << passCount << " passes of " << (passCount + failure) << "\n";
	
	
	return 0;
}

#endif