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
#ifndef _HAP_BCT_HTTP_HANDLERS_H_
#define _HAP_BCT_HTTP_HANDLERS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register web handlers for tester interaction steps of Bonjour Conformance Tests (BCT)
 *
 * Two web handlers will be registered, which can be used during BCT to trigger
 * varions "Tester Interaction" steps.
 *
 * /bct/hotplug: A GET or POST on this can be used for "Cable Change Handling" or "Hot Plugging"
 * steps.
 * /bct/name-change: A GET or POST on this can be used for "Manual Name Change".
 *
 * Eg. curl 169.254.1.9/bct/hotplug
 *     curl -d {} 169.254.1.9/bct/name-change
 *
 * This should be used only for Certification accessories. Irrespective of when this API is called,
 * the handlers will get enabled only after hap_start().
 */
void hap_bct_register_http_handlers();

/**
 * @brief De-register the handlers registered using hap_bct_register_http_handlers()
 */
void hap_bct_unregister_http_handlers();

#ifdef __cplusplus
}
#endif

#endif /* _HAP_BCT_HTTP_HANDLERS_H_ */
