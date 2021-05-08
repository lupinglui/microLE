#include "MicroBit.h"
#include "link.h"
#include "radio.h"

#include "nrf_delay.h"

MicroBit uBit;
Link* usbLink;

void print_packet(uint8_t* packet, int size, uint8_t flags = 0xff){
	if (flags != 0xff){
		uBit.display.print(flags);
		wait_ms(500);
		uBit.display.print(' ');
	}
	for (int i = 0; i < size + 1; i++)
	{
		wait_ms(500);
		uBit.display.print((char) packet[i]);
	}
	uBit.display.clear();
}

extern "C" void RADIO_IRQHandler(void)
{
	usbLink->sendPacket(TEST, (uint8_t*) "IRQ_HANDLER_START", strlen((char*) "IRQ_HANDLER_START"), 0);

	if (NRF_RADIO->EVENTS_READY) {
		NRF_RADIO->EVENTS_READY = 0;
		usbLink->sendPacket(TEST, (uint8_t*) "IRQ EVENTS_READY", strlen((char*) "IRQ EVENTS_READY"), 0);
		NRF_RADIO->TASKS_START = 1;
	}

	if (NRF_RADIO->EVENTS_END) {
        NRF_RADIO->EVENTS_END = 0;
		usbLink->sendPacket(TEST, (uint8_t*) "IRQ EVENTS_END", strlen((char*) "IRQ EVENTS_END"), 0);

		// continue TODO only for receiver, sender should not repeat
		//NRF_RADIO->TASKS_START = 1;
	}
}

void dispatchCMD(T_CMD cmd, uint8_t* packet, int size, uint8_t flags)
{
	uint8_t buf[MAX_PACKET_SIZE];

	switch (cmd)
	{
	case TEST:
		strcpy((char*) buf, "TESTING");
		usbLink->sendPacket(TEST, buf, strlen((char*) buf), 0);
		print_packet(packet, size, flags);
		usbLink->sendPacket(cmd, packet, size, flags);
		break;
	case SEND:
		strcpy((char*) buf, "SENDING");
		usbLink->sendPacket(TEST, buf, strlen((char*) buf), 0);
		usbLink->sendPacket(cmd, packet, strlen((char*) packet), flags);
		radio_send((uint32_t) 0x554433, 11, (uint8_t*) packet, size);
		strcpy((char*) buf, "SENT");
		usbLink->sendPacket(TEST, buf, strlen((char*) buf), 0);
		break;
	case RECV:
		strcpy((char*) buf, "RECEIVING");
		usbLink->sendPacket(TEST, buf, strlen((char*) buf), 0);
		strcpy((char*) buf, "");
		radio_receive((uint32_t) 0xAABBCC, 11, buf);
		//print_packet(buf, size, flags);
		usbLink->sendPacket(cmd, buf, strlen((char*) buf), flags);
		break;
	default:
		strcpy((char*) buf, "NOPE");
		usbLink->sendPacket(TEST, buf, strlen((char*) buf), 0);
		uBit.display.scroll("UNKNOWN CMD");
		uBit.display.print(cmd);
		usbLink->sendPacket(cmd, packet, size, flags);
		break;
	}
}

int main()
{
	T_CMD cmd;
	int size = MAX_PACKET_SIZE;
	uint8_t flags;
	uint8_t packet[MAX_PACKET_SIZE];

	char buf[100];

	uBit.init();

// TODO needed?
#ifdef YOTTA_CFG_TXPIN
	#ifdef YOTTA_CFG_RXPIN
		uBit.display.scroll("YOTTA");
    		#pragma message("Btlejack firmware will use custom serial pins")
    		uBit.serial.redirect(YOTTA_CFG_TXPIN, YOTTA_CFG_RXPIN);
	#endif
#endif

	usbLink = new Link(&uBit);
	radio_init();

	uBit.display.scroll("HI");

	while (1)
	{
		//uBit.display.scroll("R!");
		if (usbLink->readPacket(&cmd, packet, &size, &flags))
		{
			dispatchCMD(cmd, packet, size, flags);
		}

		strcpy(buf, "READING USB");
		usbLink->sendPacket(TEST, (uint8_t*) buf, strlen((char*) buf), 0);

		wait_ms(1000);

		// TODO: needed??
		__SEV();
		__WFE();
	}
  	release_fiber();
	return 1;
}
