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

	unsigned long fakeDefault = 200200;
	regItems[3].type = ULONG;
	regItems[3].item = &fakeDefault;

	regSize = 0;
	if((regSize = emon.attrSize(ATTR_REGISTER, regItems, 4)) != 25) {
		failure++;
		std::cout << "ERR: wrong packet size for attribute register " << regSize << "\n";
	} else {
		passCount++;
	}
	unsigned char regBuffer[regSize];
	if(regSize != emon.attrBuilder(ATTR_REGISTER, regItems, 4, regBuffer)) {
		std::cout << "ERR: estimated size does not equal actual size\n";
		failure++;
	} else {
		passCount++;
	}

	unsigned char regCmp[] = { 0x0, 0x5, 0x15, 0x0, 0x4, 0x0, 0x0, 0x4, 0x53, 0x64,
					0x4, 0x21, 0x23, 0x4, 0x21, 0x3, 0x9, 0x8, 0xe, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 };
	if(memcmp(regBuffer, regCmp, regSize) == 0) {
		passCount++;
	} else {
		failure++;
		std::cout << "ERR: attr register output does not match expected output";
	}


	DataItem postItems[4];
	AttributeIdentifier postIdent;
	postIdent.groupID = 0x6453;
	postIdent.attributeID = 0x2321;
	postIdent.attributeNumber = 0x0321;
	emon.attrIdentAsDataItems(&postIdent, postItems);

	unsigned long fakeValue = 2052066570;
	postItems[3].type = ULONG;
	postItems[3].item = &fakeValue;
	int postSize = 0;
	if((postSize = emon.attrSize(ATTR_POST, postItems, 4)) != 25) {
		failure++;
		std::cout << "ERR: wrong packet size for attribute register " << postSize << "\n";
	} else {
		passCount++;
	}

	unsigned char postBuffer[postSize];
	if(postSize != emon.attrBuilder(ATTR_POST, postItems, 4, postBuffer)) {
		std::cout << "ERR: attr post estimated size does not equal actual size expected " << postSize << "\n";
		failure++;
	} else {
		passCount++;
	}

	unsigned char postCmp[] = { 0x0, 0x5, 0x15, 0x0, 0x4, 0x0, 0x0, 0x4, 0x53, 0x64, 0x4, 0x21, 0x23, 0x4,
				0x21, 0x3, 0x9, 0xa, 0xd, 0x50, 0x7a, 0x0, 0x0, 0x0, 0x0 };
	if(memcmp(postBuffer, postCmp, postSize) == 0) {
		passCount++;
	} else {
		failure++;
		std::cout << "ERR: attr post output does not match expected output";
	}

	for(int i=0; i<postSize; i++) {
		printf("0x%x ", postBuffer[i]);
	}
	std::cout << "\n";
	
	
	std::cout << passCount << " passes of " << (passCount + failure) << "\n";
	
	
	return 0;
}

#endif