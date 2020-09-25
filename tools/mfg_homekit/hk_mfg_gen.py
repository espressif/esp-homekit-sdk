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

from __future__ import print_function
from future.moves.itertools import zip_longest
from builtins import range
import os
import string
import random
import binascii
import csv
import itertools
import shutil
import sys
import argparse
import datetime
import distutils.dir_util
import setup_info_gen
import pdb

try:
    file_path = os.path.dirname(os.path.realpath(__file__))
    nvs_gen_path = os.path.join(file_path,"../idf_tools/nvs_partition_generator/")
    mass_mfg_path = os.path.join(file_path,"../idf_tools/mass_mfg/")

    if not os.path.exists(nvs_gen_path):
        print("Trying tools from IDF_PATH")
        if os.getenv('IDF_PATH'):
            nvs_gen_path = os.path.join(os.getenv('IDF_PATH'),'','components','','nvs_flash','','nvs_partition_generator','')
            mass_mfg_path = os.path.join(os.getenv('IDF_PATH'),'','tools','','mass_mfg','')
        else:
            sys.exit("Please check IDF_PATH")
    sys.path.insert(0, nvs_gen_path)
    sys.path.insert(0, mass_mfg_path)
    import nvs_partition_gen
    import mfg_gen
except Exception as e:
    sys.exit(e)

try:
    file_path = os.path.dirname(os.path.realpath(__file__))
    srp_tool_path = os.path.join(file_path, '../' ,'srp','')
    sys.path.insert(0, srp_tool_path)
    import srp_tool as srp
except Exception as e:
    print(e)



def write_homekit_setup_info(homekit_csv_file, output_target_dir, csv_output_filenames):
    """ Write homekit setup info (setup code and setup payload generated) to output txt file
    """
    try:
        setup_code = b''
        setup_payload = b''

        hk_file = open(homekit_csv_file,'r')
        hk_file_reader = csv.reader(hk_file,delimiter=',')
        hk_keys = next(hk_file_reader)
        hk_keys = hk_keys[-5:]
        
        for data in hk_file_reader:
            csv_file = csv_output_filenames[0]
            # Create output filename with .txt extension 
            output_txt_file = output_target_dir + csv_file.replace(".csv",".txt")

            # Terminate if target file exist
            if os.path.isfile(output_txt_file):
                raise SystemExit("HomeKit setup info target file: %s ` already exists...`" % output_txt_file)
            
            # Write data to output txt file 
            hk_values = data[-5:]
            hk_data = list(zip_longest(hk_keys, hk_values))
            for data in hk_data:
                if 'setup_code' in data:
                    setup_code = data[1]
                if 'setup_payload' in data:
                    setup_payload = data[1]
            setup_info_gen.write_setup_info_to_file(setup_code,setup_payload,output_txt_file)
            print("HomeKit Setup Info File Generated: " , str(output_txt_file))
            del csv_output_filenames[0]

        hk_file.close()
    
    except Exception as std_err:
        print(std_err)
    except:
        raise


def verify_values_exist(input_values_file, keys_in_values_file):
    """ Verify all keys have corresponding values in values file
    """
    line_no = 1
    key_count_in_values_file = len(keys_in_values_file)
    
    values_file = open(input_values_file,'r')
    values_file_reader = csv.reader(values_file, delimiter=',')
    next(values_file_reader)

    for values_data in values_file_reader:
        line_no +=1
        if len(values_data) != key_count_in_values_file:
            raise SystemExit("Oops...Number of values is not equal to number of keys in file: %s ' at line No: %s'" \
            % (str(input_values_file), str(line_no)))


def verify_keys_exist(input_config_file, values_file_keys):
    """ Verify all keys from config file are present in values file
    """
    keys_missing = []
    
    config_file = open(input_config_file,'r')
    config_file_reader = csv.reader(config_file, delimiter=',')
    for line_no, config_data in enumerate(config_file_reader,1):
        if 'namespace' not in config_data:
            if values_file_keys:
                if config_data[0] == values_file_keys[0]:
                    del values_file_keys[0]
                else:
                    keys_missing.append([config_data[0], line_no])
            else:
                keys_missing.append([config_data[0], line_no])

    if keys_missing:
        for key, line_no in keys_missing:
            print("Key:`", str(key), "` at line no:", str(line_no), \
            " in config file is not found in values file...")
        config_file.close()
        raise SystemExit(1)

    config_file.close()


def verify_datatype_encoding(input_config_file):
    """ Verify datatype and encodings from config file is valid
    """
    valid_encodings = ["string", "binary", "hex2bin","u8", "i8", "u16", "u32", "i32","base64"]
    valid_datatypes = ["file","data","namespace"]
    line_no = 0
    
    config_file = open(input_config_file,'r')
    config_file_reader = csv.reader(config_file, delimiter=',')
    for config_data in config_file_reader:
        line_no+=1
        if config_data[1] not in valid_datatypes:
            raise SystemExit("Oops...config file: %s has invalid datatype at line no: %s" % (str(input_config_file), str(line_no)))
        if 'namespace' not in config_data:
            if config_data[2] not in valid_encodings:
                raise SystemExit("Oops...config file: %s has invalid encoding at line no: %s" % (str(input_config_file), str(line_no)))


def verify_file_data_count(input_config_file):
    """ Verify count of data on each line in config file is equal to 3
    (as format must be: <key,type and encoding>)
    """
    line_no = 0
    config_file = open(input_config_file, 'r')
    config_file_reader = csv.reader(config_file, delimiter=',')
    for line in config_file_reader:
        line_no += 1
        if len(line) < 3 or len(line) > 4:
            raise SystemExit("Oops...data missing in config file at line no: %s  <format needed:key,type,encoding>" % str(line_no))

    config_file.close()


def verify_hap_setup(input_config_file,is_exists):
    """ Verify hap_setup namespace does not already exist in config file
    """
    config_file = open(input_config_file,'r')
    config_file_reader = csv.reader(config_file, delimiter=',')
    for config_data in config_file_reader:
        if 'hap_setup' in config_data and not is_exists:
            config_file.close()
            raise SystemExit("Oops...`hap_setup` namespace already exists in config file...")

    config_file.close()


def verify_data_in_file(input_config_file=None, input_values_file=None, config_file_keys=None, keys_in_values_file=None):
    """
    Verify count of data on each line in config file is equal to 3 \
    ( as format must be: <key,type and encoding> ) 
    Verify datatype and encodings from config file is valid
    Verify all keys from config file are present in values file and \
    Verify each key has corresponding value in values file 
    """
    try:
        # Verify data if input config file is given
        if input_config_file:
            verify_file_data_count(input_config_file)
            verify_datatype_encoding(input_config_file)
           
        # Verify data if input values file is given
        if input_values_file:
            verify_values_exist(input_values_file, keys_in_values_file)        
            
        # Verify data if input config file and input values file is given
        if input_config_file and input_values_file:
            verify_keys_exist(input_config_file, values_file_keys)
       
        
    except Exception as std_err:
        print(std_err)
    except:
        raise



def add_homekit_data_to_file(input_config_file,input_values_file,keys_in_values_file,homekit_data,output_dir):
    """ Add homekit data generated to input csv config and values file respectively 
    """
    try:
        # Create a new target config and values file if \
        # input_config_file or input_values_file is None with prefix as `homekit_` \ 
        # and suffix as timestamp generated
        timestamp = datetime.datetime.now().strftime('%m-%d_%H-%M')
        if input_config_file is None: 
            target_config_file = output_dir + 'homekit_config_' + timestamp + '.csv'
        else:
            input_config_filename = os.path.basename(input_config_file)
            target_config_file = output_dir + 'homekit_' + input_config_filename
       
        if input_values_file is None:
            target_values_file = output_dir + 'homekit_values_' + timestamp + '.csv'
        else:
            input_values_filename = os.path.basename(input_values_file)
            target_values_file = output_dir + 'homekit_' + input_values_filename
    
        # Terminate if target files exist
        if os.path.isfile(target_config_file):
            raise SystemExit("Target config file: %s already exists...`" % target_config_file)
        if os.path.isfile(target_values_file):
            raise SystemExit("Target values file: %s already exists...`" % target_values_file)
        
        # Copy contents of input_config_file and input_values_file to 
        # target_config and target_values file
        conf_file_mode = 'w'
        if input_config_file:
            shutil.copyfile(input_config_file,target_config_file)
            conf_file_mode = 'a+'
        if  input_values_file:
            shutil.copyfile(input_values_file,target_values_file)

        hap_setup_found = False

        configfile = open(target_config_file, conf_file_mode)
        valuesfile = open(target_values_file, 'w')

        if conf_file_mode == 'a+':
            configfile.seek(0)
            config_file_reader = csv.reader(configfile, delimiter=',')
            for config_data in config_file_reader:
                if 'hap_setup' in config_data:
                    hap_setup_found = True
                    break

        if not hap_setup_found:
            configfile.write(u"hap_setup,namespace,\n")
        
        configfile.write(u"setup_id,data,binary\n")
        configfile.write(u"setup_salt,data,hex2bin\n")
        configfile.write(u"setup_verifier,data,hex2bin\n")

        valuesfile_writer = csv.writer(valuesfile, delimiter=',')
        valuesfile_writer.writerow(keys_in_values_file)
        for data in homekit_data:
            valuesfile_writer.writerow(data)
        
        configfile.close()
        valuesfile.close()
   
        return target_config_file,target_values_file

    except Exception as std_err:
        print(std_err)
    except:
        raise


def salt_vkey_gen(setup_code_created):
    """ Generate a Salt and Verification Key for each setup code generated
    """

    try:
        # The Modulus, N, and Generator, g, as specified by the 3072-bit group of RFC5054. 
        # This is required since the standard srp script does not support this
        n_3072 = "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFCE0FD108E4B82D120A93AD2CAFFFFFFFFFFFFFFFF"
        g_3072 = "5"
    
        salt_created = []
        vkey_created = []
        salt_len_needed = 32
        vkey_len_needed = 768

        for setup_code in setup_code_created:
            while True:
                salt, vkey = srp.create_salted_verification_key( 'Pair-Setup', str(setup_code), srp.SHA512, \
                srp.NG_CUSTOM, n_3072.encode(), g_3072.encode(), salt_len=16 )
                salt = binascii.b2a_hex(salt).decode()
                vkey = binascii.b2a_hex(vkey).decode()

                if len(salt) == salt_len_needed and len(vkey) == vkey_len_needed:
                    break

            salt_created.append(salt)
            vkey_created.append(vkey)

        return salt_created, vkey_created

    except Exception as std_err:
        print(std_err)
    except:
        raise


def setup_id_gen(total_value_count):
    """ Generate a setup id corresponding to each fileid
    """
    try:
        setup_id_created = []

        for x in range(total_value_count):
            setup_id = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(4))
            setup_id_created.append(setup_id)

        return setup_id_created

    except Exception as std_err:
        print(std_err)
    except:
        raise


def setup_code_gen(total_value_count):
    """ Generate a setup code corresponding to each fileid
    """
    try:
        invalid_setup_codes = ['00000000','11111111','22222222','33333333','44444444','55555555',\
                           '66666666','77777777','88888888','99999999','12345678','87654321'] 
   
        setup_code_created = []

        for _ in range(total_value_count):
       
            setup_code = ''

            # random generate setup_code
            for _ in range(8):
                random_num = str(random.randint(0,9))
                setup_code += random_num

            # generate again till valid
            while setup_code in invalid_setup_codes:
                setup_code = ''
                for _ in range(8):
                    random_num = str(randint(0,9))
                    setup_code += random_num

            # Check if the setup code has valid format
            if (len(setup_code) != 8) or (not setup_code.isdigit()):
                print("Setup code generated should be 8 numbers without any '-' in between. Eg. 11122333")
                raise SystemExit (1)

            # Add the hyphen (-) in the PIN for salt-verifier generation. So, 11122333 will become 111-22-333
            setup_code = setup_code[:3] + '-' + setup_code[3:5] + '-' + setup_code[5:]

            setup_code_created.append(setup_code)
    
        return setup_code_created
    
    except Exception as std_err:
        print(std_err)
    except:
        raise


def homekit_data_gen(no_of_accessories=None):
    """ Generate homekit data - setup_id,setup_code
    salt and verifier key
    """
    setup_id_created = setup_id_gen(no_of_accessories)
    setup_code_created = setup_code_gen(no_of_accessories) 
    salt_created, vkey_created = salt_vkey_gen(setup_code_created)
    
    return setup_id_created,setup_code_created,salt_created,vkey_created


def get_keys(keys_in_values_file, config_file_keys):
    """ Get keys from values file present in config file
    """
    values_file_keys = []
    for key in range(len(keys_in_values_file)):
        if keys_in_values_file[key] in config_file_keys:
            values_file_keys.append(keys_in_values_file[key])
    
    return values_file_keys



def main():
    try:
        filename = os.path.join('.','hk_mfg_gen.py')
        parser = argparse.ArgumentParser(prog=filename,
                                         description="Generate HomeKit specific data and create binaries and HomeKit setup info",
                                         formatter_class=argparse.RawDescriptionHelpFormatter)
        
        parser.add_argument('--conf',
                            dest='config_file',
                            help='the input configuration csv file')
        
        parser.add_argument('--values',
                            dest='values_file',
                            help='the input values csv file (the number of accessories \
                            to create HomeKit data for is equal to the data entries \
                            in this file)',
                            default=None)
        
        parser.add_argument('--prefix',
                            dest='prefix',
                            help='the accessory name as each filename prefix',
                            default=None)
        
        parser.add_argument('--fileid',
                            dest='fileid',
                            help='the unique file identifier(any key in values file) as \
                            each filename suffix (Default: setup_code)',
                            default=None)
        
        curr_dir = os.path.join('.','')
        parser.add_argument('--outdir',
                            dest='outdir',
                            default=curr_dir,
                            help='the output directory to store the files created \
                            (Default: current directory)')
        
        parser.add_argument('--cid',
                            dest='cid',
                            type=int,
                            help='the accessory category identifier',
                            default=None)
        
        parser.add_argument('--count',
                            dest='count',
                            type=int,
                            help='the number of accessories to create HomeKit data for,\
                            applicable only if --values is not given',
                            default=None)
       
        parser.add_argument("--size",
                            dest='part_size',
                            help='Size of NVS Partition in bytes (must be multiple of 4096)',
                            default=None)

        parser.add_argument("--version",
                            dest="version",
                            help='Set version. Default: v2',
                            choices=['v1','v2'],
                            default='v2',
                            type=str.lower)

        parser.add_argument("--keygen",
                            dest="keygen",
                            help='Generate keys for encryption. Default: False',
                            choices=['true','false'],
                            default= 'false',
                            type=str.lower)

        parser.add_argument("--encrypt",
                            dest="encrypt",
                            help='Set encryption mode. Default: False',
                            choices=['true','false'],
                            default='false',
                            type=str.lower)

        parser.add_argument("--keyfile",
                            dest="keyfile",
                            help='File having key for encryption (Applicable only if encryption mode is true)',
                            default = None)

        args = parser.parse_args()
        input_config_file = args.config_file
        input_values_file = args.values_file
        input_prefix = args.prefix
        input_fileid = args.fileid
        input_outdir = args.outdir
        input_cid = args.cid
        input_count = args.count
        part_size = args.part_size
        version = args.version
        input_is_keygen = args.keygen
        input_is_encrypt = args.encrypt
        input_is_keyfile = args.keyfile
        GENERATE_KEY_ONLY = False
        print_arg_str = "\nTo generate nvs partition binary with custom data --conf, --values, --prefix --cid, and --size arguments are mandatory.\
        \nTo generate nvs partition binary without any custom data --count, --prefix --cid, and --size arguments are mandatory.\
        \nTo generate encryption keys --keygen argument is mandatory."
        
        print_encrypt_arg_str = "Missing parameter. Enter --keyfile."

        if not part_size and input_is_keygen.lower() == 'true':
            if input_is_encrypt == 'true':
                sys.exit("Invalid.\nOnly --keyfile and --outdir arguments allowed.\n")
            GENERATE_KEY_ONLY = True
        
            nvs_partition_gen.check_input_args(input_config_file, input_values_file, part_size, input_is_keygen,\
                                               input_is_encrypt, input_is_keyfile, version, print_arg_str,
                                               print_encrypt_arg_str, input_outdir)
         
            nvs_partition_gen.nvs_part_gen(input_filename = input_config_file, output_filename = input_values_file,\
                                           input_part_size=part_size, is_key_gen=input_is_keygen,\
                                           encrypt_mode=input_is_encrypt, key_file=input_is_keyfile,\
                                           version_no=version, output_dir=input_outdir)
            sys.exit(0)
 
        # Verify that --cid argument and --prefix arguments are given
        if not input_cid or not input_prefix:
            parser.error(print_arg_str)

        # Verify that --values argument and --count argument are not given together
        if input_values_file and input_count:
            parser.error('--values and --count cannot be given together')
        
        # Verify if --fileid argument is given then --values argument should also be given
        if input_fileid and not input_values_file:
            parser.error('--values is needed as --fileid given')
        
        # Verify --count argument is given if --values file is not given
        if not input_config_file:
            if not input_values_file and not input_count:
                parser.error('--count or --values is needed')
        else:
            # Verify that --conf argument and --count argument are not given together
            if input_count:
                parser.error('--conf and --count cannot be given together')
            
            # Verify if --conf argument is given then --values argument should also be given
            if not input_values_file:
                parser.error('--values is needed as --conf given')
         
        # Add backslash to outdir if it is not present
        backslash = ['/','\\']
        if not any(ch in input_outdir for ch in backslash):
            input_outdir = os.path.join(args.outdir , '')

        # Set default value for fileid if --fileid argument is not given
        if not input_fileid:
            input_fileid = 'setup_code'

        if input_is_encrypt.lower() == 'true':
            if input_is_keygen.lower() == 'true' and input_is_keyfile:
                parser.error('Invalid. Cannot provide --keygen and --keyfile with --encrypt argument.')
            #else:
            #    parser.error('Invalid. --keyfile needed as --encrypt is True.')
            elif not (input_is_keygen.lower() == 'true' or input_is_keyfile):
                parser.error('Invalid. Must provide --keygen or --keyfile with --encrypt argument.')

        if input_count:
            nvs_partition_gen.check_input_args("None", "None", part_size, input_is_keygen,\
                input_is_encrypt, input_is_keyfile, version, print_arg_str, print_encrypt_arg_str, input_outdir)
        else:
            nvs_partition_gen.check_input_args(input_config_file, input_values_file, part_size, input_is_keygen,\
                input_is_encrypt, input_is_keyfile, version, print_arg_str, print_encrypt_arg_str, input_outdir)
        
        setup_id_created = []
        setup_code_created = []
        salt_created = []
        vkey_created = []
        keys_in_values_file = []
        keys_in_config_file = []
        values_file_keys = []
        homekit_data = []
        data_in_values_file = []
        csv_output_filenames = []
        setup_payload = b''
        setup_code = b''
        is_empty_line = False
        files_created = False
        total_value_count = 1
        hk_setup_info_dir = os.path.join('homekit_setup_info','')

        if input_config_file:
            # Verify config file is not empty
            if os.stat(input_config_file).st_size == 0:
                raise SystemExit("Oops...config file: %s is empty..." % input_config_file)
            
            # Verify config file does not have empty lines
            config_file = open(input_config_file,'r')
            config_file_reader = csv.reader(config_file, delimiter=',')
            for config_data in config_file_reader:
                for data in config_data:
                    empty_line = data.strip()
                    if empty_line is '':
                        is_empty_line = True
                    else:
                        is_empty_line = False
                        break
                if is_empty_line:
                    raise SystemExit("Oops...config file: %s cannot have empty lines..." % input_config_file)
                if not config_data:
                    raise SystemExit("Oops...config file: %s cannot have empty lines..." % str(input_config_file))
            config_file.close()
       
            # Extract keys from config file 
            config_file = open(input_config_file,'r')
            config_file_reader = csv.reader(config_file, delimiter=',')
            for config_data in config_file_reader:
                if not 'namespace' in config_data:
                    keys_in_config_file.append(config_data[0])
            config_file.close()

            # Verify data in the input config file 
            verify_data_in_file(input_config_file=input_config_file)
       
        is_empty_line = False

        if input_values_file:
            # Verify values file is not empty
            if os.stat(input_values_file).st_size == 0:
                raise SystemExit("Oops...values file: %s is empty..." % input_values_file)
            
            # Verify values file does not have empty lines 
            csv_values_file = open(input_values_file,'r')
            values_file_reader = csv.reader(csv_values_file, delimiter=',')
            for values_data in values_file_reader:
                for data in values_data:
                    empty_line = data.strip()
                    if empty_line is '':
                        is_empty_line = True
                    else:
                        is_empty_line = False
                        break
                if is_empty_line is True:
                    raise SystemExit("Oops...values file: %s cannot have empty lines..." % input_values_file)
                if not values_data:
                    raise SystemExit("Oops...values file: %s cannot have empty lines..." % str(input_values_file))
            csv_values_file.close()
            
            # Extract keys from values file 
            values_file = open(input_values_file,'r')
            values_file_reader = csv.reader(values_file, delimiter=',')
            keys_in_values_file = next(values_file_reader)
            values_file.close()

            values_file = open(input_values_file,'r')
            values_file_reader = csv.reader(values_file, delimiter=',')
            next(values_file_reader)
            
            # Verify file identifier exists in values file 
            if 'setup_code' not in input_fileid:
                if input_fileid not in keys_in_values_file:
                    raise SystemExit('Oops...target_file_id: %s does not exist in values file...\n' % input_fileid)
        
            # Verify data in the input values_file 
            verify_data_in_file(input_values_file=input_values_file, keys_in_values_file=keys_in_values_file)

            # Add data in values file to list
            for values in values_file_reader:
                data_in_values_file.append(values)
            
            # Get number of rows from values file which is equal to\
            # the number of accessories to create HomeKit data for
            total_value_count = len(data_in_values_file)

            values_file.close()


        if input_config_file and input_values_file:
            # Get keys from values file present in config files
            values_file_keys = get_keys(keys_in_values_file, keys_in_config_file)
            
            # Verify data in the input values_file 
            verify_data_in_file(input_config_file=input_config_file, keys_in_values_file=values_file_keys)
       
        if input_values_file:
            keys_in_values_file.extend(['setup_id','setup_code','setup_salt','setup_verifier','setup_payload'])
        else:
            keys_in_values_file = ['setup_id','setup_code','setup_salt','setup_verifier','setup_payload']
        
        # Generate homekit data
        if input_count:
            setup_id_created,setup_code_created,salt_created,vkey_created = homekit_data_gen(no_of_accessories=input_count)
        else:
            setup_id_created,setup_code_created,salt_created,vkey_created = homekit_data_gen(no_of_accessories=total_value_count)
       
        # Verify Accessory Category Identifier (cid)
        setup_info_gen.verify_cid(input_cid)
      
        # Generate setup payload and add it alongwith the homekit data generated to config and values file(if exist)\
        # if input config and input values file do not exist then new files are created
        if data_in_values_file:
            for (values, setup_id, setup_code, salt, vkey) in \
            list(zip_longest(data_in_values_file,setup_id_created,setup_code_created,\
            salt_created, vkey_created)):
                setup_payload,setup_code = setup_info_gen.setup_payload_create(input_cid,\
                setup_code=setup_code,setup_id=setup_id)
                values.extend([setup_id, setup_code, salt, vkey,setup_payload])
                homekit_data.append(values)
        else:
            for (setup_id, setup_code, salt, vkey) in \
            list(zip_longest(setup_id_created,setup_code_created,\
            salt_created, vkey_created)):
                setup_payload,setup_code = setup_info_gen.setup_payload_create(input_cid,\
                setup_code=setup_code,setup_id=setup_id)
                homekit_data.append([setup_id, setup_code, salt, vkey,setup_payload])


        target_config_file,target_values_file = add_homekit_data_to_file(input_config_file, \
        input_values_file, keys_in_values_file, homekit_data, input_outdir)

        # Generate csv and bin file
        csv_output_filenames, files_created, target_values_file = mfg_gen.main(input_config_file=target_config_file, input_values_file=target_values_file,\
                                                           target_file_name_prefix=input_prefix, file_identifier=input_fileid,\
                                                           output_dir_path=input_outdir, part_size=part_size,\
                                                           input_version=version, input_is_keygen=input_is_keygen,\
                                                           input_is_encrypt=input_is_encrypt ,input_is_keyfile=input_is_keyfile)

        print("HomeKit config data generated is added to file: " , target_config_file)
        print("HomeKit values data generated is added to file: " , target_values_file)
        
        if not files_created:
            raise SystemExit("Oops...csv,bin,setup info files could not be created...Please check your input files...")

        # Create new directory(if doesn't exist) to store homekit setup info generated
        output_target_dir = input_outdir + hk_setup_info_dir 
        if not os.path.isdir(output_target_dir):
            distutils.dir_util.mkpath(output_target_dir)

        # Write homekit setup info generated to output_target_file
        write_homekit_setup_info(target_values_file, output_target_dir, csv_output_filenames)


    except Exception as std_err:
        print(std_err)
    except:
        raise



if __name__ == "__main__":
    main()
