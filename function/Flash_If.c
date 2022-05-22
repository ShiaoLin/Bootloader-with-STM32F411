/**
**********************************
  @File  Flash_If.c
  @Version  Ver 2.2
  @Brief  Flash Read/Write
  @Change
          DATE      VERSION           DETAIL
    2021/11/01        2.2.2           Modified the function, Flash_Wear_Read, Flash_Wear_Write, Find_Addr, add a new parameter to solve a bug
                                      Delete the unnecessary function, Get_Length
    2021/10/04        2.2.1           Modified the function, Flash_Wear_Read, add a new parameter to solve a bug
    2021/08/24        2.2.0           Combined with Controller Firmware
    2021/08/24        2.1.0           Flash_Wear_Read/Write can assign which flash want to read/write
    
    2021/08/20        2.0.0           Add Flash Wear Leveling
                                      Separate Flash_If_Read/Write to Flash_If_Read/Write_Word and Flash_If_Read/Write_Byte
**********************************
**/
#include "Flash_If.h"

#if(STM32F103C8Tx)
uint32_t Flash_If_GetPageAddress(uint32_t addr)
{
  uint8_t number = 0;
  uint32_t pageaddr = ADDR_FLASH_PAGE_0;
  while(addr < (pageaddr | (FLASH_PAGE_SIZE * number++)));
  return pageaddr | FLASH_PAGE_SIZE * (number-1);
}
uint8_t Flash_If_GetPageNumber(uint32_t addr, uint32_t NumToWrite)
{
  return 0;
}
#elif(STM32F411CEU6)
static uint32_t CurrHeaderAddr = 0;
static uint32_t NextHeaderAddr = 0;
static uint32_t CurrSector = 0;

static uint8_t GetSectorNumber(uint32_t addr)
{
  if      (addr < ADDR_FLASH_SECTOR_1)  return FLASH_SECTOR_0;
  else if (addr < ADDR_FLASH_SECTOR_2)  return FLASH_SECTOR_1;
  else if (addr < ADDR_FLASH_SECTOR_3)  return FLASH_SECTOR_2;
  else if (addr < ADDR_FLASH_SECTOR_4)  return FLASH_SECTOR_3;
  else if (addr < ADDR_FLASH_SECTOR_5)  return FLASH_SECTOR_4;
  else if (addr < ADDR_FLASH_SECTOR_6)  return FLASH_SECTOR_5;
  else if (addr < ADDR_FLASH_SECTOR_7)  return FLASH_SECTOR_6;
  else return FLASH_SECTOR_7;
}

static uint32_t GetSectorMaxAddress(void)
{
  switch(CurrSector)
  {
    case FLASH_SECTOR_0:
      return ADDR_FLASH_SECTOR_1;
    case FLASH_SECTOR_1:
      return ADDR_FLASH_SECTOR_2;
    case FLASH_SECTOR_2:
      return ADDR_FLASH_SECTOR_3;
    case FLASH_SECTOR_3:
      return ADDR_FLASH_SECTOR_4;
    case FLASH_SECTOR_4:
      return ADDR_FLASH_SECTOR_5;
    case FLASH_SECTOR_5:
      return ADDR_FLASH_SECTOR_6;
    case FLASH_SECTOR_6:
      return ADDR_FLASH_SECTOR_7;
    case FLASH_SECTOR_7:
      return 0x08080000;
  }
  return 0;
}

#elif(STM32F429IGT6)
uint8_t Flash_If_GetSectorNumber(uint32_t addr)
{
  if      (addr < ADDR_FLASH_SECTOR_1)  return FLASH_SECTOR_0;
  else if (addr < ADDR_FLASH_SECTOR_2)  return FLASH_SECTOR_1;
  else if (addr < ADDR_FLASH_SECTOR_3)  return FLASH_SECTOR_2;
  else if (addr < ADDR_FLASH_SECTOR_4)  return FLASH_SECTOR_3;
  else if (addr < ADDR_FLASH_SECTOR_5)  return FLASH_SECTOR_4;
  else if (addr < ADDR_FLASH_SECTOR_6)  return FLASH_SECTOR_5;
  else if (addr < ADDR_FLASH_SECTOR_7)  return FLASH_SECTOR_6;
  else if (addr < ADDR_FLASH_SECTOR_12) return FLASH_SECTOR_7;
  else if (addr < ADDR_FLASH_SECTOR_13) return FLASH_SECTOR_12;
  else if (addr < ADDR_FLASH_SECTOR_14) return FLASH_SECTOR_13;
  else if (addr < ADDR_FLASH_SECTOR_15) return FLASH_SECTOR_14;
  else if (addr < ADDR_FLASH_SECTOR_16) return FLASH_SECTOR_15;
  else if (addr < ADDR_FLASH_SECTOR_17) return FLASH_SECTOR_16;
  else if (addr < ADDR_FLASH_SECTOR_18) return FLASH_SECTOR_17;
  else if (addr < ADDR_FLASH_SECTOR_19) return FLASH_SECTOR_18;
  else return FLASH_SECTOR_19;
}
#endif
static uint8_t Read_Byte(uint32_t addr)
{
  return *(__IO uint8_t*)addr;
}

static uint32_t Read_Word(uint32_t addr)
{
  return *(__IO uint32_t*)addr;
}

static bool Find_Addr(uint32_t addr, uint32_t numToFind)
{
  uint32_t Temp = 0;
  while(Temp != HEADER && CurrHeaderAddr < GetSectorMaxAddress())
  {  
    Flash_If_Read_Word(CurrHeaderAddr, &Temp, 1);
    CurrHeaderAddr += 4;
  }

  if(CurrHeaderAddr >= GetSectorMaxAddress())
  {
    CurrHeaderAddr = addr;
    NextHeaderAddr = CurrHeaderAddr + 4 + numToFind*4;
    return false;
  }
  NextHeaderAddr = CurrHeaderAddr + numToFind*4;
  CurrHeaderAddr -= 4;
  return true;
}

/**
  * @brief  Read data from parameter flash
  * @param  addr :Address wants to read
  * @param  pBuffer :uint32_t Buffer to store data
  * @retval none
  */
void Flash_Wear_Read(uint32_t addr, uint32_t* pBuffer, uint32_t numToRead)
{
  CurrHeaderAddr = addr;
  NextHeaderAddr = addr;
  CurrSector = GetSectorNumber(addr);
  if(Find_Addr(addr, numToRead))
  {
    Flash_If_Read_Word(CurrHeaderAddr + 4, pBuffer, numToRead);
  }
  else
  {
    Flash_If_Read_Word(CurrHeaderAddr, pBuffer, numToRead);
  }
}

/**
  * @brief  Write data to parameter flash
  * @param  addr :Address wants to write
  * @param  pBuffer :uint32_t Data buffer to write
  * @retval none
  */
void Flash_Wear_Write(uint32_t addr, uint32_t* pBuffer, uint32_t numToWrite)
{
  CurrHeaderAddr = addr;
  NextHeaderAddr = addr;
  CurrSector = GetSectorNumber(addr);
  uint32_t Temp = 0;
  if(Find_Addr(addr, numToWrite))
  {
    for(uint32_t addridx = CurrHeaderAddr; addridx < NextHeaderAddr; addridx +=4)
    {
      Flash_If_Write_Word(addridx,&Temp,1);
    }
    CurrHeaderAddr = NextHeaderAddr;
    if(CurrHeaderAddr + 4 + numToWrite*4 >= GetSectorMaxAddress())
    {
      CurrHeaderAddr = addr;
    }
    else
      CurrHeaderAddr = NextHeaderAddr;
  }
  if(CurrHeaderAddr == addr)
  Flash_If_Erase(CurrSector);
  Temp = HEADER;
  Flash_If_Write_Word(CurrHeaderAddr, &Temp, 1);
  Flash_If_Write_Word(CurrHeaderAddr + 4, pBuffer, numToWrite);
  NextHeaderAddr = CurrHeaderAddr + 8 + numToWrite*4;
}

/**
  * @brief  Read data from flash (in byte)
  * @param  ReadAddr :Data Address
  * @param  pBuffer :uint8_t Data buffer to write
  * @param  NumtoRead :Data length
  * @retval none
  */
void Flash_If_Read_Byte(uint32_t readAddr, uint8_t* pBuffer, uint32_t numToRead)
{
  uint8_t *p;
  for(p = pBuffer; p < pBuffer + numToRead; p++)
  {
    *p = Read_Byte(readAddr);
    readAddr += 1;
  }
}
/**
  * @brief  Read data from flash (in word)
  * @param  ReadAddr :Data Address
  * @param  pBuffer :uint32_t Buffer to store data
  * @param  NumtoRead :Data length
  * @retval none
  */


void Flash_If_Read_Word(uint32_t readAddr, uint32_t* pBuffer, uint32_t numToRead)
{
  uint32_t *p;
  for(p = pBuffer; p < pBuffer + numToRead; p++)
  {
    *p = Read_Word(readAddr);
    readAddr += 4;
  }
}

/**
  * @brief  Write data from flash (in byte)
  * @param  ReadAddr :Data Address
  * @param  pBuffer :uint8_t Data buffer to write
  * @param  NumtoRead :Data length
  * @retval none
  */
void Flash_If_Write_Byte(uint32_t writeAddr, uint8_t* pBuffer, uint32_t numToWrite)
{
  HAL_StatusTypeDef FlashStatus = HAL_OK;
  uint32_t end_addr = 0;
//  if(writeAddr < ADDR_SYSTEM_MEMORY)
//  {
//    HAL_UART_Transmit(&huart2,(uint8_t*)0xFF,1,0x10);
//    return;
//  }
  
  HAL_FLASH_Unlock();
  end_addr = writeAddr + numToWrite;
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR | FLASH_FLAG_PGAERR);
  FlashStatus = FLASH_WaitForLastOperation(10000);
  if(FlashStatus == HAL_OK)
  {
    while(writeAddr < end_addr)
    {
      if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, writeAddr, *pBuffer) != HAL_OK)
      {
        break;
      }
      writeAddr++;
      pBuffer++;
    }
  }
  HAL_FLASH_Lock();
}

/**
  * @brief  Write data from flash (in word)
  * @param  ReadAddr :Data Address
  * @param  pBuffer :uint32_t Data buffer to write
  * @param  NumtoRead :Data length
  * @retval none
  */
void Flash_If_Write_Word(uint32_t writeAddr, uint32_t* pBuffer, uint32_t numToWrite)
{
  HAL_StatusTypeDef FlashStatus = HAL_OK;
  uint32_t end_addr = 0;
  if(writeAddr < (ADDR_SYSTEM_MEMORY || writeAddr % 4))
    return;
  
  HAL_FLASH_Unlock();
  end_addr = writeAddr + numToWrite*4;
  FlashStatus = FLASH_WaitForLastOperation(10000);
  if(FlashStatus == HAL_OK)
  {
    while(writeAddr < end_addr)
    {
      if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, writeAddr, *pBuffer) != HAL_OK)
      {
        break;
      }
      writeAddr += 4;
      pBuffer++;
    }
  }
  HAL_FLASH_Lock();
}

/**
  * @brief  Erase the whole sector
  * @param  sectorNum :The sector need to erase
  * @retval none
  */
void Flash_If_Erase(uint8_t sectorNum)
{
  uint32_t SectorError = 0;
  HAL_FLASH_Unlock();
  FLASH_EraseInitTypeDef EraseInitStruct;
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.Sector = sectorNum;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.NbSectors = 1;
  HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
  HAL_FLASH_Lock();
}

