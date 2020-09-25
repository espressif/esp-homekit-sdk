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
import sys
import os
import csv
import subprocess
import binascii
import argparse
try:
    file_path = os.path.dirname(os.path.realpath(__file__))
    nvs_gen_path = os.path.join(file_path,"../idf_tools/nvs_partition_generator/")

    if not os.path.exists(nvs_gen_path):
        print("Trying tools from IDF_PATH")
        if os.getenv('IDF_PATH'):
            nvs_gen_path = os.path.join(os.getenv('IDF_PATH'),'','components','','nvs_flash','','nvs_partition_generator','')
        else:
            sys.exit("Please check IDF_PATH")
    sys.path.insert(0, nvs_gen_path)
    import nvs_partition_gen
    srp_tool_path = os.path.join(file_path, '../' ,'srp','')
    sys.path.insert(0, srp_tool_path)
    import srp_tool as srp


except Exception as e:
    print(e)

def main():
    try:
        parser = argparse.ArgumentParser(prog='./factory_nvs_gen.py',\
        description="Factory NVS Generator",\
        formatter_class=argparse.RawDescriptionHelpFormatter)
        factory_nvs_part_gen_group = parser.add_argument_group('To generate Factory NVS partition')
        factory_nvs_part_gen_group.add_argument('setup_code',\
        help='the input setup code without hyphens Eg.: 11122333 ')
        factory_nvs_part_gen_group.add_argument('setup_id',\
        help='the input setup identifer Eg.: ES32')
        factory_nvs_part_gen_group.add_argument('--infile',dest='infile',\
        help='the input csv file with extension(full path) to add HomeKit data generated to this file Eg.: fan_data.csv')
        factory_nvs_part_gen_group.add_argument('outfile',\
        help='the output csv and bin filename without extension Eg.: fan_factory')
        factory_nvs_part_gen_group.add_argument('--size', \
        help='Size of NVS Partition in bytes (must be multiple of 4096)', \
        default='0x4000')
        factory_nvs_part_gen_group.add_argument('--version', \
        help='Set version. Default: v2',choices=['v1','v2'], \
        default='v2',type=str.lower)
        factory_nvs_part_gen_group.add_argument("--keygen", \
        help='Generate keys for encryption. Creates an `encryption_keys.bin` file. Default: false', \
        choices=['true','false'],default= 'false',type=str.lower)
        factory_nvs_part_gen_group.add_argument("--encrypt", \
        help='Set encryption mode. Default: false', \
        choices=['true','false'],default='false',type=str.lower)
        factory_nvs_part_gen_group.add_argument("--keyfile", \
        help='File having key for encryption (Applicable only if encryption mode is true)',
        default = None)
        factory_nvs_part_gen_group.add_argument('--outdir', dest='outdir', \
        help='the output directory to store the files created(Default: current directory)', default=os.getcwd())
        factory_nvs_part_gen_group.add_argument('--salt', \
        help='Predefined salt if any', \
        default=None)

        args = parser.parse_args()
        part_size = args.size
        is_key_gen = args.keygen
        encrypt_mode = args.encrypt
        version_no = args.version
        key_file = args.keyfile
        output_dir_path = args.outdir
        salt_input = args.salt

        # Check if the setup code has valid format
        setup_code = args.setup_code
        if (len(setup_code) != 8) or (not setup_code.isdigit()):
            print("Setup code should be 8 numbers without any '-' in between. Eg. 11122333")
            sys.exit (1)

        # Add the hyphen (-) in the PIN for salt-verifier generation. So, 11122333 will become 111-22-333
        setup_code = setup_code[:3] + '-' + setup_code[3:5] + '-' + setup_code[5:]
        setup_id = args.setup_id
        file_prefix = args.outfile

        if not args.infile:
            csv_file = file_prefix + ".csv"
        else:
            if not os.path.isfile(args.infile):
                sys.exit("Oops...file: `" + args.infile + "` does not exist...")
            else:
                csv_file = file_prefix + ".csv"
        bin_file = file_prefix + ".bin"
        # Consider enabling RFC5054 compatibility for interoperation with non srp SRP-6a implementations
        #srp.rfc5054_enable()

        # The Modulus, N, and Generator, g, as specified by the 3072-bit group of RFC5054. This is required since the standard srp script does not support this
        n_3072 = "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFCE0FD108E4B82D120A93AD2CAFFFFFFFFFFFFFFFF"
        g_3072 = "5"

        # Generate the Salt and Verifier for the given PIN
        salt, vkey = srp.create_salted_verification_key( 'Pair-Setup', setup_code, srp.SHA512, srp.NG_CUSTOM, n_3072, g_3072, salt_len=16 , salt=salt_input)

        # Create the CSV file with appropriate data
        f = open(csv_file, 'w')
        if not args.infile:
            f.write("key,type,encoding,value\n")
        else:
            in_f = open(args.infile, 'r')
            data = in_f.read()
            f.write(data)
            in_f.close()

        f.close()

        hap_setup_found = False

        f = open(csv_file,'a+')
        f.seek(0)
        f_reader = csv.reader(f, delimiter='\n')
        for data in f_reader:
            if 'hap_setup,namespace,,' in data:
                hap_setup_found = True
                break

        if not hap_setup_found:
            f.write("hap_setup,namespace,,\n")

        f.write("setup_id,data,binary," + setup_id + "\n")
        f.write("setup_salt,data,hex2bin," + binascii.b2a_hex(salt).zfill(32).decode("utf-8") + "\n")
        f.write("setup_verifier,data,hex2bin," + binascii.b2a_hex(vkey).zfill(768).decode("utf-8"))
        f.close()

        print("CSV Generated: " + csv_file)

        print_arg_str = "Invalid.\nTo generate nvs partition binary --input, --output and --size arguments are mandatory.\nTo generate encryption keys --keygen argument is mandatory."
        print_encrypt_arg_str = "Missing parameter. Enter --keyfile or --keygen."

        nvs_partition_gen.check_input_args(csv_file, bin_file, part_size, is_key_gen,\
                                            encrypt_mode, key_file, version_no, print_arg_str, print_encrypt_arg_str, output_dir_path)

        nvs_partition_gen.nvs_part_gen(input_filename=csv_file, output_filename=bin_file,\
                                        input_part_size=part_size, is_key_gen=is_key_gen,\
                                        encrypt_mode=encrypt_mode, key_file=key_file,\
                                        encr_key_prefix=None, version_no=version_no, output_dir=output_dir_path)

        print("NVS Flash Binary Generated: " + bin_file)

    except Exception as e:
        sys.exit(e)

if __name__ == "__main__":
    main()
