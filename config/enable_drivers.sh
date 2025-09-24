# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
#
# Copyright 2023 Deutsche Telekom AG.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#########################################################################

#########################################################################
# 1. Run usbmuxd utility
#########################################################################
usbmuxd
##########################################################################
# Insert the below set of drivers, these drivers are required to support
# 1. RNDIS Devices
# 2. CDC ETHER Devices
# 3. HUAWEI MODEM (NCM)
# 4. IPhone (IPHETH)
# 5. MMBIM Devices
# 6. QMI Devices
##########################################################################

drivers=("usbnet" "cdc_ether" "ax88179_178a" "rndis_host" "cdc-wdm" "cdc_ncm" "cdc_mbim" "qmi_wwan" "huawei_cdc_ncm" "usbserial" "usb_wwan" "option" "ipheth")

for str in ${drivers[@]}; do
        modprobe $str
done
