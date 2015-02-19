#ifndef EMONCMS_H
#define EMONCMS_H

#define BYTES(x) ((unsigned char *)(x))

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

typedef int (*networkRead)(void *callbackData, int numBytes, void *buffer);

bool isEMonCMSPacket(unsigned char type);
bool parseEMonCMSPacket(HeaderInfo *header, DataItem *items[], networkRead read, void *callbackData);

#endif
