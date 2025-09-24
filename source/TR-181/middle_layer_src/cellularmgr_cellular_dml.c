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

#include "cellularmgr_plugin_main_apis.h"
#include "cellularmgr_cellular_internal.h"
#include "cellularmgr_cellular_apis.h"
#include "cellularmgr_sm.h"
#include "cellularmgr_cellular_dml.h"
#include "cellularmgr_global.h"
#include "cellularmgr_utils.h"

extern PBACKEND_MANAGER_OBJECT               g_pBEManager;

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
/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        Cellular_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
Cellular_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_RDK_Enable", TRUE))
    {
        *pBool = pstDmlCellular->X_RDK_Enable;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "X_RDK_Enable", TRUE))
    {
        //Ignore when same configuration call
        if( bValue == pstDmlCellular->X_RDK_Enable )
        {
            return TRUE;
        }

        //Modem Operating Mode Online/Offline
        if( RETURN_OK == CellularMgr_SetModemEnable( bValue ) )
        {
            pstDmlCellular->X_RDK_Enable = bValue;
        }

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

BOOL
Cellular_SetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG                       uValue
)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
#ifdef HYBRID_SUPPORT
    if( AnscEqualString(ParamName, "X_RDK_DeviceType", TRUE))
    {
        pstDmlCellular->X_RDK_DeviceType = uValue;
        // Status of the DeviceType is updated, so call HAL and update the device
        CellularMgr_SetDeviceType(pstDmlCellular);
        return TRUE;
    }
#endif

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

*********************************************************************/
BOOL
Cellular_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_RDK_Status", TRUE))   
    {
        pstDmlCellular->X_RDK_Status = CellularMgrSMGetCurrentState( );
        *puLong = pstDmlCellular->X_RDK_Status;
        return TRUE;
    }
#ifdef HYBRID_SUPPORT
    if( AnscEqualString(ParamName, "X_RDK_DeviceType", TRUE))
    {
        pstDmlCellular->X_RDK_DeviceType = CellularMgr_GetModemDeviceType();
        *puLong = pstDmlCellular->X_RDK_DeviceType;
        return TRUE;
    }
#endif
    if( AnscEqualString(ParamName, "X_RDK_ControlInterfaceStatus", TRUE))   
    {
        pstDmlCellular->X_RDK_ControlInterfaceStatus = CellularMgr_GetModemControlInterfaceStatus( );
        *puLong = pstDmlCellular->X_RDK_ControlInterfaceStatus;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_DataInterfaceLink", TRUE))   
    {
        *puLong = pstDmlCellular->X_RDK_DataInterfaceLink;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Cellular_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    CellularMgr_GetDeviceInfo(pstDmlCellular);
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_RDK_Model", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstDmlCellular->X_RDK_Model);
        return 0;
    }

    if( AnscEqualString(ParamName, "X_RDK_HardwareRevision", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstDmlCellular->X_RDK_HardwareRevision);
        return 0;
    }

    if( AnscEqualString(ParamName, "X_RDK_Vendor", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstDmlCellular->X_RDK_Vendor);
        return 0;
    }

    if( AnscEqualString(ParamName, "X_RDK_ControlInterface", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstDmlCellular->X_RDK_ControlInterface);
        return 0;
    }

    if( AnscEqualString(ParamName, "X_RDK_DataInterface", TRUE))
    {
        /* collect value */
        CellularMgr_GetDataInterface(pstDmlCellular->X_RDK_DataInterface);
        AnscCopyString(pValue, pstDmlCellular->X_RDK_DataInterface);
        return 0;
    }
    
    if( AnscEqualString(ParamName, "CellularConfig", TRUE))
    {
        CcspTraceInfo(("Data Get Not supported\n"));
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if( AnscEqualString(ParamName, "CellularConfig", TRUE))
    {
        if( CellularMgr_BlobUnpack( pString))
        {
            CcspTraceInfo(("Data Received from WebConfig\n"))
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
Cellular_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Cellular_Commit
    (
        ANSC_HANDLE                 hInsContext
    )

{
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Cellular_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}

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
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if( AnscEqualString(ParamName, "Imei", TRUE))
    {
        /* collect value */
        CellularMgr_GetModemIMEI(pstDmlCellular->X_RDK_Imei);
        AnscCopyString(pValue, pstDmlCellular->X_RDK_Imei);
        return 0;
    }

    return -1;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        DeviceManagement_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
DeviceManagement_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "FactoryReset", TRUE))
    {
        /* always return false when get */
        *pBool = FALSE;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        DeviceManagement_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
DeviceManagement_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "FactoryReset", TRUE))
    {
        CcspTraceInfo (("Cellular Modem FactoryReset...\n"));
        CellularMgr_FactoryReset();
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

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
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if( AnscEqualString(ParamName, "CurrentImageVersion", TRUE))
    {
        /* collect value */
        CellularMgr_GetModemFirmwareVersion(pstDmlCellular->CurrentImageVersion);
        AnscCopyString(pValue, pstDmlCellular->CurrentImageVersion);
        return 0;
    }

    if( AnscEqualString(ParamName, "FallbackImageVersion", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstDmlCellular->FallbackImageVersion);
        return 0;
    }

    return -1;
}

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
    *  HomeNetwork_GetParamStringValue
    *  NetworkInUse_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_Interface_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Cellular_Interface_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    return pstDmlCellular->ulInterfaceNoEntries;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        Cellular_Interface_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Cellular_Interface_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    *pInsNumber = nIndex + 1;

    return (&pstDmlCellular->pstInterfaceInfo[nIndex]); /* return the handle */
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        Cellular_Interface_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
Cellular_Interface_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE) )
    {
        *pBool = pstInterfaceInfo->Enable;
        return TRUE;
    }
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
    if( AnscEqualString(ParamName, "Upstream", TRUE) )
    {
        *pBool = pstInterfaceInfo->Upstream;
        return TRUE;
    }
#endif

    if( AnscEqualString(ParamName, "X_RDK_PhyConnectedStatus", TRUE))   
    {
        /* State Registed or Connected then Physical Status should be true or false */
        CellularPolicySmState_t smState = CellularMgrSMGetCurrentState( );

        if( ( CELLULAR_STATE_REGISTERED == smState ) || 
            ( CELLULAR_STATE_CONNECTED == smState ) )
        {
            pstInterfaceInfo->X_RDK_PhyConnectedStatus = TRUE;
        }
        else
        {
            pstInterfaceInfo->X_RDK_PhyConnectedStatus = FALSE;
        }

        *pBool = pstInterfaceInfo->X_RDK_PhyConnectedStatus;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_LinkAvailableStatus", TRUE))   
    {
        /* State Connected then Link Available Status should be true or false */
        CellularPolicySmState_t smState = CellularMgrSMGetCurrentState( );

        if ( CELLULAR_STATE_CONNECTED == smState )
        {
            pstInterfaceInfo->X_RDK_LinkAvailableStatus = TRUE;
        }
        else
        {
            pstInterfaceInfo->X_RDK_LinkAvailableStatus = FALSE;
        }

        *pBool = pstInterfaceInfo->X_RDK_LinkAvailableStatus;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_Interface_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_Interface_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        if( pstInterfaceInfo->Enable == bValue )
        {
            return TRUE;
        }

        CellularMgr_SetModemInterfaceEnable( bValue );
        pstInterfaceInfo->Enable = bValue;
        return TRUE;
    }
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
    if( AnscEqualString(ParamName, "Upstream", TRUE))
    {
        pstInterfaceInfo->Upstream = bValue;
        return TRUE;
    }
#endif

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_Interface_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_Interface_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))   
    {
        //CellularMgr_GetModemInterfaceStatus( &(pstInterfaceInfo->Status) );
	CellularMgr_ServingSystemInfo (pstInterfaceInfo, NULL);
        *puLong = pstInterfaceInfo->Status;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_RegisteredService", TRUE))   
    {
        CellularMgrGetNetworkRegisteredService(&pstInterfaceInfo->X_RDK_RegisteredService);
        *puLong = pstInterfaceInfo->X_RDK_RegisteredService;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "LastChange", TRUE))   
    {
        *puLong = pstInterfaceInfo->LastChange;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "RegistrationRetries", TRUE))   
    {
        *puLong = pstInterfaceInfo->RegistrationRetries;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "MaxRegistrationRetries", TRUE))   
    {
        *puLong = pstInterfaceInfo->MaxRegistrationRetries;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "RegistrationRetryTimer", TRUE))   
    {
        *puLong = pstInterfaceInfo->RegistrationRetryTimer;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_Interface_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_Interface_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "RegistrationRetries", TRUE))   
    {
        pstInterfaceInfo->RegistrationRetries = uValue;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "MaxRegistrationRetries", TRUE))   
    {
        pstInterfaceInfo->MaxRegistrationRetries = uValue;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "RegistrationRetryTimer", TRUE))   
    {
        pstInterfaceInfo->RegistrationRetryTimer = uValue;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_Interface_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Cellular_Interface_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstInterfaceInfo->Alias);
        return 0;
    }

    if( AnscEqualString(ParamName, "Name", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstInterfaceInfo->Name);
        return 0;
    }

    if( AnscEqualString(ParamName, "LowerLayers", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstInterfaceInfo->LowerLayers);
        return 0;
    }
  
    if( AnscEqualString(ParamName, "IMEI", TRUE))
    {
        /* collect value */
        CellularMgr_GetModemIMEI(pstInterfaceInfo->Imei);
        AnscCopyString(pValue, pstInterfaceInfo->Imei);
        return 0;
    }

    if( AnscEqualString(ParamName, "SupportedAccessTechnologies", TRUE))
    {
        /* collect value */
	CellularMgr_GetModemSupportedRadioTechnology(pstInterfaceInfo->SupportedAccessTechnologies);
        AnscCopyString(pValue, pstInterfaceInfo->SupportedAccessTechnologies);
        return 0;
    }

    if( AnscEqualString(ParamName, "PreferredAccessTechnologies", TRUE))
    {
        /* collect value */
	CellularMgr_GetModemPreferredRadioTechnology( pstInterfaceInfo->PreferredAccessTechnologies );
        AnscCopyString(pValue, pstInterfaceInfo->PreferredAccessTechnologies);
        return 0;
    }

    if( AnscEqualString(ParamName, "CurrentAccessTechnology", TRUE))
    {
        /* collect value */
	CellularMgr_GetModemCurrentRadioTechnology( pstInterfaceInfo->CurrentAccessTechnology );
        AnscCopyString(pValue, pstInterfaceInfo->CurrentAccessTechnology);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_Interface_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_Interface_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "LowerLayers", TRUE))
    {
        /* save update to backup */
        AnscCopyString(pstInterfaceInfo->LowerLayers, pString);
        return TRUE;
    }
    
    if( AnscEqualString(ParamName, "PreferredAccessTechnologies", TRUE))
    {
        /* save update to backup */
        if( RETURN_OK == CellularMgr_SetModemPreferredRadioTechnology( pString))
        {
           AnscCopyString(pstInterfaceInfo->PreferredAccessTechnologies, pString);
           return TRUE;
        }   
	else
	   return FALSE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_Interface_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
Cellular_Interface_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_Interface_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Cellular_Interface_Commit
    (
        ANSC_HANDLE                 hInsContext
    )

{
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_Interface_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Cellular_Interface_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}

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
    )
{
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Imei", TRUE))
    {
        /* collect value */
        CellularMgr_GetModemIMEI(pstInterfaceInfo->Imei);
        AnscCopyString(pValue, pstInterfaceInfo->Imei);
        return 0;
    }

    if( AnscEqualString(ParamName, "Iccid", TRUE))
    {
        /* collect value */
        CellularMgr_GetModemCurrentICCID(pstInterfaceInfo->Iccid);
        AnscCopyString(pValue, pstInterfaceInfo->Iccid);
        return 0;
    }
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PlmnAccess_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PlmnAccess_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_INTERFACE_INFO     pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);

    CellularMgr_GetPlmnInformation(pstPlmnInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "RoamingStatus", TRUE))   
    {
        *puLong = pstPlmnInfo->RoamingStatus;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        PlmnAccess_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
PlmnAccess_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULAR_INTERFACE_INFO     pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    
    CellularMgr_GetPlmnInformation(pstPlmnInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "RoamingEnable", TRUE) )
    {
        *pBool = pstPlmnInfo->RoamingEnable;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        HomeNetwork_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
HomeNetwork_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_INTERFACE_INFO     pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);

    CellularMgr_GetPlmnInformation(pstPlmnInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Mcc", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstPlmnInfo->HomeNetwork_MCC);
        return 0;
    }

    if( AnscEqualString(ParamName, "Mnc", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstPlmnInfo->HomeNetwork_MNC);
        return 0;
    }

    if( AnscEqualString(ParamName, "Name", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstPlmnInfo->HomeNetwork_Name);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        NetworkInUse_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
NetworkInUse_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_INTERFACE_INFO     pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);

    CellularMgr_GetPlmnInformation(pstPlmnInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Mcc", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstPlmnInfo->NetworkInUse_MCC);
        return 0;
    }

    if( AnscEqualString(ParamName, "Mnc", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstPlmnInfo->NetworkInUse_MNC);
        return 0;
    }

    if( AnscEqualString(ParamName, "Name", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstPlmnInfo->NetworkInUse_Name);
        return 0;
    }

    return -1;
}

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

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        AvailableNetworks_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
AvailableNetworks_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PCELLULAR_INTERFACE_INFO     pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);

	if ( ( AnscGetTickInSeconds() - pstPlmnInfo->ulAvailableNetworkListLastUpdatedTime ) < CELLULAR_AVAILABLE_NETWORK_LIST_REFRESH_THRESHOLD )
    {
		return FALSE;
    }
	else 
    {
    	pstPlmnInfo->ulAvailableNetworkListLastUpdatedTime =  AnscGetTickInSeconds();
    	return TRUE;
	}	
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        AvailableNetworks_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
AvailableNetworks_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{    
    PCELLULAR_INTERFACE_INFO     pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);

    CellularMgr_GetAvailableNetworksInformation( &pstPlmnInfo->pstAvailableNetworks, &pstPlmnInfo->ulAvailableNetworkNoOfEntries );

    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        AvailableNetworks_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
AvailableNetworks_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULAR_INTERFACE_INFO     pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);

    return pstPlmnInfo->ulAvailableNetworkNoOfEntries;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        AvailableNetworks_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
AvailableNetworks_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{    
    PCELLULAR_INTERFACE_INFO    pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_PLMNACCESS_INFO   pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);

    *pInsNumber = nIndex + 1;

    return (&(pstPlmnInfo->pstAvailableNetworks[nIndex])); /* return the handle */
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        AvailableNetworks_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
AvailableNetworks_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULAR_PLMN_AVAILABLENETWORK_INFO  pstAvailableNetworks =(PCELLULAR_PLMN_AVAILABLENETWORK_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Allowed", TRUE) )
    {
        *pBool = pstAvailableNetworks->Allowed;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        AvailableNetworks_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
AvailableNetworks_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_PLMN_AVAILABLENETWORK_INFO  pstAvailableNetworks =(PCELLULAR_PLMN_AVAILABLENETWORK_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Mcc", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstAvailableNetworks->MCC);
        return 0;
    }

    if( AnscEqualString(ParamName, "Mnc", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstAvailableNetworks->MNC);
        return 0;
    }

    if( AnscEqualString(ParamName, "Name", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstAvailableNetworks->Name);
        return 0;
    }

    return -1;
}

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_RadioSignal.

    *  RadioSignal_GetParamUlongValue
    *  RadioSignal_GetParamIntValue
    *  RadioSignal_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        RadioSignal_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
RadioSignal_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_INTERFACE_INFO            pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_INTERFACE_SERVING_INFO    pstServingInfo   = &(pstInterfaceInfo->stServingInfo);

    CellularMgr_RadioSignalGetSignalInfo( pstServingInfo );

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Rat", TRUE))   
    {
        *puLong = pstServingInfo->Rat;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Rfcn", TRUE))   
    {
        *puLong = pstServingInfo->Rfcn;
        return TRUE;
    }
    
    if( AnscEqualString(ParamName, "RadioEnvConditions", TRUE))   
    {
        pstInterfaceInfo->RadioEnvConditions = CellularMgr_GetRadioEnvConditions( );
        *puLong = pstInterfaceInfo->RadioEnvConditions;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        RadioSignal_GetParamIntValue
            (   
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
RadioSignal_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    PCELLULAR_INTERFACE_INFO            pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_INTERFACE_SERVING_INFO    pstServingInfo   = &(pstInterfaceInfo->stServingInfo);

    CellularMgr_RadioSignalGetSignalInfo( pstServingInfo );

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Rssi", TRUE))
    {
        *pInt = pstServingInfo->Rssi;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Snr", TRUE))
    {
        *pInt = pstServingInfo->Snr;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Rsrp", TRUE))
    {
        *pInt = pstServingInfo->Rsrp;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Rsrq", TRUE))
    {
        *pInt = pstServingInfo->Rsrq;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Trx", TRUE))
    {
        *pInt = pstServingInfo->Trx;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        RadioSignal_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
RadioSignal_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_INTERFACE_INFO            pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_INTERFACE_SERVING_INFO    pstServingInfo   = &(pstInterfaceInfo->stServingInfo);

    CellularMgr_GetServingInfo( pstServingInfo );

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "CellId", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstServingInfo->CellId);
        return 0;
    }

    if( AnscEqualString(ParamName, "PlmnId", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstServingInfo->PlmnId);
        return 0;
    }

    if( AnscEqualString(ParamName, "AreaCode", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pstServingInfo->AreaCode);
        return 0;
    }

    return -1;
}

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
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        NeighborCell_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
NeighborCell_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        NeighborCell_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
NeighborCell_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{    
    //TODO - Need to list all neighbourcell from Modem

    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        NeighborCell_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
NeighborCell_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULAR_INTERFACE_INFO            pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    return pstInterfaceInfo->ulNeighbourNoOfEntries;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        NeighborCell_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
NeighborCell_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{    
    PCELLULAR_INTERFACE_INFO    pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    *pInsNumber = nIndex + 1;

    return (&(pstInterfaceInfo->pstNeighbourInfo[nIndex])); /* return the handle */
}

/**********************************************************************
    caller:     owner of this object
    prototype:
        ULONG
        NeighborCell_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );
    description:
        This function is called to retrieve string parameter value;
    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;
                char*                       ParamName,
                The parameter name;
                char*                       pValue,
                The string value buffer;
                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;
    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.
**********************************************************************/

ULONG
NeighborCell_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO   pstNeighbourInfo = (PCELLULAR_INTERFACE_NEIGHBOUR_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "PlmnId", TRUE) )
    {
        AnscCopyString(pValue, pstNeighbourInfo->PlmnId);
        return 0;
    }

    if( AnscEqualString(ParamName, "AreaCode", TRUE) )
    {
        AnscCopyString(pValue, pstNeighbourInfo->AreaCode);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        NeighborCell_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
NeighborCell_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO   pstNeighbourInfo = (PCELLULAR_INTERFACE_NEIGHBOUR_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Rat", TRUE))   
    {
        *puLong = pstNeighbourInfo->Rat;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Rfcn", TRUE))   
    {
        *puLong = pstNeighbourInfo->Rfcn;    
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        NeighborCell_GetParamIntValue
            (   
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
NeighborCell_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO   pstNeighbourInfo = (PCELLULAR_INTERFACE_NEIGHBOUR_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "ReceivedSignal", TRUE))
    {
        *pInt = pstNeighbourInfo->ReceivedSignal;
        return TRUE;
    }

    return FALSE;
}

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
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        ContextProfile_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
ContextProfile_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        ContextProfile_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
ContextProfile_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULAR_INTERFACE_INFO                   pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    int i;

    for( i = 0; i < pstInterfaceInfo->ulContextProfileNoOfEntries; i++ )
    {
        PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO    pstContextProfileInfo = &(pstInterfaceInfo->pstContextProfileInfo[i]);

        //Get current context information
        CellularMgrGetCurrentPDPContextStatusInformation( pstContextProfileInfo );
    }

    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        ContextProfile_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
ContextProfile_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULAR_INTERFACE_INFO    pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;

    return pstInterfaceInfo->ulContextProfileNoOfEntries;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        ContextProfile_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
ContextProfile_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{    
    PCELLULAR_INTERFACE_INFO                   pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO    pstContextProfileInfo = &(pstInterfaceInfo->pstContextProfileInfo[nIndex]);

    *pInsNumber = nIndex + 1;

    return (pstContextProfileInfo); /* return the handle */
}

/**********************************************************************
    caller:     owner of this object
    prototype:
        ULONG
        ContextProfile_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );
    description:
        This function is called to retrieve string parameter value;
    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;
                char*                       ParamName,
                The parameter name;
                char*                       pValue,
                The string value buffer;
                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;
    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.
**********************************************************************/

ULONG
ContextProfile_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO   pstContextProfileInfo = (PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Apn", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Apn);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv4Adress", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv4Adress);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv4SubnetMask", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv4SubnetMask);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv4Gateway", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv4Gateway);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv4PrimaryDns", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv4PrimaryDns);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv4SecondaryDns", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv4SecondaryDns);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv6Address", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv6Address);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv6Gateway", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv6Gateway);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv6PrimaryDns", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv6PrimaryDns);
        return 0;
    }

    if( AnscEqualString(ParamName, "Ipv6SecondaryDns", TRUE) )
    {
        AnscCopyString(pValue, pstContextProfileInfo->Ipv6SecondaryDns);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        ContextProfile_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
ContextProfile_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO   pstContextProfileInfo = (PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))   
    {
        CellularMgr_ServingSystemInfo ( NULL, pstContextProfileInfo);
        *puLong = pstContextProfileInfo->Status;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Type", TRUE))   
    {
        *puLong = pstContextProfileInfo->Type;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "IpAddressFamily", TRUE))   
    {
        *puLong = pstContextProfileInfo->IpAddressFamily;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "MtuSize", TRUE))   
    {
        *puLong = pstContextProfileInfo->MtuSize;    
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        ContextProfile_GetParamIntValue
            (   
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
ContextProfile_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO   pstNeighbourInfo = (PCELLULAR_INTERFACE_NEIGHBOUR_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "ReceivedSignal", TRUE))
    {
        *pInt = pstNeighbourInfo->ReceivedSignal;
        return TRUE;
    }

    return FALSE;
}

/***********************************************************************

 APIs for Object:

    Cellular.Interface.{i}.X_RDK_Statistics.

    *  X_RDK_Statistics_GetParamUlongValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        X_RDK_Statistics_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
X_RDK_Statistics_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_INTERFACE_INFO           pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)hInsContext;
    PCELLULAR_INTERFACE_STATS_INFO     pstStatsInfo     = &(pstInterfaceInfo->stStatsInfo);

    CellularMgr_GetNetworkPacketStatisticsInfo( pstStatsInfo );

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "BytesSent", TRUE))   
    {
        *puLong = pstStatsInfo->BytesSent;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "BytesReceived", TRUE))   
    {
        *puLong = pstStatsInfo->BytesReceived;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "PacketsSent", TRUE))   
    {
        *puLong = pstStatsInfo->PacketsSent;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "PacketsReceived", TRUE))   
    {
        *puLong = pstStatsInfo->PacketsReceived;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "PacketsSentDrop", TRUE))   
    {
        *puLong = pstStatsInfo->PacketsSentDrop;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "PacketsReceivedDrop", TRUE))   
    {
        *puLong = pstStatsInfo->PacketsReceivedDrop;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "UpStreamMaxBitRate", TRUE))   
    {
        *puLong = pstStatsInfo->UpStreamMaxBitRate;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "DownStreamMaxBitRate", TRUE))   
    {
        *puLong = pstStatsInfo->DownStreamMaxBitRate;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

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
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        UiccSlot_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
UiccSlot_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{

    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

	if ( ( AnscGetTickInSeconds() - pstDmlCellular->ulUICCSlotListLastUpdatedTime ) < CELLULAR_UICCSLOT_LIST_REFRESH_THRESHOLD )
    {
		return FALSE;
    }
	else 
    {
    	pstDmlCellular->ulUICCSlotListLastUpdatedTime =  AnscGetTickInSeconds();
    	return TRUE;
	}	

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        UiccSlot_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
UiccSlot_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }
    //Allocate DML Memory if already not created
    if( NULL == pstDmlCellular->pstUICCSlotInfo )
    {
        UINT  uiTotalSlots = 0;

        if( ( RETURN_OK == CellularMgr_GetTotalUICCSlots( &uiTotalSlots ) ) &&
            ( uiTotalSlots > 0 ) )
        {
            pstDmlCellular->ulUICCNoOfEntries = uiTotalSlots;
            pstDmlCellular->pstUICCSlotInfo = (PCELLULAR_DML_INFO) malloc( sizeof(CELLULAR_DML_INFO) * uiTotalSlots );
            memset(pstDmlCellular->pstUICCSlotInfo, 0, sizeof(CELLULAR_DML_INFO) * uiTotalSlots);
        }
    }

    if( ( pstDmlCellular->pstUICCSlotInfo != NULL ) && ( pstDmlCellular->ulUICCNoOfEntries > 0 ) )
    {
        UINT  i;

        for( i = 0; i < pstDmlCellular->ulUICCNoOfEntries; i++ )
        {
            CellularMgr_GetUICCSlotInfo( i, &(pstDmlCellular->pstUICCSlotInfo[i]) );
        }
    }

    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        UiccSlot_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
UiccSlot_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return 0;
    }

    return pstDmlCellular->ulUICCNoOfEntries;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        UiccSlot_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
UiccSlot_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if ((pstDmlCellular == NULL) || (pstDmlCellular->pstUICCSlotInfo == NULL))
    {
        return NULL;
    }
    PCELLULAR_UICC_SLOT_INFO    pstUICCSlotInfo = &pstDmlCellular->pstUICCSlotInfo[nIndex];
    if (pstUICCSlotInfo == NULL)
    {
        return NULL;
    }

    pstUICCSlotInfo->uiInstanceNumber = nIndex + 1;
    *pInsNumber = pstUICCSlotInfo->uiInstanceNumber;

    return (pstUICCSlotInfo); /* return the handle */
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        UiccSlot_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
UiccSlot_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULAR_UICC_SLOT_INFO       pstSlotInfo = (PCELLULAR_UICC_SLOT_INFO)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE) )
    {
        *pBool = pstSlotInfo->PowerEnable;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        UiccSlot_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UiccSlot_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCELLULAR_UICC_SLOT_INFO       pstSlotInfo = (PCELLULAR_UICC_SLOT_INFO)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE) )
    {
        if( pstSlotInfo->PowerEnable == bValue )
        {
            return TRUE;
        }

        if( RETURN_OK == CellularMgr_SetSIMPowerEnable( pstSlotInfo->uiInstanceNumber - 1, bValue ) )
        {
            pstSlotInfo->PowerEnable = bValue;
            return TRUE;
        }
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        UiccSlot_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
UiccSlot_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_UICC_SLOT_INFO       pstSlotInfo = (PCELLULAR_UICC_SLOT_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))   
    {
        *puLong = pstSlotInfo->Status;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "FormFactor", TRUE))   
    {
        *puLong = pstSlotInfo->FormFactor;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Application", TRUE))   
    {
        *puLong = pstSlotInfo->Application;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************
    caller:     owner of this object
    prototype:
        ULONG
        UiccSlot_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );
    description:
        This function is called to retrieve string parameter value;
    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;
                char*                       ParamName,
                The parameter name;
                char*                       pValue,
                The string value buffer;
                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;
    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.
**********************************************************************/

ULONG
UiccSlot_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_UICC_SLOT_INFO       pstSlotInfo = (PCELLULAR_UICC_SLOT_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "MnoName", TRUE) )
    {
        AnscCopyString(pValue, pstSlotInfo->MnoName);
        return 0;
    }

    if( AnscEqualString(ParamName, "Iccid", TRUE) )
    {
        AnscCopyString(pValue, pstSlotInfo->Iccid);
        return 0;
    }

    if( AnscEqualString(ParamName, "Msisdn", TRUE) )
    {
        AnscCopyString(pValue, pstSlotInfo->Msisdn);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        UiccSlot_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
UiccSlot_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        UiccSlot_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
UiccSlot_Commit
    (
        ANSC_HANDLE                 hInsContext
    )

{
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        UiccSlot_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
UiccSlot_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}

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
/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        EuiccSlot_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
EuiccSlot_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "PowerEnable", TRUE) )
    {
        *pBool = pstSlotInfo->PowerEnable;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        EuiccSlot_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
EuiccSlot_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "PowerEnable", TRUE) )
    {
        pstSlotInfo->PowerEnable = bValue;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        EuiccSlot_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
EuiccSlot_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))   
    {
        *puLong = pstSlotInfo->Status;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "FormFactor", TRUE))   
    {
        *puLong = pstSlotInfo->FormFactor;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Application", TRUE))   
    {
        *puLong = pstSlotInfo->Application;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************
    caller:     owner of this object
    prototype:
        ULONG
        EuiccSlot_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );
    description:
        This function is called to retrieve string parameter value;
    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;
                char*                       ParamName,
                The parameter name;
                char*                       pValue,
                The string value buffer;
                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;
    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.
**********************************************************************/

ULONG
EuiccSlot_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Imei", TRUE) )
    {
        AnscCopyString(pValue, pstSlotInfo->Imei);
        return 0;
    }

    if( AnscEqualString(ParamName, "EIccid", TRUE) )
    {
        AnscCopyString(pValue, pstSlotInfo->EIccid);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        EuiccSlot_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
EuiccSlot_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        EuiccSlot_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
EuiccSlot_Commit
    (
        ANSC_HANDLE                 hInsContext
    )

{
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        EuiccSlot_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
EuiccSlot_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}

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
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        MnoProfile_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
MnoProfile_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        MnoProfile_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
MnoProfile_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{    
    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        MnoProfile_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
MnoProfile_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);

    return pstSlotInfo->ulMNOProfileNoOfEntries;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        MnoProfile_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
MnoProfile_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);

    *pInsNumber = nIndex + 1;

    return (&pstSlotInfo->pstMNPOProfileInfo[nIndex]); /* return the handle */
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        MnoProfile_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
MnoProfile_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULAR_MNO_PROFILE_INFO       pstMNPOProfileInfo = (PCELLULAR_MNO_PROFILE_INFO) hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE) )
    {
        *pBool = pstMNPOProfileInfo->Enable;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************
    caller:     owner of this object
    prototype:
        ULONG
        MnoProfile_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );
    description:
        This function is called to retrieve string parameter value;
    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;
                char*                       ParamName,
                The parameter name;
                char*                       pValue,
                The string value buffer;
                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;
    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.
**********************************************************************/

ULONG
MnoProfile_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_MNO_PROFILE_INFO       pstMNPOProfileInfo = (PCELLULAR_MNO_PROFILE_INFO) hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Iccid", TRUE) )
    {
        AnscCopyString(pValue, pstMNPOProfileInfo->Iccid);
        return 0;
    }

    if( AnscEqualString(ParamName, "Msisdn", TRUE) )
    {
        AnscCopyString(pValue, pstMNPOProfileInfo->Msisdn);
        return 0;
    }

    if( AnscEqualString(ParamName, "Imsi", TRUE) )
    {
        AnscCopyString(pValue, pstMNPOProfileInfo->Imsi);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        MnoProfile_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
MnoProfile_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_MNO_PROFILE_INFO       pstMNPOProfileInfo = (PCELLULAR_MNO_PROFILE_INFO) hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))   
    {
        *puLong = pstMNPOProfileInfo->Status;    
        return TRUE;
    }

    return FALSE;
}

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
/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE Cellular_AccessPoint_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);

    description:

        This function is called to add a new entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle of new added entry.

**********************************************************************/

ANSC_HANDLE Cellular_AccessPoint_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber)
{
    PCELLULARMGR_CELLULAR_DATA              pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO                      pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO    pstProfileInfo = NULL;
    UINT                                    ulTotalCount; 

    //Increment one index
    ulTotalCount = pstDmlCellular->ulAccessPointNoOfEntries + 1;

    //Allocate new memory for AP Info
    pstProfileInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)malloc((ulTotalCount) * sizeof(CELLULAR_INTERFACE_ACCESSPOINT_INFO));

    if( NULL == pstProfileInfo )
    {
        return NULL;
    }

    //Copy existing AP info to new memory
    memset( pstProfileInfo, 0, (ulTotalCount) * sizeof(CELLULAR_INTERFACE_ACCESSPOINT_INFO) );
    memcpy( pstProfileInfo, pstDmlCellular->pstAPInfo, (pstDmlCellular->ulAccessPointNoOfEntries) * sizeof(CELLULAR_INTERFACE_ACCESSPOINT_INFO) );

    //Free existing resource
    if( NULL != pstDmlCellular->pstAPInfo )
    {
        free(pstDmlCellular->pstAPInfo);
        pstDmlCellular->pstAPInfo = NULL;
    }

    //Assign new memory to actual structure
    pstDmlCellular->ulAccessPointNoOfEntries = ulTotalCount;
    pstProfileInfo[ulTotalCount - 1].bIsThisNewlyAddedRecord = TRUE;
    pstProfileInfo[ulTotalCount - 1].X_RDK_Roaming = TRUE;
    pstDmlCellular->pstAPInfo = pstProfileInfo;
    *pInsNumber = pstDmlCellular->ulAccessPointNoOfEntries;

    //Needs to update current time during add since new profile can be updated withing TTL threshold
    pstDmlCellular->ulAccessPointListLastUpdatedTime =  AnscGetTickInSeconds();

    return ((ANSC_HANDLE)&(pstDmlCellular->pstAPInfo[pstDmlCellular->ulAccessPointNoOfEntries - 1]));
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG Cellular_AccessPoint_DeleteEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);

    description:

        This function is called to delete an exist entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ANSC_HANDLE                 hInstance
                The exist entry handle;

    return:     The status of the operation.

**********************************************************************/

ULONG Cellular_AccessPoint_DeleteEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance)
{
    PCELLULARMGR_CELLULAR_DATA              pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO                      pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO    pstProfileInfo =  (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInstance;
    UINT                                    ulTotalCount; 
    ANSC_STATUS                             returnStatus   = ANSC_STATUS_SUCCESS;

    //Check if this is default profile or not
    if( TRUE == pstProfileInfo->X_RDK_Default )
    {
        CcspTraceError(("%s Failed to delete default profile\n",__FUNCTION__))
        return -1;
    }

    CcspTraceError(("%s Deleting this '%d' Profile\n",__FUNCTION__,pstProfileInfo->ProfileIndex))

    //Delete Profile
    CellularMgr_AccessPointDeleteProfile( pstProfileInfo );
    
    pstDmlCellular->ulAccessPointListLastUpdatedTime =  0; //To resync next time after delete

    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        Cellular_AccessPoint_IsUpdated
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is checking whether the table is updated or not.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     TRUE or FALSE.

**********************************************************************/
BOOL
Cellular_AccessPoint_IsUpdated
    (
        ANSC_HANDLE                 hInsContext
    )
{

    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

	if ( ( 0 < pstDmlCellular->ulAccessPointNoOfEntries ) &&
         ( ( AnscGetTickInSeconds() - pstDmlCellular->ulAccessPointListLastUpdatedTime ) < CELLULAR_ACCESSPOINT_LIST_REFRESH_THRESHOLD ) )
    {
		return FALSE;
    }
	else 
    {
    	pstDmlCellular->ulAccessPointListLastUpdatedTime =  AnscGetTickInSeconds();
    	return TRUE;
	}	

    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        Cellular_AccessPoint_Synchronize
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to synchronize the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Cellular_AccessPoint_Synchronize
    (
        ANSC_HANDLE                 hInsContext
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    CellularMgr_AccessPointGetProfileList( &(pstDmlCellular->pstAPInfo), (int*)&pstDmlCellular->ulAccessPointNoOfEntries);

    return ANSC_STATUS_SUCCESS;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_AccessPoint_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Cellular_AccessPoint_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{   
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return 0;
    }

    return pstDmlCellular->ulAccessPointNoOfEntries;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        Cellular_AccessPoint_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Cellular_AccessPoint_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if ((pstDmlCellular == NULL) || (pstDmlCellular->pstAPInfo == NULL))
    {
        return NULL;
    }

    *pInsNumber = nIndex + 1;

    return (&pstDmlCellular->pstAPInfo[nIndex]); /* return the handle */
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        BOOL
        Cellular_AccessPoint_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:
        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.
**********************************************************************/
BOOL
Cellular_AccessPoint_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE) )
    {
        *pBool = pstAPInfo->Enable;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_DefaultProfile", TRUE) )
    {
        *pBool = pstAPInfo->X_RDK_Default;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_Roaming", TRUE) )
    {
        *pBool = pstAPInfo->X_RDK_Roaming;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_AccessPoint_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_AccessPoint_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        pstAPInfo->Enable = bValue;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_DefaultProfile", TRUE))
    {
        pstAPInfo->X_RDK_Default = bValue;
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_Roaming", TRUE))
    {
        pstAPInfo->X_RDK_Roaming = bValue;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************
    caller:     owner of this object
    prototype:
        ULONG
        Cellular_AccessPoint_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );
    description:
        This function is called to retrieve string parameter value;
    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;
                char*                       ParamName,
                The parameter name;
                char*                       pValue,
                The string value buffer;
                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;
    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.
**********************************************************************/

ULONG
Cellular_AccessPoint_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Alias", TRUE) )
    {
        AnscCopyString(pValue, pstAPInfo->Alias);
        return 0;
    }

    if( AnscEqualString(ParamName, "Apn", TRUE) )
    {
        AnscCopyString(pValue, pstAPInfo->APN);
        return 0;
    }

    if( AnscEqualString(ParamName, "Username", TRUE) )
    {
        AnscCopyString(pValue, pstAPInfo->Username);
        return 0;
    }

    if( AnscEqualString(ParamName, "Password", TRUE) )
    {
        AnscCopyString(pValue, pstAPInfo->Password);
        return 0;
    }

    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_AccessPoint_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_AccessPoint_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        /* save update to backup */
        AnscCopyString(pstAPInfo->Alias, pString);
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Apn", TRUE) )
    {
        /* save update to backup */
        AnscCopyString(pstAPInfo->APN, pString);
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Username", TRUE) )
    {
        /* save update to backup */
        AnscCopyString(pstAPInfo->Username, pString);
        return TRUE;
    }

    if( AnscEqualString(ParamName, "Password", TRUE) )
    {
        /* save update to backup */
        AnscCopyString(pstAPInfo->Password, pString);
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_AccessPoint_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_AccessPoint_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{    
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "X_RDK_ProfileId", TRUE))   
    {
        *puLong = pstAPInfo->ProfileIndex;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_ApnAuthentication", TRUE))   
    {
        *puLong = pstAPInfo->X_RDK_ApnAuthentication;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_IpAddressFamily", TRUE))   
    {
        *puLong = pstAPInfo->X_RDK_IpAddressFamily;    
        return TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_PdpInterfaceConfig", TRUE))   
    {
        *puLong = pstAPInfo->X_RDK_PdpInterfaceConfig;    
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_AccessPoint_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Cellular_AccessPoint_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    /* check the parameter name and set the corresponding value */
    if(AnscEqualString(ParamName, "X_RDK_ApnAuthentication", TRUE))
    {
        pstAPInfo->X_RDK_ApnAuthentication = uValue;
        return TRUE;
    }

    if(AnscEqualString(ParamName, "X_RDK_IpAddressFamily", TRUE))
    {
        pstAPInfo->X_RDK_IpAddressFamily = uValue;
        return TRUE;
    }

    if(AnscEqualString(ParamName, "X_RDK_PdpInterfaceConfig", TRUE))
    {
        pstAPInfo->X_RDK_PdpInterfaceConfig = uValue;
        return TRUE;
    }

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Cellular_AccessPoint_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
Cellular_AccessPoint_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    //Check if there is any duplicate available or not
    if( TRUE == pstAPInfo->bIsThisNewlyAddedRecord )
    {
        int i;

        for( i = 0; i < pstDmlCellular->ulAccessPointNoOfEntries; i++ )
        {
            if( FALSE == pstDmlCellular->pstAPInfo[i].bIsThisNewlyAddedRecord )
            {
                if ( ( 0 == strcmp(pstDmlCellular->pstAPInfo[i].APN, pstAPInfo->APN) ) && \
                     ( 0 == strcmp(pstDmlCellular->pstAPInfo[i].Username, pstAPInfo->Username) )  && \
                     ( 0 == strcmp(pstDmlCellular->pstAPInfo[i].Password, pstAPInfo->Password) ) && \
                     ( pstDmlCellular->pstAPInfo[i].X_RDK_IpAddressFamily == pstAPInfo->X_RDK_IpAddressFamily ) && \
                     ( pstDmlCellular->pstAPInfo[i].X_RDK_ApnAuthentication == pstAPInfo->X_RDK_ApnAuthentication ) )
                {
                    CcspTraceError(("%s Validation Failed, Duplicate Profile Error\n",__FUNCTION__));
                    pstDmlCellular->ulAccessPointListLastUpdatedTime =  0; //To resync next time
                    return FALSE;
                }
            }
        } 
    }
    else
    {
        int i;

        for( i = 0; i < pstDmlCellular->ulAccessPointNoOfEntries; i++ )
        {
            if( pstDmlCellular->pstAPInfo[i].ProfileIndex != pstAPInfo->ProfileIndex )
            {
                if ( ( 0 == strcmp(pstDmlCellular->pstAPInfo[i].APN, pstAPInfo->APN) ) && \
                     ( 0 == strcmp(pstDmlCellular->pstAPInfo[i].Username, pstAPInfo->Username) )  && \
                     ( 0 == strcmp(pstDmlCellular->pstAPInfo[i].Password, pstAPInfo->Password) ) && \
                     ( pstDmlCellular->pstAPInfo[i].X_RDK_IpAddressFamily == pstAPInfo->X_RDK_IpAddressFamily ) && \
                     ( pstDmlCellular->pstAPInfo[i].X_RDK_ApnAuthentication == pstAPInfo->X_RDK_ApnAuthentication ) )
                {
                    CcspTraceError(("%s Validation Failed, Duplicate Profile Error\n",__FUNCTION__));
                    pstDmlCellular->ulAccessPointListLastUpdatedTime =  0; //To resync next time
                    return FALSE;
                }
            }
        } 
    }

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_AccessPoint_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Cellular_AccessPoint_Commit
    (
        ANSC_HANDLE                 hInsContext
    )

{    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)hInsContext;

    //Check if this new record or not
    if( TRUE == pstAPInfo->bIsThisNewlyAddedRecord )
    {
        pstAPInfo->bIsThisNewlyAddedRecord = FALSE;

        //Add Profile
        CellularMgr_AccessPointCreateProfile( pstAPInfo );
        pstDmlCellular->ulAccessPointListLastUpdatedTime =  0; //To resync next time after add
    }
    else
    {
        //Modify Profile
        CellularMgr_AccessPointModifyProfile( pstAPInfo );
        pstDmlCellular->ulAccessPointListLastUpdatedTime =  0; //To resync next time after modify
    }

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Cellular_AccessPoint_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Cellular_AccessPoint_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return ANSC_STATUS_SUCCESS;
}
