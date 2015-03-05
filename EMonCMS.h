#ifndef __EMONCMS_H__
#define __EMONCMS_H__

#define T_USHORT(x) (*((unsigned short *)(x)))

/**
 * The is an enum to specify data formats to send over the
 * low power radio
 **/
enum dataTypes {
	STRING = 0,
	CHAR = 1,
	UCHAR = 2,
	SHORT = 3,
	USHORT = 4,
	INT = 5,
	UINT = 6,
	LONG = 7,
	ULONG = 9,
	FLOAT = 10
};

/**
 * Status codes from the OEMan Communications specification
 **/
enum Status {
	SUCCESS = 0x00,
	FAILURE = 0x01,
	UNSUPPORTED_ATTRIBUTE = 0x86,
	INVALID_VALUE = 0x87
};

/**
 * Radio packet types for request types
 **/
enum RequestType {
	NODE_REGISTER = 'R',
	ATTR_REGISTER = 'A',
	ATTR_POST = 'P',
	ATTR_FAILURE
};

/**
 * Initial header of OEMan Low-power Radio spec packet
 **/
typedef struct {
	unsigned short dataSize; /** Size of the data items section in bytes **/
	unsigned char status; /** The status, SUCCESS in requests **/
	unsigned char dataCount; /** Number of data items **/
} HeaderInfo;

/**
 * A data item, consisting of a type and a pointer to data
 **/
typedef struct {
	unsigned char type;
	void *item;
} DataItem;

/**
 * Attributes are identified by the 3 values in this structure.
 **/
typedef struct {
	unsigned short groupID;
	unsigned short attributeID;
	unsigned short attributeNumber;
} AttributeIdentifier;

/**
 * NetworkSender implemented by the program to send data packets to the gateway
 * @param type Packet type
 * @param buffer data to send
 * @param length length of buffer
 * @return the length of the buffer on success
 **/
typedef int (*NetworkSender)(unsigned char type, unsigned char *buffer, int length);

/**
 * Function implemented by host program to retrieve the value of a piece of data.
 * It is expected that the pointer to the data set in the dataitem is a global
 * variable or static.
 * @param attr the identifier for the requested attribute reading
 * @param item the item that the implementation is expected to fill
 * @return 0 on success
 **/
typedef int (*AttributeReader)(AttributeIdentifier *attr, DataItem *item);

/**
 * Contains the information necessary to identifier, register and read
 * an attribute value.
 **/
typedef struct {
	AttributeIdentifier attr; /** The attribute identifier **/
	AttributeReader reader; /** the function to read the value for this attribute **/
	bool registered; /** user should set to false on creation **/
} AttributeValue;

class EMonCMS {
	public:
		EMonCMS(AttributeValue values[], int length, NetworkSender sender, unsigned short nodeID = 0);
		~EMonCMS();
		/* methods for receiving packets */
		bool isEMonCMSPacket(unsigned char type);
		bool parseEMonCMSPacket(HeaderInfo *header, unsigned char type, unsigned char *buffer, DataItem items[]);
		/* methods for sending packets */
		int attrSize(RequestType type, DataItem *item, int length);
		int attrBuilder(RequestType type, DataItem *items, int length, unsigned char *buffer);
		int attrSender(RequestType type, DataItem *items, int length);
		void attrIdentAsDataItems(AttributeIdentifier *ident, DataItem *attrItems);
		
		/* getters and setters */
		unsigned short getNodeID();
		AttributeValue *getAttribute(AttributeIdentifier *attr);
	protected:
		unsigned short nodeID;
		AttributeValue *attrValues;
		int attrValuesLength;
		NetworkSender networkSender;
		bool checkHeader(HeaderInfo *header, unsigned char size);
		int getTypeSize(unsigned char type);
		int dataItemToBuffer(DataItem *item, unsigned char *buffer);
		bool requestAttribute(HeaderInfo *header, DataItem items[]);
};

#endif
