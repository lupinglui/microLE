#include "MicroBit.h"
#include "link.h"
#include "radio.h"

#include "nrf_delay.h"

MicroBit uBit;
Link* usbLink;

extern "C" void RADIO_IRQHandler(void)
{
	if (NRF_RADIO->EVENTS_READY) {
		NRF_RADIO->EVENTS_READY = 0;
		NRF_RADIO->TASKS_START = 1;
	}

	if (NRF_RADIO->EVENTS_END) {
        	NRF_RADIO->EVENTS_END = 0;

		char buf[] = "IRQ";
		usbLink->sendPacket(TEST, (uint8_t*) buf, 3, 0);
		uBit.display.scroll("IRQ");

		/* continue TODO only for receiver, sender should not repeat */
		//NRF_RADIO->TASKS_START = 1;
	}
}

void dispatchCMD(T_CMD cmd, uint8_t* packet, int size, uint8_t flags)
{
	uint8_t buf[MAX_PACKET_SIZE];

	switch (cmd)
	{
	case TEST:
		uBit.display.print(flags);
		for (int i = 0; i < size; i++)
		{
			wait_ms(1000);
			uBit.display.print((char) packet[i]);
		}
		uBit.display.clear();
		buf[0] = (uint8_t) 'T';
		buf[1] = (uint8_t) 'E';
		buf[2] = (uint8_t) 'S';
		buf[3] = (uint8_t) 'T';
		usbLink->sendPacket(TEST, buf, 4, 0);
		break;
	case SEND:
		radio_send((uint32_t) 0x554433, 11, (uint8_t*) packet, size);
		strcpy((char*) buf, "SENT");
		usbLink->sendPacket(TEST, buf, strlen((char*) buf), 0);
		break;
	case RECV:
		radio_receive((uint32_t) 0xAABBCC, 11, buf);
		strcpy((char*) buf, "RECEIVING");
		usbLink->sendPacket(TEST, buf, strlen((char*) buf), 0);
		break;
	default:
		uBit.display.scroll("UNKNOWN CMD");
		uBit.display.print(cmd);
		buf[0] = (uint8_t) 'N';
		buf[1] = (uint8_t) 'O';
		buf[2] = (uint8_t) 'P';
		buf[3] = (uint8_t) 'E';
		usbLink->sendPacket(TEST, buf, 4, 0);
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
