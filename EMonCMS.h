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

typedef struct {
	unsigned char status;
	unsigned char dataCount;
	unsigned short dataSize;
	unsigned char type;
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
	HeaderInfo header;
	DataItem nodeID;
} RegisterRequest;

class EMonCMS {
	public:
		EMonCMS();
		~EMonCMS();
		bool isEMonCMSPacket(unsigned char type);
		bool parseEMonCMSPacket(HeaderInfo *header, unsigned char *buffer, DataItem items[]);
	protected:
		unsigned char nodeID;
		bool checkHeader(HeaderInfo *header, unsigned char size);
		int getTypeSize(unsigned char type);
};

#endif
