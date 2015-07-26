#include "app.h" 
#include "uart.h"
#include "string.h"
#include "linearkeypad.h"
#include "eep.h"


/*
*------------------------------------------------------------------------------
* Public Variables
* Buffer[0] = seconds, Buffer[1] = minutes, Buffer[2] = Hour,
* Buffer[3] = day, Buffer[4] = date, Buffer[5] = month, Buffer[6] = year
*------------------------------------------------------------------------------
*/
UINT8 max[NO_OF_DIGITS] = {0x39,0x35,0x39,0x39};

void APP_conversion(void);
void APP_resetDisplayBuffer(void);
void APP_resetBuffer1(void);
void APP_resetBuffer2(void);
void APP_resetDisplayBuffer(void);


/*
*------------------------------------------------------------------------------
* app - the app structure. 
*------------------------------------------------------------------------------
*/
typedef struct _App
{
	APP_STATE state;				//maintain the state 
	UINT8 displayBuffer[6];			//store for display the rtc value 
	UINT8 mildstone1[NO_OF_DIGITS];//store the mildstone1 value
	UINT8 mildstone2[NO_OF_DIGITS];//store the mildstone2 value
	UINT8 blinkIndex;				//index to blink the DIGIT
	UINT16 mildstone1Value;			// store mildstone1 value
	UINT16 mildstone2Value;			// store mildstone2 value
	UINT16 currentTime;
	UINT8 mildstone1Flag;
	UINT8 mildstone2Flag;

}APP;

#pragma idata app_data
APP app = {0};
#pragma idata


/*
*------------------------------------------------------------------------------
* void APP_init(void)
*
* Summary	: Initialize application
*
* Input		: None
*
* Output	: None
*------------------------------------------------------------------------------
*/
void APP_init( void )
{
	UINT8 i;

	//Set Date and Time
	//WriteRtcTimeAndDate(writeTimeDateBuffer);

	//Read the value of PRESET TIME from EPROM
	for(i = 0 ; i < NO_OF_DIGITS;i++)
	{
		app.mildstone1[i] = Read_b_eep (EEPROM_MILDSTONE1_ADDRESS  + i);  
		Busy_eep();	
	}

	app.mildstone1Value = (UINT16)(((app.mildstone1[3]- '0' )* 10 )+ ( app.mildstone1[2] - '0') )* 60 +(((app.mildstone1[1]- '0' )* 10 )+ (app.mildstone1[0] - '0'));

	for(i = 0 ; i < NO_OF_DIGITS;i++)
	{
		app.mildstone2[i] = Read_b_eep (EEPROM_MILDSTONE2_ADDRESS  + i);  
		Busy_eep();	
	}

	app.mildstone2Value = (UINT16)(((app.mildstone2[3]- '0' )* 10 )+ ( app.mildstone2[2] - '0') )* 60 +(((app.mildstone2[1]- '0' )* 10 )+ (app.mildstone2[0] - '0'));
	
	APP_resetDisplayBuffer();
	DigitDisplay_updateBufferPartial(app.displayBuffer , 0, 4);

	GREEN_LAMP = RESET;
	RED_LAMP = RESET;

	CLOCK_LED = TRUE;


}


/*
*------------------------------------------------------------------------------
* void APP_task(void)
*
* Summary	: 
*
* Input		: None
*
* Output	: None
*------------------------------------------------------------------------------
*/


void APP_task( void )
{
	UINT8 i;


		switch(app.state)
		{
			case HALT_STATE:
			

			if (LinearKeyPad_getPBState(COUNT_PB) == KEY_PRESSED)
			{
				ResetAppTime();
				app.state = COUNT_STATE;

			
			}
			
			 if (LinearKeyPad_getPBState(MILDSTONE1_PB) == KEY_PRESSED)
			{
				//Reset Target buffer which is used to hold preset data
				APP_resetBuffer1();

				//Blink first digit in the display
				app.blinkIndex = 0;
				DigitDisplay_updateBuffer(app.mildstone1);
				DigitDisplay_blinkOn_ind(500, app.blinkIndex);


				GREEN_LAMP = RESET;
				RED_LAMP = RESET;

				app.mildstone1Flag = TRUE;
				//Change state to setting state
				app.state = SETTING_STATE;
			
			}

			//Check the keypress Status of HOOTER_OFF_PB 
			 if(LinearKeyPad_getPBState(MILDSTONE2_PB) == KEY_PRESSED)
			{
				//Reset Target buffer which is used to hold preset data
				APP_resetBuffer2();

				//Blink first digit in the display
				app.blinkIndex = 0;
				DigitDisplay_updateBuffer(app.mildstone2);
				DigitDisplay_blinkOn_ind(500, app.blinkIndex);

				GREEN_LAMP = RESET;
				RED_LAMP = RESET;

				app.mildstone2Flag = TRUE;
				//Change state to setting state
				app.state = SETTING_STATE;
			}

			
			break;

			case SETTING_STATE:

			// Code to handle Digit index PB
			if (LinearKeyPad_getPBState(DIGIT_INDEX_PB ) == KEY_PRESSED) 
			{
				app.blinkIndex++;

				if(app.blinkIndex > (NO_OF_DIGITS -  1))
					app.blinkIndex = 0 ;

				DigitDisplay_blinkOn_ind(500, app.blinkIndex);

			}

			// Code to handle increment PB
			if (LinearKeyPad_getPBState(INCREMENT_PB) == KEY_PRESSED) 
			{

				if(	app.mildstone1Flag == TRUE )
				{
					app.mildstone1[app.blinkIndex]++;
					if(app.mildstone1[app.blinkIndex] > max[app.blinkIndex])
						app.mildstone1[app.blinkIndex] = '0';			
							
					DigitDisplay_updateBuffer(app.mildstone1);
				}
				if(	app.mildstone2Flag == TRUE)
				{
					app.mildstone2[app.blinkIndex]++;
					if(app.mildstone2[app.blinkIndex] > max[app.blinkIndex])
						app.mildstone2[app.blinkIndex] = '0';			
							
					DigitDisplay_updateBuffer(app.mildstone2);

				}

			}
			
			if ((LinearKeyPad_getPBState(MILDSTONE1_PB) == KEY_PRESSED) && (app.mildstone1Flag == TRUE))
			{
				app.mildstone1Flag = FALSE;
				//Turn of blink
				DigitDisplay_blinkOff();

				//Display the preset value on the display
				DigitDisplay_updateBuffer(app.mildstone1);


				//Store preset value in the EEPROM
				for(i = 0; i < NO_OF_DIGITS ; i++ )
				{
					Write_b_eep( EEPROM_MILDSTONE1_ADDRESS  + i ,app.mildstone1[i] );
					Busy_eep( );
				}

				app.mildstone1Value = (UINT16)(((app.mildstone1[3]- '0' )* 10 )+ ( app.mildstone1[2] - '0') )* 60 +(((app.mildstone1[1]- '0' )* 10 )+ (app.mildstone1[0] - '0'));
				app.state = HALT_STATE;	
				
			}
			if ((LinearKeyPad_getPBState(MILDSTONE2_PB) == KEY_PRESSED) && (app.mildstone2Flag == TRUE))
			{
				app.mildstone2Flag = FALSE;
				//Turn of blink
				DigitDisplay_blinkOff();

				//Display the preset value on the display
				DigitDisplay_updateBuffer(app.mildstone2);


				//Store preset value in the EEPROM
				for(i = 0; i < NO_OF_DIGITS ; i++ )
				{
					Write_b_eep( EEPROM_MILDSTONE2_ADDRESS  + i ,app.mildstone2[i] );
					Busy_eep( );
				}

				app.mildstone2Value = (UINT16)(((app.mildstone2[3]- '0' )* 10 )+ ( app.mildstone2[2] - '0') )* 60 +(((app.mildstone2[1]- '0' )* 10 )+ (app.mildstone2[0] - '0'));
				app.state = HALT_STATE;
				
			}


				
			break;
			
			case COUNT_STATE:

	
				if (LinearKeyPad_getPBState(HALT_PB) == KEY_PRESSED)
				{
					APP_resetDisplayBuffer();
					DigitDisplay_updateBufferPartial(app.displayBuffer , 0, 4);
					GREEN_LAMP = RESET;
					RED_LAMP = RESET;
	
					app.state = HALT_STATE;
					break;
				}


				app.currentTime = GetAppTime();

				if(app.currentTime < 6000)
				{
					APP_conversion();
					DigitDisplay_updateBufferPartial(app.displayBuffer , 0, 4); 
				
				}
				else
				{
					APP_resetDisplayBuffer();
					DigitDisplay_updateBufferPartial(app.displayBuffer , 0, 4);
					app.state = HALT_STATE;	
					break;					
				}

				if( (app.currentTime >= app.mildstone1Value ) && (app.currentTime < app.mildstone2Value) )
				{
					GREEN_LAMP = SET;
				}
				else if (app.currentTime >= app.mildstone2Value)
				{
					GREEN_LAMP = RESET;
					RED_LAMP = SET;					
				}

				break;			
		}
			
}		


void APP_conversion(void)
{
	UINT16 mtemp ,stemp ;
	mtemp = (app.currentTime / 60);
	stemp = (app.currentTime % 60);
			
	app.displayBuffer[0] = (UINT8)(stemp%10) + '0';        //Seconds LSB
	app.displayBuffer[1] = (UINT8)(stemp/10) + '0'; 		//Seconds MSB
	app.displayBuffer[2] = (UINT8)(mtemp%10) + '0';        //Minute LSB
	app.displayBuffer[3] = (UINT8)(mtemp/10) + '0' ; 		//Minute MSB
	app.displayBuffer[4] = '0';        				//Minute LSB
	app.displayBuffer[5] = '0' ; 					//Minute MSB
}


void APP_resetDisplayBuffer(void)
{
	UINT8 i ;
	for(i = 0; i < 6; i++)			//reset all digits
	{
		app.displayBuffer[i] = '0';
	}
}

	
void APP_resetBuffer1(void)
{
	UINT8 i ;
	for(i = 0; i < NO_OF_DIGITS ; i++)			//reset all digits
	{
		app.mildstone1[i] = '0';
	}
}

void APP_resetBuffer2(void)
{
	UINT8 i ;
	for(i = 0; i < NO_OF_DIGITS ; i++)			//reset all digits
	{
		app.mildstone2[i] = '0';
	}
}




	