//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************


//*****************************************************************************
//
// Application Name     -   SSL Demo
// Application Overview -   This is a sample application demonstrating the
//                          use of secure sockets on a CC3200 device.The
//                          application connects to an AP and
//                          tries to establish a secure connection to the
//                          Google server.
// Application Details  -
// docs\examples\CC32xx_SSL_Demo_Application.pdf
// or
// http://processors.wiki.ti.com/index.php/CC32xx_SSL_Demo_Application
//
//*****************************************************************************


//*****************************************************************************
//
//! \addtogroup ssl
//! @{
//
//*****************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

#include "hw_nvic.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_apps_rcm.h"
#include "systick.h"
#include "gpio.h"
#include "math.h"

//Common interface includes
//#include "pinmux.h"
#include "pin_mux_config.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"
#include "i2c_if.h"

// Custom includes
#include "utils/network_utils.h"
#include "inttypes.h"

#include "glcdfont.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "oled_test.h"
#include "spi.h"

#include "hw_adc.h"
#include "pin.h"
#include "adc_userinput.h"
#include "hw_gprcm.h"
#include "adc.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ONE 0b0101110110110101
#define TWO 0b0101110110101110
#define THREE 0b0101110110101101
#define FOUR 0b0101110101110110
#define FIVE 0b0101110101110101
#define SIX 0b0101110101101110
#define SEVEN 0b0101110101101101
#define EIGHT 0b0101101110110110
#define NINE 0b0101101110110101
#define ZERO 0b0101110110110110
#define MUTE 0b0110101110101101
#define LAST 0b0101101110101101

//UART
#define CONSOLE              UARTA0_BASE
#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define MAX_STRING_LENGTH    80

//OLED
#define MASTER_MODE      1

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100
#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"

#define BUTTON_PRESS_DELAY 80000000


//IR
volatile int count = 0;
volatile int Check_count = 0;
volatile unsigned long PulseWidth = 0;
volatile unsigned long PulseWidth_Check = 0;
volatile int FullData = 0;
unsigned long buffer[18];
unsigned long ulStatus;
unsigned long irData;
int start = 0;

//UART
volatile int g_iCounter = 0;
char cString[MAX_STRING_LENGTH+1];
char cCharacter;
int iStringLength = 0;
unsigned long UARTulStatus;


//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                18    /* Current Date */
#define MONTH               5     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                02    /* Time - hours */
#define MINUTE              50    /* Time - minutes */
#define SECOND              30     /* Time - seconds */


#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "a35f0bp0zgf135-ats.iot.us-west-1.amazonaws.com" // CHANGE ME
#define GOOGLE_DST_PORT       8443


#define POSTHEADER "POST /things/Donggeon_CC3200_Board/shadow HTTP/1.1\r\n"             // CHANGE ME
#define HOSTHEADER "Host: a35f0bp0zgf135-ats.iot.us-west-1.amazonaws.com\r\n"  // CHANGE ME
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"var\" :\""                                           \
                        "Hello phone, "                                     \
                        "message from Mark's CC3200 via AWS IoT at 1:17!"                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"




#define GETHEADER "GET /things/Donggeon_CC3200_Board/shadow HTTP/1.1\r\n"


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// some helpful macros for systick

// the cc3200's fixed clock frequency of 80 MHz
// note the use of ULL to indicate an unsigned long long constant
#define SYSCLKFREQ 80000000ULL

// macro to convert ticks to microseconds
#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

// macro to convert microseconds to ticks
#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

// systick reload value set to 40ms period
// (PERIOD_SEC) * (SYSCLKFREQ) = PERIOD_TICKS
#define SYSTICK_RELOAD_VAL 3200000UL

// track systick counter periods elapsed
// if it is not 0, we know the transmission ended
volatile int systick_cnt = 0;

extern void (* const g_pfnVectors[])(void);
//********************OLED*********************************************************
static unsigned char g_ucTxBuff[TR_BUFF_SIZE];

// Global variables to track the state of text input
volatile unsigned long last_button_time = 0;
volatile char last_button = '0';
volatile int button_press_count = 0;
char text_buffer[256] = {0};
int text_cursor = 0;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// an example of how you can use structs to organize your pin settings for easier maintenance
typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;

static const PinSetting IR_Module = { .port = GPIOA0_BASE, .pin = 0x1};

/////////////////////////////ADC Setup//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long uiAdcInputPin = PIN_60;
unsigned long uiAdcInputPin1 = PIN_59;
unsigned int uiChannel;
unsigned long ulSample;
float sound;

//*****************************************************************************
//                 GLOBAL VARIABLES -- End: df
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static int set_time();
static void BoardInit(void);
static int http_post(int iTLSSockID, const char *str);

unsigned char Convert2Char(unsigned long irData);
void ProcessTextInput(const char* text_buffer);
void PrintData(unsigned long irData);
unsigned long Gather_All(unsigned long* buffer);
int System_Start(int start, const char* text_buffer);
void Start_notification(void);
void Full_Char(void);
void Print_youreNotHome(void);
void Send_notification(void);
void End_notification(void);
void Print_youreHome(void);
void AWS_Connection(void);
static int http_get(int iTLSSockID);
void Print_BabyCrying(void);
//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
void MasterMain(void)
{
    // Initialize the message
    memcpy(g_ucTxBuff,MASTER_MSG,sizeof(MASTER_MSG));

    // Reset SPI
    MAP_SPIReset(GSPI_BASE);

    // Configure SPI interface
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    // Enable SPI for communication
    MAP_SPIEnable(GSPI_BASE);
}

static void BoardInit(void) {
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////ADC functions/////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void ADC_Setup(void){
#ifdef CC3200_ES_1_2_1

    HWREG(GPRCM_BASE + GPRCM_O_ADC_CLK_CONFIG) = 0x00000043;
    HWREG(ADC_BASE + ADC_O_ADC_CTRL) = 0x00000004;
    HWREG(ADC_BASE + ADC_O_ADC_SPARE0) = 0x00000100;
    HWREG(ADC_BASE + ADC_O_ADC_SPARE1) = 0x0355AA00;
#endif

    MAP_PinTypeADC(uiAdcInputPin, PIN_MODE_255);

    switch (uiAdcInputPin)
    {
    case PIN_58:
        uiChannel = ADC_CH_1;
        break;
    case PIN_59:
        uiChannel = ADC_CH_2;
        break;
    case PIN_60:
        uiChannel = ADC_CH_3;
        break;
    default:
        uiChannel = ADC_CH_3;
        break;
    }

    //
    // Configure ADC timer which is used to timestamp the ADC data samples
    //
    MAP_ADCTimerConfig(ADC_BASE, 2 ^ 17);

    //
    // Enable ADC timer which is used to timestamp the ADC data samples
    //
    MAP_ADCTimerEnable(ADC_BASE);

    //
    // Enable ADC module
    //
    MAP_ADCEnable(ADC_BASE);

    //
    // Enable ADC channel
    //
    MAP_ADCChannelEnable(ADC_BASE, uiChannel);
}
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////IR moudle functions/////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
/**
 * Reset SysTick Counter
 */
static inline void SysTickReset(void) {
    // any write to the ST_CURRENT register clears it
    // after clearing it automatically gets reset without
    // triggering exception logic
    // see reference manual section 3.2.1
    HWREG(NVIC_ST_CURRENT) = 1;

    // clear the global count variable
    systick_cnt = 0;
}
/**
 * SysTick Interrupt Handler
 *
 * Keep track of whether the systick counter wrapped
 */
static void SysTickHandler(void) {
    // increment every time the systick handler fires
    systick_cnt++;
}

/**
 * Initializes SysTick Module
 */
static void SysTickInit(void) {

    // configure the reset value for the systick countdown register
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);

    // register interrupts on the systick module
    MAP_SysTickIntRegister(SysTickHandler);

    // enable interrupts on systick
    // (trigger SysTickHandler when countdown reaches 0)
    MAP_SysTickIntEnable();

    // enable the systick module itself
    MAP_SysTickEnable();
}

static void GPIOIntHandler(void) {
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus(GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);
    count++;

    PulseWidth = (SYSTICK_RELOAD_VAL - SysTickValueGet()) >>18 ;
    buffer[count] = PulseWidth;

    if(count == 17) {
        Check_count = 1;
        count = 0;
    }
    SysTickReset();
}

// Key mappings
const char *key_map[] = {
    " ",    // 0
    "",     // 1 - is now finalize character button
    "ABC",  // 2
    "DEF",  // 3
    "GHI",  // 4
    "JKL",  // 5
    "MNO",  // 6
    "PQRS", // 7
    "TUV",  // 8
    "WXYZ"  // 9
};
void displayFullText() {
    fillRect(0,80, SSD1351WIDTH, 8 , BLACK);
    setCursor(0, 80);
    Outstr("TX:");
    setCursor(18,80);
    Outstr(text_buffer);
}
int finalizeCharacter(int start) {
    if (last_button != '0') {
        text_buffer[text_cursor++] = key_map[last_button - '0'][button_press_count % strlen(key_map[last_button - '0'])];
        text_buffer[text_cursor] = '\0';  // Null terminate string
        last_button = '0';  // Reset last button
        displayFullText();
    }
    start = System_Start(start,text_buffer);

    return start;
}
void displayIntermediateCharacter(char character) {
    int x = 6 * (text_cursor + 3); // Assuming character width of 6 pixels
    int y = 80;
    fillRect(x, y, 6, 8, BLACK); // Clear the space where the character will be displayed
    drawChar(x, y, character, WHITE, BLACK, 1);
}

int System_Start(int start, const char* text_buffer) {
    if (strcmp(text_buffer, "ST") == 0) {
        if(start == 0){
            Message("START!!!!\n\r");
            Start_notification();
            Print_youreNotHome();
        }
        start = 1;
        return start;
    }
    else if ((strcmp(text_buffer, "END") == 0)){
        if(start == 1){
            Message("END!!!!\n\r");
            End_notification();
            Print_youreHome();
        }
        start = 0;
        return start;
    }
    else{
    return start;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////

//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

//*****************************************************************************
//
//! Main
//!
//! \param  none
//!
//! \return None
//!
//*****************************************************************************
/* Right now the button press delay is working
*
* implemented the 1 button to finalize a character when you want two characters that are
* on the same button
*
* Last is where the UART communication will go
*
*/
void PrintToOLED(unsigned long irData) {
   unsigned long current_time = SysTickValueGet();  // Get current tick count
   unsigned long elapsed_time = last_button_time == 0 ? BUTTON_PRESS_DELAY + 1 : TICKS_TO_US(current_time - last_button_time);

   // Handle character input from number keys
   if (irData == TWO || irData == THREE || irData == FOUR|| irData == FIVE || irData == SIX
           || irData == SEVEN || irData == EIGHT || irData == NINE) {
       char key = Convert2Char(irData);
       if (key == last_button) {
           if (elapsed_time < BUTTON_PRESS_DELAY) {
               // Loop through characters on the same key press
               button_press_count++;
           } else {
               // Time delay passed, finalize the last character and start new
               start = finalizeCharacter(start);
               button_press_count = 0;  // Start from first character of the button
           }
       } else {
           // Different key pressed, finalize previous character if any
           start = finalizeCharacter(start);
           last_button = key;
           button_press_count = 0;
       }
       last_button_time = current_time;
       // Display the current text buffer on OLED
       displayIntermediateCharacter(key_map[last_button - '0'][button_press_count % strlen(key_map[last_button - '0'])]);
    //One is now enter key
     }else if(irData == ONE){
       start = finalizeCharacter(start);
     } else if (irData == ZERO) {
       start = finalizeCharacter(start);
       // Space
       text_buffer[text_cursor++] = ' ';
       start = finalizeCharacter(start);
     } else if (irData == MUTE) {
       // Backspace
       start = finalizeCharacter(start);
       if (text_cursor > 0) text_buffer[--text_cursor] = '\0';
       start = finalizeCharacter(start);
     } else if (irData == LAST) {
  //   ProcessTextInput(text_buffer);
     AWS_Connection();
       text_cursor = 0;
       text_buffer[0] = '\0';
       start = finalizeCharacter(start);

     } else {
         Message("Error: Try again\n\r");
     }
}

void ProcessTextInput(const char* text_buffer) {
    int i = 0;
    char *str = text_buffer;
    printf("%s\n", str);
    while (text_buffer[i] != '\0') {
    //    MAP_UARTCharPut(UARTA1_BASE, text_buffer[i]);
        i++;
    }
}

void PrintData(unsigned long irData){

    if(irData == ONE) {
        Report("1\n\r");

    } else if(irData == TWO) {
        Message("2\n\r");

    }else if(irData == THREE) {
        Message("3\n\r");

    }else if(irData == FOUR) {
        Message("4\n\r");

    }else if(irData == FIVE) {
        Message("5\n\r");

    }else if(irData == SIX) {
        Message("6\n\r");

    }else if(irData == SEVEN) {
        Message("7\n\r");

    }else if(irData == EIGHT) {
        Message("8\n\r");

    }else if(irData == NINE) {
        Message("9\n\r");

    }else if(irData == ZERO) {
        Message("0\n\r");

    }else if(irData == MUTE) {
        Message("MUTE\n\r");

    } else if(irData == LAST) {
        Message("LAST\n\r");

    } else {
        Message("Error: Try again\n\r");

    }
}

unsigned long Gather_All(unsigned long* buffer) {
    unsigned long value = 0;
    int i;
    for(i = 2; i < 18; i++) {
        value += buffer[i] << (17 - i);
    }
    return value;
}

unsigned char Convert2Char(unsigned long irData){
    unsigned char val = '0';
    if(irData == ONE) {
        val = '1';
    } else if(irData == TWO) {
        val = '2';
    }else if(irData == THREE) {
        val = '3';
    }else if(irData == FOUR) {
        val = '4';
    }else if(irData == FIVE) {
        val = '5';
    }else if(irData == SIX) {
        val = '6';
    }else if(irData == SEVEN) {
        val = '7';
    }else if(irData == EIGHT) {
        val = '8';
    }else if(irData == NINE) {
        val = '9';
    }else if(irData == ZERO) {
        val = ' ';
    } else {
        Message("Error: Try again\n\r");
    }
    return val;
}
void AWS_Connection(void){
    long lRetVal = -1;
    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }
    http_post(lRetVal, text_buffer);

    http_get(lRetVal);

    sl_Stop(SL_STOP_TIMEOUT);
}

void Send_notification(void){
    long lRetVal = -1;
    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }
    http_post(lRetVal, "Sound is too loud! you're baby is crying!!!");

    sl_Stop(SL_STOP_TIMEOUT);
}

void Start_notification(void){
    long lRetVal = -1;
    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }
    http_post(lRetVal, "System started it, you will get notification if baby is crying!!!");

    sl_Stop(SL_STOP_TIMEOUT);
}

void End_notification(void){
    long lRetVal = -1;
    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }
    http_post(lRetVal, "Now you're home, can't get notification, so watch your baby!!!");

    sl_Stop(SL_STOP_TIMEOUT);
}



 void main() {

    //
    // Initialize board configuration
    //
    BoardInit();

    PinMuxConfig();

    // Enable SysTick
    SysTickInit();

    InitTerm();
    ClearTerm();
    //IR GPIO Interrupt
    MAP_GPIOIntRegister(IR_Module.port, GPIOIntHandler);
    MAP_GPIOIntTypeSet(IR_Module.port, IR_Module.pin, GPIO_FALLING_EDGE);
    ulStatus = MAP_GPIOIntStatus(IR_Module.port, false);
    MAP_GPIOIntClear(IR_Module.port, ulStatus);
    MAP_GPIOIntEnable(IR_Module.port, IR_Module.pin);

    //SPI
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralReset(PRCM_GSPI);
    MasterMain();

    //ADC (Microphone)
    ADC_Setup();

    UART_PRINT("My terminal works!\n\r");

    Adafruit_Init();
    fillScreen(BLACK);
    //setCursor(0,35);
    //Outstr("RX:");
    setCursor(0,80);
    Outstr("TX:");
    setCursor(3,0);

    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    Print_youreHome();
    //Print_BabyCrying();
    while (1) {

        if (Check_count == 1) {
            FullData = Gather_All(buffer);
            PrintData(FullData);
            PrintToOLED(FullData);
            Check_count = 0;
        }
        PulseWidth_Check = (SYSTICK_RELOAD_VAL - SysTickValueGet()) >> 13;
        if(count != 17 && PulseWidth_Check > 300){
            count = 0;
            PulseWidth_Check = 0;
            }

      if(start == 1){
        if (MAP_ADCFIFOLvlGet(ADC_BASE, uiChannel))
        {
            ulSample = MAP_ADCFIFORead(ADC_BASE, uiChannel);
            float voltage = (((float)((ulSample >> 2) & 0x0FFF)) * 1.4) / 4096;

            if(voltage >= 1.399658){
            UART_PRINT("Sound is too loud!!! something happended to your baby!!! %f\n\r", voltage);
            Send_notification();
            Print_BabyCrying();
            }
        }

      }
    }
    MAP_ADCChannelDisable(ADC_BASE, uiChannel);


}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//
//*****************************************************************************

static int http_post(int iTLSSockID, const char *text_buffer) {
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char jsonData[512];
    char *pcBufHeaders;
    int lRetVal = 0;

    // Change to JSON format
    snprintf(jsonData, sizeof(jsonData), "{"
             "\"state\": {\r\n"
             "\"desired\" : {\r\n"
             "\"var\" :\"%s\"\r\n"
             "}\r\n"
             "}\r\n"
             "}\r\n\r\n", text_buffer);

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);
    pcBufHeaders += strlen(CLHEADER1);

    // Calculate string length
    int dataLength = strlen(jsonData);
    sprintf(cCLLength, "%d", dataLength);
    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);
    strcpy(pcBufHeaders, jsonData);
    pcBufHeaders += strlen(jsonData);

    UART_PRINT(acSendBuff);

    // Send the POST request to the server
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if (lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r", lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    // Receive the response from the server
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if (lRetVal < 0) {
        UART_PRINT("Receive failed. Error Number: %i\n\r", lRetVal);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    } else {
        acRecvbuff[lRetVal] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    UART_PRINT("POSTED!!\r\n\r\n");
    return 0;
}

//*****************************************************************************
//
//! This function sends a GET request to the server.
//!
//! \param iTLSSockID - TLS socket ID
//!
//! \return 0 on success, negative error code on failure
//!
//*****************************************************************************
// HTTP GET request function
static int http_get(int iTLSSockID) {
    UART_PRINT("\r\n\r\n");
    UART_PRINT("NOW GETTING!!!\r\n\r\n");
    char acSendBuff[512];
    char acRecvbuff[1460];
    char* pcBufHeaders;
    int lRetVal = 0;

    // Construct GET header
    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);  // CHANGE ME
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    UART_PRINT(acSendBuff);

    // Send the GET request to the server
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if (lRetVal < 0) {
        UART_PRINT("GET request failed. Error Number: %i\n\r", lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    // Receive the response from the server
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if (lRetVal < 0) {
        UART_PRINT("Receive failed. Error Number: %i\n\r", lRetVal);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    } else {
        acRecvbuff[lRetVal] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////OLED Functions//////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Full_Char(void){

        int startX = 0; // Starting X coordinate for printing
        int startY = 0; // Starting Y coordinate for printing
        int charWidth = 6; // Width of each character
        int charHeight = 8; // Height of each character
        int spacing = 1; // Spacing between characters
        fillScreen(BLACK);
      //  unsigned char c = font[c];
           int c;
        for ( c = 0; c < 1275; c++) { // Loop through ASCII characters
            drawChar(startX, startY, font[c], WHITE, BLACK, 1); // Draw the character
            startX += charWidth + spacing; // Move the X coordinate for next character
            if ((startX + charWidth) >= WIDTH) { // If the next character exceeds the screen width
                startX = 0; // Move to the start of the next row
                startY += charHeight + spacing; // Move to the next row
                if ((startY + charHeight) >= HEIGHT) // If the next row exceeds the screen height
                    break; // Break the loop (screen is filled)
            }
        }


}

void Hellow_Word(void){
    fillScreen(BLACK);
        int cursor_x = 100;
        int cursor_y = 200;
        unsigned int textcolor = WHITE;
        unsigned int textbgcolor = BLACK;
     //   unsigned int textsize = 10000;

        setCursor(30,  50);
        setTextSize(1);

        Outstr ("Hello World!");


        delay(100);

}

void Print_BabyCrying(void){
    halfScreen(BLACK);
        int cursor_x = 100;
        int cursor_y = 200;
        unsigned int textcolor = WHITE;
        unsigned int textbgcolor = BLACK;
     //   unsigned int textsize = 10000;

        setCursor(15,  35);
        setTextSize(1);

        Outstr ("Baby is crying!!!");

        delay(100);

}

void Print_youreHome(void){
    halfScreen(BLACK);
        int cursor_x = 100;
        int cursor_y = 200;
        unsigned int textcolor = WHITE;
        unsigned int textbgcolor = BLACK;
     //   unsigned int textsize = 10000;

        setCursor(12,  35);
        setTextSize(1);

        Outstr ("You are home now");

        delay(100);

}
void Print_youreNotHome(void){
    halfScreen(BLACK);
        int cursor_x = 100;
        int cursor_y = 200;
        unsigned int textcolor = WHITE;
        unsigned int textbgcolor = BLACK;
     //   unsigned int textsize = 10000;

        setCursor(20,  35);
        setTextSize(1);

        Outstr ("You left home");

        delay(100);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
