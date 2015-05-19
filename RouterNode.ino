#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Time.h>
#include <DS1302RTC.h>
#include <DES.h>
#include <EMonCMS.h>
#include "Definitions.h"
#include "SerialEventHandler.h"
#include "Sleep.h"

#define DEBUG
#include "Debug.h"

/* Radio and communication related definitions */
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

unsigned char incoming_buffer[MAX_PACKET_SIZE];
unsigned char outgoing_buffer[MAX_PACKET_SIZE];
uint8_t encryptionKey[24];

EMonCMS *emon = NULL;

/* Attribute definitions */
enum ATTRS {
	//ATTR_TIME,
	ATTR_PRESSURE,
	NUM_ATTR
};

AttributeValue attrVal[NUM_ATTR];

/* Data to store attribute readings */
uint16_t sensorReading = 0;
uint64_t timeData = 0;

/* declare unions to aid converting between data types and bytes */
union floatBytes {
	uint8_t bytes[4];
	float value;
};

union intBytes {
	uint8_t bytes[2];
	int value;
};

/* Time in millis of last post sent time */
unsigned long lastAttributePostTime = 0;

/* Real-time clock */
DS1302RTC rtc(RTC_CLK, RTC_DATA, RTC_RST);

DES des;

/* Sleep controller */
Sleep sleep(&rtc, &radio, EEPROM_ALARM_START);

/* puts the current time since boot into a data item */
/*
bool timeAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	LOG("timeAttributeReader: enter\r\n");
	timeData = millis();
	item->type = ULONG;
	item->item = &timeData;
	LOG("timeAttributeReader: done\r\n");
	return true;
}
*/

/* gets a raw pressure reading over i2c */
int16_t getRawPressure() {
	if(digitalRead(EN_PIN1) == LOW) {
		digitalWrite(EN_PIN1, HIGH);
		delay(250);
	}
	Wire.requestFrom(4, 2);
	if(Wire.available()) {
		uint8_t buffer[2];
		for(int i = 0; i < 2; i++) {
			buffer[i] = Wire.read();
		}
		
		sensorReading = (buffer[1] << 8) | buffer[0];
		//LOG(F("pressureAttributeReader: value read as ")); LOG(sensorReading); LOG(F("\r\n"));
  		return sensorReading;
	} else {
		//LOG(F("getRawPressure: Sensor read failed\r\n"));
		return 0;
	}
}

/* puts the raw pressure reading into a data item.
 *  when the gateway supports it the float of pressure will be used instead.
 */
bool pressureAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	if(getRawPressure() > 0 && item != NULL) {
  		/* conversion from meters to kPa at 4deg c */
  		float kpa = getDepth() * 0.10197442889f;
  		/*
  		item->item = &kpa;
		item->type = FLOAT;
		*/
		item->item = &sensorReading;
		item->type = USHORT;
		return true;
	} else {
		return false;
	}
}

/* gets the current depth in meters */
float getDepth() {
	floatBytes m, c;
	intBytes base;
	for(int i = 0; i < 4; i++) {
		m.bytes[i] = EEPROM.read(EEPROM_CALIB_GRAD + i);
		c.bytes[i] = EEPROM.read(EEPROM_CALIB_CONST + i);
	}

	for(int i = 0; i < 2; i++) {
		base.bytes[i] = EEPROM.read(EEPROM_CALIB_BASE + i);
	}

	/* y = mx + c */
	/* use where y = 0 to get the base calibration value */
	int calibBase = (int)((-c.value)/m.value);

	/* displace the sensor reading by the difference of
	 * the base values of the linear calculation to the set base */
	float depth = (float)(sensorReading - base.value + calibBase) * m.value + c.value;

	/* because of rounding errors depth will sometimes be a really tiny number
	 *  or slightyly less than 0. Ignore anything less accurate than a 1cm
	 */
	if(depth < 0.01) {
		depth = 0;
	}

	return depth;
}

/* writes data to the hardware radio */
uint16_t networkWriter(uint8_t type, uint8_t *buffer, uint16_t length) {
	int size = 0;
	uint8_t *send_buffer = NULL;
	/* if there's encryption, use it */
	if(EEPROM.read(EEPROM_ENCRYPT_ENABLE)) {
		size = encryptPacket(buffer, outgoing_buffer, length);
		send_buffer = outgoing_buffer;
	} else {
		size = length;
		send_buffer = buffer;
	}
	if(!mesh.write(send_buffer, type, size)){
		// If a write fails, check connectivity to the mesh network
		if(mesh.checkConnection()){
			//refresh the network address
			mesh.renewAddress(); 
        	if(!mesh.write(send_buffer, type, size)){
     	  		LOG("networkWriter: failed\r\n");
        		return 0;
			}
		}
    }
#ifdef DEBUG
	LOG(F("Unencrypted...\r\n"));
	char sbuff[7];
	for(int i = 0; i < length; i++) {
		sprintf(sbuff, "0x%x, ", buffer[i]);
		LOG(sbuff);
	}
	LOG(F("\r\nEncrypted/Sent...\r\n"));
	for(int i = 0; i < size; i++) {
		sprintf(sbuff, "0x%x, ", send_buffer[i]);
		LOG(sbuff);
	}
	LOG(F("\r\n"));
#endif
	
	return length;
}

void nodeIDRegistered(uint16_t emonNodeID) {
	/* save the node id into EEPROM */
	EEPROM.write(EMONNODEIDEEPROM1, (emonNodeID >> 8) & 0xFF);
	EEPROM.write(EMONNODEIDEEPROM2, (emonNodeID & 0xFF));
}

void attributeRegistered(AttributeIdentifier *attr) {
	/* save that attribute is registered to EEPROM */
	for(int i = 0; i < NUM_ATTR; i++) {
		if(emon->compareAttribute(attr, &(attrVal[i].attr))) {
			EEPROM.write(ATTR_REGISTERED_START + i, 1);
		}
	}
}

/* called to enter programming mode */
void programmingMode() {
	/* enable serial */
	Serial.begin(115200);
	Serial.println("Programming Mode");
	SerialEventHandler serialEvent(&rtc);
	/* power up peripherals */
	digitalWrite(RTC_EN, HIGH);
	radio.begin();
	/* attempt to read from serial and parse */
	while(true) {
		serialEvent.parseSerial();
	}
}

int encryptPacket(uint8_t *input, uint8_t *output, uint8_t data_size) {
#ifdef S_DEBUG
	char buff[8];
	LOG(F("Unencrypted data is... "));
	for(int i = 0; i < data_size; i++) {
		sprintf(buff, "0x%x, ", input[i]);
		LOG(buff);
	}
	LOG(F("\r\n"));
#endif
	output[0] = (data_size + (8 - (data_size % 8)) - 1) / 8;
	output[0] += ((data_size + (8 - (data_size % 8)) - 1) % 8) ? 1 : 0;
	des.do_3des_encrypt(input, data_size, &(output[1]), encryptionKey);
#ifdef S_DEBUG
	LOG(F("Encrypted data is... "));
	for(int i = 0; i < (data_size + (8 - (data_size % 8))) + 1; i++) {
		sprintf(buff, "0x%x, ", (output[i]));
		LOG(buff);
	}
	LOG(F("\r\n"));
#endif
	return data_size + (8 - (data_size % 8)) + 1;
}

bool decryptPacket(uint8_t *input, uint8_t *output, int read_size) {
	if(!EEPROM.read(EEPROM_ENCRYPT_ENABLE)) {
		LOG(F("Encryption disabled. Not decrypting incoming message\r\n"));
		for(int i = 0; i < read_size; i++) {
			output[i] = input[i];
		}
	} else {
		int block_data_size = input[0] * 8;
		if(read_size < block_data_size) {
			LOG(F("Read size is less than block data size\r\n"));
			return false;
		} else {
			des.do_3des_decrypt(&(input[1]), block_data_size, output, encryptionKey, des.get_IV_int());
		}
	}
	return true;
}

void setup() {
	/* use the 1.1V internal analog reference voltage, for battery level */
	analogReference(INTERNAL);

	/* enable pullup on the pin that controls programming mode */
	pinMode(PROG_MODE_PIN, INPUT_PULLUP);

	/* set peripherals default off */
	pinMode(EN_PIN1, OUTPUT);
	pinMode(EN_PIN2, OUTPUT);
	digitalWrite(EN_PIN1, LOW);
	digitalWrite(EN_PIN2, LOW);

	/* intitialise i2c */
	Wire.begin();

	/* Enable the RTC */
	pinMode(RTC_EN, OUTPUT);
	digitalWrite(RTC_EN, HIGH);

	/* seed the random nubmer table with the current timestamp */
	randomSeed(rtc.get());

	/* if the EEPROM is anything but 0 then reset all fields */
	if(EEPROM.read(RESETEEPROM)) {
		for(int i = 0; i < 1024; i++) {
			EEPROM.write(i, 0);
		}
	}

	/* if the RF24 Node ID is unset, assign a random one */
	if(EEPROM.read(RF24NODEIDEEPROM) == 0) {
		/* random to no more than 250 because for some reason it
		 *  doesn't register with the gateway correctly otherwise.
		 */
		EEPROM.write(RF24NODEIDEEPROM, random(220, 248));
	}

	/* if the progrmaming mode pin is pulled down, enter programming mode */
	if(!digitalRead(PROG_MODE_PIN)) {
		programmingMode();
	} else {
		/* initialise debug */
		DEBUG_INIT;
	}

	/* if encryption is enabled, read the key to RAM */
	if(EEPROM.read(EEPROM_ENCRYPT_ENABLE)) {
		LOG(F("READING ENCRYPTION KEY\r\n"));
		char keybuf[7];
		for(int i = 0; i < 24; i++) {
			encryptionKey[i] = EEPROM.read(EEPROM_ENCRYPT_KEY + i);
			sprintf(keybuf, "0x%x, ", encryptionKey[i]);
			LOG(keybuf);
		}
		LOG(F("\r\n"));
	}

	/* read the rf24 node ID and set it */
	LOG(F("Node id is ")); LOG(EEPROM.read(RF24NODEIDEEPROM)); LOG(F("\r\n"));
	mesh.setNodeID(EEPROM.read(RF24NODEIDEEPROM));
	radio.begin();
	radio.setPALevel(RF24_PA_HIGH);
	
	LOG(F("Connecting to mesh...\r\n"));
	mesh.begin(MESH_DEFAULT_CHANNEL, RF24_1MBPS);

	LOG(F("Setting up attributes\r\n"));
	/* setup the time reading attribute */
	/*
	attrVal[ATTR_TIME].attr.groupID = 10;
	attrVal[ATTR_TIME].attr.attributeID = 20;
	attrVal[ATTR_TIME].attr.attributeNumber = 40;
	attrVal[ATTR_TIME].reader = timeAttributeReader;
	*/
	
	attrVal[ATTR_PRESSURE].attr.groupID = 0x0403;
	attrVal[ATTR_PRESSURE].attr.attributeID = 0x1010;
	attrVal[ATTR_PRESSURE].attr.attributeNumber = 0x0;
	attrVal[ATTR_PRESSURE].reader = pressureAttributeReader;

	/* load whether each attribute has been registered */
	for(int i = 0; i < NUM_ATTR; i++) {
		attrVal[i].registered = EEPROM.read(ATTR_REGISTERED_START + i);
	}

	/* read the emoncms node id and init emonCMS */
	LOG(F("Setting up emon lib... "));
	/* read the emon node if from EEPROM, by default after a reset this is 0, so unset */
	uint16_t eMonNodeID = ((EEPROM.read(EMONNODEIDEEPROM1) & 0xFF) << 8) | (EEPROM.read(EMONNODEIDEEPROM2) & 0xFF);
	emon = new EMonCMS(attrVal, NUM_ATTR, networkWriter, attributeRegistered, nodeIDRegistered, eMonNodeID);
	LOG(F("done\r\n"));
}

void loop() {
	/* See if it's sleeping time */
	sleep.checkSleep();
	
	mesh.update();
	/* check to see whether we have a node id */
	emon->registerNode();

	/* attempt to post the pressure attribute every ATTR_POST_WAIT milliseconds, if the attribute is registered */
	if(millis() - lastAttributePostTime > ATTR_POST_WAIT && attrVal[ATTR_PRESSURE].registered) {
		if(emon->postAttribute(&(attrVal[ATTR_PRESSURE].attr)) > 0) {
			LOG(F("Sent attribute post\r\n"));
		} else {
			LOG(F("Failed to post attribute\r\n"));
		}
		lastAttributePostTime = millis();
	}
	
	if(network.available()) {
		RF24NetworkHeader header;
		network.peek(header);

		/* checks whether the header is a parseable ttype */
		if(emon->isEMonCMSPacket(header.type)) {
			/* Setup an EMonCMS packet */
			int read = 0;
			/* read the incoming packet */
			if((read = network.read(header, incoming_buffer, MAX_PACKET_SIZE)) > sizeof(HeaderInfo)) {
				/* decrypt it, this does nothing if encryption is disabled */
				decryptPacket(incoming_buffer, outgoing_buffer, read);
				/* do a sanity check to ensure the size reported in the header matches the size received */
				if(((HeaderInfo *)outgoing_buffer)->dataSize < (MAX_PACKET_SIZE - sizeof(HeaderInfo))) {
					/* allocate the number of data items specified in the header */
					DataItem items[((HeaderInfo *)outgoing_buffer)->dataCount];
					if(((HeaderInfo *)outgoing_buffer)->dataSize > (read - sizeof(HeaderInfo))) {
						LOG(F("Size mismatch for incoming packet\r\n"));
						LOG(F("Received ")); LOG(read - sizeof(HeaderInfo));
						LOG(F(" expected ")); LOG(((HeaderInfo *)outgoing_buffer)->dataSize);
						LOG(F("\r\n"));
					} else {
						LOG(F("Parsing incoming packet...\r\n"));
						/* try and parse the incoming packet */
						if(!emon->parseEMonCMSPacket(((HeaderInfo *)outgoing_buffer),
							header.type,
							&(outgoing_buffer[sizeof(HeaderInfo)]),
							items))
						{
							LOG(F("Failed to parse EMonCMS packet\r\n"));
						}
					}
				} else {
					LOG(F("Received packet too large, discarding\r\n"));
					network.read(header,0,0); 
				}
			} else {
				LOG(F("Failed to read header bytes\r\n"));
				LOG(F("Received ")); LOG(read); LOG(F(" bytes\r\n"));
			}
		} else {
			network.read(header,0,0); 
			LOG(F("Unknown packet type, discarding\r\n"));
		}
	}
}
