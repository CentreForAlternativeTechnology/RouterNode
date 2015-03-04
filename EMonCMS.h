#ifndef __EMONCMS_H__
#define __EMONCMS_H__

#define BYTES(x) ((unsigned char *)(x))
#define T_USHORT(x) (*((unsigned short *)(x)))

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

enum Status {
	SUCCESS = 0x00,
	FAILURE = 0x01,
	UNSUPPORTED_ATTRIBUTE = 0x86,
	INVALID_VALUE = 0x87
};

enum RequestType {
	NODE_REGISTER,
	ATTR_REGISTER,
	ATTR_POST,
	ATTR_FAILURE
};

typedef struct {
	unsigned short dataSize;
	unsigned char status;
	unsigned char dataCount;
} HeaderInfo;

typedef struct {
	unsigned char type;
	void *item;
} DataItem;

typedef struct {
	unsigned char length;
	char *data;
} CharArray;

typedef struct {
	unsigned short groupID;
	unsigned short attributeID;
	unsigned short attributeNumber;
} AttributeIdentifier;

typedef int (*NetworkSender)(unsigned char type, unsigned char *buffer, int length);
typedef int (*AttributeReader)(AttributeIdentifier *attr, DataItem *item);

typedef struct {
	AttributeIdentifier attr;
	AttributeReader reader;
	bool registered;
} AttributeValue;

class EMonCMS {
	public:
		EMonCMS(AttributeValue values[], int length, NetworkSender sender);
		~EMonCMS();
		bool isEMonCMSPacket(unsigned char type);
		bool parseEMonCMSPacket(HeaderInfo *header, unsigned char type, unsigned char *buffer, DataItem items[]);
		int attrSize(RequestType type, DataItem *item, int length);
		int attrBuilder(RequestType type, DataItem *items, int length, unsigned char *buffer);
		void attrIdentAsDataItems(AttributeIdentifier *ident, DataItem *attrItems);
		unsigned short getNodeID();
		AttributeValue *getAttribute(AttributeIdentifier *attr);
	protected:
		unsigned short nodeID;
		bool checkHeader(HeaderInfo *header, unsigned char size);
		int getTypeSize(unsigned char type);
		int dataItemToBuffer(DataItem *item, unsigned char *buffer);
		bool requestAttribute(HeaderInfo *header, DataItem items[]);
		AttributeValue *attrValues;
		int attrValuesLength;
		NetworkSender networkSender;
};

#endif
