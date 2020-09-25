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

/* BCT HTTP Handlers
*/
#include <sdkconfig.h>
#include <esp_http_server.h>
#include <hap_platform_httpd.h>
#include <hap.h>
#include <hap_bct.h>

#define SUCCESS_RESP    "{\"status\":0}"

#ifdef CONFIG_LEGACY_BCT /* BCT 1.4 and earlier */
#define NEW_BCT_NAME    "New - Bonjour Service Name"
#else
#define NEW_BCT_NAME    "New-BCT"
#endif /* CONFIG_LEGACY_BCT */

static int hap_bct_name_change_handler(httpd_req_t *req)
{
    hap_bct_change_name(NEW_BCT_NAME);
    return httpd_resp_send(req, SUCCESS_RESP, strlen(SUCCESS_RESP));
}
static struct httpd_uri bct_name_change_get = {
	.uri = "/bct/name-change",
    .method = HTTP_GET,
    .handler = hap_bct_name_change_handler,
};
static struct httpd_uri bct_name_change_post = {
	.uri = "/bct/name-change",
    .method = HTTP_POST,
    .handler = hap_bct_name_change_handler,
};

static int hap_bct_hot_plug_handler(httpd_req_t *req)
{
    hap_bct_hot_plug();
    return httpd_resp_send(req, SUCCESS_RESP, strlen(SUCCESS_RESP));
}
static struct httpd_uri bct_hotplug_get = {
	.uri = "/bct/hotplug",
    .method = HTTP_GET,
    .handler = hap_bct_hot_plug_handler,
};

static struct httpd_uri bct_hotplug_post = {
	.uri = "/bct/hotplug",
    .method = HTTP_POST,
    .handler = hap_bct_hot_plug_handler,
};

void hap_bct_register_http_handlers()
{
    httpd_handle_t *httpd_handle = hap_platform_httpd_get_handle();
    httpd_register_uri_handler(*httpd_handle, &bct_name_change_get);
    httpd_register_uri_handler(*httpd_handle, &bct_name_change_post);
    httpd_register_uri_handler(*httpd_handle, &bct_hotplug_get);
    httpd_register_uri_handler(*httpd_handle, &bct_hotplug_post);
}

void hap_bct_unregister_http_handlers()
{
    httpd_handle_t *httpd_handle = hap_platform_httpd_get_handle();
    httpd_unregister_uri_handler(*httpd_handle, "/bct/name-change", HTTP_GET);
    httpd_unregister_uri_handler(*httpd_handle, "/bct/name-change", HTTP_POST);
    httpd_unregister_uri_handler(*httpd_handle, "/bct/hotplug", HTTP_GET);
    httpd_unregister_uri_handler(*httpd_handle, "/bct/hotplug", HTTP_POST);
}
