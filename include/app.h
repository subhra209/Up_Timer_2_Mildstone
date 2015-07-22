#include "board.h"
#include "typedefs.h"
#include "digitdisplay.h"
#include "rtc_driver.h"


typedef enum 
{
	HALT_STATE = 0,
	COUNT_STATE,
	SETTING_STATE
}APP_STATE;


typedef enum
{
	MODE_CHANGE_INPUT_MASK = 0X80,
	NEXT_INPUT_MASK = 0X40,
	SET_INPUT_MASK = 0X20
}MASK_INTR_DATA;


typedef enum
{
	HALT_PB = 0,
	COUNT_PB = 0,
	MODE_CHANGE_PB,
	DIGIT_INDEX_PB,
	INCREMENT_PB,
	MILDSTONE1_PB,
	MILDSTONE2_PB
}PBS;



extern UINT8 APP_comCallBack( far UINT8 *rxPacket, far UINT8* txCode,far UINT8** txPacket);
extern void APP_init(void);
extern void APP_task(void);
