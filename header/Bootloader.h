#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "main.h"


typedef enum
{
  ST_IDLE = 0x01,
  ST_RECEIVE,
  ST_WRITE,
  ST_CHECK,
  ST_JUMP,
  ST_ERROR,
  ST_RECOVERY
}State_t;

typedef enum
{
  EV_NULL = 0x00,
  EV_UPGRADE = 0x01,
  EV_INIT = 0xFF,
}Event_t;

typedef struct
{
  State_t state;
  Event_t event;
  uint16_t AppCRC;
  uint32_t AppSize;
  uint16_t BackupCRC;
  uint32_t BackupSize;
}Bootloader_t;



#define dfRxLength 64
#define dfDataLength 64
#define BOOTLOADER_ADDRESS    (uint32_t)0x0800C000
#define BOOTLOADER_SECTOR     3
#define BOOTLOADER_NUMBER     4
#define BACKUP_ADDRESS        (uint32_t)0x08040000
#define BACKUP_SECTOR         6
#define APPLICATION_ADDRESS   (uint32_t)0x08020000
#define APPLICATION_SECTOR    5
extern Bootloader_t hBootloader;
typedef  void (*pFunction)(void);
void Bootloader_PrepareUpgrade(void);
void Bootloader_Init(Bootloader_t *hBootloader);
void Bootloader_EventHandler(Bootloader_t *hBootloader);


#ifdef __cplusplus
}
#endif
#endif /* _BOOTLOADER_H_ */
