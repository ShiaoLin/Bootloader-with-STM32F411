/**
**********************************
  @File  bootloader.c
  @Version  Ver 3.1
  @Brief  f411 bootloader
  @Change
          DATE          VERSION           DETAIL
          2021/10/15    3.1               Seperate APP_Parameter address and Boot_Parameter address
          2021/10/06    3.0               Work now
          
  @Attention
        Need to modified CDC_Transmit_FS and u16RxCount in usbd_cdc_if.c first
**********************************
**/
#include "Flash_If.h"
#include "CRC16.h"
#include "usb_device.h"
#include "Bootloader.h"
#include "tim.h"
uint8_t ACK[] = {0x06,0x0D,0x0A};
uint8_t NAK[] = {0x15,0x0D,0x0A};
uint8_t u8Buffer[64] = {0};
Bootloader_t hBootloader;
static bool timeoutFlag = false;
static uint32_t u32WriteAddr = 0;
static uint8_t u8RxErrorCount = 0;
pFunction JumpToApplication;
uint32_t JumpAddress;

extern uint8_t UserRxBufferFS[];
extern uint8_t UserTxBufferFS[];
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
extern uint16_t u16RxCount;

extern USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_StatusTypeDef USBD_DeInit(USBD_HandleTypeDef *pdev);
void MX_USB_DEVICE_DeInit(void)
{
  USBD_DeInit(&hUsbDeviceFS);
}
uint16_t FlashCRC(Bootloader_t* hBootloader,uint32_t flashAddr)
{
  uint16_t CRC_Temp;
  uint8_t Buf[1] = {0};
  CRC_Temp = AccumulateCRC16(Buf,0);
  uint32_t size = 0;
  if (flashAddr == APPLICATION_ADDRESS)
  {
    size = hBootloader->AppSize;
  }
  else if (flashAddr == BACKUP_ADDRESS)
  {
    size = hBootloader->BackupSize;
  }
  for(uint32_t idx = 0; idx < size; idx++)
  {
    Flash_If_Read_Byte(flashAddr+idx,Buf,1);
    CRC_Temp = AccumulateCRC16(Buf,1);
  }
  return CRC_Temp;
}

bool FlashCheck(Bootloader_t* hBootloader,uint32_t flashAddr)
{
  uint16_t CRC_Temp;
  uint8_t Buf[1] = {0};
  CRC_Temp = AccumulateCRC16(Buf,0);
  uint32_t size = 0;
  uint16_t CRC_Store = 0;
  if (flashAddr == APPLICATION_ADDRESS)
  {
    size = hBootloader->AppSize;
    CRC_Store = hBootloader->AppCRC;
  }
  else if (flashAddr == BACKUP_ADDRESS)
  {
    size = hBootloader->BackupSize;
    CRC_Store = hBootloader->BackupCRC;
  }
  for(uint32_t idx = 0; idx < size; idx++)
  {
    Flash_If_Read_Byte(flashAddr+idx,Buf,1);

    CRC_Temp = AccumulateCRC16(Buf,1);
  }
  if(CRC_Temp == CRC_Store)
    return true;
  else
    return false;
}

/**
  * @brief  Jump to application from bootloader
  * @retval none
  */
void Jump2App(void)
{
  __set_PRIMASK(1);
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  HAL_RCC_DeInit();
  for (uint16_t i = 0; i < 8; i++)
  {
    NVIC->ICER[i]=0xFFFFFFFF;
    NVIC->ICPR[i]=0xFFFFFFFF;
  }
  JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
  JumpToApplication = (pFunction) JumpAddress;
  __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
  JumpToApplication();
}

void Bootloader_GetSectorNumber(void)
{
  for(uint8_t i = 0; i < 6; i++)
  {
    HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
    HAL_Delay(100);
  }
  for(uint8_t i = 0; i < 4; i++)
  {
    HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
    HAL_Delay(400);
  }
}

static void Bootloader_FlashWrite(Bootloader_t *hBootloader)
{
  uint32_t u32Buf[BOOTLOADER_NUMBER] = {0};
  Flash_Wear_Read(BOOTLOADER_ADDRESS, u32Buf, BOOTLOADER_NUMBER);
  u32Buf[0] = (uint32_t)(hBootloader->event<<24);
  u32Buf[1] = (uint32_t)((hBootloader->AppCRC<<16) | (hBootloader->BackupCRC));
  u32Buf[2] = hBootloader->AppSize;
  u32Buf[3] = hBootloader->BackupSize;
  Flash_Wear_Write(BOOTLOADER_ADDRESS, u32Buf, BOOTLOADER_NUMBER);
}

static uint8_t Bootloader_FlashDataCopy(Bootloader_t *hBootloader,uint32_t u32SourceAddr, uint32_t u32TargetAddr)
{
  uint8_t u8ErrorCount = 0;
  bool result = true;
  uint16_t u16DataCRC = 0;
  uint32_t u32DataSize = 0;
  uint8_t u8TargetSector = 0;
  if (u32SourceAddr == APPLICATION_ADDRESS)
  {
    u16DataCRC = hBootloader->AppCRC;
    u32DataSize = hBootloader->AppSize;
  }
  else if (u32SourceAddr == BACKUP_ADDRESS)
  {
    u16DataCRC = hBootloader->BackupCRC;
    u32DataSize = hBootloader->BackupSize;
  }
  else
  {
    result = false;
  }
  if (u32TargetAddr == APPLICATION_ADDRESS)
  {
    u8TargetSector = APPLICATION_SECTOR;
    hBootloader->AppCRC = u16DataCRC;
    hBootloader->AppSize = u32DataSize;
  }
  else if (u32TargetAddr == BACKUP_ADDRESS)
  {
    u8TargetSector = BACKUP_SECTOR;
    hBootloader->BackupCRC = u16DataCRC;
    hBootloader->BackupSize = u32DataSize;
  }
  else
  {
    result = false;
  }
  if (result == 0)
  {
    return result;
  }
  do
  {
    Flash_If_Erase(u8TargetSector);
    for (uint16_t idx = 0; idx <=(u32DataSize/dfDataLength); idx++)
    {
      Flash_If_Read_Byte(u32SourceAddr+idx*dfDataLength,u8Buffer,dfDataLength);
      Flash_If_Write_Byte(u32TargetAddr+idx*dfDataLength,u8Buffer,dfDataLength);
    }
    result = FlashCheck(hBootloader,u32TargetAddr);
    if (result)
    {
      break;
    }
    else
    {
      u8ErrorCount += 1;
    }
  }while(u8ErrorCount < 2 );
  return result;
}

static void Bootloader_Idle(Bootloader_t *hBootloader)
{
  bool result = 0;
  if (hBootloader->event == EV_INIT)
  {
    if (u16RxCount != 0)
    {
      if (!(u16RxCount > dfRxLength || !CheckCRC(UserRxBufferFS,u16RxCount)) && (UserRxBufferFS[0] == 0x05 && UserRxBufferFS[1] == 0x1E))
      {
        Flash_If_Erase(APPLICATION_SECTOR);
        u32WriteAddr = 0;
        CDC_Transmit_FS(ACK,3);
        __HAL_TIM_SetCounter(&htim4,0);
        htim4.Instance->SR = 0;
        HAL_TIM_Base_Start_IT(&htim4);
        hBootloader->state = ST_RECEIVE;
      }
      u16RxCount = 0;
    }
  }
  else if (hBootloader->event == EV_NULL)
  {
    hBootloader->state = ST_CHECK;
  }
  else if (hBootloader->event == EV_UPGRADE)
  {
    result = Bootloader_FlashDataCopy(hBootloader,APPLICATION_ADDRESS,BACKUP_ADDRESS);
    HAL_Delay(2000);
    if (result)
    {
      u32WriteAddr = 0;
      Flash_If_Erase(APPLICATION_SECTOR);
      HAL_Delay(1000);
      CDC_Transmit_FS(ACK,3);
      __HAL_TIM_SetCounter(&htim4,0);
      htim4.Instance->SR = 0;
      HAL_TIM_Base_Start_IT(&htim4);
      hBootloader->state = ST_RECEIVE;
    }
    else
    {
      hBootloader->state = ST_ERROR;
    }
  }
}
static void Bootloader_Receive(Bootloader_t *hBootloader)
{
  if (timeoutFlag)
  {
    u16RxCount = 0;
    u8RxErrorCount = 0;
    hBootloader->state = ST_ERROR;
    return;
  }
  if (u16RxCount != 0)
  {
    HAL_TIM_Base_Stop_IT(&htim4);
    __HAL_TIM_SetCounter(&htim4,0);
    __set_PRIMASK(1);
    /*receive data package error*/
    if (u16RxCount > dfRxLength || !CheckCRC(UserRxBufferFS,u16RxCount))
    {
      u8RxErrorCount += 1;
      CDC_Transmit_FS(NAK,3);
      if (u8RxErrorCount > 5)
      {
        u8RxErrorCount = 0;
        hBootloader->state = ST_ERROR;
      }
    }

    /*receive last package of data*/
    else if(UserRxBufferFS[0] == 0x04 && UserRxBufferFS[1] == 0x1E)
    {
      hBootloader->AppSize = UserRxBufferFS[4]<<24 | UserRxBufferFS[5]<<16 | UserRxBufferFS[6]<<8 | UserRxBufferFS[7];
      hBootloader->AppCRC = UserRxBufferFS[8]<<8 | UserRxBufferFS[9];
      CDC_Transmit_FS(ACK,3);
      if (hBootloader->BackupSize == 0xFFFFFFFF || hBootloader->BackupSize == 0)
      {
        Bootloader_FlashDataCopy(hBootloader,APPLICATION_ADDRESS,BACKUP_ADDRESS);
      }

      hBootloader->state = ST_CHECK;
    }
    else if(UserRxBufferFS[0] == 0x05 && UserRxBufferFS[1] == 0x1E)
    {
      u16RxCount = 0;
      CDC_Transmit_FS(ACK,3);
    }
    /*recevie application data*/
    else
    {
      Flash_If_Write_Byte(APPLICATION_ADDRESS + u32WriteAddr, UserRxBufferFS, u16RxCount-2);
      u32WriteAddr += (uint32_t)(u16RxCount-2);
      CDC_Transmit_FS(ACK,3);
    }
    u16RxCount = 0;
    __set_PRIMASK(0);
    if (hBootloader->state == ST_RECEIVE)
    {
      htim4.Instance->SR = 0;
      HAL_TIM_Base_Start_IT(&htim4);
    }
  }
  /*no receive*/
  else
  {
    HAL_Delay(5);
  }
}

static void Bootloader_Check(Bootloader_t *hBootloader)
{
  uint8_t u8ErrorCount = 0;
  bool result = 0;
  do
  {
    result = FlashCheck(hBootloader,APPLICATION_ADDRESS);
    if (result == 0)
    {
      u8ErrorCount += 1;
    }
    else
    {
      break;
    }
  }while (u8ErrorCount <= 2);
  if (result)
  {
    hBootloader->state = ST_JUMP;
  }
  else
  {
    hBootloader->state = ST_ERROR;
  }
}

static void Bootloader_Recovery(Bootloader_t *hBootloader)
{
  uint8_t u8ErrorCount = 0;
  bool result = 0;
  do
  {
    result = FlashCheck(hBootloader,BACKUP_ADDRESS);
    if (result)
    {
      result = Bootloader_FlashDataCopy(hBootloader,BACKUP_ADDRESS,APPLICATION_ADDRESS);
      break;
    }
    else
    {
      u8ErrorCount += 1;
    }
  }while(u8ErrorCount <= 2);
  if (result)
  {
    hBootloader->state = ST_JUMP;
  }
  else
  {
    Flash_If_Erase(BACKUP_SECTOR);
    HAL_Delay(1000);
    hBootloader->state = ST_ERROR;
  }
}

static void Bootloader_ErrorHandler(Bootloader_t *hBootloader)
{
  if (FlashCheck(hBootloader,BACKUP_ADDRESS))
  {
    hBootloader->state = ST_RECOVERY;
  }
  else
  {
    hBootloader->AppCRC = 0;
    hBootloader->AppSize = 0;
    hBootloader->BackupCRC = 0;
    hBootloader->BackupSize = 0;
    hBootloader->event = EV_INIT;
    hBootloader->state = ST_IDLE;
    timeoutFlag = false;
    Bootloader_FlashWrite(hBootloader);
    Flash_If_Erase(APPLICATION_SECTOR);
    Flash_If_Erase(BACKUP_SECTOR);
  }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == htim4.Instance)
  {
    timeoutFlag = true;
  }
  return;
}
void Bootloader_PrepareUpgrade(void)
{
	/*close peripheral*/
  MX_USB_DEVICE_Init();
  uint32_t u32Buf[BOOTLOADER_NUMBER] = {0};
  Flash_Wear_Read(BOOTLOADER_ADDRESS,u32Buf, BOOTLOADER_NUMBER);
  u32Buf[0] |= 0x01000000;
  Flash_Wear_Write(BOOTLOADER_ADDRESS,u32Buf, BOOTLOADER_NUMBER);
  HAL_Delay(100);
  __set_PRIMASK(1); // Close all mid-range
  NVIC_SystemReset();//Reset
}

void Bootloader_Init(Bootloader_t *hBootloader)
{
  timeoutFlag = false;
  u32WriteAddr = 0;
  u8RxErrorCount = 0;
  uint32_t u32Buf[BOOTLOADER_NUMBER] = {0};
  Flash_Wear_Read(BOOTLOADER_ADDRESS, u32Buf, BOOTLOADER_NUMBER);
  hBootloader->state = ST_IDLE;
  hBootloader->event = (Event_t)(u32Buf[0]>>24);
  hBootloader->AppCRC = (uint16_t)(u32Buf[1]>>16);
  hBootloader->BackupCRC = (uint16_t)(u32Buf[1]);
  hBootloader->AppSize = u32Buf[2];
  hBootloader->BackupSize = u32Buf[3];
}
void Bootloader_EventHandler(Bootloader_t *hBootloader)
{
  if (hBootloader->state == ST_IDLE)
  {
    Bootloader_Idle(hBootloader);
  }
  else if (hBootloader->state == ST_RECEIVE)
  {
    Bootloader_Receive(hBootloader);
  }
  else if (hBootloader->state == ST_CHECK)
  {
    Bootloader_Check(hBootloader);
  }
  else if (hBootloader->state == ST_RECOVERY)
  {
    Bootloader_Recovery(hBootloader);
  }
  else if (hBootloader->state == ST_ERROR)
  {
    Bootloader_ErrorHandler(hBootloader);
  }
  else if (hBootloader->state == ST_JUMP)
  {
    MX_USB_DEVICE_DeInit();
    HAL_TIM_Base_Stop_IT(&htim4);
    __HAL_TIM_SetCounter(&htim4,0);
    htim4.Instance->SR = 0;
    HAL_TIM_Base_DeInit(&htim4);
    hBootloader->event = EV_NULL;
    Bootloader_FlashWrite(hBootloader);
    HAL_Delay(100);
    Jump2App();
  }
}
