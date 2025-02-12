#!/usr/bin/env python
#
# Copyright 2018 Espressif Systems (Shanghai) PTE LTD
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


import argparse


def base36encode(integer):
    '''
    Base36 Encoding
    '''
    chars, encoded = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ', ''

    while integer > 0:
        integer, remainder = divmod(integer, 36)
        encoded = chars[remainder] + encoded

    return encoded


def setup_payload_data_gen(setup_id, input_product_data=None):
    '''
    Setup Payload Data
    '''
    payload_data_encoding = ''
    curr_setup_payload_data_len = len(setup_id)

    # Add product data to payload data (if provided)
    if input_product_data:
        SETUP_PAYLOAD_DATA_LEN = 22
        product_data = input_product_data << 56
        product_number = base36encode(product_data)
        # Get current length of Setup Payload Data Encoding
        curr_setup_payload_data_len = \
            curr_setup_payload_data_len + len(product_number)
        # Set payload data encoding
        payload_data_encoding = payload_data_encoding + product_number

        # Prefix with zeros if length is not the required length after encoding
        if curr_setup_payload_data_len != SETUP_PAYLOAD_DATA_LEN:
            no_of_zeros_required = \
                SETUP_PAYLOAD_DATA_LEN - curr_setup_payload_data_len
            zeros_prefix = '0' * no_of_zeros_required
            payload_data_encoding = zeros_prefix + payload_data_encoding

    payload_data_encoding = setup_id + payload_data_encoding
    print("Setup Payload Data Encoding: {}".format(payload_data_encoding))
    return payload_data_encoding


def setup_payload_header_gen(cid, setup_code, input_product_data=None):
    '''
    Setup Payload Header
    '''
    SETUP_PAYLOAD_HEADER_LEN = 9
    nw_flags = 0xa  # For IP accessories supporting WAC. Use 0x4 for BLE
    version_category_flags_and_setup_code = \
        (cid << 31) | (nw_flags << 27) | setup_code

    # Add product data to setup payload header (if provided)
    if input_product_data:
        prod_no = 0x01  # Product Number data is set
        version_category_flags_and_setup_code = \
            version_category_flags_and_setup_code | (prod_no << 40)

    # Base36 encoding
    setup_payload_header_encoding = base36encode(
        version_category_flags_and_setup_code)

    # Get current length of Setup Payload Header Encoding
    setup_payload_header_encoding_len = len(setup_payload_header_encoding)
    # Prefix with zeros if length is not the required length after encoding
    if setup_payload_header_encoding_len != SETUP_PAYLOAD_HEADER_LEN:
        no_of_zeros_required = \
            SETUP_PAYLOAD_HEADER_LEN - setup_payload_header_encoding_len
        zeros_prefix = '0' * no_of_zeros_required
        setup_payload_header_encoding = \
            zeros_prefix + setup_payload_header_encoding

    print("Setup Payload Header Encoding: {}".format(
        setup_payload_header_encoding)
        )
    return setup_payload_header_encoding


def setup_payload_gen(cid, setup_code, setup_id, product_data=None):
    '''
    Setup payload
    '''
    setup_payload_header_encoding = setup_payload_header_gen(
        cid, setup_code, input_product_data=product_data)
    payload_data_encoding = setup_payload_data_gen(
        setup_id,
        input_product_data=product_data
        )
    setup_payload = 'X-HM://{0}{1}'.format(
        setup_payload_header_encoding,
        payload_data_encoding)
    return(setup_payload)


def main():
    try:
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('AccessoryCategory',
                            type=int,
                            metavar='<AccessoryCategory>')

        parser.add_argument('SetupCode',
                            type=int,
                            metavar='<SetupCode>')

        parser.add_argument('SetupID',
                            type=str,
                            metavar='<SetupID>')

        parser.add_argument('ProductData',
                            nargs='?',
                            type=str,
                            metavar='<ProductData>')

        args = parser.parse_args()
        cid = args.AccessoryCategory
        setup_code = args.SetupCode
        setup_id = args.SetupID
        product_data = None
        if args.ProductData:
            product_data = int(args.ProductData[-8:], 16)

        setup_payload = setup_payload_gen(
            cid,
            setup_code,
            setup_id,
            product_data=product_data)
        print('Setup Payload: {}'.format(setup_payload))

    except SystemExit:
        print('Eg. ./setup_payload_gen.py 7 51808582 7OSX 1122334455667788')
    except Exception:
        raise


if __name__ == "__main__":
    main()
