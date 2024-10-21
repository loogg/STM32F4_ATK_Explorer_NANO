/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "sampleapp_common.h"
#include <board.h>

void app_set_led (uint16_t id, bool led_state)
{
   if (id == APP_DATA_LED_ID)
   {
      HAL_GPIO_WritePin (LED0_GPIO_Port, LED0_Pin, led_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
   }
   else if (id == APP_PROFINET_SIGNAL_LED_ID)
   {
      HAL_GPIO_WritePin (LED1_GPIO_Port, LED1_Pin, led_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
   }
}

bool app_get_button (uint16_t id)
{
   return false;
}
