#include "link.h"

Link::Link(MicroBit *_uBit)
{
	/* Set microbit instance. */
	uBit = _uBit;
	serial = &uBit->serial;

	/* Upgrade the default tx buffer. */
	serial->baud(115200);
	serial->setTxBufferSize(254);
	serial->setRxBufferSize(254);

	/* Clear RX and TX. */
	serial->clearRxBuffer();
	serial->clearTxBuffer();

	received = 0;
}

uint8_t Link::crc(uint8_t *pData, int size, uint8_t prevCrc)
{
	for (int i = 0; i<size; i++)
		prevCrc ^= pData[i];

	return prevCrc;
}

/*
 * read one byte from serial
 * - returns true, after a complete packet was recieved
 */
bool Link::readPacket(T_CMD* cmd, uint8_t *buf, int *bufSize, uint8_t *flags)
{
	int read = 0;
      	int i;
	int payloadSize;

	/* Pipe data in if any. */
	if (serial->isReadable())
	{
		read = (uint8_t) serial->read(&packetBuf[received], 1);
		if (read != MICROBIT_SERIAL_IN_USE)
			received += read;
	}

	/* no data available */
	if (read <= 0)
		return false;

	/* Packet must start with 0xBC. */
	if (packetBuf[0] != PREAMBLE)
	{
		/* Chomp one byte. */
		for (i = 1; i < received; i++)
			packetBuf[i - 1] = packetBuf[i];
		received--;
		return false;
	}

	/* header not yet received completely */
	if (received <= HEADERSIZE)
		return false;

	/* Parse packet. */
	payloadSize = packetBuf[2] | (packetBuf[3] << 8);

	/* packet not yet received completely */
	if (received < (payloadSize + HEADERSIZE + 1))
		return false;

	/* Check crc. */
	if (packetBuf[payloadSize + HEADERSIZE] != crc(packetBuf, payloadSize + HEADERSIZE, 0xff))
		return false;

	/* Fill data. */
	*cmd = (T_CMD) (packetBuf[1] & 0x0F);
	*flags = packetBuf[1] >> 4;

	/* Copy data. */
	if (*bufSize >= payloadSize)
		*bufSize = payloadSize;

	for (i = 0; i < *bufSize; i++)
		buf[i] = packetBuf[i + HEADERSIZE];

	/* Chomp packet. */
	for (i = 0; i < (received - payloadSize - HEADERSIZE - 1); i++)
		packetBuf[i] = packetBuf[i + payloadSize + HEADERSIZE + 1];
	received -= payloadSize + HEADERSIZE + 1;

	/* Success/ */
	return true;
}

bool Link::sendPacket(T_CMD cmd, uint8_t *payload, int size, uint8_t flags)
{
	if (size == 0)
		size = strlen((char*) payload);

	int i;
	uint8_t header[4];
	uint8_t checksum = 0xff;

	/* Fill header. */
	header[0] = PREAMBLE;
	header[1] = cmd | ((flags & 0xF) << 4);
	header[2] = size & 0xFF;
	header[3] = size >> 8;

	/* Compute checksum. */
	for (i = 0; i < 4; i++)
		checksum ^= header[i];

	/* Send header first. */
	if (!serial->send(header, 4, ASYNC))
		return false;

	/* Send payload next. */
	for (i = 0; i < size; i++)
		checksum ^= payload[i];

	/* Only send data if we have data to send. */
	if (!serial->send(payload, size, ASYNC))
		return false;

	/* Send checksum. */
	if (!serial->send(&checksum, 1, ASYNC))
		return false;

	return true;
}

