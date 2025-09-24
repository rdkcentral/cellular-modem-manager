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

#ifndef _CELLULARMGR_BUS_UTILS_H_
#define _CELLULARMGR_BUS_UTILS_H_

#include "cellularmgr_plugin_main_apis.h"
#include "dmsb_tr181_psm_definitions.h"
#include "cellularmgr_utils.h"
#include "ccsp_psm_helper.h"

#define DML_LTE_IFACE_PATH                "Device.Cellular.Interface.1"
#define WAN_DBUS_PATH                     "/com/cisco/spvtg/ccsp/wanmanager"
#define WAN_COMPONENT_NAME                "eRT.com.cisco.spvtg.ccsp.wanmanager"
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
#define DML_PARAM_VALUE_IPSOURCE_DHCP     "DHCP"
#define DML_PARAM_VALUE_IPSOURCE_STATIC   "Static"
#define DML_NO_OF_INTERFACE_ENTRIES       "Device.X_RDK_WanManager.InterfaceNumberOfEntries"
#define DML_IFTABLE_IFACE_NAME            "Device.X_RDK_WanManager.Interface.%d.Name"
#define DML_IFTABLE_IFACE_IPV4_SOURCE       "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.1.IP.IPv4Source"
#define DML_IFTABLE_IFACE_IPV6_SOURCE       "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.1.IP.IPv6Source"
#define DML_IFTABLE_IFACE_LINK_STATUS     "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.1.VlanStatus"
#define DML_IFTABLE_IFACE_WAN_NAME        "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.1.Name"
#define DML_IFTABLE_IFACE_PHY_STATUS      "Device.X_RDK_WanManager.Interface.%d.BaseInterfaceStatus"
#define DML_IFTABLE_IFACE_PHY_PATH        "Device.X_RDK_WanManager.Interface.%d.BaseInterface"
#else
#define DML_NO_OF_INTERFACE_ENTRIES       "Device.X_RDK_WanManager.CPEInterfaceNumberOfEntries"                      
#define DML_IFTABLE_IFACE_NAME            "Device.X_RDK_WanManager.CPEInterface.%d.Name"
#define DML_IFTABLE_IFACE_LINK_STATUS     "Device.X_RDK_WanManager.CPEInterface.%d.Wan.LinkStatus"
#define DML_IFTABLE_IFACE_WAN_NAME        "Device.X_RDK_WanManager.CPEInterface.%d.Wan.Name"
#define DML_IFTABLE_IFACE_PHY_STATUS      "Device.X_RDK_WanManager.CPEInterface.%d.Phy.Status"
#define DML_IFTABLE_IFACE_PHY_PATH        "Device.X_RDK_WanManager.CPEInterface.%d.Phy.Path"
#endif
#define UP_STR                            "Up"
#define DOWN_STR                          "Down"

int CellularMgr_RdkBus_GetParamValue ( char *pComponentName, char *pComponentPath, char *pParamName, char *pParamValue );
int CellularMgr_RdkBus_SetParamValue ( char *pComponentName, char *pComponentPath, char *pParamName, char *pParamValue, enum dataType_e type, bool bCommit);

int CellularMgr_RdkBus_SetParamValuesToDB( char *pParamName, char *pParamVal );
int CellularMgr_RdkBus_GetParamValuesFromDB( char *pParamName, char *pReturnVal, int returnValLength );

#endif
