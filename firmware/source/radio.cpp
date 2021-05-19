#include "link.h"
#include "radio.h"

uint8_t tx_buffer_inq[254];

/**
 * channel_to_freq(int channel)
 *
 * Convert a BLE channel number into the corresponding frequency offset
 * for the nRF51822.
 **/

uint8_t channel_to_freq(int channel)
{
	//usbLink->sendPacket(TEST, (uint8_t*) "channel_to_freq", strlen((char*) "channel_to_freq"), 0);

    if (channel == 37)
        return 2;
    else if (channel == 38)
        return 26;
    else if (channel == 39)
        return 80;
    else if (channel < 11)
        return 2*(channel + 2);
    else
        return 2*(channel + 3);
}


/**
 * radio_init()
 *
 * initialize radio.
 **/

void radio_init(void)
{
	if ((NRF_FICR->OVERRIDEEN & FICR_OVERRIDEEN_BLE_1MBIT_Msk) == (FICR_OVERRIDEEN_BLE_1MBIT_Override << FICR_OVERRIDEEN_BLE_1MBIT_Pos))
       	{
		NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
		NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
		NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
		NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
		NRF_RADIO->OVERRIDE4 = NRF_FICR->BLE_1MBIT[4] | 0x80000000;
	}

	// Enable the High Frequency clock on the processor. This is a pre-requisite for
	// the RADIO module. Without this clock, no communication is possible.
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	// Transmit with max power.
	NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);
	
	// Set Mode to BLE
	NRF_RADIO->MODE = (RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos);
	
	// transmit on logical address 0
	NRF_RADIO->TXADDRESS = 0;
	// a bit mask, listen only to logical address 0
	NRF_RADIO->RXADDRESSES = 1;

	///* ######not used if inquiry is used#######
	// see NRF51 spec pls
	NRF_RADIO->PCNF0 = (
		(((0UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) |  // Length of S0 field in bytes 0-1.   
		(((0UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) |  // Length of S1 field in bits 0-8.    
		(((0UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk)    // Length of length field in bits 0-8.
	);

	NRF_RADIO->PCNF1 = (
		(((0UL) << RADIO_PCNF1_MAXLEN_Pos) & RADIO_PCNF1_MAXLEN_Msk)   | // Maximum length of payload in bytes [0-255]
		(((255UL) << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk) | // Expand the payload with N bytes in addition to LENGTH [0-255]
		(((2UL) << RADIO_PCNF1_BALEN_Pos) & RADIO_PCNF1_BALEN_Msk)       | // Base address length in number of bytes.
		(((RADIO_PCNF1_ENDIAN_Little) << RADIO_PCNF1_ENDIAN_Pos) & RADIO_PCNF1_ENDIAN_Msk) |  // Endianess of the S0, LENGTH, S1 and PAYLOAD fields.
		(((1UL) << RADIO_PCNF1_WHITEEN_Pos) & RADIO_PCNF1_WHITEEN_Msk) // Enable packet whitening
	);

	// CRC Config
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits
	NRF_RADIO->CRCINIT = 0xFFFFUL;   // Initial value
	NRF_RADIO->CRCPOLY = 0x11021UL;  // CRC poly: x^16 + x^12^x^5 + 1

	// init whitening
	NRF_RADIO->DATAWHITEIV = 0x18;
	//*/

	// configure interrupts
	NRF_RADIO->INTENSET = 0x00000008;
}

/**
 * radio_disable()
 *
 * Disable the radio.
 **/

void radio_disable(void)
{
	if (NRF_RADIO->STATE <= 0){
		//usbLink->sendPacket(TEST, (uint8_t*) "radio_disable_(if)", strlen((char*) "radio_disable_(if)"), 0);
		return;
	}

	NVIC_DisableIRQ(RADIO_IRQn);
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->TASKS_DISABLE = 1;
	while (NRF_RADIO->EVENTS_DISABLED == 0);

	//usbLink->sendPacket(TEST, (uint8_t*) "radio_disable", strlen((char*) "radio_disable"), 0);
}

void radio_send(uint32_t accessAddress, int channel, uint8_t* tx_buffer, int size)
{
	//usbLink->sendPacket(TEST, (uint8_t*) "radio_send_start", strlen((char*) "radio_send_start"), 0);
	radio_disable();

	// set channel
	NRF_RADIO->FREQUENCY = channel_to_freq(channel);

	// set access addr
	NRF_RADIO->PREFIX0 = (accessAddress & 0xff0000)>>16;
	NRF_RADIO->BASE0 = (accessAddress & 0x0000ffff);

	// Switch packet buffer to tx_buffer.
	NRF_RADIO->PACKETPTR = (uint32_t) tx_buffer;

	//usbLink->sendPacket(TEST, tx_buffer, size, 0);

	// Will enable START when ready, disable radio when packet is sent, then enable rx.
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk  | RADIO_SHORTS_DISABLED_RXEN_Msk;
	
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->TASKS_TXEN = 1;

	NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);

	//usbLink->sendPacket(TEST, (uint8_t*) "radio_send_end", strlen((char*) "radio_send_end"), 0);
}

void inquiry(void)
{
  int channel = 11;
  uint32_t accessAddress = 0xAABBCC;

  if ((NRF_FICR->OVERRIDEEN & FICR_OVERRIDEEN_BLE_1MBIT_Msk)
  				== (FICR_OVERRIDEEN_BLE_1MBIT_Override
  				<< FICR_OVERRIDEEN_BLE_1MBIT_Pos)) {
  	NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
  	NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
  	NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
  	NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
  	NRF_RADIO->OVERRIDE4 = NRF_FICR->BLE_1MBIT[4] | 0x80000000;
  }

  tx_buffer_inq[0] = 0x41;
  tx_buffer_inq[1] = 0x42;
  tx_buffer_inq[2] = 0x43;
  tx_buffer_inq[3] = 0x44;
  tx_buffer_inq[4] = 0x45;
  tx_buffer_inq[5] = 0x46;
  tx_buffer_inq[6] = 0x47;
  tx_buffer_inq[7] = 0x48;
  tx_buffer_inq[8] = 0x49;
  tx_buffer_inq[9] = 0x4a;

  /* No shorts on disable. */
  NRF_RADIO->SHORTS = 0x0;

  /* Switch radio to TX. */
  radio_disable();

  // Enable the High Frequency clock on the processor. This is a pre-requisite for
  // the RADIO module. Without this clock, no communication is possible.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

  /* Transmit with max power. */
  NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);

  /* Listen on channel 6 (2046 => index 1 in BLE). */
  NRF_RADIO->FREQUENCY = channel_to_freq(channel);
  NRF_RADIO->DATAWHITEIV = channel;

  NRF_RADIO->MODE = (RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos);
  NRF_RADIO->PREFIX0 = (accessAddress & 0xff0000)>>16;
  NRF_RADIO->BASE0 = (accessAddress & 0x0000ffff)<<16;

  NRF_RADIO->CRCCNF  = ((0UL) << RADIO_CRCCNF_LEN_Pos); 

  NRF_RADIO->TXADDRESS = 0; // transmit on logical address 0
  NRF_RADIO->RXADDRESSES = 1; // a bit mask, listen only to logical address 0

  NRF_RADIO->PCNF0 = (
    (((0UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk) |  /* Length of S0 field in bytes 0-1.    */
    (((0UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk) |  /* Length of S1 field in bits 0-8.     */
    (((0UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk)    /* Length of length field in bits 0-8. */
  );
  NRF_RADIO->PCNF1 = (
    (((0UL) << RADIO_PCNF1_MAXLEN_Pos) & RADIO_PCNF1_MAXLEN_Msk)    | /* Max payload length. */
    (((10UL) << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk)   |/* Expansion of payload length  */
    (((2UL) << RADIO_PCNF1_BALEN_Pos) & RADIO_PCNF1_BALEN_Msk)       |/* Base address length */
    (((RADIO_PCNF1_ENDIAN_Little) << RADIO_PCNF1_ENDIAN_Pos) & RADIO_PCNF1_ENDIAN_Msk) |  /* Endianess */
    (((1UL) << RADIO_PCNF1_WHITEEN_Pos) & RADIO_PCNF1_WHITEEN_Msk)                         /* Whitening */
  );

  /* Switch packet buffer to tx_buffer. */
  NRF_RADIO->PACKETPTR = (uint32_t)tx_buffer_inq;

  /* T_IFS set to 150us. */
  NRF_RADIO->TIFS = 150;

  NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;
  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_EnableIRQ(RADIO_IRQn);

  /* Will enable START when ready, disable radio when packet is sent, then enable rx. */
  NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk  | RADIO_SHORTS_DISABLED_RXEN_Msk;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_TXEN = 1;

  /* From now, radio will send data and notify the result to Radio_IRQHandler */
}

void radio_receive(uint32_t accessAddress, int channel, uint8_t* rx_buffer)
{
	//usbLink->sendPacket(TEST, (uint8_t*) "radio_receive_start", strlen((char*) "radio_receive_start"), 0);
	radio_disable();
	
	// set channel
	NRF_RADIO->FREQUENCY = channel_to_freq(channel);
	
	// Set default access address used on advertisement channels.
	NRF_RADIO->PREFIX0 = (accessAddress & 0xff0000)>>16;
	NRF_RADIO->BASE0 = (accessAddress & 0x0000ffff);
	
	// set receive buffer
	NRF_RADIO->PACKETPTR = (uint32_t) rx_buffer;
	
	// Enable RSSI Measurement.
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
	
	//enable receiver (once enabled, it will listen)
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->TASKS_RXEN = 1;
	
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	//usbLink->sendPacket(TEST, (uint8_t*) "radio_receive_end", strlen((char*) "radio_receive_end"), 0);
}

