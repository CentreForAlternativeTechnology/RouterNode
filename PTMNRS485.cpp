#include "PTMNRS485.h"

PTMNRS485::PTMNRS485(unsigned short rxPin, unsigned short txPin, unsigned short txEnablePin) {
	this->sensorSerial = new SoftwareSerial(rxPin, txPin);
	this->sensorSerial->begin(PTMNRS485_BAUD);
	this->txEnablePin = txEnablePin;
	this->reading = 0;
	pinMode(this->txEnablePin, OUTPUT);
  	digitalWrite(this->txEnablePin, LOW);
}

PTMNRS485::~PTMNRS485() {
	delete(this->sensorSerial);
}

short PTMNRS485::getReading() {
	return this->reading;
}

short *PTMNRS485::getReadingPtr() {
	return &(this->reading);
}

void PTMNRS485::requestPressure() {
	unsigned char buffer[sizeof(PTM_REQUEST_PRESSURE) + 2];
	memcpy(buffer, PTM_REQUEST_PRESSURE, sizeof(PTM_REQUEST_PRESSURE));
	unsigned short crc = this->calcCRC16(buffer, sizeof(PTM_REQUEST_PRESSURE));
	buffer[sizeof(PTM_REQUEST_PRESSURE)] = crc & 0xff;
	buffer[sizeof(PTM_REQUEST_PRESSURE) + 1] = (crc >> 8) & 0xff;
	this->transmitFrame(buffer, sizeof(PTM_REQUEST_PRESSURE) + 2);
}

short PTMNRS485::parseRequestPressure() {
	unsigned char buffer[7];
	for(int i = 0; i < 7; i++) {
		buffer[i] = this->sensorSerial->read();
	}
	unsigned short rcrc = (buffer[6] << 8) | (buffer[5] & 0xff);
	unsigned short acrc = calcCRC16(buffer, 5);
	short value = (buffer[3] << 8) | buffer[4];
	//TODO verify the crc, for now live life on the edge
	return value;
}

void PTMNRS485::update() {
	switch(mode) {
		case PTM_IDLE:
			this->requestPressure();
			this->mode = PTM_WAITING;
			break;
		case PTM_WAITING:
			if(this->sensorSerial->available() >= 7) {
				short value = this->parseRequestPressure();
				if(value != 0) {
					this->reading = value;
				}
				this->mode = PTM_IDLE;
			}
			break;
	}
}

unsigned short PTMNRS485::calcCRC16(unsigned char *data, unsigned short count) {
	unsigned int fcs = 0xFFFFU;
	unsigned int d, i, k;
	for(int i = 0; i < count; i++) {
		d = (((unsigned int)(*data++)) << 0U);
		for(k = 0; k < 8; k++) {
			if(((fcs ^ d) & 0x0001U) == 1) {
				fcs = (fcs >> 1) ^ 0xA001U;
			} else {
				fcs = (fcs >> 1);
			}
			d >>= 1;
		}
	}
	return fcs;
}

void PTMNRS485::transmitFrame(const uint8_t *frame, size_t length) {
	while(this->sensorSerial->available()) { this->sensorSerial->read(); }
	digitalWrite(this->txEnablePin, HIGH);
	delay(1);
	this->sensorSerial->write(frame, length);
	delay(1);
	digitalWrite(this->txEnablePin, LOW);
}
