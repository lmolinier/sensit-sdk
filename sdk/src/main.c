/*!******************************************************************
 * \file main.c
 * \brief Sens'it SDK template
 * \author Sens'it Team
 * \copyright Copyright (c) 2018 Sigfox, All Rights Reserved.
 *
 * This file is an empty main template.
 * You can use it as a basis to develop your own firmware.
 *******************************************************************/
/******* INCLUDES **************************************************/
#include "sensit_types.h"
#include "sensit_api.h"
#include "error.h"
#include "button.h"
#include "battery.h"
#include "radio_api.h"
#include "hts221.h"
#include "ltr329.h"
#include "fxos8700.h"
#include "serial.h"
#include "AT_parser.h"


/******* GLOBAL VARIABLES ******************************************/
u8 firmware_version[] = "TEMPLATE";

/*******************************************************************/
static void AT_report_error_code(error_t err)
{
    u8 code[4];
    u8 i = 0;

    SERIAL_put_data((u8*)"ERR=0x", 6);
    AT_itoa((int)err, (char*)code, 16);
    while (code[i] != '\0')
    {
        SERIAL_put_data(code+i, 1);
        i++;
    }
}

int main()
{
    error_t err;
    button_e btn;
    u16 battery_level;
    u16 counter=0;

    /* Start of initialization */

    /* Configure button */
    SENSIT_API_configure_button(INTERRUPT_BOTH_EGDE);

    /* Initialize Sens'it radio */
    err = RADIO_API_init();
    ERROR_parser(err);

    /* Initialize temperature & humidity sensor */
    err = HTS221_init();
    ERROR_parser(err);

    /* Initialize light sensor */
    err = LTR329_init();
    ERROR_parser(err);

    /* Initialize accelerometer */
    err = FXOS8700_init();
    ERROR_parser(err);

    /* Initialize serial line */
    SERIAL_init();

    /* Clear pending interrupt */
    pending_interrupt = 0;

    /* End of initialization */

    while (TRUE)
    {
        /* Execution loop */

        /* Check of battery level */
        BATTERY_handler(&battery_level);

        /* RTC alarm interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_RTC) == INTERRUPT_MASK_RTC)
        {
            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_RTC;
        }

        /* Button interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_BUTTON) == INTERRUPT_MASK_BUTTON)
        {
            /* RGB Led ON during count of button presses */
            SENSIT_API_set_rgb_led(RGB_WHITE);

            /* Count number of presses */
            btn = BUTTON_handler();

            /* RGB Led OFF */
            SENSIT_API_set_rgb_led(RGB_OFF);

            if (btn == BUTTON_FOUR_PRESSES)
            {
                /* Reset the device */
                SENSIT_API_reset();
            }

            if (btn == BUTTON_TWO_PRESSES) 
            {
                error_t err;
                u8 response[8] = {0,0,0,0,0,0,0,0};
                u8 data[2] = { counter & 0xFF, (counter >> 8) & 0xFF};

                u8 data_str[3];
                SERIAL_put_data((u8*)"UL=", 3);
                for ( int i = 1 ; i <= 2 ; i++)
                {
                    AT_itoa((int)data[2-i], (char*)data_str, 16);
                    SERIAL_put_data(data_str, 2);
                }
                SERIAL_put_data((u8*)"\r\n", 2);
                err = RADIO_API_send_message(RGB_RED, data, 2, TRUE, response);
                if (err != RADIO_ERR_NONE)
                {
                    AT_report_error_code(err);
                } else 
                {
                    u8 response_str[3];
                    SERIAL_put_data((u8*)"DL=", 3);
                    for ( int i = 1 ; i < 8 ; i++)
                    {
                        AT_itoa((int)response[8-i], (char*)response_str, 16);
                        SERIAL_put_data(response_str, 2);
                    }
                }
                SERIAL_put_data((u8*)"\r\n", 2);
                counter++;
            }

            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_BUTTON;
        }

        /* Reed switch interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_REED_SWITCH) == INTERRUPT_MASK_REED_SWITCH)
        {
            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_REED_SWITCH;
        }

        /* Accelerometer interrupt handler */
        if ((pending_interrupt & INTERRUPT_MASK_FXOS8700) == INTERRUPT_MASK_FXOS8700)
        {
            /* Clear interrupt */
            pending_interrupt &= ~INTERRUPT_MASK_FXOS8700;
        }

        /* Check if all interrupt have been clear */
        if (pending_interrupt == 0)
        {
            /* Wait for Interrupt */
            SENSIT_API_sleep(FALSE);
        }
    } /* End of while */
}

/*******************************************************************/
