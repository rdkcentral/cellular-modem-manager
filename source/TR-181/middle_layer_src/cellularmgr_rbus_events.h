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
/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef RBUS_BUILD_FLAG_ENABLE

#ifndef BUS_CONTEXT_EVENTS_H
#define BUS_CONTEXT_EVENTS_H

#include <rbus.h>
#include <stdio.h>
#include "cellularmgr_global.h"
#include "cellularmgr_cellular_apis.h"
#include "cellularmgr_cellular_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CELLULARMGR_INFACE_RADIOSIGNAL_RADIOENVCONDITIONS  "Device.Cellular.Interface.%d.X_RDK_RadioEnvConditions"
#define CELLULARMGR_INFACE_RADIOSIGNAL_RSSI                "Device.Cellular.Interface.%d.RSSI"
#define CELLULARMGR_INFACE_RADIOSIGNAL_SNR                 "Device.Cellular.Interface.%d.X_RDK_SNR"
#define CELLULARMGR_INFACE_RADIOSIGNAL_RSRP                "Device.Cellular.Interface.%d.RSRP"
#define CELLULARMGR_INFACE_RADIOSIGNAL_RSRQ                "Device.Cellular.Interface.%d.RSRQ"
#define CELLULARMGR_INFACE_RADIOSIGNAL_TRX                 "Device.Cellular.Interface.%d.X_RDK_TRX"
#define CELLULARMGR_X_RDK_ENABLE                           "Device.Cellular.X_RDK_Enable"
#define CELLULARMGR_X_RDK_STATUS                           "Device.Cellular.X_RDK_Status"
#define CELLULARMGR_INFACE_STATUS                          "Device.Cellular.Interface.%d.Status"
#define CELLULARMGR_INFACE_PHY_CONNECTION_STATUS           "Device.Cellular.Interface.%d.X_RDK_PhyConnectedStatus"
#define CELLULARMGR_INFACE_LINK_AVAILABLE_STATUS           "Device.Cellular.Interface.%d.X_RDK_LinkAvailableStatus"

#define  RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL         (10)

typedef enum _CellularMGR_EnumToString{
    ENUM_RADIOENVCONDITIONS = 1,
    ENUM_X_RDK_STATUS
}CellularMGR_EnumToString;

typedef  struct                                      
{
    int RadioEnvCondSubFlag;
    unsigned int RadioEnvCondSubInterval;
    unsigned int RadioEnvCondLapsedCounterTimer;
    int RSSISubFlag;
    unsigned int RSSISubInterval;
    unsigned int RSSILapsedCounterTimer;
    int SNRSubFlag;
    unsigned int SNRSubInterval;
    unsigned int SNRLapsedCounterTimer;
    int RSRPSubFlag;
    unsigned int RSRPSubInterval;
    unsigned int RSRPLapsedCounterTimer;
    int RSRQSubFlag;
    unsigned int RSRQSubInterval;
    unsigned int RSRQLapsedCounterTimer;
    int TRXSubFlag;
    unsigned int TRXSubInterval;
    unsigned int TRXLapsedCounterTimer;
    
}CellularMGR_rbusSubListForRadioSignalSt;

typedef  struct                                      
{
    int InterfaceStatusSubFlag;
    int PhyConnectedStatusSubFlag;
    int LinkAvailableStatusSubFlag;
    
}CellularMGR_rbusSubListForInterfaceSt;

typedef  struct                                      
{
    int X_RDK_EnableSubFlag;
    int X_RDK_StatusSubFlag;
    CellularMGR_rbusSubListForRadioSignalSt stRadioSignal;
    CellularMGR_rbusSubListForInterfaceSt stInterface;
    
}CellularMGR_rbusSubListSt;

void* CellularMgr_RBUS_Events_Monitor_Thread( void *arg );
ANSC_STATUS CellularMgr_Rbus_String_EventPublish_OnValueChange(char *dm_event, void *prev_dm_value, void *dm_value, rbusValueType_t rbus_type);
rbusError_t CellularMgrDmlPublishEventHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish);
int CellularMgr_RBUS_Events_PublishLinkAvailableStatus( unsigned char bPrevLinkState, unsigned char bCurrentLinkState );
int CellularMgr_RBUS_Events_PublishPhyConnectionStatus( unsigned char bPrevPhyState, unsigned char bCurrentPhyState );
int CellularMgr_RBUS_Events_Publish_X_RDK_Enable( unsigned char bPreviousValue, unsigned char bCurrentValue );
int CellularMgr_RBUS_Events_PublishInterfaceStatus( CellularInterfaceStatus_t PrevState, CellularInterfaceStatus_t CurrentState );

#ifdef __cplusplus
}
#endif

#endif

#endif /*RBUS_BUILD_FLAG_ENABLE*/
