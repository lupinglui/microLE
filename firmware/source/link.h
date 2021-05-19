#pragma once

#include "MicroBit.h"

#define MAX_PACKET_SIZE  	0x110
#define PREAMBLE		0xBC
#define HEADERSIZE		0x4

typedef enum {
	TEST,
	SEND,
	RECV
} T_CMD;

class Link
{
private:
	MicroBitSerial *serial;
	MicroBit *uBit;

	uint8_t packetBuf[MAX_PACKET_SIZE];
	int received;
	int expected;

	uint8_t crc(uint8_t *data, int size, uint8_t prevCrc);
public:

	/* Constructor. */
	Link(MicroBit *ubit);

	/* Interface. */
	bool readPacket(T_CMD* cmd, uint8_t *buf, int *size, uint8_t *flags);
	bool sendPacket(T_CMD cmd, uint8_t *buf, int size = 0, uint8_t flags = 0);
};

extern Link* usbLink;