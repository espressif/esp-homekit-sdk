/* MFI Firmware update Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _HAP_AWS_IOT_H_
#define _HAP_AWS_IOT_H_

#include <stdint.h>
void hap_aws_iot_report_on_char(bool val);
void hap_aws_iot_report_brightness_char(int val);
esp_err_t hap_aws_iot_start(void);
void hap_aws_iot_stop(void);

#endif /* _HAP_AWS_IOT_H_ */
