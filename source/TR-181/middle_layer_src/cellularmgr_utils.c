/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cellularmgr_utils.h"
#include "cellularmgr_global.h"
#include "cap.h"
#include <stdlib.h>

extern cap_user appcaps;

#define POINTER_ASSERT(expr)                   \
  if (!(expr))                                 \
  {                                            \
    CcspTraceError(("[%s-%d] Invalid parameter error!!! \n",__FUNCTION__,__LINE__)); \
    return ANSC_STATUS_FAILURE;                         \
  }

int CellularMgr_Util_SendIPToWanMgr( CellularIPStruct *pstIPStruct )
{
    int sock   = -1;
    int sz_msg = 0;
    int bytes  = -1;

    if (pstIPStruct == NULL)
    {
        CcspTraceInfo(("%s - Invalid params received \n", __FUNCTION__));
        return RETURN_ERROR;
    }

    if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pstIPStruct->IPType )
    {
        int conn   = -1;
        ipc_msg_payload_t msg;

        sz_msg = sizeof(ipc_msg_payload_t);
        memset(&msg, 0, sizeof(ipc_msg_payload_t));

        // message header
        msg.data.dhcpv4.addressAssigned = TRUE;
        msg.msg_type = DHCPC_STATE_CHANGED;

        // message body
        strncpy(msg.data.dhcpv4.ip, pstIPStruct->IPAddress, sizeof(msg.data.dhcpv4.ip) - 1);
        strncpy(msg.data.dhcpv4.mask, pstIPStruct->SubnetMask, sizeof(msg.data.dhcpv4.mask) - 1);
        strncpy(msg.data.dhcpv4.gateway, pstIPStruct->DefaultGateWay, sizeof(msg.data.dhcpv4.gateway) - 1);
        strncpy(msg.data.dhcpv4.dnsServer, pstIPStruct->DNSServer1, sizeof(msg.data.dhcpv4.dnsServer) - 1);
        strncpy(msg.data.dhcpv4.dnsServer1, pstIPStruct->DNSServer2, sizeof(msg.data.dhcpv4.dnsServer1) - 1);
        strncpy(msg.data.dhcpv4.dhcpcInterface, pstIPStruct->WANIFName, sizeof(msg.data.dhcpv4.dhcpcInterface) - 1);
        msg.data.dhcpv4.mtuAssigned = (pstIPStruct->MTUSize != 0) ? TRUE : FALSE;
        msg.data.dhcpv4.mtuSize = pstIPStruct->MTUSize;

        sock = nn_socket(AF_SP, NN_PUSH);
        if (sock < 0)
        {
            CcspTraceInfo(("%s %d: Failed to create the socket , error = [%d][%s]\n", __FUNCTION__, __LINE__, errno, strerror(errno)));
            return RETURN_ERROR;
        }

        conn = nn_connect(sock, WAN_MANAGER_ADDR);
        if (conn < 0)
        {
            CcspTraceInfo(("%s %d: Failed to connect to the wanmanager [%s], error= [%d][%s] \n", __FUNCTION__, __LINE__, WAN_MANAGER_ADDR,errno, strerror(errno)));
            nn_close(sock);
            return RETURN_ERROR;
        }

        CcspTraceInfo(("%s %d: Connected to server socket [%s] \n", __FUNCTION__, __LINE__,WAN_MANAGER_ADDR));

        bytes = nn_send(sock, (char *) &msg, sz_msg, 0);
        if (bytes < 0)
        {
            CcspTraceInfo(("%s %d: Failed to send data to the wanmanager error=[%d][%s] \n", __FUNCTION__, __LINE__,errno, strerror(errno)));
            nn_close(sock);
            return RETURN_ERROR;
        }

        CcspTraceInfo(("%s %d: Successfully send %d bytes to wanmanager \n", __FUNCTION__, __LINE__, bytes));
        nn_close(sock);

    }
    else
    {
        // send ipv6 lease info to wanmanager via pandm

        // LTE sends us only the ipv6 address, so assigning the default value for rest of the values
#if defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE)
        char ifName[64] = {0};
        char * fifo_pattern = "dibbler-client add %s '%s' '1' '\\0' '\\0' '\\0' '\\0' '\\0' %d '1' '\\0' '\\0' '3600' '7200' ''";
        strncpy (ifName, pstIPStruct->WANIFName, sizeof(ifName) - 1);
#else
        char * fifo_pattern = "dibbler-client add '%s' '1' '\\0' '\\0' '\\0' '\\0' '\\0' %d '1' '\\0' '\\0' '3600' '7200' ''";
#endif
        char ipv6_addr [128] = {0};
        char temp [128] = {0};
        char buff [512] = {0};
        int pref_len=0;

        // copy the ipv6_addr to local buff
        strncpy (ipv6_addr, pstIPStruct->IPAddress, sizeof(ipv6_addr) - 1);
        strncpy (temp, ipv6_addr, sizeof(temp) - 1);
        char* token = NULL ;

        token = strtok(temp, "/");
        if (token != NULL) {
            token = strtok(NULL, "/");
            if (token != NULL) 
                pref_len = atoi(token);

        }
        // copy the ipv6_addr and string len to string pattern so reading side can parse it
#if defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE)
        snprintf(buff, sizeof (buff), fifo_pattern, ifName, ipv6_addr, pref_len);
#else
        snprintf(buff, sizeof (buff), fifo_pattern,ipv6_addr,pref_len);
#endif

        // open fifo
        sock = open(CCSP_COMMON_FIFO, O_WRONLY);
        if (sock < 0)
        {
            CcspTraceInfo(("%s %d: Failed to create the socket , error = [%d][%s]\n", __FUNCTION__, __LINE__, errno, strerror(errno)));
            return RETURN_ERROR;
        }

        // write data
        bytes = write(sock, buff, strlen(buff));
        if (bytes == 0)
        {
            CcspTraceInfo(("%s %d: Failed to write in fifo , error = [%d][%s]\n", __FUNCTION__, __LINE__, errno, strerror(errno)));
            close(sock);
            return RETURN_ERROR;
        }

        // close fifo
        close(sock);
    }

    return RETURN_OK;
}
