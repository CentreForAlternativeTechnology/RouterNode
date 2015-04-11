#include "Arduino.h"
#include "ARandom.h"

int aRand_getRand() {
	int read = 0;
	while(read == 0 || read == 1023) {
		read = analogRead(RAND_PIN);
	}
	return read;
}

uint32_t aRand_altRand() {
	uint32_t rand = 0;
	for(int i = 0; i < 32; i++) {
		rand |= (aRand_getRand() & 0x1) << i;
	}
	return rand;
}

uint32_t arandom() {
	uint32_t rTable[4];
	for(int i = 0; i < 4; i++) {
		rTable[i] = aRand_altRand();
	}
	uint32_t rand = 0;

	rand = rTable[0] ^ (rTable[0] << 11);
	rTable[0] = rTable[1]; rTable[1] = rTable[2]; rTable[2] = rTable[3];
	return rTable[3] = rTable[3] ^ (rTable[3] >> 19) ^ (rand ^ (rand >> 8));
}
