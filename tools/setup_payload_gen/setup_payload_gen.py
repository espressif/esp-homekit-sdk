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

def base36encode(integer):
    chars, encoded = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ', ''

    while integer > 0:
        integer, remainder = divmod(integer, 36)
        encoded = chars[remainder] + encoded

    return encoded

def main(cid=None,setupCode=None,setupID=None):
    try:
        setup_payload_string = None
        if all(arg is None for arg in [cid,setupCode,setupID]):
            if len (sys.argv) != 4 :
                print "Usage: ./setup_payload_gen.py <AccessoryCategory> <setupCode> <SetupID>"
                print "Eg.: ./setup_payload_gen.py 7 51808582 7OSX"
                raise SystemExit(1)

            cid = int(sys.argv[1]);
            setupCode = int(sys.argv[2]);
            setupID = sys.argv[3]
            setup_payload_string = "Setup Payload: "

        nw_flags = 0xa; # For IP accessories supporting WAC. Use 0x4 for BLE
        VersionCategoryFlagsAndSetupCode = (cid << 31) | (nw_flags << 27) | setupCode
        VersionCategoryFlagsAndSetupCode = base36encode(VersionCategoryFlagsAndSetupCode)

        setup_payload = "X-HM://00" + VersionCategoryFlagsAndSetupCode + setupID
        print setup_payload

        if setup_payload_string:
            print setup_payload_string + setup_payload

        return setup_payload

    except StandardError as e:
        print e
    except:
        raise



if __name__ == "__main__":
    main()