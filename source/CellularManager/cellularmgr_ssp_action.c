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

#include "cellularmgr_ssp_internal.h"
#include "cellularmgr_global.h"

PCOMPONENT_COMMON_CELLULAR_MANAGER       g_pComponentCommonCellularMgr     = NULL;
PCCSP_CCD_INTERFACE                      pSsdCcdIf                         = (PCCSP_CCD_INTERFACE) NULL;
PDSLH_LCB_INTERFACE                      pDslhLcbIf                        = (PDSLH_LCB_INTERFACE) NULL;
PDSLH_CPE_CONTROLLER_OBJECT              pDslhCpeController                = NULL;
extern  ANSC_HANDLE                      bus_handle;
extern char                              g_Subsystem[32];
extern  ULONG                            g_ulAllocatedSizePeak;

#define  DATAMODEL_XML_FILE              "/usr/rdk/cellularmanager/RdkCellularManager.xml"
#define  CCSP_INTERFACE_CR_NAME          "com.cisco.spvtg.ccsp.CR"


ANSC_STATUS CellularMgr_Init()
{
    int rc = ANSC_STATUS_FAILURE;

    g_pComponentCommonCellularMgr = (PCOMPONENT_COMMON_CELLULAR_MANAGER) AnscAllocateMemory(sizeof(COMPONENT_COMMON_CELLULAR_MANAGER));

    if(!g_pComponentCommonCellularMgr)
    {
        return ANSC_STATUS_RESOURCES;
    }

    ComponentCommonDmInit(g_pComponentCommonCellularMgr);

    g_pComponentCommonCellularMgr->Name    = AnscCloneString(RDK_COMPONENT_NAME_CELLULAR_MANAGER);
    g_pComponentCommonCellularMgr->Version = 1;
    g_pComponentCommonCellularMgr->Author  = AnscCloneString("Sky");

    /* Create ComponentCommonDatamodel interface*/
    if (!pSsdCcdIf)
    {
        pSsdCcdIf = (PCCSP_CCD_INTERFACE) AnscAllocateMemory(sizeof(CCSP_CCD_INTERFACE));

        if ( !pSsdCcdIf )
        {
            return ANSC_STATUS_RESOURCES;
        }
        else
        {
            AnscCopyString(pSsdCcdIf->Name, CCSP_CCD_INTERFACE_NAME);

            pSsdCcdIf->InterfaceId              = CCSP_CCD_INTERFACE_ID;
            pSsdCcdIf->hOwnerContext            = NULL;
            pSsdCcdIf->Size                     = sizeof(CCSP_CCD_INTERFACE);

            pSsdCcdIf->GetComponentName         = CellularMgr_GetComponentName;
            pSsdCcdIf->GetComponentVersion      = CellularMgr_GetComponentVersion;
            pSsdCcdIf->GetComponentAuthor       = CellularMgr_GetComponentAuthor;
            pSsdCcdIf->GetComponentHealth       = CellularMgr_GetComponentHealth;
            pSsdCcdIf->GetComponentState        = CellularMgr_GetComponentState;
            pSsdCcdIf->GetLoggingEnabled        = CellularMgr_GetLoggingEnabled;
            pSsdCcdIf->SetLoggingEnabled        = CellularMgr_SetLoggingEnabled;
            pSsdCcdIf->GetLoggingLevel          = CellularMgr_GetLoggingLevel;
            pSsdCcdIf->SetLoggingLevel          = CellularMgr_SetLoggingLevel;
            pSsdCcdIf->GetMemMaxUsage           = CellularMgr_GetMemMaxUsage;
            pSsdCcdIf->GetMemMinUsage           = CellularMgr_GetMemMinUsage;
            pSsdCcdIf->GetMemConsumed           = CellularMgr_GetMemConsumed;
            pSsdCcdIf->ApplyChanges             = CellularMgr_ApplyChanges;
        }
    }

    /* Create ComponentCommonDatamodel interface*/
    if (!pDslhLcbIf)
    {
        pDslhLcbIf = (PDSLH_LCB_INTERFACE) AnscAllocateMemory(sizeof(DSLH_LCB_INTERFACE));

        if (!pDslhLcbIf)
        {
            return ANSC_STATUS_RESOURCES;
        }
        else
        {
            AnscCopyString(pDslhLcbIf->Name, CCSP_LIBCBK_INTERFACE_NAME);

            pDslhLcbIf->InterfaceId              = CCSP_LIBCBK_INTERFACE_ID;
            pDslhLcbIf->hOwnerContext            = NULL;
            pDslhLcbIf->Size                     = sizeof(DSLH_LCB_INTERFACE);

            pDslhLcbIf->InitLibrary              = CellularMgr_DMLInit;
        }
    }

    pDslhCpeController = DslhCreateCpeController(NULL, NULL, NULL);

    if (!pDslhCpeController)
    {
        CcspTraceWarning(("CANNOT Create pDslhCpeController... Exit!\n"));
        return ANSC_STATUS_RESOURCES;
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CellularMgr_RegisterComponent
    (
    )
{
    ANSC_STATUS			            returnStatus                = ANSC_STATUS_SUCCESS;
    PCCC_MBI_INTERFACE              pSsdMbiIf                   = (PCCC_MBI_INTERFACE)MsgHelper_CreateCcdMbiIf((void*)bus_handle, g_Subsystem);
    char                            CrName[256];

    g_pComponentCommonCellularMgr->Health = RDK_COMMON_COMPONENT_HEALTH_Yellow;

    /* data model configuration */
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pDslhLcbIf);
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pSsdMbiIf);
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pSsdCcdIf);
    pDslhCpeController->SetDbusHandle((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)bus_handle);
    pDslhCpeController->Engage((ANSC_HANDLE)pDslhCpeController);

    if ( g_Subsystem[0] != 0 )
    {
        sprintf(CrName, "%s%s", g_Subsystem, CCSP_INTERFACE_CR_NAME);
    }
    else
    {
        sprintf(CrName, "%s", CCSP_INTERFACE_CR_NAME);
    }

    returnStatus =
        pDslhCpeController->RegisterCcspDataModel
        (
         (ANSC_HANDLE)pDslhCpeController,
         CrName, /* CCSP_INTERFACE_CR_NAME,*/              /* CCSP CR ID */
         DATAMODEL_XML_FILE,             /* Data Model XML file. Can be empty if only base data model supported. */
         RDK_COMPONENT_NAME_CELLULAR_MANAGER,            /* Component Name    */
         RDK_COMPONENT_VERSION_CELLULAR_MANAGER,         /* Component Version */
         RDK_COMPONENT_PATH_CELLULAR_MANAGER,            /* Component Path    */
         g_Subsystem /* Component Prefix  */
        );

    if ( returnStatus == ANSC_STATUS_SUCCESS || returnStatus == CCSP_SUCCESS)
    {
        /* System is fully initialized */
        g_pComponentCommonCellularMgr->Health = RDK_COMMON_COMPONENT_HEALTH_Green;
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
CellularMgr_Term
    (
    )
{
	int                             nRet  = 0;
    char                            CrName[256];
    char                            CpName[256];

    if(  g_pComponentCommonCellularMgr == NULL)
    {
        return ANSC_STATUS_SUCCESS;
    }

    if ( g_Subsystem[0] != 0 )
    {
        sprintf(CrName, "%s%s", g_Subsystem, CCSP_INTERFACE_CR_NAME);
        sprintf(CpName, "%s%s", g_Subsystem, RDK_COMPONENT_NAME_CELLULAR_MANAGER);
    }
    else
    {
        sprintf(CrName, "%s", CCSP_INTERFACE_CR_NAME);
        sprintf(CpName, "%s", RDK_COMPONENT_NAME_CELLULAR_MANAGER);
    }
    /* unregister component */
    nRet = CcspBaseIf_unregisterComponent(bus_handle, CrName, CpName );
    AnscTrace("unregisterComponent returns %d\n", nRet);

    pDslhCpeController->Cancel((ANSC_HANDLE)pDslhCpeController);
    AnscFreeMemory(pDslhCpeController);

    if ( pSsdCcdIf ) AnscFreeMemory(pSsdCcdIf);
    if (  g_pComponentCommonCellularMgr ) AnscFreeMemory( g_pComponentCommonCellularMgr);

    g_pComponentCommonCellularMgr = NULL;
    pSsdCcdIf                = NULL;
    pDslhCpeController       = NULL;

    return ANSC_STATUS_SUCCESS;
}

char*
CellularMgr_GetComponentName
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->Name;
}


ULONG
CellularMgr_GetComponentVersion
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->Version;
}


char*
CellularMgr_GetComponentAuthor
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->Author;
}


ULONG
CellularMgr_GetComponentHealth
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->Health;
}


ULONG
CellularMgr_GetComponentState
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->State;
}


BOOL
CellularMgr_GetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->LogEnable;
}



ANSC_STATUS
CellularMgr_SetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject,
        BOOL                            bEnabled
    )
{
    if(g_pComponentCommonCellularMgr->LogEnable == bEnabled)
    {
        return ANSC_STATUS_SUCCESS;
    }

    g_pComponentCommonCellularMgr->LogEnable = bEnabled;

    if(bEnabled)
    {
        g_iTraceLevel = (INT) g_pComponentCommonCellularMgr->LogLevel;
    }
    else
    {
        g_iTraceLevel = CCSP_TRACE_INVALID_LEVEL;
    }
    return ANSC_STATUS_SUCCESS;
}


ULONG
CellularMgr_GetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->LogLevel;
}


ANSC_STATUS
CellularMgr_SetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject,
        ULONG                           LogLevel
    )
{
    if(g_pComponentCommonCellularMgr->LogLevel == LogLevel)
    {
        return ANSC_STATUS_SUCCESS;
    }

    g_pComponentCommonCellularMgr->LogLevel = LogLevel;

    if(g_pComponentCommonCellularMgr->LogEnable)
    {
        g_iTraceLevel = (INT) g_pComponentCommonCellularMgr->LogLevel;
    }

    return ANSC_STATUS_SUCCESS;
}

ULONG
CellularMgr_GetMemMaxUsage
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_ulAllocatedSizePeak;
}


ULONG
CellularMgr_GetMemMinUsage
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonCellularMgr->MemMinUsage;
}


ULONG
CellularMgr_GetMemConsumed
    (
        ANSC_HANDLE                     hThisObject
    )
{
    LONG             size = 0;

    size = AnscGetComponentMemorySize(RDK_COMPONENT_NAME_CELLULAR_MANAGER);
    if (size == -1 )
        size = 0;

    return size;
}


ANSC_STATUS
CellularMgr_ApplyChanges
    (
        ANSC_HANDLE                     hThisObject
    )
{
    ANSC_STATUS                         returnStatus    = ANSC_STATUS_SUCCESS;

    /* Assume the parameter settings are committed immediately. */
    return returnStatus;
}
