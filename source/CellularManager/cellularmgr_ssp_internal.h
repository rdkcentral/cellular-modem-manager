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
 * Copyright 2022 RDK Management
 * Licensed under the Apache License, Version 2.0
 */
/*
 * Copyright [2014] [Cisco Systems, Inc.]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     LICENSE-2.0" target="_blank" rel="nofollow">http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef  _CELLULAR_MANAGER_SSP_INTERNAL_H_
#define  _CELLULAR_MANAGER_SSP_INTERNAL_H_

#include "cellularmgr_plugin_main_apis.h"

#define  RDK_COMPONENT_ID_CELLULAR_MANAGER                             "com.cisco.spvtg.ccsp.cellularmanager"
#define  RDK_COMPONENT_NAME_CELLULAR_MANAGER                           "com.cisco.spvtg.ccsp.cellularmanager"
#define  RDK_COMPONENT_VERSION_CELLULAR_MANAGER                        1
#define  RDK_COMPONENT_PATH_CELLULAR_MANAGER                           "/com/cisco/spvtg/ccsp/cellularmanager"

#define  RDK_COMMON_COMPONENT_HEALTH_Red                   1
#define  RDK_COMMON_COMPONENT_HEALTH_Yellow                2
#define  RDK_COMMON_COMPONENT_HEALTH_Green                 3

#define  RDK_COMMON_COMPONENT_STATE_Initializing           1
#define  RDK_COMMON_COMPONENT_STATE_Running                2
#define  RDK_COMMON_COMPONENT_STATE_Blocked                3
#define  RDK_COMMON_COMPONENT_STATE_Paused                 3

typedef  struct
_COMPONENT_COMMON_CELLULAR_MANAGER
{
    char*                           Name;
    ULONG                           Version;
    char*                           Author;
    ULONG                           Health;
    ULONG                           State;

    BOOL                            LogEnable;
    ULONG                           LogLevel;

    ULONG                           MemMaxUsage;
    ULONG                           MemMinUsage;
    ULONG                           MemConsumed;

}
COMPONENT_COMMON_CELLULAR_MANAGER, *PCOMPONENT_COMMON_CELLULAR_MANAGER;

#define ComponentCommonDmInit(component_com_cellularmanager)                                          \
        {                                                                                             \
            AnscZeroMemory(component_com_cellularmanager, sizeof(COMPONENT_COMMON_CELLULAR_MANAGER)); \
            component_com_cellularmanager->Name        = NULL;                                        \
            component_com_cellularmanager->Version     = 1;                                           \
            component_com_cellularmanager->Author      = "SKY";                                        \
            component_com_cellularmanager->Health      = RDK_COMMON_COMPONENT_HEALTH_Red;            \
            component_com_cellularmanager->State       = RDK_COMMON_COMPONENT_STATE_Running;         \
            if(g_iTraceLevel >= CCSP_TRACE_LEVEL_EMERGENCY)                                           \
                component_com_cellularmanager->LogLevel = (ULONG) g_iTraceLevel;                      \
            component_com_cellularmanager->LogEnable   = TRUE;                                        \
            component_com_cellularmanager->MemMaxUsage = 0;                                           \
            component_com_cellularmanager->MemMinUsage = 0;                                           \
            component_com_cellularmanager->MemConsumed = 0;                                           \
        }

ANSC_STATUS
CellularMgr_Init
(
);

ANSC_STATUS
CellularMgr_RegisterComponent
(
);

ANSC_STATUS
CellularMgr_Term
(
);

char*
CellularMgr_GetComponentName
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
CellularMgr_GetComponentVersion
    (
        ANSC_HANDLE                     hThisObject
    );

char*
CellularMgr_GetComponentAuthor
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
CellularMgr_GetComponentHealth
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
CellularMgr_GetComponentState
    (
        ANSC_HANDLE                     hThisObject
    );

BOOL
CellularMgr_GetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject
    );

ANSC_STATUS
CellularMgr_SetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject,
        BOOL                            bEnabled
    );

ULONG
CellularMgr_GetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject
    );

ANSC_STATUS
CellularMgr_SetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject,
        ULONG                           LogLevel
    );

ULONG
CellularMgr_GetMemMaxUsage
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
CellularMgr_GetMemMinUsage
    (
        ANSC_HANDLE                     hThisObject
    );

ULONG
CellularMgr_GetMemConsumed
    (
        ANSC_HANDLE                     hThisObject
    );

ANSC_STATUS
CellularMgr_ApplyChanges
    (
        ANSC_HANDLE                     hThisObject
    );

int
CellularMgr_DMLInit
(
    ULONG                       uMaxVersionSupported,
    void*                       hCosaPlugInfo
);
#endif
