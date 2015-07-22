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
UINT8 txBuffer[6] = {0};
UINT8 max[NO_OF_DIGITS] = {0x39,0x39,0x35,0x39};
UINT8 readTimeDateBuffer[6] = {0};
UINT8 writeTimeDateBuffer[] = {0X50, 0X59, 0X00, 0X03, 0x027, 0X12, 0X13};

void APP_conversion(void);
void APP_resetDisplayBuffer(void);
void APP_updateRTC(void);
void APP_resetBuffer(void);


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
	UINT8 prevDisplayBuffer[6]; 	//store rtc value for comparision
	UINT8 blinkIndex;				//index to blink the DIGIT
	UINT16 mildstone1Value;			// store mildstone1 value
	UINT16 mildstone2Value;			// store mildstone2 value
	UINT16 rtcValue;
	BOOL mildstone1Flag;
	BOOL mildstone2Flag;
	BOOL countFlag;
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
	
	//Maintain the state from EPROM
	app.state = Read_b_eep (EEPROM_STATE_ADDRESS);  
	Busy_eep();


	//Read the STORED RTC value from EPROM
	for(i = 0 ; i < 6 ;i++)
	{
		app.displayBuffer[i] = Read_b_eep (EEPROM_RTC_ADDRESS + i);  
		Busy_eep();	
	}

	//update that value on RTC
	APP_updateRTC();


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
			
			//Check the keypress Status of COUNT_PB 
			if ((LinearKeyPad_getKeyState(COUNT_PB) == TRUE) && (app.countFlag == FALSE))
			{
				app.countFlag = TRUE;
				//Reset buffer and RTC
				APP_resetDisplayBuffer();
				DigitDisplay_updateBufferPartial(app.displayBuffer , 0, 4);
				APP_updateRTC();

			}
			else if ((LinearKeyPad_getKeyState(COUNT_PB) == FALSE) && (app.countFlag == TRUE))
			{
				app.countFlag = FALSE;	
				//Store the state
				Write_b_eep( EEPROM_STATE_ADDRESS , COUNT_STATE);
				Busy_eep( );

				//Change the state to STOP state
				app.state = COUNT_STATE;
			
			}
			//Check the keypress Status of MODE_CHANGE_PB
			else if (LinearKeyPad_getKeyState(MILDSTONE1_PB) == TRUE)
			{
				//Reset Target buffer which is used to hold preset data
				APP_resetBuffer();

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
			else if(LinearKeyPad_getKeyState(MILDSTONE2_PB) == TRUE)
			{
				//Reset Target buffer which is used to hold preset data
				APP_resetBuffer();

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
			
				//ON DOT_CONTROL leds
			CLOCK_LED = ~CLOCK_LED;
			// Code to handle Digit index PB
			if (LinearKeyPad_getKeyState(DIGIT_INDEX_PB ) == TRUE) 
			{
				app.blinkIndex++;

				if(app.blinkIndex > (NO_OF_DIGITS -  1))
					app.blinkIndex = 0 ;

				DigitDisplay_blinkOn_ind(500, app.blinkIndex);

			}

			// Code to handle increment PB
			if (LinearKeyPad_getKeyState(INCREMENT_PB) == 1) 
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
			
			if (LinearKeyPad_getKeyState(MILDSTONE1_PB) == TRUE)
			{
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
				
				//Store state in the EEPROM
				Write_b_eep( EEPROM_STATE_ADDRESS , HALT_STATE);
				Busy_eep( );
				//Switch state
				app.state = HALT_STATE;	
				
			}
			if (LinearKeyPad_getKeyState(MILDSTONE2_PB) == TRUE)
			{
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
				
				//Store state in the EEPROM
				Write_b_eep( EEPROM_STATE_ADDRESS , HALT_STATE);
				Busy_eep( );
				//Switch state
				app.state = HALT_STATE;	
				
			}


				
			break;
			
			case COUNT_STATE:

	
				// On halt PB press change the state into HALT
			if ((LinearKeyPad_getKeyState(HALT_PB) == TRUE) && (app.countFlag == FALSE))
				{
					app.countFlag = TRUE;
				}

			if ((LinearKeyPad_getKeyState(HALT_PB) == FALSE) && (app.countFlag == TRUE))
				{
					app.countFlag = FALSE;				
					Write_b_eep( EEPROM_STATE_ADDRESS ,HALT_STATE);
					Busy_eep( );

					GREEN_LAMP = RESET;
					RED_LAMP = RESET;
	
					app.state = HALT_STATE;
					break;
				}


				//Read RTC data and store it in buffer
				ReadRtcTimeAndDate(readTimeDateBuffer);  

				//Convert RTC data in to ASCII
				// Separate the higher and lower nibble and store it into the display buffer 
				APP_conversion(); 

				//Store the current RTC data into EEPROM

				for(i = 0 ; i < 6  ; i++)
				{
					if( app.displayBuffer[i] !=	app.prevDisplayBuffer[i] )
					{
						Write_b_eep( (EEPROM_RTC_ADDRESS + i) , app.displayBuffer[i]);
						Busy_eep( );
					}
				}

				//manipulate the MSB_min to display upto 99m:59s 
				if(readTimeDateBuffer[2] > 0)
				{
					app.displayBuffer[3] += '6';
				}
				else 
				{
					app.displayBuffer[3] +=  '0';
				}
				//calculate and store the rtc value in the form of SEC
				app.rtcValue = (UINT16)(((	app.displayBuffer[3]- '0' )* 10 )+ ( app.displayBuffer[2] - '0') )* 60 +((( app.displayBuffer[1]- '0' )* 10 )+ ( app.displayBuffer[0] - '0'));
		

				//Update digit display buffer with the current data of RTC
				DigitDisplay_updateBufferPartial(app.displayBuffer , 0, 4);

				if( (app.rtcValue >= app.mildstone1Value ) && (app.rtcValue < app.mildstone2Value) )
				{
					GREEN_LAMP = SET;
				}
				else if (app.rtcValue >= app.mildstone2Value)
				{
					GREEN_LAMP = RESET;
					RED_LAMP = SET;					
				}


				if(readTimeDateBuffer[2] > 0)
				{
					app.displayBuffer[3] -= '6';
				}
				else 
				{
					app.displayBuffer[3] -=  '0';
				}


				for(	i = 0 ;	i < 6 ;	i++)
				{
					app.prevDisplayBuffer[i] = app.displayBuffer[i];
				}

				if(((readTimeDateBuffer[2] == 0x01) && (readTimeDateBuffer[1] == 0X39) &&
					(readTimeDateBuffer[0] == 0X59)) )
				{
			
					//Change the state
					Write_b_eep( EEPROM_STATE_ADDRESS , HALT_STATE);
					Busy_eep( );

			
					app.state = HALT_STATE;	

					return;
				}

#if defined (RTC_DATA_ON_UART)
				for(i = 0; i < 7; i++)			
				{
					txBuffer[i] = readTimeDateBuffer[i];  //store time and date 
				}
				
				COM_txBuffer(txBuffer, 7);
#endif	

				break;			
		}
			
}		


void APP_conversion(void)
{
			
	app.displayBuffer[0] = (readTimeDateBuffer[0] & 0X0F) + '0';        //Seconds LSB
	app.displayBuffer[1] = ((readTimeDateBuffer[0] & 0XF0) >> 4) + '0'; //Seconds MSB
	app.displayBuffer[2] = (readTimeDateBuffer[1] & 0X0F) + '0';        //Minute LSB
	app.displayBuffer[3] = ((readTimeDateBuffer[1] & 0XF0) >> 4) ; 		//Minute MSB
	app.displayBuffer[4] = (readTimeDateBuffer[2] & 0X0F) + '0';        //Minute LSB
	app.displayBuffer[5] = ((readTimeDateBuffer[2] & 0X30) >> 4) ; 		//Minute MSB

}


void APP_resetDisplayBuffer(void)
{
	int i ;
	for(i = 0; i < 6; i++)			//reset all digits
	{
		app.displayBuffer[i] = '0';
	}
}

	
void APP_resetBuffer(void)
{
	int i ;
	for(i = 0; i < NO_OF_DIGITS ; i++)			//reset all digits
	{
		app.mildstone1[i] = '0';
		app.mildstone2[i] = '0';
	}
}



void APP_updateRTC(void)
{
	writeTimeDateBuffer[0] = ((app.displayBuffer[1] - '0') << 4) | (app.displayBuffer[0] - '0'); //store seconds
	writeTimeDateBuffer[1] = ((app.displayBuffer[3] - '0') << 4) | (app.displayBuffer[2] - '0'); //store minutes
	writeTimeDateBuffer[2] = ((app.displayBuffer[5] - '0') << 4) | (app.displayBuffer[4] - '0'); //store minutes

	WriteRtcTimeAndDate(writeTimeDateBuffer);  //update RTC
}



	