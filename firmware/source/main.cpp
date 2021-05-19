#include "MicroBit.h"
#include "link.h"
#include "radio.h"

#include "nrf_delay.h"

MicroBit uBit;
Link* usbLink;
bool receiving = false;
uint8_t rec_buf[MAX_PACKET_SIZE];

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
	/*
	if (NRF_RADIO->EVENTS_READY) {
		NRF_RADIO->EVENTS_READY = 0;
		NRF_RADIO->TASKS_START = 1;
	}
	*/
	if (NRF_RADIO->EVENTS_END) {
        NRF_RADIO->EVENTS_END = 0;
		if (receiving){
			/*
			char rssi[7] = "RSSI: ";
			char rssi_sample[5];
			itoa(NRF_RADIO->RSSISAMPLE, rssi_sample);
			strcat(rssi, rssi_sample);
			usbLink->sendPacket(TEST, (uint8_t*) rssi);
			*/
			//receiving = false;
			if (NRF_RADIO->CRCSTATUS == 1) {
				//usbLink->sendPacket(TEST, (uint8_t*) "CRC==1");
				usbLink->sendPacket(TEST, rec_buf, 16, 0);
			}
			/*
			else{
				//usbLink->sendPacket(TEST, (uint8_t*) "CRC!=1");
				char crc[6] = "CRC: ";
				char rxcrc[4];
				itoa(NRF_RADIO->RXCRC, rxcrc);
				strcat(crc, rxcrc);
				usbLink->sendPacket(TEST, (uint8_t*) crc);
			}
			*/
			NRF_RADIO->TASKS_START = 1;
		}	
	}
}

void dispatchCMD(T_CMD cmd, uint8_t* packet, int size, uint8_t flags)
{
	uint8_t buf[MAX_PACKET_SIZE] = {};

	switch (cmd)
	{
	case TEST:
		usbLink->sendPacket(TEST, (uint8_t*) "TESTING");
		print_packet(packet, size, flags);
		usbLink->sendPacket(cmd, packet, size, flags);
		break;
	case SEND:
		while(true){
			usbLink->sendPacket(TEST, (uint8_t*) "SENDING");
			strcpy((char*) buf, "AABBCCDDEEFFGGHH");			
			radio_send((uint32_t) 0xAABBCC, 11, (uint8_t*) buf, 16);
			//inquiry();
			usbLink->sendPacket(TEST, (uint8_t*) "SENT");
			wait_ms(1000);
		}
		break;
	case RECV:
		usbLink->sendPacket(TEST, (uint8_t*) "RECEIVING");
		receiving = true;
		radio_receive((uint32_t) 0xAABBCC, 11, rec_buf);
		break;
	default:
		usbLink->sendPacket(TEST, (uint8_t*) "NOPE");
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

		usbLink->sendPacket(TEST, (uint8_t*) "READING USB");

		wait_ms(1000);

		// TODO: needed??
		__SEV();
		__WFE();
	}
  	release_fiber();
	return 1;
}
