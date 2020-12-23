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

#include <driver/rmt.h>
#include "neopixel.c"


#define	NEOPIXEL_PORT	22
#define	NR_LED		7
//#define	NR_LED		3
//#define	NEOPIXEL_WS2812
#define	NEOPIXEL_SK6812
#define	NEOPIXEL_RMT_CHANNEL    RMT_CHANNEL_2

static const char *TAG = "led strip";

static	void test_neopixel()
{
	pixel_settings_t px;
	uint32_t		pixels[NR_LED];
	int		i;
	int		rc;

	rc = neopixel_init(NEOPIXEL_PORT, NEOPIXEL_RMT_CHANNEL);
	ESP_LOGI(TAG, "neopixel_init rc = %d", rc);

	for	( i = 0 ; i < NR_LED ; i ++ )	{
		pixels[i] = 0;
	}
	px.pixels = (uint8_t *)pixels;
	px.pixel_count = NR_LED;
#ifdef	NEOPIXEL_WS2812
	strcpy(px.color_order, "GRB");
#else
	strcpy(px.color_order, "GRBW");
#endif

	memset(&px.timings, 0, sizeof(px.timings));
	px.timings.mark.level0 = 1;
	px.timings.space.level0 = 1;
	px.timings.mark.duration0 = 12;
#ifdef	NEOPIXEL_WS2812
	px.nbits = 24;
	px.timings.mark.duration1 = 14;
	px.timings.space.duration0 = 7;
	px.timings.space.duration1 = 16;
	px.timings.reset.duration0 = 600;
	px.timings.reset.duration1 = 600;
#endif
#ifdef	NEOPIXEL_SK6812
	px.nbits = 32;
	px.timings.mark.duration1 = 12;
	px.timings.space.duration0 = 6;
	px.timings.space.duration1 = 18;
	px.timings.reset.duration0 = 900;
	px.timings.reset.duration1 = 900;
#endif
	px.brightness = 0x80;
	for	( int j = 0 ; j < NR_LED ; j ++ )	{
		np_set_pixel_rgbw(&px, j , 0, 100, 0, 200);
	}

	np_show(&px, NEOPIXEL_RMT_CHANNEL);

    ESP_LOGI(TAG, "getting here (end) rc = %d", rc);

}

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

/* LED numbers below are for ESP-WROVER-KIT */
/* Red LED */
#define LEDC_IO_0 (0)
/* Green LED */
#define LEDC_IO_1 (2)
/* Blued LED */
#define LEDC_IO_2 (4)

#define PWM_DEPTH (1023)
#define PWM_TARGET_DUTY 8192

static hsp_t s_hsb_val;
static uint16_t s_brightness;
static bool s_on = false;


/**
 * @brief transform led_strip's "RGB" and other parameter
 */
static void led_strip_set_aim(uint32_t r, uint32_t g, uint32_t b, uint32_t cw, uint32_t ww, uint32_t period)
{
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
static bool led_strip_set_hsb2rgb(uint16_t h, uint16_t s, uint16_t v, rgb_t *rgb)
{
    bool res = true;
    uint16_t hi, F, P, Q, T;

    if (!rgb)
        return false;

    if (h > 360) return false;
    if (s > 100) return false;
    if (v > 100) return false;

    hi = (h / 60) % 6;
    F = 100 * h / 60 - 100 * hi;
    P = v * (100 - s) / 100;
    Q = v * (10000 - F * s) / 10000;
    T = v * (10000 - s * (100 - F)) / 10000;

    switch (hi) {
    case 0:
        rgb->r = v;
        rgb->g = T;
        rgb->b = P;
        break;
    case 1:
        rgb->r = Q;
        rgb->g = v;
        rgb->b = P;
        break;
    case 2:
        rgb->r = P;
        rgb->g = v;
        rgb->b = T;
        break;
    case 3:
        rgb->r = P;
        rgb->g = Q;
        rgb->b = v;
        break;
    case 4:
        rgb->r = T;
        rgb->g = P;
        rgb->b = v;
        break;
    case 5:
        rgb->r = v;
        rgb->g = P;
        rgb->b = Q;
        break;
    default:
        return false;
    }
    return res;
}

/**
 * @brief set the led_strip's "HSV"
 */
static bool led_strip_set_aim_hsv(uint16_t h, uint16_t s, uint16_t v)
{
    rgb_t rgb_tmp;
    bool ret = led_strip_set_hsb2rgb(h, s, v, &rgb_tmp);

    if (ret == false)
        return false;

    led_strip_set_aim(rgb_tmp.r * PWM_TARGET_DUTY / 100, rgb_tmp.g * PWM_TARGET_DUTY / 100,
            rgb_tmp.b * PWM_TARGET_DUTY / 100, (100 - s) * 5000 / 100, v * 2000 / 100, 1000);

    return true;
}

/**
 * @brief update the led_strip's state
 */
static void led_strip_update()
{
    led_strip_set_aim_hsv(s_hsb_val.h, s_hsb_val.s, s_hsb_val.b);
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
    test_neopixel();
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
int led_strip_set_on(bool value)
{
    ESP_LOGI(TAG, "led_strip_set_on : %s", value == true ? "true" : "false");

    if (value == true) {
        s_hsb_val.b = s_brightness;
        s_on = true;
    } else {
        s_brightness = s_hsb_val.b;
        s_hsb_val.b = 0;
        s_on = false;
    }
    led_strip_update();

    return 0;
}

/**
 * @brief set the saturation of the lowlevel led_strip
 */
int led_strip_set_saturation(float value)
{
    ESP_LOGI(TAG, "led_strip_set_saturation : %f", value);

    s_hsb_val.s = value;
    if (true == s_on)
        led_strip_update();

    return 0;
}

/**
 * @brief set the hue of the lowlevel led_strip
 */
int led_strip_set_hue(float value)
{
    ESP_LOGI(TAG, "led_strip_set_hue : %f", value);

    s_hsb_val.h = value;
    if (true == s_on)
        led_strip_update();

    return 0;
}

/**
 * @brief set the brightness of the lowlevel led_strip
 */
int led_strip_set_brightness(int value)
{
    ESP_LOGI(TAG, "led_strip_set_brightness : %d", value);

    s_hsb_val.b = value;
    s_brightness = s_hsb_val.b; 
    if (true == s_on)
        led_strip_update();

    return 0;
}
