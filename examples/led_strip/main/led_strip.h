/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* HomeKit LED_STRIP Example
*/

#ifndef _LED_STRIP_H_
#define _LED_STRIP_H_

/**
 * @brief initialize the LED_STRIP lowlevel module
 *
 * @param none
 *
 * @return none
 */
void test_neopixel(void);

/**
 * @brief initialize the LED_STRIP lowlevel module
 *
 * @param none
 *
 * @return none
 */
void led_strip_init(void);

/**
 * @brief deinitialize the LED_STRIP's lowlevel module
 *
 * @param none
 *
 * @return none
 */
void led_strip_deinit(void);

/**
 * @brief turn on/off the lowlevel LED_STRIP
 *
 * @param value The "On" value
 *
 * @return none
 */
int led_strip_set_on(int num, bool value);

/**
 * @brief set the saturation of the lowlevel LED_STRIP
 *
 * @param value The Saturation value
 *
 * @return 
 *     - 0 : OK
 *     - others : fail
 */
int led_strip_set_saturation(int num, float value);

/**
 * @brief set the hue of the lowlevel LED_STRIP
 *
 * @param value The Hue value
 *
 * @return 
 *     - 0 : OK
 *     - others : fail
 */
int led_strip_set_hue(int num, float value);

/**
 * @brief set the brightness of the lowlevel LED_STRIP
 *
 * @param value The Brightness value
 *
 * @return 
 *     - 0 : OK
 *     - others : fail
 */
int led_strip_set_brightness(int num, int value);

#endif /* _LED_STRIP_H_ */
