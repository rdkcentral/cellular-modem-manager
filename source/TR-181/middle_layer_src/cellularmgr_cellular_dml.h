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

#ifndef  _CELLULAR_MGR_DML_H
#define  _CELLULAR_MGR_DML_H

/***********************************************************************
 APIs for Object:

    Cellular.

    *  Cellular_GetParamBoolValue
    *  Cellular_SetParamBoolValue
    *  Cellular_GetParamUlongValue
    *  Cellular_GetParamStringValue
    *  Cellular_SetParamStringValue
    *  Cellular_Validate
    *  Cellular_Commit
    *  Cellular_Rollback
    
***********************************************************************/
BOOL
Cellular_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char* ParamName,
        BOOL* pBool
    );

BOOL
Cellular_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

BOOL
Cellular_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    );
BOOL
Cellular_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
Cellular_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
Cellular_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );

BOOL
Cellular_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
Cellular_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
Cellular_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

/***********************************************************************

 APIs for Object:

    Cellular.X_RDK_DeviceManagement.

    *  DeviceManagement_GetParamStringValue
    *  DeviceManagement_GetParamBoolValue
    *  DeviceManagement_SetParamBoolValue

***********************************************************************/

ULONG
DeviceManagement_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
DeviceManagement_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
DeviceManagement_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

/***********************************************************************

 APIs for Object:

    Cellular.X_RDK_Firmware.

    *  Firmware_GetParamStringValue

***********************************************************************/

ULONG
Firmware_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.

    *  Cellular_Interface_GetEntryCount
    *  Cellular_Interface_GetEntry
    *  Cellular_Interface_GetParamBoolValue
    *  Cellular_Interface_SetParamBoolValue
    *  Cellular_Interface_GetParamUlongValue
    *  Cellular_Interface_GetParamStringValue
    *  Cellular_Interface_SetParamStringValue
    *  Cellular_Interface_Validate
    *  Cellular_Interface_Commit
    *  Cellular_Interface_Rollback

***********************************************************************/
ULONG
Cellular_Interface_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
Cellular_Interface_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
Cellular_Interface_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
Cellular_Interface_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

BOOL
Cellular_Interface_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

BOOL
Cellular_Interface_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    );
    
ULONG
Cellular_Interface_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
Cellular_Interface_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );

BOOL
Cellular_Interface_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
Cellular_Interface_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
Cellular_Interface_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_Identification.

    *  Identification_GetParamStringValue

***********************************************************************/

ULONG
Identification_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_PlmnAccess.

    *  PlmnAccess_GetParamUlongValue
    *  PlmnAccess_GetParamBoolValue
    *  HomeNetwork_GetParamStringValue
    *  NetworkInUse_GetParamStringValue
    
***********************************************************************/
BOOL
PlmnAccess_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

BOOL
PlmnAccess_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

ULONG
HomeNetwork_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

ULONG
NetworkInUse_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_PlmnAccess.AvailableNetworks.{i}.

    *  AvailableNetworks_IsUpdated
    *  AvailableNetworks_Synchronize
    *  AvailableNetworks_GetEntryCount
    *  AvailableNetworks_GetEntry
    *  AvailableNetworks_GetParamBoolValue
    *  AvailableNetworks_GetParamStringValue

***********************************************************************/

BOOL
AvailableNetworks_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
AvailableNetworks_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
AvailableNetworks_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
AvailableNetworks_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
AvailableNetworks_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

ULONG
AvailableNetworks_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_RadioSignal.

    *  RadioSignal_GetParamUlongValue
    *  RadioSignal_GetParamIntValue
    *  RadioSignal_GetParamStringValue

***********************************************************************/
BOOL
RadioSignal_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
RadioSignal_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
RadioSignal_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.

    *  NeighborCell_IsUpdated
    *  NeighborCell_Synchronize
    *  NeighborCell_GetEntryCount
    *  NeighborCell_GetEntry
    *  NeighborCell_GetParamUlongValue
    *  NeighborCell_GetParamIntValue
    *  NeighborCell_GetParamStringValue

***********************************************************************/

BOOL
NeighborCell_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
NeighborCell_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
NeighborCell_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
NeighborCell_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
NeighborCell_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

BOOL
NeighborCell_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    );

ULONG
NeighborCell_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.

    *  ContextProfile_IsUpdated
    *  ContextProfile_Synchronize
    *  ContextProfile_GetEntryCount
    *  ContextProfile_GetEntry
    *  ContextProfile_GetParamUlongValue
    *  ContextProfile_GetParamStringValue

***********************************************************************/

BOOL
ContextProfile_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
ContextProfile_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
ContextProfile_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
ContextProfile_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
ContextProfile_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
ContextProfile_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_Statistics.

    *  X_RDK_Statistics_GetParamUlongValue

***********************************************************************/
BOOL
X_RDK_Statistics_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

/***********************************************************************

 APIs for Object:

    Cellular.X_RDK_Uicc.{i}.

    *  UiccSlot_IsUpdated
    *  UiccSlot_Synchronize
    *  UiccSlot_GetEntryCount
    *  UiccSlot_GetEntry
    *  UiccSlot_GetParamBoolValue
    *  UiccSlot_SetParamBoolValue
    *  UiccSlot_GetParamUlongValue
    *  UiccSlot_GetParamStringValue
    *  UiccSlot_Validate
    *  UiccSlot_Commit
    *  UiccSlot_Rollback

***********************************************************************/
BOOL
UiccSlot_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
UiccSlot_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
UiccSlot_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
UiccSlot_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
UiccSlot_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
UiccSlot_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

BOOL
UiccSlot_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
UiccSlot_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
UiccSlot_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
UiccSlot_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
UiccSlot_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

/***********************************************************************

 APIs for Object:

    Cellular.X_RDK_EuiccSlot.

    *  EuiccSlot_GetParamBoolValue
    *  EuiccSlot_SetParamBoolValue
    *  EuiccSlot_GetParamUlongValue
    *  EuiccSlot_GetParamStringValue
    *  EuiccSlot_Validate
    *  EuiccSlot_Commit
    *  EuiccSlot_Rollback

***********************************************************************/
BOOL
EuiccSlot_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
EuiccSlot_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

BOOL
EuiccSlot_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

ULONG
EuiccSlot_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
EuiccSlot_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
EuiccSlot_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
EuiccSlot_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

/***********************************************************************

 APIs for Object:

    Cellular.X_RDK_EuiccSlot.MnoProfile.{i}.

    *  MnoProfile_IsUpdated
    *  MnoProfile_Synchronize
    *  MnoProfile_GetEntryCount
    *  MnoProfile_GetEntry
    *  MnoProfile_GetParamBoolValue
    *  MnoProfile_GetParamStringValue
    *  MnoProfile_GetParamUlongValue

***********************************************************************/
BOOL
MnoProfile_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
MnoProfile_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
MnoProfile_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
MnoProfile_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
MnoProfile_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

ULONG
MnoProfile_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
MnoProfile_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

/***********************************************************************

 APIs for Object:

    Cellular.AccessPoint.{i}.

    *  Cellular_AccessPoint_AddEntry
    *  Cellular_AccessPoint_DeleteEntry
    *  Cellular_AccessPoint_IsUpdated
    *  Cellular_AccessPoint_Synchronize
    *  Cellular_AccessPoint_GetEntryCount
    *  Cellular_AccessPoint_GetEntry
    *  Cellular_AccessPoint_GetParamBoolValue
    *  Cellular_AccessPoint_SetParamBoolValue
    *  Cellular_AccessPoint_GetParamStringValue
    *  Cellular_AccessPoint_SetParamStringValue
    *  Cellular_AccessPoint_GetParamUlongValue
    *  Cellular_AccessPoint_SetParamUlongValue
    *  Cellular_AccessPoint_Validate
    *  Cellular_AccessPoint_Commit
    *  Cellular_AccessPoint_Rollback

***********************************************************************/

ANSC_HANDLE 
Cellular_AccessPoint_AddEntry
    (
        ANSC_HANDLE hInsContext, 
        ULONG* pInsNumber
    );

ULONG 
Cellular_AccessPoint_DeleteEntry
    (
        ANSC_HANDLE hInsContext, 
        ANSC_HANDLE hInstance
    );

BOOL
Cellular_AccessPoint_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
Cellular_AccessPoint_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
Cellular_AccessPoint_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    );

ANSC_HANDLE
Cellular_AccessPoint_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    );

BOOL
Cellular_AccessPoint_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    );

BOOL
Cellular_AccessPoint_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

ULONG
Cellular_AccessPoint_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    );

BOOL
Cellular_AccessPoint_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    );

BOOL
Cellular_AccessPoint_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    );

BOOL
Cellular_AccessPoint_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    );

BOOL
Cellular_AccessPoint_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    );

ULONG
Cellular_AccessPoint_Commit
    (
        ANSC_HANDLE                 hInsContext
    );

ULONG
Cellular_AccessPoint_Rollback
    (
        ANSC_HANDLE                 hInsContext
    );

#endif
