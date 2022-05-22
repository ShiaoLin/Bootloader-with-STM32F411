#ifndef _FLASH_IF_H_
#define _FLASH_IF_H_

#include "Board_Config.h"
#include <stdint.h>
#include <stdbool.h>


#if(STM32F103C8Tx)
#define ADDR_FLASH_PAGE_0     0x08000000
#define PAGE_SIZE             0x400
#define ADDR_SYSTEM_MEMORY    0x1FFFF000

uint32_t Flash_If_GetPageAddress(uint32_t addr);
uint8_t Flash_If_GetPageNumber(uint32_t addr, uint32_t NumToWrite);

#elif(STM32F411CEU6)

#define ADDR_FLASH_SECTOR_1   0x08004000
#define ADDR_FLASH_SECTOR_2   0x08008000
#define ADDR_FLASH_SECTOR_3   0x0800C000
#define ADDR_FLASH_SECTOR_4   0x08010000
#define ADDR_FLASH_SECTOR_5   0x08020000
#define ADDR_FLASH_SECTOR_6   0x08040000
#define ADDR_FLASH_SECTOR_7   0x08060000
#define ADDR_SYSTEM_MEMORY    0x1FFF0000

#define HEADER 0x5A5A5A5A

void Parameter_LoadFromFlash(uint32_t* paraBuf);

#elif(STM32F429IGT6)
#define ADDR_FLASH_SECTOR_1   0x08004000
#define ADDR_FLASH_SECTOR_2   0x08008000
#define ADDR_FLASH_SECTOR_3   0x0800C000
#define ADDR_FLASH_SECTOR_4   0x08010000
#define ADDR_FLASH_SECTOR_5   0x08020000
#define ADDR_FLASH_SECTOR_6   0x08040000
#define ADDR_FLASH_SECTOR_7   0x08060000
#define ADDR_FLASH_SECTOR_12  0x08080000
#define ADDR_FLASH_SECTOR_13  0x08084000
#define ADDR_FLASH_SECTOR_14  0x08088000
#define ADDR_FLASH_SECTOR_15  0x0808C000
#define ADDR_FLASH_SECTOR_16  0x08090000
#define ADDR_FLASH_SECTOR_17  0x080A0000
#define ADDR_FLASH_SECTOR_18  0x080C0000
#define ADDR_FLASH_SECTOR_19  0x080E0000
#define ADDR_SYSTEM_MEMORY    0x1FFF0000

uint8_t Flash_If_GetSectorNumber(uint32_t addr);
#endif

void Flash_Wear_Read(uint32_t addr, uint32_t* pBuffer, uint32_t numToRead);
void Flash_Wear_Write(uint32_t addr, uint32_t* pBuffer, uint32_t numToWrite);
void Flash_If_Read_Byte(uint32_t readAddr, uint8_t* pBuffer, uint32_t numToRead);
void Flash_If_Write_Byte(uint32_t writeAddr, uint8_t* pBuffer, uint32_t numToWrite);
void Flash_If_Read_Word(uint32_t readAddr, uint32_t* pBuffer, uint32_t numToRead);
void Flash_If_Write_Word(uint32_t writeAddr, uint32_t* pBuffer, uint32_t numToWrite);
void Flash_If_Erase(uint8_t sectorNum);

#endif
