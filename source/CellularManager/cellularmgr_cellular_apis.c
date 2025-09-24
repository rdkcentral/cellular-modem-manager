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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**************************************************************************

    module: cellularmgr_cellular_internal.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

        *  CellularMgr_CellularCreate
        *  CellularMgr_CellularInitialize
        *  CellularMgr_CellularRemove

**************************************************************************/
#include "cellularmgr_cellular_apis.h"
#include "cellularmgr_cellular_internal.h"
#include "cellularmgr_plugin_main_apis.h"
#include "poam_irepfo_interface.h"
#include "sys_definitions.h"
#include "cellularmgr_sm.h"
#include "dmsb_tr181_psm_definitions.h"
#include "ccsp_trace.h"
#include "msgpack.h"
#include "base64.h"
#include "cellularmgr_cellular_webconfig_api.h"

#if RBUS_BUILD_FLAG_ENABLE
#include "cellularmgr_rbus_events.h"
#endif
#ifdef HYBRID_SUPPORT
static PCELLULAR_DML_INFO             g_pstDmlCellular =  NULL;
#endif
/**********************************************************************
                STRUCTURE AND CONSTANT DEFINITIONS
**********************************************************************/

/**********************************************************************
                FUNCTION PROTOTYPES
**********************************************************************/

/**********************************************************************
                FUNCTION DEFINITIION
**********************************************************************/

static int CellularMgr_InitializeContextDefaultProfile( CellularContextInitInputStruct *pstCtxInputStruct )
{
    int retPsmGet = CCSP_SUCCESS;
    char param_value[256];
    char param_name[512];

    if( NULL == pstCtxInputStruct )
    {
        return CCSP_FAILURE;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_ID);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        unsigned int ProfileID = 0;

        _ansc_sscanf(param_value, "%d", &ProfileID);

        if( 0 < ProfileID )
        {
            pstCtxInputStruct->stIfInput.ProfileID = ProfileID;
        }
        else
        {
            pstCtxInputStruct->stIfInput.ProfileID = CELLULAR_PROFILE_ID_UNKNOWN;
        }
    }
    else
    {
        pstCtxInputStruct->stIfInput.ProfileID = CELLULAR_PROFILE_ID_UNKNOWN;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_TYPE);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, "3gpp") == 0)
        {
            pstCtxInputStruct->stIfInput.ProfileType = CELLULAR_PROFILE_TYPE_3GPP;
        }
        else if(strcmp(param_value, "3gpp2") == 0)
        {
            pstCtxInputStruct->stIfInput.ProfileType = CELLULAR_PROFILE_TYPE_3GPP2;
        }
        else
        {
            pstCtxInputStruct->stIfInput.ProfileType = CELLULAR_PROFILE_TYPE_3GPP;
        }
    }
    else
    {
        pstCtxInputStruct->stIfInput.ProfileType = CELLULAR_PROFILE_TYPE_3GPP;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_PDPTYPE);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, "IPv4") == 0)
        {
            pstCtxInputStruct->stIfInput.PDPType = CELLULAR_PDP_TYPE_IPV4;
        }
        else if(strcmp(param_value, "IPv6") == 0)
        {
            pstCtxInputStruct->stIfInput.PDPType = CELLULAR_PDP_TYPE_IPV6;
        }
        else if(strcmp(param_value, "IPv4IPv6") == 0)
        {
            pstCtxInputStruct->stIfInput.PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
        }
        else
        {
            pstCtxInputStruct->stIfInput.PDPType = CELLULAR_PDP_TYPE_IPV4;
        }
    }
    else
    {
        pstCtxInputStruct->stIfInput.PDPType = CELLULAR_PDP_TYPE_IPV4;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_AUTHENTICATION);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, "PAP") == 0)
        {
            pstCtxInputStruct->stIfInput.PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_PAP;
        }
        else if(strcmp(param_value, "CHAP") == 0)
        {
            pstCtxInputStruct->stIfInput.PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_PAP;
        }
        else
        {
            pstCtxInputStruct->stIfInput.PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_NONE;
        }
    }
    else
    {
        pstCtxInputStruct->stIfInput.PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_NONE;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_NAME);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.ProfileName, param_value);
    }
    else
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.ProfileName, "");
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_APN);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.APN, param_value);
    }
    else
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.APN, "");
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_USERNAME);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.Username, param_value);
    }
    else
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.Username, "");
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_PASSWORD);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.Password, param_value);
    }
    else
    {
        AnscCopyString(pstCtxInputStruct->stIfInput.Password, "");
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_IS_NOROAMING);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, "TRUE") == 0)
        {
            pstCtxInputStruct->stIfInput.bIsNoRoaming = TRUE;
        }
        else 
        {
            pstCtxInputStruct->stIfInput.bIsNoRoaming = FALSE;
        }
    }
    else
    {
        pstCtxInputStruct->stIfInput.bIsNoRoaming = TRUE;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_DEFAULT_PROFILE_IS_APNDISABLED);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, "TRUE") == 0)
        {
            pstCtxInputStruct->stIfInput.bIsAPNDisabled = TRUE;
        }
        else 
        {
            pstCtxInputStruct->stIfInput.bIsAPNDisabled = FALSE;
        }
    }
    else
    {
        pstCtxInputStruct->stIfInput.bIsAPNDisabled = FALSE;
    }

    pstCtxInputStruct->stIfInput.bIsThisDefaultProfile = TRUE;

    CcspTraceInfo(("************** Default Profile Information *************\n"));
    CcspTraceInfo(("\t ProfileID         : %d\n",pstCtxInputStruct->stIfInput.ProfileID));
    CcspTraceInfo(("\t ProfileType       : %d\n",pstCtxInputStruct->stIfInput.ProfileType));
    CcspTraceInfo(("\t PDPType           : %d\n",pstCtxInputStruct->stIfInput.PDPType));
    CcspTraceInfo(("\t Authentication    : %d\n",pstCtxInputStruct->stIfInput.PDPAuthentication));
    CcspTraceInfo(("\t ProfileName       : %s\n",pstCtxInputStruct->stIfInput.ProfileName));
    CcspTraceInfo(("\t APN               : %s\n",pstCtxInputStruct->stIfInput.APN));
    CcspTraceInfo(("\t Username          : %s\n",pstCtxInputStruct->stIfInput.Username));
    CcspTraceInfo(("\t Password          : %s\n",pstCtxInputStruct->stIfInput.Password));
    CcspTraceInfo(("\t IsNoRoaming       : %d\n",pstCtxInputStruct->stIfInput.bIsNoRoaming));
    CcspTraceInfo(("\t IsAPNDisabled     : %d\n",pstCtxInputStruct->stIfInput.bIsAPNDisabled));
    CcspTraceInfo(("\t IsthisDefault     : %d\n",pstCtxInputStruct->stIfInput.bIsThisDefaultProfile));

    return retPsmGet;
}

ANSC_STATUS DmlCellularResetDefaults()
{
    PCELLULAR_DML_INFO             pstDmlCellular =  NULL;
    INT                            iLoopCount;
    PCELLULAR_INTERFACE_INFO       pstTempInerfaceInfo = NULL;
    BOOLEAN                        bEnable = TRUE;


#ifdef HYBRID_SUPPORT
    // Initialize with the global pointer
    pstDmlCellular = g_pstDmlCellular;
    //Take copy of X_RDK_Enable to restore the value
    bEnable = g_pstDmlCellular->X_RDK_Enable;
#endif
    // Take a copy of the interface info pointer, before clearing the memory
    pstTempInerfaceInfo = pstDmlCellular->pstInterfaceInfo;
    // Clear the complete DML
    AnscZeroMemory(pstDmlCellular, sizeof(CELLULAR_DML_INFO));

    /*
     * Initialize Interface DML
    */
    pstDmlCellular->X_RDK_Enable                 = bEnable;
    pstDmlCellular->X_RDK_Status                 = RDK_STATUS_DOWN;
    pstDmlCellular->X_RDK_DeviceType             = RDK_DEVICETYPE_UNKNOWN; // This parameter will be updated by the device manager
    pstDmlCellular->X_RDK_ControlInterfaceStatus = CONTROL_STATUS_CLOSED;
    pstDmlCellular->X_RDK_DataInterfaceLink      = DATA_INTERFACE_LINK_RAW_IP;

    pstDmlCellular->ulInterfaceNoEntries        = 1;
    // restore the backed up copy here.
    pstDmlCellular->pstInterfaceInfo = pstTempInerfaceInfo;

    for( iLoopCount = 0; iLoopCount < pstDmlCellular->ulInterfaceNoEntries; iLoopCount++ )
    {
        PCELLULAR_INTERFACE_INFO   pstInterfaceInfo = &(pstDmlCellular->pstInterfaceInfo[iLoopCount]);
	PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO pstContextProfileInfo = NULL;

        //Take copy of pstContextProfileInfo to restore the value
	if ( pstInterfaceInfo->pstContextProfileInfo)
	{
	    pstContextProfileInfo = pstInterfaceInfo->pstContextProfileInfo;
	}

	// clear the interface info structure
    	AnscZeroMemory(pstInterfaceInfo , sizeof(CELLULAR_INTERFACE_INFO));

        pstInterfaceInfo->Enable              = TRUE;
        pstInterfaceInfo->Status              = IF_DOWN;
        pstInterfaceInfo->X_RDK_RegisteredService = REGISTERED_SERVICE_NONE;
        pstInterfaceInfo->stServingInfo.RadioEnvConditions     = RADIO_ENV_CONDITION_UNAVAILABLE;

        pstInterfaceInfo->stPlmnAccessInfo.RoamingStatus  = ROAMING_STATUS_HOME;
        pstInterfaceInfo->stPlmnAccessInfo.ulAvailableNetworkNoOfEntries  = 0;

        //Below information ready once Modem Opened
        pstInterfaceInfo->ulNeighbourNoOfEntries = 0;

        //Static Context
        pstInterfaceInfo->ulContextProfileNoOfEntries = 1;

        if (pstContextProfileInfo == NULL )

        {
            pstInterfaceInfo->pstContextProfileInfo = (PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO)AnscAllocateMemory(sizeof(CELLULAR_INTERFACE_CONTEXTPROFILE_INFO) * pstInterfaceInfo->ulContextProfileNoOfEntries);
	    if ( pstInterfaceInfo->pstContextProfileInfo == NULL )
	    {
            	return  ANSC_STATUS_FAILURE;
	    }
        }
	else
	{
	    // restore backed up value.
	    pstInterfaceInfo->pstContextProfileInfo = pstContextProfileInfo;
	}

        memset(pstInterfaceInfo->pstContextProfileInfo, 0, sizeof(CELLULAR_INTERFACE_CONTEXTPROFILE_INFO));
    }

    /*
     * Initialize UICC slot DML and needs to be build once Modem opened
    */
    pstDmlCellular->ulUICCNoOfEntries = 0;

    /*
     * Initialize EUICC slot DML and needs to be build once Modem opened
    */
    pstDmlCellular->stEUICCSlotInfo.ulMNOProfileNoOfEntries = 0;

    /*
     * Initialize AccessPoint DML and needs to be build once Modem opened
    */
    pstDmlCellular->ulAccessPointNoOfEntries = 0;

	return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS DmlCellularInitialize ( ANSC_HANDLE  hDml )
{
    ANSC_STATUS                    returnStatus   =  ANSC_STATUS_SUCCESS;
	PCELLULARMGR_CELLULAR_DATA     pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) hDml;
    PCELLULAR_DML_INFO             pstDmlCellular =  NULL;
    CellularContextInitInputStruct stCtxInputStruct = { 0 };
    CellularMgrSMInputStruct       stStateMachineInput = { 0 };
    INT                            iLoopCount;
#if RBUS_BUILD_FLAG_ENABLE
    pthread_t                      rBusThread;
#endif

    if ( pMyObject == NULL )
    {
        return  ANSC_STATUS_FAILURE;
    }

    /*
     * Initialize Cellular DML
    */
    pstDmlCellular = (PCELLULAR_DML_INFO)AnscAllocateMemory(sizeof(CELLULAR_DML_INFO));
    if ( pstDmlCellular == NULL )
    {
        return  ANSC_STATUS_FAILURE;
    }

    AnscZeroMemory(pstDmlCellular, sizeof(CELLULAR_DML_INFO));
#ifdef HYBRID_SUPPORT
    g_pstDmlCellular = pstDmlCellular;
#endif
    /*
     * Initialize Interface DML
    */
    pstDmlCellular->X_RDK_Enable                 = FALSE;
    pstDmlCellular->X_RDK_Status                 = RDK_STATUS_DOWN;
    pstDmlCellular->X_RDK_ControlInterfaceStatus = CONTROL_STATUS_CLOSED;
    pstDmlCellular->X_RDK_DataInterfaceLink      = DATA_INTERFACE_LINK_RAW_IP;

    pstDmlCellular->ulInterfaceNoEntries        = 1;
    pstDmlCellular->pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)AnscAllocateMemory(sizeof(CELLULAR_INTERFACE_INFO) * pstDmlCellular->ulInterfaceNoEntries);
    if ( pstDmlCellular->pstInterfaceInfo == NULL )
    {
        return  ANSC_STATUS_FAILURE;
    }

    AnscZeroMemory(pstDmlCellular->pstInterfaceInfo, sizeof(CELLULAR_INTERFACE_INFO) * pstDmlCellular->ulInterfaceNoEntries);

    for( iLoopCount = 0; iLoopCount < pstDmlCellular->ulInterfaceNoEntries; iLoopCount++ )
    {
        PCELLULAR_INTERFACE_INFO   pstInterfaceInfo = &(pstDmlCellular->pstInterfaceInfo[iLoopCount]);
        
        pstInterfaceInfo->Enable              = TRUE;
        pstInterfaceInfo->Status              = IF_DOWN;
        pstInterfaceInfo->X_RDK_RegisteredService = REGISTERED_SERVICE_NONE; 
        pstInterfaceInfo->RadioEnvConditions     = RADIO_ENV_CONDITION_UNAVAILABLE;
        pstInterfaceInfo->stPlmnAccessInfo.RoamingStatus  = ROAMING_STATUS_HOME;
        pstInterfaceInfo->stPlmnAccessInfo.pstAvailableNetworks = NULL;
        /* Creating Dummy UICC Entry for more info refer AccessPoint Entry count comment */
#if RBUS_BUILD_FLAG_ENABLE
        pstInterfaceInfo->stPlmnAccessInfo.ulAvailableNetworkNoOfEntries  = 1;
#else
        pstInterfaceInfo->stPlmnAccessInfo.ulAvailableNetworkNoOfEntries  = 0;
#endif
        //Below information ready once Modem Opened
        pstInterfaceInfo->ulNeighbourNoOfEntries = 0;

        //Static Context
        pstInterfaceInfo->ulContextProfileNoOfEntries = 1;

        pstInterfaceInfo->pstContextProfileInfo = (PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO)AnscAllocateMemory(sizeof(CELLULAR_INTERFACE_CONTEXTPROFILE_INFO) * pstInterfaceInfo->ulContextProfileNoOfEntries);
        if ( pstInterfaceInfo->pstContextProfileInfo == NULL )
        {
            return  ANSC_STATUS_FAILURE;
        }

        memset(pstInterfaceInfo->pstContextProfileInfo, 0, sizeof(CELLULAR_INTERFACE_CONTEXTPROFILE_INFO));
    }
    /*
     * Initialize EUICC slot DML and needs to be build once Modem opened
    */
    pstDmlCellular->stEUICCSlotInfo.ulMNOProfileNoOfEntries = 0;

    /*
     * Initialize AccessPoint DML and needs to be build once Modem opened
    */

    /*ToDo */
    /*WorkAround Applicable for DML APIs generated by auto_gen script using XML config file: 
     * COSA Create/Delete/Update dynWritableTable with AddEntry/DelEntry/IsSync/IsUpdate API
     * direct call. but where as RBUS only Support AddEntry/DelEntry.So If Dynamic table does
     * not creating first entry by AddEntry API then we have to create First Entry as Dummy for 
     * Table to make IsSync and IsUpdate API Work in RBUS.
     */
#if RBUS_BUILD_FLAG_ENABLE
    pstDmlCellular->ulAccessPointNoOfEntries = 1;
#else
    pstDmlCellular->ulAccessPointNoOfEntries = 0;
#endif
    /*
     * Initialize UICC slot DML and needs to be build once Modem opened
    */
    /* Creating Dummy UICC Entry for more info refer above AccessPoint comment */
#if RBUS_BUILD_FLAG_ENABLE
    pstDmlCellular->ulUICCNoOfEntries = 1;
#else
    pstDmlCellular->ulUICCNoOfEntries = 0;
#endif
    /* Assign to master structure */
    pMyObject->pstDmlCellular = pstDmlCellular;

    //Read all defaults from DB
    stCtxInputStruct.stIfInput.PDPContextNumber   = CELLULAR_PDP_CONTEXT_UNKNOWN;
    stCtxInputStruct.stIfInput.ProxyPort          = 0;
    snprintf(stCtxInputStruct.stIfInput.Proxy, sizeof(stCtxInputStruct.stIfInput.Proxy), "%s", "");
    CellularMgr_InitializeContextDefaultProfile( &stCtxInputStruct );

    //HAL Init
    cellular_hal_init( &stCtxInputStruct );

    //Start Cellular Manager SM
    stStateMachineInput.bModemEnable = pstDmlCellular->X_RDK_Enable;
    stStateMachineInput.pCmIfData = &(pMyObject->pstDmlCellular->pstInterfaceInfo[0]);
    stStateMachineInput.pstDmlCellular = pstDmlCellular;
    memcpy(&(stStateMachineInput.stContextProfile), &(stCtxInputStruct.stIfInput), sizeof(CellularProfileStruct));
    CellularMgr_Start_State_Machine( &stStateMachineInput );

#if RBUS_BUILD_FLAG_ENABLE
    //Initiate the thread for Cellular RBUS On-Interval case
    pthread_create( &rBusThread, NULL, &CellularMgr_RBUS_Events_Monitor_Thread, (void*)pMyObject);
#endif

    CcspTraceInfo(("%s %d - Done\n",__FUNCTION__,__LINE__));
    
    return returnStatus;
}
#ifdef HYBRID_SUPPORT
typedef struct _interfaceIndexEntry {
    unsigned int index;
    char interfaceName[16];
}interfaceIndexEntry;

interfaceIndexEntry interfaceArray[16];
unsigned int interfaceArraySize = 0;

int FindInList(char *wan_ifname) 
{  
    int i = 0;
    for (i = 0; i < interfaceArraySize; i++)
    {
        if (AnscEqualString(interfaceArray[i].interfaceName, wan_ifname, TRUE))
           return interfaceArray[i].index;
    }	 

    return 0xFF;
}
#endif
int CellularMgrGetLTEIfaceIndex ( char *wan_ifname )
{
    unsigned int totalNoOfIface = 0;
    char paramName[256];
    char paramValue[256];
    unsigned int index = 0;
#ifdef HYBRID_SUPPORT
    index = FindInList(wan_ifname);
    if (0xFF != index) {
        CcspTraceInfo(("%s %d - Found %s in previously searched entries at index %d \n", __FUNCTION__, __LINE__, wan_ifname, index));
        return index;
    }
    index = 0;
#endif
    memset ( paramValue, 0, sizeof(paramValue) );

    if ( (CellularMgr_RdkBus_GetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, DML_NO_OF_INTERFACE_ENTRIES, paramValue ) != RETURN_OK)
            || ( paramValue == NULL) )
    {
        CcspTraceError(("%s %d - Unable to fetch total no of interfaces.\n", __FUNCTION__, __LINE__));
        return -1;
    }

    totalNoOfIface = atoi (paramValue);

    memset ( paramName, 0, sizeof(paramName) );
    memset ( paramValue, 0, sizeof(paramValue) );

    for (index = 0; index < totalNoOfIface; index++)
    {
        snprintf(paramName, sizeof(paramName), DML_IFTABLE_IFACE_NAME, index + 1);
        if ( (CellularMgr_RdkBus_GetParamValue ( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue) != RETURN_OK)
                || (paramValue == NULL))
        {
            CcspTraceError(("%s %d - Unable to fetch %s.\n", __FUNCTION__, __LINE__, paramName));
            return -1;
        }

        if ( strncmp (paramValue, wan_ifname, strlen(wan_ifname)) == 0 )
        {
#ifdef HYBRID_SUPPORT
            AnscCopyString(interfaceArray[interfaceArraySize].interfaceName, wan_ifname);
            interfaceArray[interfaceArraySize].index = index + 1;
            interfaceArraySize++;
#endif
            return index + 1;
        }
    }
    CcspTraceError(("%s %d - Unable to fetch %s interface entry in interface table.\n", __FUNCTION__, __LINE__, wan_ifname));
    return -1;
}

int CellularMgrUpdatePhyStatus ( char *wan_ifname, CellularDeviceOpenStatus_t device_open_status )
{
    int index = -1;
    char paramName[256];
    char paramValue[256];
    
    index = CellularMgrGetLTEIfaceIndex(wan_ifname);

    if( -1 == index )
    {
        return RETURN_ERROR;
    }

    memset( paramName, 0, sizeof(paramName) );
    memset( paramValue, 0, sizeof(paramValue) );

    // Send Phy Status for Interface
    if ( device_open_status == DEVICE_OPEN_STATUS_READY )
    {
        // Set Phy.Path
        snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_PHY_PATH, index);
        snprintf( paramValue, sizeof(paramValue), DML_LTE_IFACE_PATH);

        if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
        {
            CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_PHY_PATH));
            return RETURN_ERROR;
        }

        // Set Phy.Status
        memset( paramName, 0, sizeof(paramName) );
        memset( paramValue, 0, sizeof(paramValue) );
        snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_PHY_STATUS, index);
        snprintf( paramValue, sizeof(paramValue), UP_STR);

        if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
        {
            CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_PHY_PATH));
            return RETURN_ERROR;
        }
    }
    else
    {
        // Set Phy.Status
        snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_PHY_STATUS, index);
        snprintf( paramValue, sizeof(paramValue), DOWN_STR);
        if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
        {
            CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_PHY_PATH));
            return RETURN_ERROR;
        }
    }

    return RETURN_OK;
}

int CellularMgrUpdateIfname ( char *wan_ifname, char *pCellularIfName )
{
    int index = -1;
    char paramName[256];
    char paramValue[256];

    if (NULL == wan_ifname)
    {
        return RETURN_ERROR;
    }


    index = CellularMgrGetLTEIfaceIndex(wan_ifname);

    if( -1 == index )
    {
        return RETURN_ERROR;
    }

    memset( paramName, 0, sizeof(paramName) );
    memset( paramValue, 0, sizeof(paramValue) );

    snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_WAN_NAME, index);
    snprintf( paramValue, sizeof(paramValue) - 1, pCellularIfName);

    if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
    {
        CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_WAN_NAME));
        return RETURN_ERROR;
    }
    return RETURN_OK;
}

int CellularMgrUpdateIpSource ( char *wan_ifname, char *pIpSource )
{
    int index = -1;
    char paramName[256];
    char paramValue[256];

    if ((NULL == wan_ifname) || (NULL == pIpSource))
    {
        return RETURN_ERROR;
    }

    index = CellularMgrGetLTEIfaceIndex(wan_ifname);

    if( -1 == index )
    {
        return RETURN_ERROR;
    }

    memset( paramName, 0, sizeof(paramName) );
    memset( paramValue, 0, sizeof(paramValue) );

    snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_IPV4_SOURCE, index);
    snprintf( paramValue, sizeof(paramValue) - 1, pIpSource);

    if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
    {
        CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_LINK_STATUS));
        return RETURN_ERROR;
    }

    memset( paramName, 0, sizeof(paramName) );
    memset( paramValue, 0, sizeof(paramValue) );

    snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_IPV6_SOURCE, index);
    snprintf( paramValue, sizeof(paramValue) - 1, pIpSource);

    if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
    {
        CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_LINK_STATUS));
        return RETURN_ERROR;
    }
    return RETURN_OK;
}

int CellularMgrUpdateLinkStatus ( char *wan_ifname, char *status )
{
    int index = -1;
    char paramName[256];
    char paramValue[256];

    index = CellularMgrGetLTEIfaceIndex(wan_ifname);
    
    if( -1 == index )
    {
        return RETURN_ERROR;
    }
#ifndef HYBRID_SUPPORT
    memset( paramName, 0, sizeof(paramName) );
    memset( paramValue, 0, sizeof(paramValue) );

    snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_WAN_NAME, index);
    snprintf( paramValue, sizeof(paramValue) - 1, wan_ifname);

    if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
    {
        CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_WAN_NAME));
        return RETURN_ERROR;
    }
#endif
    memset( paramName, 0, sizeof(paramName) );
    memset( paramValue, 0, sizeof(paramValue) );

    snprintf( paramName, sizeof(paramName), DML_IFTABLE_IFACE_LINK_STATUS, index);
    snprintf( paramValue, sizeof(paramValue) - 1, status);

    if ( CellularMgr_RdkBus_SetParamValue( WAN_COMPONENT_NAME, WAN_DBUS_PATH, paramName, paramValue, ccsp_string, true) != RETURN_OK )
    {
        CcspTraceError(("%s %d - Unable to set %s value.\n", __FUNCTION__, __LINE__, DML_IFTABLE_IFACE_LINK_STATUS));
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

int CellularMgr_AccessPointGetProfileList( CELLULAR_INTERFACE_ACCESSPOINT_INFO  **ppstAPInfo, int *profile_count )
{
    CellularProfileStruct                *pstProfileOutput = NULL;
    INT                                  iProfileCount = 0;

    if( RETURN_OK == cellular_hal_get_profile_list( &pstProfileOutput, &iProfileCount ) )
    {
        if( 0 < iProfileCount )
        {
            CELLULAR_INTERFACE_ACCESSPOINT_INFO  *pstAPInfo = NULL;

            pstAPInfo = (CELLULAR_INTERFACE_ACCESSPOINT_INFO*)malloc( (sizeof(CELLULAR_INTERFACE_ACCESSPOINT_INFO) * (iProfileCount) ) );
            
            if( NULL != pstAPInfo )
            {
                int i;

                memset( pstAPInfo, 0, ( sizeof(CELLULAR_INTERFACE_ACCESSPOINT_INFO) * (iProfileCount) ) );

                for( i = 0; i < iProfileCount; i++ )
                {
                    pstAPInfo[i].ProfileIndex       = pstProfileOutput[i].ProfileID;
                    pstAPInfo[i].PDPContextNumber   = pstProfileOutput[i].PDPContextNumber;
                    pstAPInfo[i].X_RDK_Default      = pstProfileOutput[i].bIsThisDefaultProfile;
                    pstAPInfo[i].Enable             = ( pstProfileOutput[i].bIsAPNDisabled ) ?  FALSE : TRUE;
                    snprintf(pstAPInfo[i].Alias, sizeof(pstAPInfo[i].Alias), pstProfileOutput[i].ProfileName);
                    snprintf(pstAPInfo[i].APN, sizeof(pstAPInfo[i].APN), pstProfileOutput[i].APN);
                    snprintf(pstAPInfo[i].Username, sizeof(pstAPInfo[i].Username), pstProfileOutput[i].Username);
                    snprintf(pstAPInfo[i].Password, sizeof(pstAPInfo[i].Password), pstProfileOutput[i].Password);
                    pstAPInfo[i].X_RDK_ApnAuthentication  = pstProfileOutput[i].PDPAuthentication;
                    pstAPInfo[i].X_RDK_PdpInterfaceConfig = CELLULAR_PDP_NETWORK_CONFIG_NAS;
                    pstAPInfo[i].X_RDK_Roaming            = ( pstProfileOutput[i].bIsNoRoaming ) ? FALSE : TRUE;

                    if( CELLULAR_PDP_TYPE_IPV4 == pstProfileOutput[i].PDPType )
                    {
                        pstAPInfo[i].X_RDK_IpAddressFamily = INTERFACE_PROFILE_FAMILY_IPV4;
                    }
                    else if( CELLULAR_PDP_TYPE_IPV6 == pstProfileOutput[i].PDPType )
                    {
                        pstAPInfo[i].X_RDK_IpAddressFamily = INTERFACE_PROFILE_FAMILY_IPV6;
                    }
                    else if( CELLULAR_PDP_TYPE_IPV4_OR_IPV6 == pstProfileOutput[i].PDPType )
                    {
                        pstAPInfo[i].X_RDK_IpAddressFamily = INTERFACE_PROFILE_FAMILY_IPV4_IPV6;
                    }

                    CcspTraceDebug(("%s - Profile Name[%s] ProfileID[%d] APN[%s] IP[%d] User[%s] Password[%s] Auth[%d] APN Disabled[%d] Default[%d]\n",
                                                            __FUNCTION__,
                                                            pstAPInfo[i].Alias,
                                                            pstAPInfo[i].ProfileIndex,
                                                            pstAPInfo[i].APN,
                                                            pstAPInfo[i].X_RDK_IpAddressFamily,
                                                            pstAPInfo[i].Username,
                                                            pstAPInfo[i].Password,
                                                            pstAPInfo[i].X_RDK_ApnAuthentication,
                                                            pstAPInfo[i].Enable,
                                                            pstAPInfo[i].X_RDK_Default));
                }
            }

            //Initialize passed variables
            *profile_count = 0;

            if( NULL != *ppstAPInfo )
            {
                free(*ppstAPInfo);
                *ppstAPInfo = NULL;
            }

            //Assign proper values to passed structure
            *ppstAPInfo    = pstAPInfo;
            *profile_count = iProfileCount;

            //Release HAL structure memory
            if( NULL != pstProfileOutput )
            {
                free(pstProfileOutput);
                pstProfileOutput = NULL;
            }
        }
        else
        {
            //Initialize passed variables
            *profile_count = 0;

            if( NULL != *ppstAPInfo )
            {
                free(*ppstAPInfo);
                *ppstAPInfo = NULL;
            }
        }

        return RETURN_OK;
    }

    return RETURN_OK;
}

int CellularMgr_AccessPointCreateProfile( PCELLULAR_INTERFACE_ACCESSPOINT_INFO pstAPInfo )
{
    CellularProfileStruct stProfileInput = { 0 };

    stProfileInput.ProfileID                = pstAPInfo->ProfileIndex;
    stProfileInput.PDPContextNumber         = pstAPInfo->PDPContextNumber;
    stProfileInput.bIsThisDefaultProfile    = pstAPInfo->X_RDK_Default;
    stProfileInput.bIsAPNDisabled           = ( pstAPInfo->Enable ) ? FALSE : TRUE;
    snprintf(stProfileInput.ProfileName, sizeof(stProfileInput.ProfileName), pstAPInfo->Alias);
    snprintf(stProfileInput.APN, sizeof(stProfileInput.APN), pstAPInfo->APN);
    snprintf(stProfileInput.Username, sizeof(stProfileInput.Username), pstAPInfo->Username);
    snprintf(stProfileInput.Password, sizeof(stProfileInput.Password), pstAPInfo->Password);
    stProfileInput.PDPAuthentication        = pstAPInfo->X_RDK_ApnAuthentication;
    stProfileInput.PDPNetworkConfig         = pstAPInfo->X_RDK_PdpInterfaceConfig;
    stProfileInput.bIsNoRoaming             = ( pstAPInfo->X_RDK_Roaming ) ? FALSE : TRUE;

    if( INTERFACE_PROFILE_FAMILY_IPV4 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV4;
    }
    else if( INTERFACE_PROFILE_FAMILY_IPV6 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV6;
    }
    else if( INTERFACE_PROFILE_FAMILY_IPV4_IPV6 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
    }
    else
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV4;
    }

    //Create Profile
    if ( RETURN_OK != cellular_hal_profile_create( &stProfileInput, NULL ) )
    {
        CcspTraceError(("%s - Failed to create profile information\n", __FUNCTION__));
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

int CellularMgr_AccessPointDeleteProfile( PCELLULAR_INTERFACE_ACCESSPOINT_INFO pstAPInfo )
{
    CellularProfileStruct stProfileInput = { 0 };

    stProfileInput.ProfileID                = pstAPInfo->ProfileIndex;
    stProfileInput.PDPContextNumber         = pstAPInfo->PDPContextNumber;
    stProfileInput.bIsThisDefaultProfile    = pstAPInfo->X_RDK_Default;
    stProfileInput.bIsAPNDisabled           = ( pstAPInfo->Enable ) ? FALSE : TRUE;
    snprintf(stProfileInput.ProfileName, sizeof(stProfileInput.ProfileName), pstAPInfo->Alias);
    snprintf(stProfileInput.APN, sizeof(stProfileInput.APN), pstAPInfo->APN);
    snprintf(stProfileInput.Username, sizeof(stProfileInput.Username), pstAPInfo->Username);
    snprintf(stProfileInput.Password, sizeof(stProfileInput.Password), pstAPInfo->Password);
    stProfileInput.PDPAuthentication        = pstAPInfo->X_RDK_ApnAuthentication;
    stProfileInput.PDPNetworkConfig         = pstAPInfo->X_RDK_PdpInterfaceConfig;
    stProfileInput.bIsNoRoaming             = ( pstAPInfo->X_RDK_Roaming ) ? FALSE : TRUE;

    if( INTERFACE_PROFILE_FAMILY_IPV4 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV4;
    }
    else if( INTERFACE_PROFILE_FAMILY_IPV6 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV6;
    }
    else if( INTERFACE_PROFILE_FAMILY_IPV4_IPV6 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
    }

    //Delete Profile
    if ( RETURN_OK != cellular_hal_profile_delete( &stProfileInput, NULL ) )
    {
        CcspTraceError(("%s - Failed to delete profile information\n", __FUNCTION__));
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

int CellularMgr_AccessPointModifyProfile( PCELLULAR_INTERFACE_ACCESSPOINT_INFO pstAPInfo )
{
    CellularProfileStruct stProfileInput = { 0 };

    stProfileInput.ProfileID                = pstAPInfo->ProfileIndex;
    stProfileInput.PDPContextNumber         = pstAPInfo->PDPContextNumber;
    stProfileInput.bIsThisDefaultProfile    = pstAPInfo->X_RDK_Default;
    stProfileInput.bIsAPNDisabled           = ( pstAPInfo->Enable ) ? FALSE : TRUE;
    snprintf(stProfileInput.ProfileName, sizeof(stProfileInput.ProfileName), pstAPInfo->Alias);
    snprintf(stProfileInput.APN, sizeof(stProfileInput.APN), pstAPInfo->APN);
    snprintf(stProfileInput.Username, sizeof(stProfileInput.Username), pstAPInfo->Username);
    snprintf(stProfileInput.Password, sizeof(stProfileInput.Password), pstAPInfo->Password);
    stProfileInput.PDPAuthentication        = pstAPInfo->X_RDK_ApnAuthentication;
    stProfileInput.PDPNetworkConfig         = pstAPInfo->X_RDK_PdpInterfaceConfig;
    stProfileInput.bIsNoRoaming             = ( pstAPInfo->X_RDK_Roaming ) ? FALSE : TRUE;

    if( INTERFACE_PROFILE_FAMILY_IPV4 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV4;
    }
    else if( INTERFACE_PROFILE_FAMILY_IPV6 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV6;
    }
    else if( INTERFACE_PROFILE_FAMILY_IPV4_IPV6 == pstAPInfo->X_RDK_IpAddressFamily )
    {
        stProfileInput.PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
    }

    //Modify Profile
    if ( RETURN_OK != cellular_hal_profile_modify( &stProfileInput, NULL ) )
    {
        CcspTraceError(("%s - Failed to modify profile information\n", __FUNCTION__));
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

CELLULAR_RADIO_ENV_CONDITIONS CellularMgr_GetRadioEnvConditions( void ) 
{
    CELLULAR_RDK_STATUS            enRDKStatus = (CELLULAR_RDK_STATUS)CellularMgrSMGetCurrentState( );
    CELLULAR_RADIO_ENV_CONDITIONS  enRadioEnvCondition;
    
    switch( enRDKStatus )
    {
        case RDK_STATUS_DOWN:
        case RDK_STATUS_DEACTIVATED:
        case RDK_STATUS_DEREGISTERED:
        {
            enRadioEnvCondition = RADIO_ENV_CONDITION_UNAVAILABLE;
        }
        break;

        case RDK_STATUS_REGISTERED:
        case RDK_STATUS_CONNECTED:
        {
            CellularSignalInfoStruct stSignalInfo = { 0 };

            /*
            * EXCELLENT : ( RSRP > -85 )
            * GOOD:   (-85  >=  RSRP >  -95) 
            * FAIR:   (-95  >=  RSRP >  -105) 
            * POOR:   (-105 >= RSRP > -115) 
            */

            if( RETURN_OK == cellular_hal_get_signal_info( &stSignalInfo ) )
            {
                CcspTraceDebug(("%s - RSRP: %d for enRadioEnvCondition\n", __FUNCTION__, stSignalInfo.RSRP));
                if( stSignalInfo.RSRP == 0 )
                {
                    enRadioEnvCondition = RADIO_ENV_CONDITION_UNAVAILABLE;
                }
                else
                {
                    if( stSignalInfo.RSRP > CELLULAR_RADIO_ENV_EXCELLENT_THRESHOLD )
                    {
                        enRadioEnvCondition = RADIO_ENV_CONDITION_EXCELLENT;
                    }
                    else if( ( CELLULAR_RADIO_ENV_GOOD_THRESHOLD_HIGH >= stSignalInfo.RSRP ) && ( stSignalInfo.RSRP > CELLULAR_RADIO_ENV_GOOD_THRESHOLD_LOW ) )
                    {
                        enRadioEnvCondition = RADIO_ENV_CONDITION_GOOD;
                    }
                    else if( ( CELLULAR_RADIO_ENV_FAIR_THRESHOLD_HIGH >= stSignalInfo.RSRP ) && ( stSignalInfo.RSRP > CELLULAR_RADIO_ENV_FAIR_THRESHOLD_LOW ) )
                    {
                        enRadioEnvCondition = RADIO_ENV_CONDITION_FAIR;
                    }
                    else if( ( CELLULAR_RADIO_ENV_POOR_THRESHOLD_HIGH >= stSignalInfo.RSRP ) && ( stSignalInfo.RSRP > CELLULAR_RADIO_ENV_POOR_THRESHOLD_LOW ) )
                    {
                        enRadioEnvCondition = RADIO_ENV_CONDITION_POOR;
                    }
                    else
                    {
                        enRadioEnvCondition = RADIO_ENV_CONDITION_POOR;
                    }
                }
            }
            else
            {
                CcspTraceError(("%s : cellular_hal_get_signal_info failed.\n", __FUNCTION__));
                enRadioEnvCondition = RADIO_ENV_CONDITION_UNAVAILABLE;
            }
        }
        break;

        default:
        {
            enRadioEnvCondition = RADIO_ENV_CONDITION_UNAVAILABLE;
        }
    }

    return enRadioEnvCondition;
}

int CellularMgr_RadioSignalGetSignalInfo( CELLULAR_INTERFACE_SERVING_INFO *pstServingInfo )
{
    CellularSignalInfoStruct stSignalInfo = { 0 };

    if( RETURN_OK == cellular_hal_get_signal_info( &stSignalInfo ) )
    {
        pstServingInfo->Rssi   = stSignalInfo.RSSI;
        pstServingInfo->Snr    = stSignalInfo.SNR;
        pstServingInfo->Rsrp   = stSignalInfo.RSRP;
        pstServingInfo->Rsrq   = stSignalInfo.RSRQ;
        pstServingInfo->Trx    = stSignalInfo.TXPower;

        return RETURN_OK;
    }

    return RETURN_ERROR;
}

void* CellularMgr_FactoryResetThread(void* arg)
{
    CcspTraceInfo(("Successfully pthread created for Device.Cellular.X_RDK_DeviceManagement.FactoryReset \n"));

    UNREFERENCED_PARAMETER(arg);

    //detach thread from caller stack
    pthread_detach(pthread_self());

    // cellular_hal_modem_factory_reset HAL Function
    if ( RETURN_OK == cellular_hal_modem_factory_reset() )
    {
        CcspTraceInfo (("%s : Resetting Cellular to factory default settings\n", __FUNCTION__));

        // cellular_hal_modem_reset HAL Function
        if ( RETURN_OK == cellular_hal_modem_reset() )
        {
            CcspTraceInfo (("%s : Rebooting Cellular to factory default settings\n", __FUNCTION__));
        }
        else
        {
            CcspTraceError(("%s : returns error returning.\n", __FUNCTION__));
        }
        //Restarting the Cellular after HAL configuration
        exit(0);
    }
    else
    {
        CcspTraceError(("%s : returns error returning.\n", __FUNCTION__));
    }

    CcspTraceInfo(("%s - Exit\n",__FUNCTION__));

    //Cleanup current thread when exit
    pthread_exit(NULL);
}

int CellularMgr_ServingSystemInfo( CELLULAR_INTERFACE_INFO  *pstInterfaceInfo, CELLULAR_INTERFACE_CONTEXTPROFILE_INFO *pstContextProfileInfo)
{
    int registration_status;
    int roaming_status;
    int attach_status;

    cellular_get_serving_info(&registration_status, &roaming_status, &attach_status);
    if (pstInterfaceInfo != NULL) {
        if( DEVICE_NAS_STATUS_REGISTERED == (CellularDeviceNASStatus_t)registration_status ) {
            pstInterfaceInfo->Status = IF_UP;
        }
        else {
           pstInterfaceInfo->Status = IF_DOWN;
        }
    }
    if (pstContextProfileInfo != NULL) {
        if ( PROFILE_STATUS_ACTIVE == (CellularContextProfileStatus_t)attach_status ) {
             pstContextProfileInfo->Status = CONTEXTPROFILE_STATUS_ACTIVE;
        }
        else {
             pstContextProfileInfo->Status = CONTEXTPROFILE_STATUS_INACTIVE;
        }
    }
    return RETURN_OK;
}
int CellularMgr_SetModemEnable( BOOLEAN bEnable )
{
#if RBUS_BUILD_FLAG_ENABLE
    BOOLEAN bPrevEnable = CellularMgrSMGetCellularEnable( );
    CellularMgr_RBUS_Events_Publish_X_RDK_Enable(bPrevEnable, bEnable);
#endif

    CellularMgrSMSetCellularEnable(bEnable);
    return ( cellular_hal_set_modem_operating_configuration( ( TRUE == bEnable ) ?  CELLULAR_MODEM_SET_ONLINE : CELLULAR_MODEM_SET_OFFLINE ));
}

int CellularMgr_GetModemIMEI( char *pcIMEI )
{
    return ( cellular_hal_get_device_imei( pcIMEI ) );
}

int CellularMgr_GetModemIMEISoftwareVersion( char *pcIMEI_SV )
{
    return ( cellular_hal_get_device_imei_sv( pcIMEI_SV ) );
}

int CellularMgr_GetModemCurrentICCID( char *pcICCID )
{
    return ( cellular_hal_get_modem_current_iccid( pcICCID ) );
}

int CellularMgr_GetNetworkPacketStatisticsInfo( PCELLULAR_INTERFACE_STATS_INFO pstStatsInfo )
{
    CELLULAR_RDK_STATUS  enRDKStatus = (CELLULAR_RDK_STATUS)CellularMgrSMGetCurrentState( );

    //Needs to collect statistics only when network connected case
    if( enRDKStatus == RDK_STATUS_CONNECTED )
    {
        CellularPacketStatsStruct stNetworkPacket = { 0 };

        if( RETURN_OK == cellular_hal_get_packet_statistics( &stNetworkPacket ) )
        {
            pstStatsInfo->BytesSent             = stNetworkPacket.BytesSent;
            pstStatsInfo->BytesReceived         = stNetworkPacket.BytesReceived;
            pstStatsInfo->PacketsSent           = stNetworkPacket.PacketsSent;
            pstStatsInfo->PacketsReceived       = stNetworkPacket.PacketsReceived;
            pstStatsInfo->PacketsSentDrop       = stNetworkPacket.PacketsSentDrop;
            pstStatsInfo->PacketsReceivedDrop   = stNetworkPacket.PacketsReceivedDrop;
            pstStatsInfo->UpStreamMaxBitRate    = stNetworkPacket.UpStreamMaxBitRate;
            pstStatsInfo->DownStreamMaxBitRate  = stNetworkPacket.DownStreamMaxBitRate;

            return RETURN_OK;
        }
    }
    else
    {
        memset(pstStatsInfo, 0, sizeof(CELLULAR_INTERFACE_STATS_INFO));
        return RETURN_OK;
    }

    return RETURN_ERROR;
}

CELLULAR_CONTROL_INTERFACE_STATUS CellularMgr_GetModemControlInterfaceStatus( void )
{
    if( TRUE == cellular_hal_IsModemControlInterfaceOpened( ) )
    {
        return CONTROL_STATUS_OPENED;
    }
    else
    {   
        return CONTROL_STATUS_CLOSED;
    }
}

int CellularMgr_GetModemInterfaceStatus( CellularInterfaceStatus_t *interface_status )
{
    return ( cellular_hal_get_current_modem_interface_status( interface_status ) );
}

int CellularMgr_SetModemInterfaceEnable( BOOLEAN bEnable )
{
    //Network attach/detach operation
    if( bEnable )
    {
        return ( cellular_hal_set_modem_network_attach( ) );
    }
    else
    {
        return ( cellular_hal_set_modem_network_detach( ) );
    }
}

int CellularMgr_SetSIMPowerEnable( UINT uiSlotID, BOOLEAN bEnable )
{
    return cellular_hal_sim_power_enable(uiSlotID,bEnable);
}

int CellularMgr_GetTotalUICCSlots( UINT *puiTotalSlots )
{
    return cellular_hal_get_total_no_of_uicc_slots(puiTotalSlots);
}

int CellularMgr_GetUICCSlotInfo(UINT uiSlotID, PCELLULAR_UICC_SLOT_INFO  pstUICCSlotInfo)
{
    CellularUICCSlotInfoStruct stSlotInfo = {0};

    if( RETURN_OK == cellular_hal_get_uicc_slot_info( uiSlotID, &stSlotInfo ) )
    {
        pstUICCSlotInfo->PowerEnable = stSlotInfo.CardEnable;
        
        if( CELLULAR_UICC_STATUS_VALID == stSlotInfo.Status )
        {
            pstUICCSlotInfo->Status = SIM_STATUS_VALID;
        }
        else
        {
            pstUICCSlotInfo->Status = SIM_STATUS_EMPTY;
        }

        snprintf( pstUICCSlotInfo->Iccid, sizeof(pstUICCSlotInfo->Iccid), stSlotInfo.iccid );
        snprintf( pstUICCSlotInfo->Msisdn, sizeof(pstUICCSlotInfo->Msisdn), stSlotInfo.msisdn );
        snprintf( pstUICCSlotInfo->MnoName, sizeof(pstUICCSlotInfo->MnoName), stSlotInfo.MnoName );

        return RETURN_OK;
    }

    return RETURN_ERROR;
}

int CellularMgr_GetActiveCardStatus( CELLULAR_INTERFACE_SIM_STATUS *enCardStatus )
{
    CellularUICCStatus_t card_status;

    *enCardStatus = SIM_STATUS_EMPTY;

    if( RETURN_OK == cellular_hal_get_active_card_status( &card_status ) )
    {   
        if( CELLULAR_UICC_STATUS_VALID == card_status )
        {
            *enCardStatus = SIM_STATUS_VALID;
        }
        else
        {
            *enCardStatus = SIM_STATUS_ERROR;
        }

        return RETURN_OK;
    }

    return RETURN_ERROR;
}

int CellularMgr_GetModemFirmwareVersion(char *pcFirmwareVersion)
{
    return( cellular_hal_get_modem_firmware_version( pcFirmwareVersion ) );
}

int CellularMgr_GetPlmnInformation( PCELLULAR_PLMNACCESS_INFO pstPlmnAccessInfo)
{
    CellularCurrentPlmnInfoStruct stPlmnInfo = {0};

    if( RETURN_OK == cellular_hal_get_current_plmn_information( &stPlmnInfo ) )
    {
        pstPlmnAccessInfo->RoamingEnable = stPlmnInfo.roaming_enabled;

        if( TRUE == pstPlmnAccessInfo->RoamingEnable )
        {
            pstPlmnAccessInfo->RoamingStatus = ROAMING_STATUS_VISITOR;
        }
        else
        {
            pstPlmnAccessInfo->RoamingStatus = ROAMING_STATUS_HOME;
        }

        snprintf( pstPlmnAccessInfo->NetworkInUse_MCC, sizeof(pstPlmnAccessInfo->NetworkInUse_MCC), "%d", stPlmnInfo.MCC );
        snprintf( pstPlmnAccessInfo->NetworkInUse_MNC, sizeof(pstPlmnAccessInfo->NetworkInUse_MNC), "%d", stPlmnInfo.MNC );
        snprintf( pstPlmnAccessInfo->NetworkInUse_Name, sizeof(pstPlmnAccessInfo->NetworkInUse_Name), "%s", stPlmnInfo.plmn_name );

        snprintf( pstPlmnAccessInfo->HomeNetwork_MCC, sizeof(pstPlmnAccessInfo->HomeNetwork_MCC), "%d", stPlmnInfo.MCC );
        snprintf( pstPlmnAccessInfo->HomeNetwork_MNC, sizeof(pstPlmnAccessInfo->HomeNetwork_MNC), "%d", stPlmnInfo.MNC );
        snprintf( pstPlmnAccessInfo->HomeNetwork_Name, sizeof(pstPlmnAccessInfo->HomeNetwork_Name), "%s", stPlmnInfo.plmn_name );

        return RETURN_OK;
    }

    return RETURN_ERROR;
}

int CellularMgr_GetAvailableNetworksInformation( PCELLULAR_PLMN_AVAILABLENETWORK_INFO *ppAvailableNetworkInfo, unsigned int *puiTotalCount )
{
    CellularNetworkScanResultInfoStruct *network_info = NULL;
    unsigned int total_network_count = 0;

    if( NULL != *ppAvailableNetworkInfo )
    {
        free( *ppAvailableNetworkInfo );
        *ppAvailableNetworkInfo = NULL;
    }
 
    *puiTotalCount = 0;

    if( RETURN_OK == cellular_hal_get_available_networks_information( &network_info, &total_network_count ) )
    {
        if( 0 < total_network_count )
        {
            CELLULAR_PLMN_AVAILABLENETWORK_INFO *pstTmpInfo = NULL;
            int i = 0;

            pstTmpInfo = (CELLULAR_PLMN_AVAILABLENETWORK_INFO*) malloc( sizeof(CELLULAR_PLMN_AVAILABLENETWORK_INFO) * total_network_count );

            for( i = 0; i < total_network_count; i++ )
            {
                snprintf( pstTmpInfo[i].MCC, sizeof(pstTmpInfo[i].MCC), "%d", network_info[i].MCC );
                snprintf( pstTmpInfo[i].MNC, sizeof(pstTmpInfo[i].MNC), "%d", network_info[i].MNC );
                snprintf( pstTmpInfo[i].Name, sizeof(pstTmpInfo[i].Name), "%s", network_info[i].network_name );
                pstTmpInfo[i].Allowed = network_info[i].network_allowed_flag;
            }

            *puiTotalCount = total_network_count;
            *ppAvailableNetworkInfo = pstTmpInfo;
            
            if( NULL != network_info )
            { 
                free(network_info);
                network_info = NULL; 
            }
        }
    }

    return RETURN_OK;
}

int CellularMgr_GetModemSupportedRadioTechnology( char *pSupportedRAT )
{
    return ( cellular_hal_get_modem_supported_radio_technology(pSupportedRAT));
}

int CellularMgr_GetModemPreferredRadioTechnology( char *pPreferredRAT )
{
    return ( cellular_hal_get_modem_preferred_radio_technology( pPreferredRAT ));
}

int CellularMgr_SetModemPreferredRadioTechnology( char *pPreferredRAT )
{
    //Allow LTE and AUTO mode only 
    if ((pPreferredRAT != NULL) && ((strcmp (pPreferredRAT,"LTE") == 0) || (strcmp(pPreferredRAT,"AUTO") == 0)))
    {
       return ( cellular_hal_set_modem_preferred_radio_technology( pPreferredRAT ));
    }
    else 
       return RETURN_ERROR;

}

int CellularMgr_GetModemCurrentRadioTechnology( char *pCurrentRAT )
{
    return ( cellular_hal_get_modem_current_radio_technology(pCurrentRAT ));
}

BOOL CellularMgr_BlobUnpack(char* blob)
{
    CcspTraceInfo(("CellularMgr_BlobUnpack Function\n"));

    if(blob != NULL)
    {
        CcspTraceInfo(("---------------start of b64 decode--------------\n"));

        char * decodeMsg =NULL;
        int decodeMsgSize =0;
        int size =0;
        int err;
        //int i=0;
        //int j=0;
        //int k=0;

        msgpack_zone mempool;
        msgpack_object deserialized;
        msgpack_unpack_return unpack_ret;

        decodeMsgSize = b64_get_decoded_buffer_size(strlen(blob));

        decodeMsg = (char *) malloc(sizeof(char) * decodeMsgSize);

        size = b64_decode( blob, strlen(blob), decodeMsg );
        CcspTraceInfo(("base64 decoded data contains %d bytes\n",size));

        msgpack_zone_init(&mempool, 2048);
        unpack_ret = msgpack_unpack(decodeMsg, size, NULL, &mempool, &deserialized);

        switch(unpack_ret)
        {
            case MSGPACK_UNPACK_SUCCESS:
                CcspTraceInfo(("MSGPACK_UNPACK_SUCCESS :%d\n",unpack_ret));
                break;
            case MSGPACK_UNPACK_EXTRA_BYTES:
                CcspTraceWarning(("MSGPACK_UNPACK_EXTRA_BYTES :%d\n",unpack_ret));
                break;
            case MSGPACK_UNPACK_CONTINUE:
                CcspTraceWarning(("MSGPACK_UNPACK_CONTINUE :%d\n",unpack_ret));
                break;
            case MSGPACK_UNPACK_PARSE_ERROR:
                CcspTraceWarning(("MSGPACK_UNPACK_PARSE_ERROR :%d\n",unpack_ret));
                break;
            case MSGPACK_UNPACK_NOMEM_ERROR:
                CcspTraceWarning(("MSGPACK_UNPACK_NOMEM_ERROR :%d\n",unpack_ret));
                break;
            default:
                CcspTraceWarning(("Message Pack decode failed with error: %d\n", unpack_ret));
        }

        msgpack_zone_destroy(&mempool);
        //End of msgpack decoding
        CcspTraceInfo(("---------------End of b64 decode--------------\n"));

        if(unpack_ret == MSGPACK_UNPACK_SUCCESS)
        {
            celldoc_t *cd;
            cd = celldoc_convert( decodeMsg, size+1 );
            err = errno;
            CcspTraceError(( "errno: %s\n", celldoc_strerror(err) ));

            if ( decodeMsg )
            {
                free(decodeMsg);
                decodeMsg = NULL;
            }

            if (NULL !=cd)
            {
                CcspTraceInfo(("Subdoc_Name : %s\n", cd->subdoc_name));
                CcspTraceInfo(("Version : %lu\n", (long)cd->version));
                CcspTraceInfo(("Transaction_Id : %lu\n",(long) cd->transaction_id));
                CcspTraceInfo(("CellularModemEnable : %s\n", (1 == cd->param->cellular_modem_enable)?"true":"false"));
/*
                CcspTraceWarning(("cd->table_param->entries_count %d\n",(int) cd->table_param->entries_count));

                for(i = 0; i < (int)cd->table_param->entries_count ; i++)
                {
                    CcspTraceWarning(("cd->table_param->entries[%d].mno_name  %s\n",i, cd->table_param->entries[i].mno_name ));
                    CcspTraceWarning(("cd->table_param->entries[%d].mno_enable %s\n",i, (1 == cd->table_param->entries[i].mno_enable)?"true":"false"));
                    CcspTraceWarning(("cd->table_param->entries[%d].mno_iccid  %s\n",i, cd->table_param->entries[i].mno_iccid));
                }

                CcspTraceWarning(("cd->table_param1->entries_count %d\n",(int) cd->table_param1->entries_count));

                for(j =0; j< (int) cd->table_param1->entries_count; j++)
                {
                    CcspTraceWarning(("cd->table_param1->entries[%d].int_enable : %s\n",j, (1 == cd->table_param1->entries[j].int_enable)?"true":"false"));
                    CcspTraceWarning(("cd->table_param1->entries[%d].int_roaming_enable : %s\n",j, (1 == cd->table_param1->entries[j].int_roaming_enable)?"true":"false"));
                }

                CcspTraceWarning(("cd->table_param2->entries_count %d\n",(int) cd->table_param2->entries_count));

                for(k =0; k< (int) cd->table_param2->entries_count; k++)
                {
                    CcspTraceWarning(("cd->table_param2->entries[%d].access_mno_name : %s\n",k, cd->table_param2->entries[k].access_mno_name));
                    CcspTraceWarning(("cd->table_param2->entries[%d].access_enable : %s\n",k, (1 == cd->table_param2->entries[k].access_enable)?"true":"false"));
                    CcspTraceWarning(("cd->table_param2->entries[%d].access_roaming_enable : %s\n",k, (1 == cd->table_param2->entries[k].access_roaming_enable)?"true":"false"));
                    CcspTraceWarning(("cd->table_param2->entries[%d].access_apn : %s\n",k, cd->table_param2->entries[k].access_apn));
                    CcspTraceWarning(("cd->table_param2->entries[%d].access_apnauthentication : %s\n",k, cd->table_param2->entries[k].access_apnauthentication));
                    CcspTraceWarning(("cd->table_param2->entries[%d].access_ipaddressfamily : %s\n",k, cd->table_param2->entries[k].access_ipaddressfamily));
                }
 */

                execData *execDatacell = NULL ;
                execDatacell = (execData*) malloc (sizeof(execData));
                if ( execDatacell != NULL )
                {
                    memset(execDatacell, 0, sizeof(execData));

                    execDatacell->txid = cd->transaction_id;
                    execDatacell->version = cd->version;
                    execDatacell->numOfEntries = 0;

                    strncpy(execDatacell->subdoc_name,"cellularconfig",sizeof(execDatacell->subdoc_name)-1);

                    execDatacell->user_data = (void*) cd ;
                    execDatacell->calcTimeout = NULL ;
                    execDatacell->executeBlobRequest = Process_Cellularmgr_WebConfigRequest;
                    execDatacell->rollbackFunc = NULL ;
                    execDatacell->freeResources = freeResources_CELL ;
                    PushBlobRequest(execDatacell);
                    CcspTraceInfo(("PushBlobRequest Complete\n"));
                    return TRUE;
                }
                else
                {
                    CcspTraceError(("execData memory allocation failed\n"));
                    celldoc_destroy( cd );

                    return FALSE;
                }
	        } 
            return TRUE;
        }
        else
        {
            if ( decodeMsg )
            {
                free(decodeMsg);
                decodeMsg = NULL;
            }
            CcspTraceError(("Corrupted cellular modem enable msgpack value\n"));
            return FALSE;
        }
    }
    return TRUE;
}

void CellularMgr_FactoryReset(void)
{
    pthread_t tid;
    int ret;
    ret = pthread_create(&tid, NULL, &CellularMgr_FactoryResetThread, NULL);
    if ( ret != 0 )
    {
        CcspTraceError(("%s : Failed to create thread due to returns error: %d\n", ret, __FUNCTION__));
    }
}

int CellularMgr_GetServingInfo( CELLULAR_INTERFACE_SERVING_INFO *pstServingInfo )
{
    CellularCurrentPlmnInfoStruct plmn_info = { 0 };

    if( RETURN_OK == cellular_hal_get_current_plmn_information( &plmn_info ) )
    {
        snprintf( pstServingInfo->CellId, sizeof(pstServingInfo->CellId), "%d", plmn_info.cell_id );
        snprintf( pstServingInfo->PlmnId, sizeof(pstServingInfo->PlmnId), "%d", plmn_info.MCC*1000+plmn_info.MNC );
        snprintf( pstServingInfo->AreaCode, sizeof(pstServingInfo->AreaCode), "%d", plmn_info.area_code );
    }

    return RETURN_OK;
}

int CellularMgr_GetDeviceInfo( PCELLULAR_DML_INFO pstDeviceInfo )
{
    CellularDeviceInfoStruct devInfo = { 0 };

    if ((AnscGetTickInSeconds() - CELLULAR_HAL_FETCH_INTERVAL) < pstDeviceInfo->deviceInfoLastUpdatedTime)
    {
        /* Last HAL Get call was less than CELLULAR_HAL_FETCH_INTERVAL sec pPloam already has recent data */
        return RETURN_OK;
    }

    if( RETURN_OK == cellular_hal_get_device_information( &devInfo ) )
    {
        snprintf( pstDeviceInfo->X_RDK_Model, sizeof(pstDeviceInfo->X_RDK_Model), "%s", devInfo.Model );
        snprintf( pstDeviceInfo->X_RDK_HardwareRevision, sizeof(pstDeviceInfo->X_RDK_HardwareRevision), "%s", devInfo.HardwareRevision );
        snprintf( pstDeviceInfo->X_RDK_Vendor, sizeof(pstDeviceInfo->X_RDK_Vendor), "%s", devInfo.Vendor );
        snprintf( pstDeviceInfo->X_RDK_ControlInterface, sizeof(pstDeviceInfo->X_RDK_ControlInterface), "%s", devInfo.ControlInterface );

        /* Update Last Fetch Time in DML Structure */
        pstDeviceInfo->deviceInfoLastUpdatedTime = AnscGetTickInSeconds();
    }

    return RETURN_OK;
}
int CellularMgr_GetSupportedAccessTechnologies(char *pSupportedAccessTechnologies)
{
    return( cellular_hal_get_supported_access_technologies( pSupportedAccessTechnologies ) );
}

int CellularMgr_GetPreferedAccessTechnologies(char *pPreferedAccessTechnologies)
{
    return( cellular_hal_get_prefered_access_technologies( pPreferedAccessTechnologies ) );
}

int CellularMgr_GetCurrentAccessTechnology(char *pCurrentAccessTechnology)
{
    return( cellular_hal_get_current_access_technology( pCurrentAccessTechnology ) );
}
int CellularMgr_GetDataInterface(char *pDataInterface)
{
    return( cellular_hal_get_data_interface( pDataInterface ) );
}
CELLULAR_RDK_DEVICETYPE CellularMgr_GetModemDeviceType()
{
    return cellular_hal_get_modem_device_type();
}
int CellularMgr_SetDeviceType(PCELLULAR_DML_INFO pstDeviceInfo)
{
    return cellular_hal_set_modem_device_type(pstDeviceInfo->X_RDK_DeviceType, (void *)&pstDeviceInfo->ModemInterfaceInfo);
}
