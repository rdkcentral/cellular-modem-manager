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


#include "cellularmgr_plugin_main.h"
#include "cellularmgr_plugin_main_apis.h"
#include "cellularmgr_cellular_dml.h"
#include "cellularmgr_global.h"


void *                                g_pDslhDmlAgent;
COSAGetParamValueByPathNameProc       g_GetParamValueByPathNameProc;
COSASetParamValueByPathNameProc       g_SetParamValueByPathNameProc;
COSAGetParamValueStringProc           g_GetParamValueString;
COSAGetParamValueUlongProc            g_GetParamValueUlong;
COSAGetParamValueIntProc              g_GetParamValueInt;
COSAGetParamValueBoolProc             g_GetParamValueBool;
COSASetParamValueStringProc           g_SetParamValueString;
COSASetParamValueUlongProc            g_SetParamValueUlong;
COSASetParamValueIntProc              g_SetParamValueInt;
COSASetParamValueBoolProc             g_SetParamValueBool;
COSAGetInstanceNumbersProc            g_GetInstanceNumbers;
COSAValidateHierarchyInterfaceProc    g_ValidateInterface;
COSAGetHandleProc                     g_GetRegistryRootFolder;
COSAGetInstanceNumberByIndexProc      g_GetInstanceNumberByIndex;
COSAGetInterfaceByNameProc            g_GetInterfaceByName;
COSAGetHandleProc                     g_GetMessageBusHandle;
COSAGetSubsystemPrefixProc            g_GetSubsystemPrefix;
PCCSP_CCD_INTERFACE                   g_pCellularCcdIf;
ANSC_HANDLE                           g_MessageBusHandle;
char*                                 g_SubsystemPrefix;
COSARegisterCallBackAfterInitDmlProc  g_RegisterCallBackAfterInitDml;
COSARepopulateTableProc               g_COSARepopulateTable;
PBACKEND_MANAGER_OBJECT               g_pBEManager;
extern ANSC_HANDLE                    g_MessageBusHandle_Irep;
extern char                           g_SubSysPrefix_Irep[32];

int ANSC_EXPORT_API CellularMgr_DMLInit
    (
        ULONG                       uMaxVersionSupported,
        void*                       hCosaPlugInfo         /* PCOSA_PLUGIN_INFO passed in by the caller */
    )
{
    PCOSA_PLUGIN_INFO               pPlugInfo  = (PCOSA_PLUGIN_INFO)hCosaPlugInfo;

    COSAGetParamValueByPathNameProc pGetParamValueByPathNameProc = (COSAGetParamValueByPathNameProc)NULL;
    COSASetParamValueByPathNameProc pSetParamValueByPathNameProc = (COSASetParamValueByPathNameProc)NULL;
    COSAGetParamValueStringProc     pGetStringProc              = (COSAGetParamValueStringProc       )NULL;
    COSAGetParamValueUlongProc      pGetParamValueUlongProc     = (COSAGetParamValueUlongProc        )NULL;
    COSAGetParamValueIntProc        pGetParamValueIntProc       = (COSAGetParamValueIntProc          )NULL;
    COSAGetParamValueBoolProc       pGetParamValueBoolProc      = (COSAGetParamValueBoolProc         )NULL;
    COSASetParamValueStringProc     pSetStringProc              = (COSASetParamValueStringProc       )NULL;
    COSASetParamValueUlongProc      pSetParamValueUlongProc     = (COSASetParamValueUlongProc        )NULL;
    COSASetParamValueIntProc        pSetParamValueIntProc       = (COSASetParamValueIntProc          )NULL;
    COSASetParamValueBoolProc       pSetParamValueBoolProc      = (COSASetParamValueBoolProc         )NULL;
    COSAGetInstanceNumbersProc      pGetInstanceNumbersProc     = (COSAGetInstanceNumbersProc        )NULL;

    COSAGetCommonHandleProc         pGetCHProc                  = (COSAGetCommonHandleProc           )NULL;
    COSAValidateHierarchyInterfaceProc
                                    pValInterfaceProc           = (COSAValidateHierarchyInterfaceProc)NULL;
    COSAGetHandleProc               pGetRegistryRootFolder      = (COSAGetHandleProc                 )NULL;
    COSAGetInstanceNumberByIndexProc
                                    pGetInsNumberByIndexProc    = (COSAGetInstanceNumberByIndexProc  )NULL;
    COSAGetHandleProc               pGetMessageBusHandleProc    = (COSAGetHandleProc                 )NULL;
    COSAGetInterfaceByNameProc      pGetInterfaceByNameProc     = (COSAGetInterfaceByNameProc        )NULL;
    ULONG                           ret                         = 0;
    int        rc = -1;

    if ( uMaxVersionSupported < THIS_PLUGIN_VERSION )
    {
      /* this version is not supported */
        return -1;
    }

    pPlugInfo->uPluginVersion       = THIS_PLUGIN_VERSION;
#ifndef RBUS_BUILD_FLAG_ENABLE
    /* Register the back-end apis for the data model */
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_GetParamBoolValue",  Cellular_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_SetParamBoolValue",  Cellular_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_GetParamUlongValue",  Cellular_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_GetParamStringValue",  Cellular_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_SetParamStringValue",  Cellular_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Validate",  Cellular_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Commit",  Cellular_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Rollback",  Cellular_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DeviceManagement_GetParamStringValue",  DeviceManagement_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DeviceManagement_GetParamBoolValue",  DeviceManagement_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DeviceManagement_SetParamBoolValue",  DeviceManagement_SetParamBoolValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_GetEntryCount",  Cellular_Interface_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_GetEntry",  Cellular_Interface_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_GetParamBoolValue",  Cellular_Interface_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_SetParamBoolValue",  Cellular_Interface_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_SetParamUlongValue",  Cellular_Interface_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_GetParamUlongValue",  Cellular_Interface_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_GetParamStringValue",  Cellular_Interface_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_SetParamStringValue",  Cellular_Interface_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_Validate",  Cellular_Interface_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_Commit",  Cellular_Interface_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_Interface_Rollback",  Cellular_Interface_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Identification_GetParamStringValue",  Identification_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Firmware_GetParamStringValue",  Firmware_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "PlmnAccess_GetParamBoolValue",  PlmnAccess_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "PlmnAccess_GetParamUlongValue",  PlmnAccess_GetParamUlongValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "HomeNetwork_GetParamStringValue",  HomeNetwork_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NetworkInUse_GetParamStringValue",  NetworkInUse_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "AvailableNetworks_GetEntryCount",  AvailableNetworks_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "AvailableNetworks_GetEntry",  AvailableNetworks_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "AvailableNetworks_IsUpdated",  AvailableNetworks_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "AvailableNetworks_Synchronize",  AvailableNetworks_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "AvailableNetworks_GetParamBoolValue",  AvailableNetworks_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "AvailableNetworks_GetParamStringValue",  AvailableNetworks_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "RadioSignal_GetParamUlongValue",  RadioSignal_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "RadioSignal_GetParamIntValue",  RadioSignal_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "RadioSignal_GetParamStringValue",  RadioSignal_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NeighborCell_GetEntryCount",  NeighborCell_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NeighborCell_GetEntry",  NeighborCell_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NeighborCell_IsUpdated",  NeighborCell_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NeighborCell_Synchronize",  NeighborCell_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NeighborCell_GetParamUlongValue",  NeighborCell_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NeighborCell_GetParamIntValue",  NeighborCell_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "NeighborCell_GetParamStringValue",  NeighborCell_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ContextProfile_GetEntryCount",  ContextProfile_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ContextProfile_GetEntry",  ContextProfile_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ContextProfile_IsUpdated",  ContextProfile_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ContextProfile_Synchronize",  ContextProfile_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ContextProfile_GetParamUlongValue",  ContextProfile_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "ContextProfile_GetParamStringValue",  ContextProfile_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "X_RDK_Statistics_GetParamUlongValue",  X_RDK_Statistics_GetParamUlongValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_IsUpdated",  UiccSlot_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_Synchronize",  UiccSlot_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_GetEntryCount",  UiccSlot_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_GetEntry",  UiccSlot_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_GetParamBoolValue",  UiccSlot_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_SetParamBoolValue",  UiccSlot_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_GetParamStringValue",  UiccSlot_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_GetParamUlongValue",  UiccSlot_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_Validate",  UiccSlot_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_Commit",  UiccSlot_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "UiccSlot_Rollback",  UiccSlot_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "EuiccSlot_GetParamBoolValue",  EuiccSlot_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "EuiccSlot_SetParamBoolValue",  EuiccSlot_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "EuiccSlot_GetParamStringValue",  EuiccSlot_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "EuiccSlot_GetParamUlongValue",  EuiccSlot_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "EuiccSlot_Validate",  EuiccSlot_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "EuiccSlot_Commit",  EuiccSlot_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "EuiccSlot_Rollback",  EuiccSlot_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "MnoProfile_IsUpdated",  MnoProfile_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "MnoProfile_Synchronize",  MnoProfile_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "MnoProfile_GetEntryCount",  MnoProfile_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "MnoProfile_GetEntry",  MnoProfile_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "MnoProfile_GetParamBoolValue",  MnoProfile_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "MnoProfile_GetParamStringValue",  MnoProfile_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "MnoProfile_GetParamUlongValue",  MnoProfile_GetParamUlongValue);
    
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_AddEntry",  Cellular_AccessPoint_AddEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_DeleteEntry",  Cellular_AccessPoint_DeleteEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_IsUpdated",  Cellular_AccessPoint_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_Synchronize",  Cellular_AccessPoint_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_GetEntryCount",  Cellular_AccessPoint_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_GetEntry",  Cellular_AccessPoint_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_GetParamBoolValue",  Cellular_AccessPoint_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_SetParamBoolValue",  Cellular_AccessPoint_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_GetParamUlongValue",  Cellular_AccessPoint_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_SetParamUlongValue",  Cellular_AccessPoint_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_GetParamStringValue",  Cellular_AccessPoint_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_SetParamStringValue",  Cellular_AccessPoint_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_Validate",  Cellular_AccessPoint_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_Commit",  Cellular_AccessPoint_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Cellular_AccessPoint_Rollback",  Cellular_AccessPoint_Rollback);
#endif
    g_pDslhDmlAgent                 = pPlugInfo->hDmlAgent;
    pGetParamValueByPathNameProc = (COSAGetParamValueByPathNameProc)pPlugInfo->AcquireFunction("COSAGetParamValueByPathName");
    if( pGetParamValueByPathNameProc != NULL)
    {
        g_GetParamValueByPathNameProc = pGetParamValueByPathNameProc;
    }
    else
    {
        goto EXIT;
    }
    pSetParamValueByPathNameProc = (COSASetParamValueByPathNameProc)pPlugInfo->AcquireFunction("COSASetParamValueByPathName");

    if( pSetParamValueByPathNameProc != NULL)
    {
        g_SetParamValueByPathNameProc = pSetParamValueByPathNameProc;
    }
    else
    {
        goto EXIT;
    }
    pGetStringProc = (COSAGetParamValueStringProc)pPlugInfo->AcquireFunction("COSAGetParamValueString");

    if( pGetStringProc != NULL)
    {
        g_GetParamValueString = pGetStringProc;
    }
    else
    {
        goto EXIT;
    }
    pGetParamValueUlongProc = (COSAGetParamValueUlongProc)pPlugInfo->AcquireFunction("COSAGetParamValueUlong");

    if( pGetParamValueUlongProc != NULL)
    {
        g_GetParamValueUlong = pGetParamValueUlongProc;
    }
    else
    {
        goto EXIT;
    }

    pGetParamValueIntProc = (COSAGetParamValueUlongProc)pPlugInfo->AcquireFunction("COSAGetParamValueInt");

    if( pGetParamValueIntProc != NULL)
    {
        g_GetParamValueInt = pGetParamValueIntProc;
    }
    else
    {
        goto EXIT;
    }
    pGetParamValueBoolProc = (COSAGetParamValueBoolProc)pPlugInfo->AcquireFunction("COSAGetParamValueBool");

    if( pGetParamValueBoolProc != NULL)
    {
        g_GetParamValueBool = pGetParamValueBoolProc;
    }
    else
    {
        goto EXIT;
    }
    pSetStringProc = (COSASetParamValueStringProc)pPlugInfo->AcquireFunction("COSASetParamValueString");

    if( pSetStringProc != NULL)
    {
        g_SetParamValueString = pSetStringProc;
    }
    else
    {
        goto EXIT;
    }
    pSetParamValueUlongProc = (COSASetParamValueUlongProc)pPlugInfo->AcquireFunction("COSASetParamValueUlong");

    if( pSetParamValueUlongProc != NULL)
    {
        g_SetParamValueUlong = pSetParamValueUlongProc;
    }
    else
    {
        goto EXIT;
    }

    pSetParamValueIntProc = (COSASetParamValueIntProc)pPlugInfo->AcquireFunction("COSASetParamValueInt");

    if( pSetParamValueIntProc != NULL)
    {
        g_SetParamValueInt = pSetParamValueIntProc;
    }
    else
    {
        goto EXIT;
    }
    pSetParamValueBoolProc = (COSASetParamValueBoolProc)pPlugInfo->AcquireFunction("COSASetParamValueBool");

    if( pSetParamValueBoolProc != NULL)
    {
        g_SetParamValueBool = pSetParamValueBoolProc;
    }
    else
    {
        goto EXIT;
    }
    pGetInstanceNumbersProc = (COSAGetInstanceNumbersProc)pPlugInfo->AcquireFunction("COSAGetInstanceNumbers");

    if( pGetInstanceNumbersProc != NULL)
    {
        g_GetInstanceNumbers = pGetInstanceNumbersProc;
    }
    else
    {
        goto EXIT;
    }
    pValInterfaceProc = (COSAValidateHierarchyInterfaceProc)pPlugInfo->AcquireFunction("COSAValidateHierarchyInterface");

    if ( pValInterfaceProc )
    {
        g_ValidateInterface = pValInterfaceProc;
    }
    else
    {
        goto EXIT;
    }
    pGetRegistryRootFolder = (COSAGetHandleProc)pPlugInfo->AcquireFunction("COSAGetRegistryRootFolder");

    if ( pGetRegistryRootFolder != NULL )
    {
        g_GetRegistryRootFolder = pGetRegistryRootFolder;
    }
    else
    {
        goto EXIT;
    }
    pGetInsNumberByIndexProc = (COSAGetInstanceNumberByIndexProc)pPlugInfo->AcquireFunction("COSAGetInstanceNumberByIndex");

    if ( pGetInsNumberByIndexProc != NULL )
    {
        g_GetInstanceNumberByIndex = pGetInsNumberByIndexProc;
    }
    else
    {
        goto EXIT;
    }
    pGetInterfaceByNameProc = (COSAGetInterfaceByNameProc)pPlugInfo->AcquireFunction("COSAGetInterfaceByName");

    if ( pGetInterfaceByNameProc != NULL )
    {
        g_GetInterfaceByName = pGetInterfaceByNameProc;
    }
    else
    {
        goto EXIT;
    }
    g_pCellularCcdIf = g_GetInterfaceByName(g_pDslhDmlAgent, CCSP_CCD_INTERFACE_NAME);

    if ( !g_pCellularCcdIf )
    {
        CcspTraceError(("g_pCellularCcdIf is NULL !\n"));

        goto EXIT;
    }
    g_RegisterCallBackAfterInitDml = (COSARegisterCallBackAfterInitDmlProc)pPlugInfo->AcquireFunction("COSARegisterCallBackAfterInitDml");

    if ( !g_RegisterCallBackAfterInitDml )
    {
        goto EXIT;
    }
    g_COSARepopulateTable = (COSARepopulateTableProc)pPlugInfo->AcquireFunction("COSARepopulateTable");

    if ( !g_COSARepopulateTable )
    {
        goto EXIT;
    }
    /* Get Message Bus Handle */
    g_GetMessageBusHandle = (PFN_CCSPCCDM_APPLY_CHANGES)pPlugInfo->AcquireFunction("COSAGetMessageBusHandle");
    if ( g_GetMessageBusHandle == NULL )
    {
        goto EXIT;
    }
    g_MessageBusHandle = (ANSC_HANDLE)g_GetMessageBusHandle(g_pDslhDmlAgent);
    if ( g_MessageBusHandle == NULL )
    {
        goto EXIT;
    }
    g_MessageBusHandle_Irep = g_MessageBusHandle;

    /* Get Subsystem prefix */
    g_GetSubsystemPrefix = (COSAGetSubsystemPrefixProc)pPlugInfo->AcquireFunction("COSAGetSubsystemPrefix");
    if ( g_GetSubsystemPrefix != NULL )
    {
        char*   tmpSubsystemPrefix;

        if ( tmpSubsystemPrefix = g_GetSubsystemPrefix(g_pDslhDmlAgent) )
        {
            AnscCopyString(g_SubSysPrefix_Irep, tmpSubsystemPrefix);
        }

        /* retrieve the subsystem prefix */
        g_SubsystemPrefix = g_GetSubsystemPrefix(g_pDslhDmlAgent);
    }

    /* Create backend framework */
    g_pBEManager = (PBACKEND_MANAGER_OBJECT)BackEndManagerCreate();

    if ( g_pBEManager && g_pBEManager->Initialize )
    {
        g_pBEManager->hCosaPluginInfo = pPlugInfo;

        g_pBEManager->Initialize   ((ANSC_HANDLE)g_pBEManager);
    }

    return  0;
EXIT:

    return -1;
}
