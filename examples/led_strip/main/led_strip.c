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

/* HomeKit led_strip Example Hardware Implementation
*/

#include <stdio.h>

#include	<string.h>
#include	<esp_event.h>	//	for usleep



// #include "driver/ledc.h"
#include <esp_log.h>

static const char *TAG = "led strip";


// static	void test_neopixel()
// {
// 	rc = neopixel_init(NEOPIXEL_PORT, NEOPIXEL_RMT_CHANNEL);
// 	ESP_LOGI(TAG, "neopixel_init rc = %d", rc);

// 	for	( i = 0 ; i < NR_LED; i ++ )	{
// 		pixels[i] = 0;
// 	}
// 	px.pixels = (uint8_t *)pixels;
// 	px.pixel_count = NR_LED;
// #ifdef	NEOPIXEL_WS2812
// 	strcpy(px.color_order, "GRB");
// #else
// 	strcpy(px.color_order, "GRBW");
// #endif

// 	memset(&px.timings, 0, sizeof(px.timings));
// 	px.timings.mark.level0 = 1;
// 	px.timings.space.level0 = 1;
// 	px.timings.mark.duration0 = 12;
// #ifdef	NEOPIXEL_WS2812
// 	px.nbits = 24;
// 	px.timings.mark.duration1 = 14;
// 	px.timings.space.duration0 = 7;
// 	px.timings.space.duration1 = 16;
// 	px.timings.reset.duration0 = 600;
// 	px.timings.reset.duration1 = 600;
// #endif
// #ifdef	NEOPIXEL_SK6812
// 	px.nbits = 32;
// 	px.timings.mark.duration1 = 12;
// 	px.timings.space.duration0 = 6;
// 	px.timings.space.duration1 = 18;
// 	px.timings.reset.duration0 = 900;
// 	px.timings.reset.duration1 = 900;
// #endif
// 	px.brightness = 0x80;
// 	for	( i = 1 ; i < NR_LED - 1 ; i ++ )	{
// 		np_set_pixel_rgbw(&px, i , 100, 0, 0, 50);
// 	}

// 	np_show(&px, NEOPIXEL_RMT_CHANNEL);

//     ESP_LOGI(TAG, "getting here (end) rc = %d", rc);

// }

typedef struct rgb {
    uint8_t r;  // 0-100 %
    uint8_t g;  // 0-100 %
    uint8_t b;  // 0-100 %
} rgb_t;


typedef struct hsp {
    uint16_t h;  // 0-360
    uint16_t s;  // 0-100
    uint16_t b;  // 0-100
} hsp_t;

static hsp_t s_hsb_val;
static uint16_t s_brightness;
static bool s_on = false;


/**
 * @brief transform led_strip's "RGB" and other parameter
 */
static void led_strip_set_aim(int num, uint32_t h, uint32_t s, uint32_t v)
{   
    // segment_hue = h;
    // segment_saturation = s;
    // segment_intensity = v;
    // leds_changed = true;
    // int start = 1;
    // int end = NR_LED;
    // if(num == 0){
    //     start = 0;
    //     end = 1;
    // }
    // int rgbw[4];
    // hsi2rgbw(h, s, v, &rgbw);
    // for	( int j = start ; j < end ; j ++ )	{
	// 	np_set_pixel_rgbw(&px, j , rgbw[0], rgbw[1], rgbw[2], rgbw[3]);
	// }

	// np_show(&px, NEOPIXEL_RMT_CHANNEL);
    // ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, r);
    // ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, g);
    // ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, b);
    // ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    // ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    // ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
    
}

/**
 * @brief transform led_strip's "HSV" to "RGB"
 */
static bool led_strip_set_hsb2rgb(uint16_t h, uint16_t s, uint16_t v)
{
    if (h > 360) return false;
    if (s > 100) return false;
    if (v > 100) return false;

    return true;
}

/**
 * @brief set the led_strip's "HSV"
 */
static bool led_strip_set_aim_hsv(int num, uint16_t h, uint16_t s, uint16_t v)
{
    bool ret = led_strip_set_hsb2rgb(h, s, v);

    if (ret == false)
        return false;

    led_strip_set_aim(num, h, s, v);

    return true;
}

/**
 * @brief update the led_strip's state
 */
static void led_strip_update(int num)
{
    led_strip_set_aim_hsv(num, s_hsb_val.h, s_hsb_val.s, s_hsb_val.b);
}



/**
 * @brief initialize the led_strip lowlevel module
 */
void led_strip_init(void)
{
    // enable ledc module
    // periph_module_enable(PERIPH_LEDC_MODULE);

    // config the timer
    // ledc_timer_config_t ledc_timer = {
    //     //set frequency of pwm
    //     .freq_hz = 5000,
    //     //timer mode,
    //     .speed_mode = LEDC_HIGH_SPEED_MODE,
    //     //timer index
    //     .timer_num = LEDC_TIMER_0
    // };
    // // ledc_timer_config(&ledc_timer);
    
    // //config the channel
    // ledc_channel_config_t ledc_channel = {
    //     //set LEDC channel 0
    //     .channel = LEDC_CHANNEL_0,
    //     //set the duty for initialization.(duty range is 0 ~ ((2**bit_num)-1)
    //     .duty = 100,
    //     //GPIO number
    //     .gpio_num = LEDC_IO_0,
    //     //GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
    //     .intr_type = LEDC_INTR_FADE_END,
    //     //set LEDC mode, from ledc_mode_t
    //     .speed_mode = LEDC_HIGH_SPEED_MODE,
    //     //set LEDC timer source, if different channel use one timer,
    //     //the frequency and bit_num of these channels should be the same
    //     .timer_sel = LEDC_TIMER_0
    // };
    // //set the configuration
    // ledc_channel_config(&ledc_channel);

    // //config ledc channel1
    // ledc_channel.channel = LEDC_CHANNEL_1;
    // ledc_channel.gpio_num = LEDC_IO_1;
    // ledc_channel_config(&ledc_channel);
    // //config ledc channel2
    // ledc_channel.channel = LEDC_CHANNEL_2;
    // ledc_channel.gpio_num = LEDC_IO_2;
    // ledc_channel_config(&ledc_channel);
    // test_neopixel();
}

/**
 * @brief deinitialize the led_strip's lowlevel module
 */
void led_strip_deinit(void)
{
    // ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    // ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    // ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, 0);
}

/**
 * @brief turn on/off the lowlevel led_strip
 */
int led_strip_set_on(int num, bool value, bool *segment_on, bool *leds_changed, uint32_t *segment_intensity, uint32_t *segment_intensity_off)
{
    ESP_LOGI(TAG, "led_strip_set_on num : %d", num);
    ESP_LOGI(TAG, "led_strip_set_on : %s", value == true ? "true" : "false");

    if(*segment_on == true && value == false){
        *segment_intensity_off = *segment_intensity;
        *segment_intensity = 0;
        *segment_on = false;
        *leds_changed = true;
    }
    if(*segment_on == false && value == true){
        *segment_intensity = *segment_intensity_off;
        *segment_on = true;
        *leds_changed = true;
    }

    return 0;
}

/**
 * @brief set the saturation of the lowlevel led_strip
 */
int led_strip_set_saturation(int num, float value, bool *segment_on, bool *leds_changed, uint32_t *segment_saturation)
{
    ESP_LOGI(TAG, "led_strip_set_saturation : %f", value);
    *segment_saturation = value / 100;
    
    if (*segment_on == true)
    {
        *leds_changed = true;
    }
    // s_hsb_val.s = value;
    // if (true == s_on)
    //     led_strip_update(num);

    return 0;
}

/**
 * @brief set the hue of the lowlevel led_strip
 */
int led_strip_set_hue(int num, float value, bool *segment_on, bool *leds_changed, uint32_t *segment_hue)
{
    ESP_LOGI(TAG, "led_strip_set_hue : %f", value);
    *segment_hue = value;
    
    if (*segment_on == true)
    {
        *leds_changed = true;
    }
    // s_hsb_val.h = value;
    // if (true == s_on)
    //     led_strip_update(num);

    return 0;
}

/**
 * @brief set the brightness of the lowlevel led_strip
 */
int led_strip_set_brightness(int num, int value, bool *segment_on, bool *leds_changed, uint32_t *segment_intensity)
{
    ESP_LOGI(TAG, "led_strip_set_brightness : %d", value);
    *segment_intensity = value / 100;
    
    if (*segment_on == true)
    {
        *leds_changed = true;
    }

    // s_hsb_val.b = value;
    // s_brightness = s_hsb_val.b; 
    // if (true == s_on)
        // led_strip_update(num);

    return 0;
}
