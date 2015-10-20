#include "flash.h"
#include "stm8s.h"

#define FLASH_WRITE_ADDR FLASH_DATA_START_PHYSICAL_ADDRESS

int flash_write_buffer(char * buff, int size)
{
	int i;
	
	/* Define flash programming Time*/
	FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
	
	/* Unlock flash data eeprom memory */
	FLASH_Unlock(FLASH_MEMTYPE_DATA);
	/* Wait until Data EEPROM area unlocked flag is set*/
	while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET)
	{}
	
	/* Erase block 0 and verify it */
	/* This function is executed from RAM */
	FLASH_EraseBlock(0, FLASH_MEMTYPE_DATA);
	
	/* Wait until End of high voltage flag is set*/
	while (FLASH_GetFlagStatus(FLASH_FLAG_HVOFF) == RESET)
	{}
	
	for (i = 0; i < size; i++)
	{
		FLASH_ProgramByte(FLASH_WRITE_ADDR + i, buff[i]);
		/* Wait until End of high voltage flag is set*/
		while (FLASH_GetFlagStatus(FLASH_FLAG_EOP) == RESET)
		{}
	}
	
	FLASH_Lock(FLASH_MEMTYPE_DATA);
	return 0;
}

int flash_read_buffer(char * buff, int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		buff[i] = FLASH_ReadByte(FLASH_WRITE_ADDR + i);
	}
	return 0;
}
