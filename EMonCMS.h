#ifndef __EMONCMS_H__
#define __EMONCMS_H__

#include "Debug.h"

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
 * User implemented event which is triggered when the node ID is registered
 **/
typedef void (*NodeIDRegistered)(unsigned short emonNodeID);

/**
 * User implemented event which is triggered when a attribute is successfully registered.
 * @param attr the attribute identifier for the registered attribute
 **/
typedef void (*AttributeRegistered)(AttributeIdentifier *attr);

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
		/**
		 * @param values list of attributes which can be read from this node
		 * @param length length of attribute list
		 * @param sender NetworkSender for sending responses to incoming requests
		 * @param attrRegistered attribute registered callback
		 * @param node registered callback
		 * @param nodeID defaults to 0, node id for emoncms
		 **/
		EMonCMS(AttributeValue values[],
			int length, 
			NetworkSender sender,
			AttributeRegistered attrRegistered = NULL,
			NodeIDRegistered nodeRegistered = NULL,
			unsigned short nodeID = 0
			);
		~EMonCMS();
		/* methods for receiving packets */
		/**
		 * Checks the incoming type to see if it is an accepted type
		 * @param type type of incoming packet to check
		 * @return true if it is an emon cms packet type
		 **/
		bool isEMonCMSPacket(unsigned char type);
		/**
		 * Parses an incoming emon cms packet 
		 * @param header incomiing emon cms header
		 * @param type the type of the incoming packet
		 * @param buffer the raw unparsed data items
		 * @param items a list of data items the size of count in the header
		 * @return returns true if the function succeeded
		 **/
		bool parseEMonCMSPacket(HeaderInfo *header, unsigned char type, unsigned char *buffer, DataItem items[]);
		/* methods for sending packets */
		/**
		 * Calculates the buffer size for the buffer passed to attrBuilder
		 * @param type type of request to be sent
		 * @param item list of data items
		 * @param length length of list of data items
		 * @return the size of the buffer
		 **/
		int attrSize(RequestType type, DataItem *item, int length);
		/**
		 * Creates a packet into the given buffer containing the given data items.
		 * Items for each request type:
		 * 	NODE_REGISTER: None
		 * 	ATTR_REGISTER: Group ID, Attribute ID, Attribute Number, Attribure default
		 * 	ATTR_POST: Group ID, Attribute ID, Attribute Number, Attribute Value
		 * 	ATTR_FAILURE: Group ID, Attribute ID, Attribute Number
		 * @param type the type of the request to send
		 * @param items list of data items to send
		 * @param length length of list of data items to send
		 * @param buffer buffer to write packet into, including header
		 * @return the size of the buffer on success
		 **/
		int attrBuilder(RequestType type, DataItem *items, int length, unsigned char *buffer);
		/**
		 * Wraps attrBuilder and sends requests through the NetworkSender
		 * @param type type of request to send
		 * @param items list of items to attach
		 * @param length length of list of items to attach
		 * @return the size of the sent data on success
		 **/
		int attrSender(RequestType type, DataItem *items, int length);
		/**
		 * Converts and AttributeIdentifier to a list of DataItems.
		 * @param ident the incoming Attribute Identifier
		 * @param attrItems an array of DataItems of length 3
		 **/
		void attrIdentAsDataItems(AttributeIdentifier *ident, DataItem *attrItems);
		
		/* getters and setters */
		/**
		 * Returns the assigned node ID
		 * @return the node ID form emon cms
		 **/
		unsigned short getNodeID();
		/**
		 * Gets the data about an attribute including registration status
		 * and the function to get it from an Attribute Identifier.
		 * @param attr Attribute Identifier to get the data for.
		 * @return NULL on not found, otherwise Attribute Value struct.
		 **/
		AttributeValue *getAttribute(AttributeIdentifier *attr);
	protected:
		unsigned short nodeID; /** the EMonCMS node ID **/
		AttributeValue *attrValues; /** list of registered attributes on this node **/
		int attrValuesLength; /** length of list of registered attributes on this node **/
		NetworkSender networkSender; /** function to send data to the radios **/
		AttributeRegistered attrRegistered; /** attribute registered callback **/
		NodeIDRegistered nodeRegistered; /** node registered callback **/
		
		/**
		 * Compared the data count in the header to the size
		 **/
		bool checkHeader(HeaderInfo *header, unsigned char size);
		/**
		 * Gets the size of the item in a DataItem
		 * @param type the type of item
		 * @return the size of the given type
		 **/
		int getTypeSize(unsigned char type);
		/**
		 * Transfers a data item into a char array
		 * @param item item to put in char array
		 * @param buffer buffer to transfer to
		 * @return size of transferred item on success
		 **/
		int dataItemToBuffer(DataItem *item, unsigned char *buffer);
		/**
		 * Function to respond to a request for an attribute.
		 * Sends through the NetworkSender specified in constructor.
		 * @param header header of incoming request
		 * @param items item list containing attribute identifier
		 * @return true if building and sending succeeded
		 **/
		bool requestAttribute(HeaderInfo *header, DataItem items[]);
};

#endif
