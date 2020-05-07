/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

/* WDT operating mode->WDT Disabled */
#pragma config WDTE = OFF
/* Low voltage programming enabled, RE3 pin is MCLR */
#pragma config LVP = ON

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#define _XTAL_FREQ                   64000000UL

/* RE2 button pin */
#define Button_Read()                (PORTEbits.RE2)
/* Button is active in low-logic level  */
#define BUTTON_PRESSED                false
/* Time is expressed in ms */
#define BUTTON_DEBOUNCING_TIME        10

#define TIMER_PRESCALER               2 
#define FREQUENCY_TO_PR_CONVERT(F)    (uint8_t)(((_XTAL_FREQ)/(4*(F))/(TIMER_PRESCALER))-1)


/* duty cycle is always 1 microsecond */
/* but the period (frequency) is variable, as in the list */
/* where the frequencies are expressed in Hz: */
const uint8_t frequencies_list[] = {                 
    FREQUENCY_TO_PR_CONVERT( 50000UL),
    FREQUENCY_TO_PR_CONVERT(100000UL),
    FREQUENCY_TO_PR_CONVERT(200000UL),
    FREQUENCY_TO_PR_CONVERT(400000UL),
    FREQUENCY_TO_PR_CONVERT(800000UL)
};
#define FREQUENCY_LIST_DIMENSION      (sizeof(frequencies_list)/sizeof(frequencies_list[0]))

typedef enum{
    BT_NOCHANGE,
    BT_PRESS
} button_t;

static button_t ButtonCheck(void);
static void     PWM2_Initialize(void);
static void     PORT_Initialize(void);
static void     PPS_Initialize(void);
static void     CLK_Initialize(void);
static void     TMR4_Initialize(void);
static void     TMR4_LoadPeriodRegister(uint8_t periodVal);

static button_t ButtonCheck(void)
{
    button_t result = BT_NOCHANGE;
    static bool old_button_state = !BUTTON_PRESSED;
    bool button_state = Button_Read();
    /* detecting only the button-press event */
    if( (button_state == BUTTON_PRESSED) && (old_button_state != BUTTON_PRESSED) )
    {
        /*  wait for debouncing time */
        __delay_ms(BUTTON_DEBOUNCING_TIME);
        while(Button_Read() == BUTTON_PRESSED)
        {
            /*  then check again */
            /*  and waits in the loop until the button is released */
            result = BT_PRESS;
        }
    }
    old_button_state = button_state;
    return result;
}


static void PWM2_Initialize(void)
{
	/* MODE PWM; EN enabled; FMT right_aligned */
    CCP2CONbits.MODE = 0x0C;
    CCP2CONbits.FMT = 0;
    CCP2CONbits.EN = 1;

    /* Constant on-time setting is 1 us */
    CCPR2 = 32;
    
    /* Select timer 4 */
    CCPTMRSbits.C2TSEL = 2;
}

static void PORT_Initialize(void)
{
    /* RC7 is output for PWM2 */
    TRISCbits.TRISC7 = 0;
    /* RE2 is digital input with pull-up for push-button */
    TRISEbits.TRISE2 = 1;
    ANSELEbits.ANSELE2 = 0;
    WPUEbits.WPUE2 = 1;
}   
    
static void PPS_Initialize(void)
{
    /* Configure RC7 for PWM2 output */
    RC7PPS = 0x06;
}

static void CLK_Initialize(void)
{
    /* Configure NOSC HFINTOSC; NDIV 1; FOSC = 64MHz */
    OSCCON1bits.NOSC = 6;
    OSCCON1bits.NDIV = 0;

    /* HFFRQ 64_MHz */
    OSCFRQbits.HFFRQ = 8;
}

static void TMR4_Initialize(void)
{
    /* TIMER4 clock source is FOSC/4 */
    T4CLKCONbits.CS = 1;

    /* TIMER4 counter reset */
    T4TMR = 0x00;

    /* TIMER4 ON, prescaler 1:2, postscaler 1:1 */
    T4CONbits.OUTPS = 0;
    T4CONbits.CKPS = 1;
    T4CONbits.ON = 1;

    /* Configure initial period register value */
    PR4 = frequencies_list[0];
}

static void TMR4_LoadPeriodRegister(uint8_t periodVal)
{
    /* Configure the period register */
    PR4 = periodVal;
}

void main(void)
{
    uint8_t index = 0;
    /*  Initialize the device */
    PORT_Initialize();
    PPS_Initialize();
    CLK_Initialize();
    TMR4_Initialize();
    PWM2_Initialize();

    while (1)
    {
        if(ButtonCheck() == BT_PRESS)
        {
            /* When a button press is detected, the index is updated */
            index++;
            if(index >= FREQUENCY_LIST_DIMENSION)
                index = 0;

            /* and the frequency is changed to the next one in the list */
            TMR4_LoadPeriodRegister(frequencies_list[index]);
        }        
    }
}

