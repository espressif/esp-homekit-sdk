/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#include <stdio.h>
#include <sys/errno.h>
#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32C2
#include "esp32c2/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32C6
#include "esp32c6/rom/ets_sys.h"
#elif CONFIG_IDF_TARGET_ESP32C5
#include "esp32c5/rom/ets_sys.h"
#else
#error "please include ets_sys.h"
#endif

#ifdef CONFIG_I2C_SOFTWARE
#include "driver/gpio.h"
#else
#include "esp_idf_version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0)
#include "driver/i2c_master.h"
#else
#include "driver/i2c.h"
#endif
#endif

#include <string.h>

#include "esp_log.h"
#include "esp_mfi_i2c.h"

static const char *TAG = "mfi_i2c";

#ifndef CONFIG_I2C_SOFTWARE

#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          CONFIG_IC2_SPEED
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TX_BUF_DISABLE   0
#define ACK_CHECK_EN                1
#define ACK_VAL                     0
#define NACK_VAL                    1

static uint8_t i2c_master_sda_gpio = CONFIG_SDA_GPIO;
static uint8_t i2c_master_scl_gpio = CONFIG_SCL_GPIO;

#define I2C_MASTER_SDA_GPIO         CONFIG_SDA_GPIO
#define I2C_MASTER_SCL_GPIO         CONFIG_SCL_GPIO

#define I2C_MASTER_MAX_READ         CONFIG_I2C_MAX_READ_COUNT
#define I2C_MASTER_RETRY_TIMES      500
#define I2C_MASTER_TICKS_TIMES      2 * I2C_MASTER_RETRY_TIMES
#define I2C_MASTER_MAX_RETRY        10
#define I2C_MASTER_INTERNAL_TIMES   8 * I2C_MASTER_RETRY_TIMES
/**
 * @brief Initialize I2C information
 */

void esp_mfi_i2c_configure(uint8_t sda_gpio, uint8_t scl_gpio)
{
    i2c_master_sda_gpio = sda_gpio;
    i2c_master_scl_gpio = scl_gpio;
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0)
/* New I2C master driver (driver/i2c_master.h) from ESP-IDF v5.2+.
 *
 * Callers pass slvaddr in the legacy 8-bit "write address" form (e.g. 0x20
 * for the MFi Auth 3.0 CP, whose 7-bit address is 0x10). The new API takes
 * the 7-bit address directly, so we extract it via `slvaddr >> 1`.
 *
 * The bus and device handles are created once at init and reused across
 * every transaction. Add/remove-device per call (as in earlier attempts)
 * causes erratic behaviour on the second and subsequent transactions.
 *
 * For the read path we deliberately use separate i2c_master_transmit()
 * (which ends with STOP) and i2c_master_receive() (which begins with
 * START), matching the MFi spec section 74.5.6 which requires STOP then
 * START between the register-write and read phases. NOT
 * i2c_master_transmit_receive(), which uses a repeated-START. */

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;
static uint8_t s_dev_addr_7bit = 0xFF; /* sentinel: no device added yet */

#define I2C_TIMEOUT_MS 1000

int esp_mfi_i2c_init(void)
{
    if (s_bus) {
        return 0; /* already initialised */
    }
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = i2c_master_sda_gpio,
        .scl_io_num = i2c_master_scl_gpio,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_config, &s_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
        return -EINVAL;
    }
    return 0;
}

/* Lazily create or recreate the device handle.
 * The MFi auth chip uses a fixed address so the recreate branch should
 * never fire in practice, but it is kept for safety. */
static esp_err_t mfi_i2c_ensure_dev(uint8_t slvaddr)
{
    uint8_t addr_7bit = slvaddr >> 1;
    if (s_dev != NULL && s_dev_addr_7bit == addr_7bit) {
        return ESP_OK;
    }
    if (s_dev != NULL) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr_7bit,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(ret));
        return ret;
    }
    s_dev_addr_7bit = addr_7bit;
    return ESP_OK;
}

int esp_mfi_i2c_write(uint8_t slvaddr, uint8_t regaddr, uint8_t *buff, uint32_t len)
{
    if (!buff || !s_bus) {
        return -EINVAL;
    }
    if (mfi_i2c_ensure_dev(slvaddr) != ESP_OK) {
        return -EIO;
    }

    ESP_LOGI(TAG, "Writing to HW I2C");

    /* Build [regaddr | payload]. The driver injects the device address
     * and R/W bit automatically — do not include them in this buffer. */
    uint8_t *wbuf = malloc(len + 1);
    if (!wbuf) {
        return -ENOMEM;
    }
    wbuf[0] = regaddr;
    memcpy(wbuf + 1, buff, len);

    esp_err_t ret = ESP_FAIL;
    for (int i = 0; i < I2C_MASTER_MAX_RETRY; i++) {
        ret = i2c_master_transmit(s_dev, wbuf, len + 1, I2C_TIMEOUT_MS);
        if (ret == ESP_OK) {
            break;
        }
        ets_delay_us(500); /* MFi spec: wait 500 us before retry on NACK */
    }
    free(wbuf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write data to slave fail: %s", esp_err_to_name(ret));
    }
    return ret;
}

int esp_mfi_i2c_read(uint8_t slvaddr, uint8_t regaddr, uint8_t *buff, uint32_t len)
{
    if (!buff || !s_bus) {
        return -EINVAL;
    }
    if (mfi_i2c_ensure_dev(slvaddr) != ESP_OK) {
        return -EIO;
    }

    ESP_LOGI(TAG, "Reading from HW I2C");

    /* Mirror the legacy nested-retry structure: inner loop retries the
     * register-write phase up to I2C_MASTER_MAX_READ times (gives the
     * chip time to be ready, especially for block 3 certificate reads
     * that need t_CERT after register selection), outer loop retries
     * the full write+read transaction up to I2C_MASTER_MAX_RETRY times. */
    esp_err_t ret = ESP_FAIL;
    for (int i = 0; i < I2C_MASTER_MAX_RETRY; i++) {
        /* Phase 1: write the register address. Ends with STOP. */
        for (int j = 0; j < I2C_MASTER_MAX_READ; j++) {
            ret = i2c_master_transmit(s_dev, &regaddr, 1, I2C_TIMEOUT_MS);
            if (ret == ESP_OK) {
                break;
            }
            ets_delay_us(I2C_MASTER_INTERNAL_TIMES);
        }
        ets_delay_us(I2C_MASTER_INTERNAL_TIMES);

        /* Phase 2: separate START + read. Per MFi spec 74.5.6. */
        ret = i2c_master_receive(s_dev, buff, len, I2C_TIMEOUT_MS);
        if (ret == ESP_OK) {
            break;
        }
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read data from slave fail: %s", esp_err_to_name(ret));
    }
    return ret;
}

#else /* ESP_IDF_VERSION < 5.2.0 — legacy driver/i2c.h API */

int esp_mfi_i2c_init(void)
{
    int ret = 0;
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = i2c_master_sda_gpio,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = i2c_master_scl_gpio,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_param_config(i2c_master_port, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C parameter initial fail %d", ret);
        return -EINVAL;
    }

    ret = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver initial fail %d", ret);
        return -EINVAL;
    }

    return 0;
}
/**
 * @brief write data buffer to slave
 */
int esp_mfi_i2c_write(uint8_t slvaddr, uint8_t regaddr, uint8_t *buff, uint32_t len)
{
    uint16_t i;

    if (!buff)
        return -EINVAL;

    ESP_LOGI(TAG, "Writing to HW I2C");

    int ret = 0;
    i2c_cmd_handle_t cmd = NULL;
    i = 0;

    do {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);

        // Send write address of the CP
        i2c_master_write_byte(cmd, slvaddr, ACK_CHECK_EN);
        // Send register address to slave.
        i2c_master_write_byte(cmd, regaddr, ACK_CHECK_EN);
        // Send data out.
        i2c_master_write(cmd, buff, len, ACK_CHECK_EN);

        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TICKS_TIMES / portTICK_PERIOD_MS);
        i ++;
        i2c_cmd_link_delete(cmd);
        ets_delay_us(I2C_MASTER_RETRY_TIMES);
    } while(ret != ESP_OK && i < I2C_MASTER_MAX_RETRY);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write data to slave fail %d.", ret);
    }
    return ret;
}

/**
 * @brief read data form slave
 */
int esp_mfi_i2c_read(uint8_t slvaddr, uint8_t regaddr, uint8_t *buff, uint32_t len)
{
    uint16_t i, j = 0;

    if (!buff)
        return -EINVAL;

    ESP_LOGI(TAG, "Reading from HW I2C");

    int ret = 0;
    i2c_cmd_handle_t cmd = NULL;
    i = 0;

    do {
        for (j = 0; j < I2C_MASTER_MAX_READ; j++) {
            cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);

            // Send write address of the CP
            i2c_master_write_byte(cmd, slvaddr, ACK_CHECK_EN);
            // Send register address to slave.
            i2c_master_write_byte(cmd, regaddr, ACK_CHECK_EN);

            i2c_master_stop(cmd);
            ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TICKS_TIMES / portTICK_PERIOD_MS);
            i2c_cmd_link_delete(cmd);
            if (ret == ESP_OK) {
                break;
            } else {
                ets_delay_us(I2C_MASTER_INTERNAL_TIMES);
            }
        }

        ets_delay_us(I2C_MASTER_INTERNAL_TIMES);

        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);

        i2c_master_write_byte(cmd, slvaddr + 1, ACK_CHECK_EN);
        if (len == 1)
            i2c_master_read_byte(cmd, buff, NACK_VAL);
        else {
            i2c_master_read(cmd, buff, len - 1, ACK_VAL);
            i2c_master_read_byte(cmd, buff + len - 1, NACK_VAL);
        }

        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TICKS_TIMES / portTICK_PERIOD_MS);
        i ++;
        i2c_cmd_link_delete(cmd);
    } while (ret != ESP_OK && i < I2C_MASTER_MAX_RETRY);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read data from slave fail %d.", ret);
    }

    return ret;
}

#endif /* ESP_IDF_VERSION >= 5.2.0 */

#else

#define MFI_CP_BUSY_RETRY_TIMES CONFIG_I2C_RETRY_COUNT

#define GPIO_Pin_26 GPIO_SEL_26
#define GPIO_Pin_27 GPIO_SEL_27

#define gpio_set_output_level(a, b)                 gpio_set_level(a, b)
#define gpio_get_input_level(a)                     gpio_get_level(a)

#define GPIO_Mode_Input_OutOD                       GPIO_MODE_INPUT_OUTPUT_OD
#define GPIO_PullDown_DIS                           GPIO_PULLDOWN_DISABLE
#define GPIO_PullUp_EN                              GPIO_PULLUP_ENABLE
#define GPIO_Mode_Out_OD                            GPIO_MODE_OUTPUT_OD

#define I2C_MASTER_SDA_MUX                          PERIPHS_IO_MUX_GPIO26_U
#define I2C_MASTER_SCL_MUX                          PERIPHS_IO_MUX_GPIO27_U
#define I2C_MASTER_SDA_GPIO                         26
#define I2C_MASTER_SDA_PIN                          GPIO_Pin_26
#define I2C_MASTER_SDA_FUNC                         FUNC_GPIO26_GPIO26
#define I2C_MASTER_SCL_GPIO                         27
#define I2C_MASTER_SCL_PIN                          GPIO_Pin_27
#define I2C_MASTER_SCL_FUNC                         FUNC_GPIO27_GPIO27

#define I2C_MASTER_SDA_HIGH_SCL_HIGH()              \
    gpio_set_output_level(I2C_MASTER_SDA_GPIO,1);   \
    gpio_set_output_level(I2C_MASTER_SCL_GPIO,1);

#define I2C_MASTER_SDA_HIGH_SCL_LOW()               \
    gpio_set_output_level(I2C_MASTER_SDA_GPIO,1);   \
    gpio_set_output_level(I2C_MASTER_SCL_GPIO,0);

#define I2C_MASTER_SDA_LOW_SCL_HIGH()               \
    gpio_set_output_level(I2C_MASTER_SDA_GPIO,0);   \
    gpio_set_output_level(I2C_MASTER_SCL_GPIO,1);

#define I2C_MASTER_SDA_LOW_SCL_LOW()                \
  gpio_set_output_level(I2C_MASTER_SDA_GPIO,0);     \
  gpio_set_output_level(I2C_MASTER_SCL_GPIO,0);

#define i2c_master_wait ets_delay_us

static void i2c_master_init(void);
static void i2c_master_stop(void);

static uint8_t s_last_sda;
static uint8_t s_last_scl;

/**
 * @brief set I2C SDA and SCL bit value for half CLK cycle
 */
static void i2c_master_setDC(uint8_t SDA, uint8_t SCL)
{
    SDA &= 0x01;
    SCL &= 0x01;
    s_last_sda = SDA;
    s_last_scl = SCL;

    if ((0 == SDA) && (0 == SCL)) {
        I2C_MASTER_SDA_LOW_SCL_LOW();
    } else if ((0 == SDA) && (1 == SCL)) {
        I2C_MASTER_SDA_LOW_SCL_HIGH();
    } else if ((1 == SDA) && (0 == SCL)) {
        I2C_MASTER_SDA_HIGH_SCL_LOW();
    } else {
        I2C_MASTER_SDA_HIGH_SCL_HIGH();
    }
}

/**
 * @brief get I2C SDA bit value
 */
static uint8_t i2c_master_getDC(void)
{
    uint8_t sda_out;

    sda_out = gpio_get_input_level(GPIO_ID_PIN(I2C_MASTER_SDA_GPIO));

    return sda_out;
}

/**
 * @brief initialize I2C bus to enable i2c operations
 */
static void i2c_master_init(void)
{
    uint8_t i;

    i2c_master_setDC(1, 0);
    i2c_master_wait(5);

    // when SCL = 0, toggle SDA to clear up
    i2c_master_setDC(0, 0) ;
    i2c_master_wait(5);
    i2c_master_setDC(1, 0) ;
    i2c_master_wait(5);

    // set data_cnt to max value
    for (i = 0; i < 28; i++) {
        i2c_master_setDC(1, 0);
        i2c_master_wait(5); // sda 1, scl 0
        i2c_master_setDC(1, 1);
        i2c_master_wait(5); // sda 1, scl 1
    }

    // reset all
    i2c_master_stop();
}

/**
 * @brief configure SDA and SCL GPIO to open-drain output mode
 */
static void i2c_master_gpio_init(void)
{

    gpio_config_t io_config;
    io_config.intr_type = GPIO_PIN_INTR_DISABLE;
    io_config.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    io_config.pin_bit_mask = I2C_MASTER_SDA_PIN;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.pull_up_en  = GPIO_PULLUP_ENABLE;

    gpio_config(&io_config);

    io_config.intr_type = GPIO_PIN_INTR_DISABLE;
    io_config.mode = GPIO_MODE_OUTPUT_OD;
    io_config.pin_bit_mask = I2C_MASTER_SCL_PIN;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.pull_up_en  = GPIO_PULLUP_ENABLE;


    gpio_config(&io_config);

    gpio_pullup_en(I2C_MASTER_SDA_GPIO);
    gpio_pullup_en(I2C_MASTER_SCL_GPIO);
    gpio_pulldown_dis(I2C_MASTER_SDA_GPIO);
    gpio_pulldown_dis(I2C_MASTER_SCL_GPIO);

    i2c_master_init();
}

/**
 * @brief set I2C to start operation
 */
static void i2c_master_start(void)
{
    i2c_master_setDC(1, s_last_scl);
    i2c_master_wait(5);
    i2c_master_setDC(1, 1);
    i2c_master_wait(5); // sda 1, scl 1
    i2c_master_setDC(0, 1);
    i2c_master_wait(5); // sda 0, scl 1
}

/**
 * @brief set i2c to stop sending state
 */
static void i2c_master_stop(void)
{
    i2c_master_wait(5);

    i2c_master_setDC(0, s_last_scl);
    i2c_master_wait(5); // sda 0
    i2c_master_setDC(0, 1);
    i2c_master_wait(5); // sda 0, scl 1
    i2c_master_setDC(1, 1);
    i2c_master_wait(5); // sda 1, scl 1
}

/**
 * @brief set ack to i2c bus as level value
 */
static void i2c_master_setAck(uint8_t level)
{
    i2c_master_setDC(s_last_sda, 0);
    i2c_master_wait(5);
    i2c_master_setDC(level, 0);
    i2c_master_wait(5); // sda level, scl 0
    i2c_master_setDC(level, 1);
    i2c_master_wait(8); // sda level, scl 1
    i2c_master_setDC(level, 0);
    i2c_master_wait(5); // sda level, scl 0
    i2c_master_setDC(1, 0);
    i2c_master_wait(5);
}

/**
 * @brief confirm if peer send ack
 */
static uint8_t i2c_master_getAck(void)
{
    uint8_t retVal;

    i2c_master_setDC(s_last_sda, 0);
    i2c_master_wait(5);
    i2c_master_setDC(1, 0);
    i2c_master_wait(5);
    i2c_master_setDC(1, 1);
    i2c_master_wait(5);

    retVal = i2c_master_getDC();
    i2c_master_wait(5);
    i2c_master_setDC(1, 0);
    i2c_master_wait(5);

    return retVal;
}

/**
 * @brief get I2C response
 */
static bool i2c_master_checkAck(void)
{
    if (i2c_master_getAck()) {
        return false;
    } else {
        return true;
    }
}

/**
 * @brief send I2C ack signal
 */
static void i2c_master_send_ack(void)
{
    i2c_master_setAck(0x0);
}

/**
 * @brief send I2C not ack signal
 */
static void i2c_master_send_nack(void)
{
    i2c_master_setAck(0x1);
}

/**
 * @brief read byte from i2c bus
 */
static uint8_t i2c_master_readByte(void)
{
    uint8_t retVal = 0;
    uint8_t k, i;

    i2c_master_wait(5);
    i2c_master_setDC(s_last_sda, 0);
    i2c_master_wait(5); // sda 1, scl 0

    for (i = 0; i < 8; i++) {
        i2c_master_wait(5);
        i2c_master_setDC(1, 0);
        i2c_master_wait(5); // sda 1, scl 0
        i2c_master_setDC(1, 1);
        i2c_master_wait(5); // sda 1, scl 1

        k = i2c_master_getDC();
        i2c_master_wait(5);

        if (i == 7) {
            i2c_master_wait(3);
        }

        k <<= (7 - i);
        retVal |= k;
    }

    i2c_master_setDC(1, 0);
    i2c_master_wait(5); // sda 1, scl 0

    return retVal;
}

/**
 * @brief write byte to i2c
 */
static void i2c_master_writeByte(uint8_t wrdata)
{
    uint8_t dat;
    int8_t i;

    i2c_master_wait(5);
    i2c_master_setDC(s_last_sda, 0);
    i2c_master_wait(5);

    for (i = 7; i >= 0; i--) {
        dat = wrdata >> i;
        i2c_master_setDC(dat, 0);
        i2c_master_wait(5);
        i2c_master_setDC(dat, 1);
        i2c_master_wait(5);

        if (i == 0) {
            i2c_master_wait(3);   ////
        }

        i2c_master_setDC(dat, 0);
        i2c_master_wait(5);
    }

}

/**
 * @brief write byte to i2c and get check ack
 */
static bool i2c_write_byte(uint8_t data, uint8_t iter)
{
    if (iter == 0) {
        iter = 1;
    }

    while (iter--) {
        i2c_master_writeByte(data);

        if (i2c_master_getAck()) {
            i2c_master_stop();
            ets_delay_us(500);  // Wait 500us and retry.
            i2c_master_start();
        } else {
            return true;
        }
    }
    i2c_master_stop();
    return false;
}

/**
 * @brief write byte to target register of target I2C slave
 */
static void i2c_write_reg(uint8_t slave_add, uint8_t reg_add, uint16_t data)
{
    i2c_master_start();
    if (false == i2c_write_byte(slave_add, 30)) {
        ESP_LOGE(TAG, "1Slave is busy, [%02x]TIME OUT\n", slave_add);
    }
    if (false == i2c_write_byte(reg_add, 30)) {
        ESP_LOGE(TAG, "2Slave is busy, [%02x]TIME OUT\n", reg_add);
    }
    if (false == i2c_write_byte((data >> 8), 30)) {
        ESP_LOGE(TAG, "3Slave is busy, [%02x]TIME OUT\n", data);
    }
    if (false == i2c_write_byte((data & 0xFF), 30)) {
        ESP_LOGE(TAG, "3Slave is busy, [%02x]TIME OUT\n", data);
    }
    i2c_master_stop();
}

/**
 * @brief a block of data to I2C
 */
static void i2s_write_arr(const uint8_t *data, size_t len)
{
    if (data == NULL || len <= 0) return;

    i2c_master_start();

    int i = 0;
    for (i = 0; i < len; ++i) {
        if (false == i2c_write_byte(data[i], 30)) {
            ESP_LOGE(TAG, "Slave is busy, [%02x]TIME OUT\n", data[i]);
        }
    }
    i2c_master_stop();
}

/**
 * @brief Initialize I2C information
 */
int esp_mfi_i2c_init(void)
{
    i2c_master_gpio_init();
    return 0;
}

/**
 * @brief Finish I2C information
 */
int esp_mfi_i2c_end(void)
{
    return 0;
}

/**
 * @brief write one byte data to slave
 */
static int esp_mfi_i2c_write_byte(uint8_t data, uint16_t retrytimes)
{
    uint16_t times = retrytimes;
    if (0 == times) {
        times = 1;
    }

    while (times--) {
        i2c_master_writeByte(data);
        if (i2c_master_getAck()) {
            i2c_master_wait(500);  /**< Wait 500us and retry */
            i2c_master_stop();
            i2c_master_wait(500);  /**< Wait 500us and retry */
            i2c_master_start();
        } else {
            return 0;
        }
    }

    i2c_master_stop();

    return -ETIME;
}

/**
 * @brief write data buffer to slave
 */
int esp_mfi_i2c_write(uint8_t slvaddr, uint8_t regaddr, uint8_t *buff, uint32_t len)
{
    uint16_t i;

    if (!buff)
        return -EINVAL;

    ESP_LOGI(TAG, "Writing to SW I2C");

    i2c_master_start();

    /**< Send write address of the CP */
    if (esp_mfi_i2c_write_byte(slvaddr, MFI_CP_BUSY_RETRY_TIMES) != 0) {
        ESP_LOGE(TAG, "Slave is busy, time out.");
        return -ETIME;
    }

    /**< Send register address to slave */
    if (esp_mfi_i2c_write_byte(regaddr, 0) != 0) {
        ESP_LOGE(TAG, "Write register address[0x%02x] to slave.", regaddr);
        return -ENODATA;
    }

    /**< Send data out */
    for (i = 0; i < len; ++i) {
        if (esp_mfi_i2c_write_byte(*buff++, 0) != 0) {
            ESP_LOGE(TAG, "Write data[0x%02x] to slave.", *buff);
            return -ENODATA;
        }
    }

    i2c_master_stop();

    return 0;
}

/**
 * @brief read data form slave
 */
int esp_mfi_i2c_read(uint8_t slvaddr, uint8_t regaddr, uint8_t *buff, uint32_t len)
{
    uint16_t i;

    if (!buff)
        return -EINVAL;

    ESP_LOGI(TAG, "Reading from SW I2C");

    i2c_master_start();

    /**< Send write address of the CP */
    if (esp_mfi_i2c_write_byte(slvaddr, MFI_CP_BUSY_RETRY_TIMES) != 0) {
        ESP_LOGE(TAG, "Slave is busy, time out.");
        return -ETIME;
    }

    /**< Send register address to slave */
    if (esp_mfi_i2c_write_byte(regaddr, 0) != 0) {
        ESP_LOGE(TAG, "Write register address[0x%02x] to slave.", regaddr);
        return -ENODATA;
    }
    i2c_master_stop();

    i2c_master_wait(4000);

    /**< Send read address of the CP */
    if (esp_mfi_i2c_write_byte((slvaddr + 1), MFI_CP_BUSY_RETRY_TIMES) != 0) {
        ESP_LOGE(TAG, "Slave is busy, time out.");
        return -ETIME;
    }

    for (i = 0; i < len; ++i) {
        buff[i] = i2c_master_readByte();

        i2c_master_setAck((i == (len - 1)) ? 1 : 0);
    }

    i2c_master_stop();

    return 0;
}
#endif
