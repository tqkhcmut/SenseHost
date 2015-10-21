#ifndef _one_wire_h_
#define _one_wire_h_

#include "stm8s.h"

//// you can exclude onewire_search by defining that to 0
//#ifndef ONEWIRE_SEARCH
#define ONEWIRE_SEARCH
//#endif
//
//// You can exclude CRC checks altogether by defining this to 0
//#ifndef ONEWIRE_CRC
#define ONEWIRE_CRC
//#endif
//
//// Select the table-lookup method of computing the 8-bit CRC
//// by setting this to 1.  The lookup table enlarges code size by
//// about 250 bytes.  It does NOT consume RAM (but did in very
//// old versions of OneWire).  If you disable this, a slower
//// but very compact algorithm is used.
//#ifndef ONEWIRE_CRC8_TABLE
#define ONEWIRE_CRC8_TABLE
//#endif
//
//// You can allow 16-bit CRC checks by defining this to 1
//// (Note that ONEWIRE_CRC must also be 1.)
//#ifndef ONEWIRE_CRC16
#define ONEWIRE_CRC16
//#endif

//#ifndef bool
//typedef enum 
//{
//	FALSE = 0,
//	TRUE = !FALSE
//} bool;
//#endif

// Platform specific I/O definitions

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif

#if defined(USE_STDPERIPH_DRIVER)
#define	ONEWIRE_PIN				GPIO_Pin_0
#define ONEWIRE_GPIO			GPIOB

uint8_t	DIRECT_READ(void);
void	DIRECT_MODE_INPUT(void);
void	DIRECT_MODE_OUTPUT(void);
void	DIRECT_WRITE_LOW(void);
void	DIRECT_WRITE_HIGH(void);

#elif defined(STM8S207)

#define	ONEWIRE_PIN				GPIO_PIN_1
#define ONEWIRE_GPIO			GPIOC

#define 	DIRECT_READ() 				{return GPIO_ReadInputPin(ONEWIRE_GPIO, ONEWIRE_PIN);}
#define 	DIRECT_MODE_INPUT() 	{GPIO_Init(ONEWIRE_GPIO, ONEWIRE_PIN, GPIO_MODE_IN_FL_NO_IT);}
#define 	DIRECT_MODE_OUTPUT()	{GPIO_Init(ONEWIRE_GPIO, ONEWIRE_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);}
#define 	DIRECT_WRITE_LOW()		{GPIO_WriteLow(ONEWIRE_GPIO, ONEWIRE_PIN);}
#define 	DIRECT_WRITE_HIGH()		{GPIO_WriteHigh(ONEWIRE_GPIO, ONEWIRE_PIN);}

#error "Please define I/O register types here"
#endif
	
struct OneWire
{
	void (* Init)(void);
	
	// Perform a 1-Wire reset cycle. Returns 1 if a device responds
	// with a presence pulse.  Returns 0 if there is no device or the
	// bus is shorted or otherwise held low for more than 250uS
	uint8_t (* reset)(void);
	
	// Issue a 1-Wire rom select command, you do the reset first.
	void (* select)(const uint8_t rom[8]);
	
	// Issue a 1-Wire rom skip command, to address all on bus.
	void (* skip)(void);
	
	// Write a byte. If 'power' is one then the wire is held high at
	// the end for parasitically powered devices. You are responsible
	// for eventually depowering it by calling depower() or doing
	// another read or write.
	void (* write)(uint8_t v, uint8_t power);
	
	void (* write_bytes)(const uint8_t *buf, uint16_t count, bool power);
	
	// Read a byte.
	uint8_t (* read)(void);
	
	void (* read_bytes)(uint8_t *buf, uint16_t count);
	
	// Write a bit. The bus is always left powered at the end, see
	// note in write() about that.
	void (* write_bit)(uint8_t v);
	
	// Read a bit.
	uint8_t (* read_bit)(void);
	
	// Stop forcing power onto the bus. You only need to do this if
	// you used the 'power' flag to write() or used a write_bit() call
	// and aren't about to do another read or write. You would rather
	// not leave this powered if you don't have to, just in case
	// someone shorts your bus.
	void (* depower)(void);
	
#ifdef ONEWIRE_SEARCH
	// Clear the search state so that if will start from the beginning again.
	void (* reset_search)(void);
	
	// Setup the search to find the device type 'family_code' on the next call
	// to search(*newAddr) if it is present.
	void (* target_search)(uint8_t family_code);
	
	// Look for the next device. Returns 1 if a new address has been
	// returned. A zero might mean that the bus is shorted, there are
	// no devices, or you have already retrieved all of them.  It
	// might be a good idea to check the CRC to make sure you didn't
	// get garbage.  The order is deterministic. You will always get
	// the same devices in the same order.
	uint8_t (* search)(uint8_t *newAddr);
#endif
	
#ifdef ONEWIRE_CRC
	// Compute a Dallas Semiconductor 8 bit CRC, these are used in the
	// ROM and scratchpad registers.
	uint8_t (* crc8)(const uint8_t *addr, uint8_t len);
	
#ifdef ONEWIRE_CRC16
	// Compute the 1-Wire CRC16 and compare it against the received CRC.
	// Example usage (reading a DS2408):
	//    // Put everything in a buffer so we can compute the CRC easily.
	//    uint8_t buf[13];
	//    buf[0] = 0xF0;    // Read PIO Registers
	//    buf[1] = 0x88;    // LSB address
	//    buf[2] = 0x00;    // MSB address
	//    WriteBytes(net, buf, 3);    // Write 3 cmd bytes
	//    ReadBytes(net, buf+3, 10);  // Read 6 data bytes, 2 0xFF, 2 CRC16
	//    if (!CheckCRC16(buf, 11, &buf[11])) {
	//        // Handle error.
	//    }     
	//          
	// @param input - Array of bytes to checksum.
	// @param len - How many bytes to use.
	// @param inverted_crc - The two CRC16 bytes in the received data.
	//                       This should just point into the received data,
	//                       *not* at a 16-bit integer.
	// @param crc - The crc starting value (optional)
	// @return True, iff the CRC matches.
	bool (* check_crc16)(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc);
	
	// Compute a Dallas Semiconductor 16 bit CRC.  This is required to check
	// the integrity of data received from many 1-Wire devices.  Note that the
	// CRC computed here is *not* what you'll get from the 1-Wire network,
	// for two reasons:
	//   1) The CRC is transmitted bitwise inverted.
	//   2) Depending on the endian-ness of your processor, the binary
	//      representation of the two-byte return value may have a different
	//      byte order than the two bytes you get from 1-Wire.
	// @param input - Array of bytes to checksum.
	// @param len - How many bytes to use.
	// @param crc - The crc starting value (optional)
	// @return The CRC16, as defined by Dallas Semiconductor.
	uint16_t (* crc16)(const uint8_t* input, uint16_t len, uint16_t crc);
#endif
#endif
};

extern struct OneWire _one_wire;

#endif
