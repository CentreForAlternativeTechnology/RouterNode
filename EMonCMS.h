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

enum status {
	SUCCESS = 0
};

enum RequestType {
	NODE_REGISTER,
	ATTR_REGISTER,
	ATTR_POST
};

typedef struct {
	unsigned char status;
	unsigned char dataCount;
	unsigned short dataSize;
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

typedef struct {
	HeaderInfo header;
	DataItem nodeID;
} RegisterRequest;

class EMonCMS {
	public:
		EMonCMS();
		~EMonCMS();
		bool isEMonCMSPacket(unsigned char type);
		bool parseEMonCMSPacket(HeaderInfo *header, unsigned char type, unsigned char *buffer, DataItem items[]);
		int attrSize(RequestType type, DataItem *item, int length);
		int attrBuilder(RequestType type, DataItem *items, int length, unsigned char *buffer);
		void attrIdentAsDataItems(AttributeIdentifier *ident, DataItem *attrItems);
	protected:
		unsigned short nodeID;
		bool checkHeader(HeaderInfo *header, unsigned char size);
		int getTypeSize(unsigned char type);
		int dataItemToBuffer(DataItem *item, unsigned char *buffer);
};

#endif
