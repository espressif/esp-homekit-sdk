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

import sys
import os
import csv
import itertools
import argparse
import random
import string
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + "/../setup_payload_gen/")
import setup_payload_gen


def verify_cid(cid):
    """ Verify accessory category id is positive number less than 255 (1 byte long)
    """
    if not 1 <= cid <=255:
        raise SystemExit("Oops..(Accessory Category Identifier) cid should be between 1-255...\n")


def setup_id_gen(no_of_accessories):
    """ Generate setup id 
    """
    try:
        setup_id_created = []

        for x in range(no_of_accessories):
            setup_id = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(4))
            setup_id_created.append(setup_id)

        return setup_id_created

    except StandardError as std_err:
        print std_err
    except:
        raise    


def setup_code_gen(no_of_accessories):
    """ Generate setup code
    """
    try:
        invalid_setup_codes = ['00000000','11111111','22222222','33333333','44444444','55555555',\
                           '66666666','77777777','88888888','99999999','12345678','87654321'] 
   
        setup_code_created = []

        for _ in range(no_of_accessories):
       
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
                print "\nSetup code generated should be 8 numbers without any '-' in between. Eg. 11122333 \n"
                raise SystemExit(1)

            # Add the hyphen (-) in the PIN for salt-verifier generation. So, 11122333 will become 111-22-333
            setup_code = setup_code[:3] + '-' + setup_code[3:5] + '-' + setup_code[5:]

            setup_code_created.append(setup_code)
    
        return setup_code_created
    
    except StandardError as std_err:
        print std_err
    except:
        raise    


def write_setup_info_to_file(setup_code,setup_payload,output_txt_file):
    """ Write setup info (setup code and setup payload generated) to file
    """
    if os.path.isfile(output_txt_file):
        if os.stat(output_txt_file).st_size == 0:
            txt_file = open(output_txt_file,"w")
        else:
            txt_file = open(output_txt_file,"a")
    else:
        txt_file = open(output_txt_file,"w")
    txt_file.write("Setup Code: " + setup_code + "\n")
    txt_file.write("Setup Payload: " + setup_payload + "\n")
    txt_file.close()


def setup_payload_create(cid,setup_code=None,setup_id=None):
    """ Generate setup payload
    """
    try:
        s_code = setup_code.replace('-','')
        setup_payload = setup_payload_gen.main(cid=cid,setupCode=int(s_code),setupID=setup_id)
        return setup_payload,setup_code

    except StandardError as std_err:
        print std_err
    except:
        raise    

def main(cid=None,output_target_file=None,target_values_file=None):
    try:
        if all(arg is None for arg in [cid,output_target_file,target_values_file]):
            parser = argparse.ArgumentParser(prog='./setup_info_gen.py',
                                             description="Generate HomeKit Setup Info (Setup Code and Setup Payload)",\
                                             formatter_class=argparse.RawDescriptionHelpFormatter)
            
            parser.add_argument('--cid',
                                dest='cid',
                                type=int,
                                required=True,
                                help='the accessory category identifier')
            
            parser.add_argument('--outfile',
                                dest='filename',
                                required=True,
                                help='the output filename to write setup info generated')
            
            parser.add_argument('--count',
                                dest='count',
                                type=int,
                                required=True,
                                help='the number of accessories to create setup info for')
            
            parser.add_argument('--outdir',
                                dest='outdir',
                                default='./',
                                help='the output directory to store the file created\
                                (Default: current directory)')
                
        args = parser.parse_args()
        
        # Verify if output_dir_path argument is given then output directory exists
        if not os.path.isdir(args.outdir):
            parser.error('--outdir ' + args.outdir + ' does not exist...')

        # Add '/' to outdir if it is not present
        if not args.outdir.endswith('/'):
            args.outdir = args.outdir + '/'
        
        # Verify Accessory Category Identifier 
        verify_cid(args.cid)
       
        setup_id_generated = []
        setup_code_generated = []
       
        # Generate setup id for 'count' no. of accessories
        setup_id_generated = setup_id_gen(args.count)
        # Generate setup code for 'count' no. of accessories
        setup_code_generated = setup_code_gen(args.count)

        # Generate setup payload and write setup info (setup code and
        # setup payload generated) to output txt file
        for sid, scode in itertools.izip_longest(setup_id_generated, setup_code_generated):
            setup_payload, setup_code = setup_payload_create(args.cid, setup_code=scode, setup_id=sid)
            output_txt_file = args.outdir + args.filename + '-' + str(setup_code) + '.txt' 
            write_setup_info_to_file(setup_code,setup_payload,output_txt_file)
            print "File: " + output_txt_file + " created..."

    except StandardError as std_err:
        print std_err
    except:
        raise    



if __name__ == "__main__":
    main()
