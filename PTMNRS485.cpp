#include "PTMNRS485.h"
#include "Debug.h"

PTMNRS485::PTMNRS485(unsigned short rxPin, unsigned short txPin, unsigned short txEnablePin) {
	this->sensorSerial = new SoftwareSerial(rxPin, txPin);
	this->sensorSerial->begin(PTMNRS485_BAUD);
	this->txEnablePin = txEnablePin;
	this->reading = 0;
	this->mode = PTM_IDLE;
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

void PTMNRS485::setMode(PTMNRS485_MODE mode) {
	switch(mode) {
		case PTM_IDLE: case PTM_WAITING:
			this->mode = mode;
			break;
		default:
			LOG(F("setMode: unknown mode"));
			break;
	}
}

PTMNRS485_MODE PTMNRS485::getMode() {
	return this->mode;
}

void PTMNRS485::requestPressure() {
	LOG(F("requestPressure: enter\r\n"));
	unsigned char buffer[sizeof(PTM_REQUEST_PRESSURE) + 2];
	LOG(F("copy all the memory!\r\n"));
	memcpy(buffer, PTM_REQUEST_PRESSURE, sizeof(PTM_REQUEST_PRESSURE));
	LOG(F("calc crc\r\n"));
	//unsigned short crc = this->calcCRC16(buffer, sizeof(PTM_REQUEST_PRESSURE));
	//LOG(F("put crc in buffer\r\n"));
	//buffer[sizeof(PTM_REQUEST_PRESSURE)] = crc & 0xff;
	//buffer[sizeof(PTM_REQUEST_PRESSURE) + 1] = (crc >> 8) & 0xff;
	buffer[sizeof(PTM_REQUEST_PRESSURE)] = 0x24;
	buffer[sizeof(PTM_REQUEST_PRESSURE) + 1] = 0xeb;
	LOG(F("transmit frame\r\n"));
	this->transmitFrame(buffer, sizeof(PTM_REQUEST_PRESSURE) + 2);
	LOG(F("requestPressure: frame transmitted\r\n"));
}

short PTMNRS485::parseRequestPressure() {
	unsigned char buffer[7];
	for(int i = 0; i < 7; i++) {
		buffer[i] = this->sensorSerial->read();
	}
	//unsigned short rcrc = (buffer[6] << 8) | (buffer[5] & 0xff);
	//unsigned short acrc = calcCRC16(buffer, 5);
	short value = (buffer[3] << 8) | buffer[4];
	//TODO verify the crc, for now live life on the edge
	return value;
}

bool PTMNRS485::blockingRead() {
	LOG(F("blockingRead: enter\r\n"));
	int retryCount = 0;
	do {
		LOG(F("blockingRead: l1\r\n"));
		this->setMode(PTM_IDLE);
		this->requestTime = millis();
		do {
			LOG(F("blockingRead: l2\r\n"));
			this->update();
			delay(5);
		} while((this->getMode() == PTM_WAITING) && ((millis() - this->requestTime) < READ_TIMEOUT));
		retryCount++;
	} while(this->getMode() == PTM_WAITING && retryCount < MAX_RETRIES);
	LOG(F("blockingRead: exit\r\n"));
	return retryCount <= MAX_RETRIES;
}

void PTMNRS485::update() {
	LOG(F("update: enter\r\n"));
	switch(mode) {
		case PTM_IDLE:
			this->requestPressure();
			LOG(F("Sensor pressure request sent\r\n"));
			this->setMode(PTM_WAITING);
			break;
		case PTM_WAITING:
			if(this->sensorSerial->available() >= 7) {
				short value = this->parseRequestPressure();
				if(value != 0) {
					this->reading = value;
					LOG(F("Sensor read ")); LOG(this->reading); LOG(F("\r\n"));
				} else {
					LOG(F("Error reading sensor value\r\n"));
				}
				this->setMode(PTM_IDLE);
			}
			break;
		default:
			LOG(F("update: err: unknown mode, resetting to idle\r\n"));
			this->setMode(PTM_IDLE);
			break;
	}
	LOG(F("update: exit\r\n"));
}

unsigned short PTMNRS485::calcCRC16(unsigned char *data, unsigned short count) {
	LOG(F("calcCRC16: enter: count: ")); LOG(count); LOG(F("\r\n"));
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
	LOG(F("calcCRC16: exit\r\n"));
	return fcs;
}

void PTMNRS485::transmitFrame(const uint8_t *frame, size_t length) {
	LOG(F("transmitFrame: enter\r\n"));
	while(this->sensorSerial->available()) { this->sensorSerial->read(); }
	LOG(F("transmitFrame: enabling transmit\r\n"));
	digitalWrite(this->txEnablePin, HIGH);
	delay(1);
	this->sensorSerial->write(frame, length);
	delay(1);
	digitalWrite(this->txEnablePin, LOW);
	LOG(F("transmitFrame: exit\r\n"));
}
