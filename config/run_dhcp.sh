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

IFACE=$1
CMD=$2

# Usage:: ./run_dhcp.sh <interface name> <start/stop>
# eg:: ./run_dhcp.sh erouter0

UDHCPC_PID_FILE=/tmp/udhcpc.erouter0.pid
RUNDHCP_PID_FILE=/tmp/udhcpc.erouter0.rundhcp.pid

#------------------------------------------------------------------
# do_stop_dhcp
#------------------------------------------------------------------
do_stop_dhcp() {
   if [ -f "$UDHCPC_PID_FILE" ] ; then
      echo "Stopping UDHCPC"
      kill -USR2 `cat $UDHCPC_PID_FILE` && kill `cat $UDHCPC_PID_FILE`
      rm -f $UDHCPC_PID_FILE
   fi
}

#------------------------------------------------------------------
# do_start_dhcp
#------------------------------------------------------------------
do_start_dhcp() {

   if [ -f "$UDHCPC_PID_FILE" ] ; then
      do_stop_dhcp
   fi

   echo "Starting UDHCPC"   
   udhcpc -S -b -i $IFACE -p $UDHCPC_PID_FILE
}

#------------------------------------------------------------------
# do_wan_update
#------------------------------------------------------------------
do_wan_update() {
   redirection_flag=`syscfg get redirection_flag`
   echo "Redirection Flag before wanupdate :: ${redirection_flag}"
   if [ "$redirection_flag" = "true" ]; then
      echo "Resetting redirection_flag before wan-start"
      dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_CaptivePortalEnable bool false
   fi
   echo "Updating WAN with IP :: ${WAN_IPETH} MASK :: ${WAN_MASK}"
   sysevent set ipv4_wan_subnet $WAN_MASK
   sysevent set ipv4_wan_ipaddr $WAN_IPETH
   wan_status_update=`sysevent get wan-status` 
   if [ "x$wan_status_update" != "xstarted" ]; then 
       sysevent set current_ipv4_link_state up 
       sysevent set wan_service-status started 
       sysevent set wan-status started
       echo "Updated WAN Status"
   fi
   redirection_flag=`syscfg get redirection_flag`
   echo "Redirection Flag after wanupdate :: ${redirection_flag}"
   if [ "$redirection_flag" = "true" ]; then
      echo "Resetting redirection_flag after wan-start"
      dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_CaptivePortalEnable bool false
   fi
}

if [ "$CMD" = "start" ]; then
    script_pid=$$
    echo "$script_pid" > "$RUNDHCP_PID_FILE"
    while true
        do
        do_start_dhcp
        WAN_IPETH=$(ifconfig -a erouter0|grep -w inet |awk '{print $2}' | cut -c 6-)
        WAN_MASK=$(ifconfig -a erouter0|grep -w inet |awk '{print $4}' | cut -c 6-)

        if [ ! -z "$WAN_IPETH" ]
        then
            # Updated the LAN side DHCP Name server
            cat /etc/resolv.conf > /etc/resolv.dnsmasq

            do_wan_update
            echo "WAN IP is :: ${WAN_IPETH}"
            break
        fi
        echo "Retrying DHCP...."
    done
    
   sysevent set cellular_wan_v4_subnet $WAN_MASK
   sysevent set cellular_wan_v4_ip  $WAN_IPETH
 else
    do_stop_dhcp
    if [ -f "$RUNDHCP_PID_FILE" ] ; then
	kill -USR2 `cat $RUNDHCP_PID_FILE` && kill `cat $RUNDHCP_PID_FILE`
	rm -f $RUNDHCP_PID_FILE
    fi
    sysevent set cellular_wan_v4_ip "0.0.0.0"
    sysevent set cellular_wan_v4_subnet "0.0.0.0"
 fi

