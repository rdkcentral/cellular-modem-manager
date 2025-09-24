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

/*
 * 
 * code generated with: python ../scripts/rbus_code_generator_ccsp_style.py ../../../../../config/RdkCellularManager.xml cellularmgr
 */
#ifdef RBUS_BUILD_FLAG_ENABLE

#include <rbus.h>
#include <cellularmgr_rbus_helpers.h>
#include <cellularmgr_rbus_events.h>
#include <rtMemory.h>
#include <rtLog.h>
#include <stdlib.h>
#include <string.h>

#include "cellularmgr_plugin_main_apis.h"
#include "cellularmgr_cellular_internal.h"
#include "cellularmgr_cellular_apis.h"
#include "cellularmgr_sm.h"
#include "cellularmgr_global.h"
#include "cellularmgr_utils.h"

#define  ARRAY_SZ(x) (sizeof(x) / sizeof((x)[0]))

extern PBACKEND_MANAGER_OBJECT               g_pBEManager;
extern CellularMGR_rbusSubListSt gRBUSSubListSt;

rbusHandle_t gBusHandle = NULL;
char const gComponentID[] = "cellularmgr";

#define CELLULARMGR_INFACE_TABLE                 "Device.Cellular.Interface."
#define CELLULARMGR_INFACE_CONTXTPROFILE_TABLE   "Device.Cellular.Interface.%d.X_RDK_ContextProfile."
#define CELLULARMGR_ACCESSPOINT_TABLE            "Device.Cellular.AccessPoint."
#define CELLULARMGR_UICC_TABLE                   "Device.Cellular.X_RDK_Uicc."
#define PLMNACCESS_AVAILABLENETWORK_TABLE        "Device.Cellular.Interface.%d.X_RDK_PlmnAccess.AvailableNetworks."

rbusError_t registerGeneratedDataElements(rbusHandle_t handle);

rbusError_t Sample_RegisterRow(char const* tableName, uint32_t instNum, char const* alias, void* context)
{
    rbusError_t rc;
    rc = rbusTable_registerRow(gBusHandle, tableName, instNum, alias);
    if(rc == RBUS_ERROR_SUCCESS)
    {
        SetRowContext(tableName, instNum, alias, context);    
    }
    return rc;    
}

rbusError_t Sample_UnregisterRow(char const* tableName, uint32_t instNum)
{
    rbusError_t rc;
    char rowName[RBUS_MAX_NAME_LENGTH];
    snprintf(rowName, RBUS_MAX_NAME_LENGTH, "%s%d", tableName, instNum);
    rc = rbusTable_unregisterRow(gBusHandle, rowName);
    if(rc == RBUS_ERROR_SUCCESS)
    {
        RemoveRowContextByName(rowName);    
    }
    return rc;    
}

rbusError_t cellularmgr_Init()
{
    rbusError_t rc;
    rbusHandle_t rbusHandle = NULL;
    rc = rbus_open(&rbusHandle, gComponentID);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s-%d : rbus open failed\n",__FUNCTION__, __LINE__));
        return rc;
    }
    rc = registerGeneratedDataElements(rbusHandle);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s-%d : Register Data Element Failed\n",__FUNCTION__, __LINE__));
        rbus_close(rbusHandle);
        return rc;
    }
    Context_Init();
    gBusHandle = rbusHandle;

    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    if(pMyObject == NULL)
    {
        CcspTraceError(("%s-%d : pMyObject is Null\n",__FUNCTION__, __LINE__));
        rbus_close(rbusHandle);
        Context_Release();
        return RBUS_ERROR_BUS_ERROR;
    }
    
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if(pstDmlCellular == NULL)
    {
        CcspTraceError(("%s-%d : pstDmlCellular is Null\n",__FUNCTION__, __LINE__));
        rbus_close(rbusHandle);
        Context_Release();
        return RBUS_ERROR_BUS_ERROR;
    }
    char paramName[256] = {0};
    memset ( paramName, 0, sizeof(paramName) );

    for(int i = 0; i < pstDmlCellular->ulInterfaceNoEntries; i++)
    {
        PCELLULAR_INTERFACE_INFO   pstInterfaceInfo = &(pstDmlCellular->pstInterfaceInfo[i]);

        if (Sample_RegisterRow(CELLULARMGR_INFACE_TABLE, (i+1), NULL, pstInterfaceInfo) != RBUS_ERROR_SUCCESS)
        {
            CcspTraceError(("%s-%d : Failed to Add Cellular Interface(%d) Table(%s) \n", __FUNCTION__, __LINE__, (i+1), CELLULARMGR_INFACE_TABLE));
        }

        snprintf(paramName, sizeof(paramName), CELLULARMGR_INFACE_CONTXTPROFILE_TABLE, (i + 1));

        for(int j = 0; j < pstInterfaceInfo->ulContextProfileNoOfEntries; j++)
        {
            PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO   pstContextProfileInfo = &(pstInterfaceInfo->pstContextProfileInfo[j]);
            if (Sample_RegisterRow(paramName, (j+1), NULL, pstContextProfileInfo) != RBUS_ERROR_SUCCESS)
            {
                CcspTraceError(("%s-%d : Failed to Add ContextProfile Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (j+1), paramName));
            }
        }

        snprintf(paramName, sizeof(paramName), PLMNACCESS_AVAILABLENETWORK_TABLE, (i + 1));

    	PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
        if (pstPlmnInfo != NULL)
        {
            for(int m = 0; m < pstPlmnInfo->ulAvailableNetworkNoOfEntries; m++)
            {
                if (Sample_RegisterRow(paramName, (m+1), NULL, NULL) != RBUS_ERROR_SUCCESS)
                {
                    CcspTraceError(("%s-%d : Failed to Add AvailableNetworks Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (m+1), paramName));
                }
            }
        }
    }

    for(int k = 0; k < pstDmlCellular->ulAccessPointNoOfEntries; k++)
    {
        if (Sample_RegisterRow(CELLULARMGR_ACCESSPOINT_TABLE, (k+1), NULL, NULL) != RBUS_ERROR_SUCCESS)
        {
            CcspTraceError(("%s-%d : Failed to Add AccessPoint Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (k+1), CELLULARMGR_ACCESSPOINT_TABLE));
        }
    }

    for(int l = 0; l < pstDmlCellular->ulUICCNoOfEntries; l++)
    {
        if (Sample_RegisterRow(CELLULARMGR_UICC_TABLE, (l+1), NULL, NULL) != RBUS_ERROR_SUCCESS)
        {
            CcspTraceError(("%s-%d : Failed to Add UICC Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (l+1), CELLULARMGR_UICC_TABLE));
        }
    }

    CcspTraceInfo(("%s-%d : Rbus Initialised successfully \n",__FUNCTION__, __LINE__));
    return RBUS_ERROR_SUCCESS;
}

rbusError_t cellularmgr_Unload()
{
    rbusError_t rc = RBUS_ERROR_SUCCESS;
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    if(pMyObject == NULL)
    {
        CcspTraceError(("%s-%d : pMyObject is Null\n",__FUNCTION__, __LINE__));
        rc = RBUS_ERROR_BUS_ERROR;
        goto exit;
    }

    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if(pstDmlCellular == NULL)
    {
        CcspTraceError(("%s-%d : pstDmlCellular is Null\n",__FUNCTION__, __LINE__));
        rc = RBUS_ERROR_BUS_ERROR;
        goto exit;
    }
    char paramName[256] = {0};
    memset ( paramName, 0, sizeof(paramName) );

    for(int i = 0; i < pstDmlCellular->ulInterfaceNoEntries; i++)
    {
        PCELLULAR_INTERFACE_INFO   pstInterfaceInfo = &(pstDmlCellular->pstInterfaceInfo[i]);

        if (Sample_UnregisterRow(CELLULARMGR_INFACE_TABLE, (i+1))!= RBUS_ERROR_SUCCESS)
        {
            CcspTraceError(("%s-%d : Failed to Del Cellular Interface(%d) Table(%s) \n", __FUNCTION__, __LINE__, (i+1), CELLULARMGR_INFACE_TABLE));
        }

        snprintf(paramName, sizeof(paramName), CELLULARMGR_INFACE_CONTXTPROFILE_TABLE, (i + 1));

        for(int j = 0; j < pstInterfaceInfo->ulContextProfileNoOfEntries; j++)
        {
            if (Sample_UnregisterRow(paramName, (j+1)) != RBUS_ERROR_SUCCESS)
            {
                CcspTraceError(("%s-%d : Failed to Del ContextProfile Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (i+1), paramName));
            }
        }

        snprintf(paramName, sizeof(paramName), PLMNACCESS_AVAILABLENETWORK_TABLE, (i + 1));

        PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
        if (pstPlmnInfo != NULL)
        {
            for(int m = 0; m < pstPlmnInfo->ulAvailableNetworkNoOfEntries; m++)
            {
                if (Sample_UnregisterRow(paramName, (m+1)) != RBUS_ERROR_SUCCESS)
                {
                    CcspTraceError(("%s-%d : Failed to Del AvailableNetworks Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (m+1), paramName));
                }
            }
        }

    }

    for(int k = 0; k < pstDmlCellular->ulAccessPointNoOfEntries; k++)
    {
        if (Sample_UnregisterRow(CELLULARMGR_ACCESSPOINT_TABLE, (k+1)) != RBUS_ERROR_SUCCESS)
        {
            CcspTraceError(("%s-%d : Failed to Del AccessPoint Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (k+1), paramName));
        }
    }

    for(int l = 0; l < pstDmlCellular->ulUICCNoOfEntries; l++)
    {
        if (Sample_UnregisterRow(CELLULARMGR_UICC_TABLE, (l+1)) != RBUS_ERROR_SUCCESS)
        {
            CcspTraceError(("%s-%d : Failed to Del AccessPoint Inst(%d) Table(%s) \n", __FUNCTION__, __LINE__, (l+1), CELLULARMGR_UICC_TABLE));
        }
    }

exit:
    if(gBusHandle)
    {
        rc = rbus_close(gBusHandle);
        gBusHandle = NULL;
    }
    Context_Release();
    CcspTraceInfo(("%s-%d : Rbus De-Initialized Successfully \n",__FUNCTION__, __LINE__));
    return rc;    
}

static rbusError_t Cellular_Interface_GetEntryCount_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "InterfaceNumberOfEntries") == 0)
    {
        rbusProperty_SetUInt64(property, pstDmlCellular->ulInterfaceNoEntries);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "X_RDK_Enable") == 0)
    {
        rbusProperty_SetBoolean(property, pstDmlCellular->X_RDK_Enable);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }
    if(strcmp(context.name, "X_RDK_Status") == 0)
    {
        pstDmlCellular->X_RDK_Status = CellularMgrSMGetCurrentState( );
        if (pstDmlCellular->X_RDK_Status == RDK_STATUS_DOWN)
        {
            rbusProperty_SetString(property, "DOWN");
        }
        else if (pstDmlCellular->X_RDK_Status == RDK_STATUS_DEACTIVATED)
        {
            rbusProperty_SetString(property, "DEACTIVATED");
        }
        else if (pstDmlCellular->X_RDK_Status == RDK_STATUS_DEREGISTERED)
        {
            rbusProperty_SetString(property, "DEREGISTERED");
        }
        else if (pstDmlCellular->X_RDK_Status == RDK_STATUS_REGISTERED)
        {
            rbusProperty_SetString(property, "REGISTERED");
        }
        else if (pstDmlCellular->X_RDK_Status == RDK_STATUS_CONNECTED)
        {
            rbusProperty_SetString(property, "CONNECTED");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
#if defined HYBRID_SUPPORT
    else if(strcmp(context.name, "X_RDK_DeviceType") == 0)
    {
        pstDmlCellular->X_RDK_DeviceType = CellularMgr_GetModemDeviceType();
        if (pstDmlCellular->X_RDK_DeviceType ==  RDK_DEVICETYPE_UNKNOWN)
        {
            rbusProperty_SetString(property, "UNKNOWN");
        }
        if (pstDmlCellular->X_RDK_DeviceType ==  RDK_DEVICETYPE_SOCMODEM)
        {
            rbusProperty_SetString(property, "SOCMODEM");
        }
        if (pstDmlCellular->X_RDK_DeviceType ==  RDK_DEVICETYPE_USBMODEM)
        {
            rbusProperty_SetString(property, "USBMODEM");
        }
        if (pstDmlCellular->X_RDK_DeviceType ==  RDK_DEVICETYPE_USBRNDIS)
        {
            rbusProperty_SetString(property, "RNDIS");
        }
    }
#endif
    else if(strcmp(context.name, "X_RDK_Model") == 0)
    {
        CellularMgr_GetDeviceInfo(pstDmlCellular);
        rbusProperty_SetString(property, pstDmlCellular->X_RDK_Model);
    }
    else if(strcmp(context.name, "X_RDK_HardwareRevision") == 0)
    {
        CellularMgr_GetDeviceInfo(pstDmlCellular);
        rbusProperty_SetString(property, pstDmlCellular->X_RDK_HardwareRevision);
    }
    else if(strcmp(context.name, "X_RDK_Vendor") == 0)
    {
        CellularMgr_GetDeviceInfo(pstDmlCellular);
        rbusProperty_SetString(property, pstDmlCellular->X_RDK_Vendor);
    }
    else if(strcmp(context.name, "X_RDK_ControlInterface") == 0)
    {
        CellularMgr_GetDeviceInfo(pstDmlCellular);
        rbusProperty_SetString(property, pstDmlCellular->X_RDK_ControlInterface);
    }
    else if(strcmp(context.name, "X_RDK_ControlInterfaceStatus") == 0)
    {
        pstDmlCellular->X_RDK_ControlInterfaceStatus = CellularMgr_GetModemControlInterfaceStatus( );
        if (pstDmlCellular->X_RDK_ControlInterfaceStatus == CONTROL_STATUS_OPENED)
        {
            rbusProperty_SetString(property, "OPENED");
        }
        else if (pstDmlCellular->X_RDK_ControlInterfaceStatus == CONTROL_STATUS_CLOSED)
        {
            rbusProperty_SetString(property, "CLOSED");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "X_RDK_DataInterface") == 0)
    {
        CellularMgr_GetDataInterface(pstDmlCellular->X_RDK_DataInterface);
        rbusProperty_SetString(property, pstDmlCellular->X_RDK_DataInterface);
    }
    else if(strcmp(context.name, "X_RDK_DataInterfaceLink") == 0)
    {
        if (pstDmlCellular->X_RDK_DataInterfaceLink == DATA_INTERFACE_LINK_RAW_IP)
        {
            rbusProperty_SetString(property, "IP_RAW");
        }
        else if (pstDmlCellular->X_RDK_DataInterfaceLink == DATA_INTERFACE_LINK_802P3)
        {
            rbusProperty_SetString(property, "802.3");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "CellularConfig") == 0)
    {
        CcspTraceInfo(("%s-%d : Data Get Not supported\n",__FUNCTION__, __LINE__));
        rbusProperty_SetString(property, "None");
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_SetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }
    if(strcmp(context.name, "CellularConfig") == 0)
    {
        const char* val;
        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        CcspTraceInfo(("%s-%d : Data Received from WebConfig\n",__FUNCTION__, __LINE__));
        CellularMgr_BlobUnpack(val);
        /*context.userData is this objects data and val is the property's new value*/
    }
#if defined HYBRID_SUPPORT
    if(strcmp(context.name, "X_RDK_DeviceType") == 0)
    {
        const char* val;
        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        if (strcmp(val,"UNKNOWN") == 0)
        {
            pstDmlCellular->X_RDK_DeviceType = RDK_DEVICETYPE_UNKNOWN;
        }
        if (strcmp(val,"SOCMODEM") == 0)
        {
            pstDmlCellular->X_RDK_DeviceType = RDK_DEVICETYPE_SOCMODEM;
        }
        if (strcmp(val,"USBMODEM") == 0)
        {
            pstDmlCellular->X_RDK_DeviceType = RDK_DEVICETYPE_USBMODEM;
        }
        if (strcmp(val,"USBRNDIS") == 0)
        {
            pstDmlCellular->X_RDK_DeviceType = RDK_DEVICETYPE_USBRNDIS;
        }

         CellularMgr_SetDeviceType(pstDmlCellular);
        /*context.userData is this objects data and val is the property's new value*/
    }
#endif
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Validate_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Commit_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Rollback_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_Cellular_Validate_Cellular_Commit_Cellular_Rollback(void* context)
{
    if(Cellular_Validate_rbus(context) == 0)
    {
        if(Cellular_Commit_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(Cellular_Rollback_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t Cellular_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "X_RDK_Enable") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;
        if( val != pstDmlCellular->X_RDK_Enable )
        {
            if( RETURN_OK == CellularMgr_SetModemEnable( val ) )
            {
                pstDmlCellular->X_RDK_Enable = val;
            }
        }
        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }
    
    if(opts->commit)
    {
      return do_Cellular_Validate_Cellular_Commit_Cellular_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t DeviceManagement_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Imei") == 0)
    {
        CellularMgr_GetModemIMEI(pstDmlCellular->X_RDK_Imei);
        rbusProperty_SetString(property, pstDmlCellular->X_RDK_Imei);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t DeviceManagement_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "FactoryReset") == 0)
    {
        rbusProperty_SetBoolean(property, 0);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t DeviceManagement_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "FactoryReset") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        CcspTraceInfo(("%s-%d : Cellular Modem FactoryReset...\n",__FUNCTION__, __LINE__));
        CellularMgr_FactoryReset();

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Firmware_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "CurrentImageVersion") == 0)
    {
        CellularMgr_GetModemFirmwareVersion(pstDmlCellular->CurrentImageVersion);
        rbusProperty_SetString(property, pstDmlCellular->CurrentImageVersion);
    }
    else if(strcmp(context.name, "FallbackImageVersion") == 0)
    {
        rbusProperty_SetString(property, pstDmlCellular->FallbackImageVersion);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_Interface_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;
   
    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Enable") == 0)
    {
        rbusProperty_SetBoolean(property, pstInterfaceInfo->Enable);
    }
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
    else if(strcmp(context.name, "Upstream") == 0)
    {
        rbusProperty_SetBoolean(property, pstInterfaceInfo->Upstream);
    }
#endif
    else if(strcmp(context.name, "X_RDK_PhyConnectedStatus") == 0)
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
        rbusProperty_SetBoolean(property, pstInterfaceInfo->X_RDK_PhyConnectedStatus);
    }
    else if(strcmp(context.name, "X_RDK_LinkAvailableStatus") == 0)
    {
        rbusProperty_SetBoolean(property, pstInterfaceInfo->X_RDK_LinkAvailableStatus);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_Interface_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_INTERFACE_SERVING_INFO    pstServingInfo = &(pstInterfaceInfo->stServingInfo);

    if(strcmp(context.name, "LastChange") == 0)
    {
        rbusProperty_SetUInt32(property, pstInterfaceInfo->LastChange);
    }
    else if(strcmp(context.name, "RegistrationRetries") == 0)
    {
        rbusProperty_SetUInt32(property, pstInterfaceInfo->RegistrationRetries);
    }
    else if(strcmp(context.name, "MaxRegistrationRetries") == 0)
    {
        rbusProperty_SetUInt32(property, pstInterfaceInfo->MaxRegistrationRetries);
    }
    else if(strcmp(context.name, "RegistrationRetryTimer") == 0)
    {
        rbusProperty_SetUInt32(property, pstInterfaceInfo->RegistrationRetryTimer);
    }
    else if(strcmp(context.name, "X_RDK_RFCN") == 0)
    {
        rbusProperty_SetUInt32(property, pstServingInfo->Rfcn);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_Interface_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_INTERFACE_SERVING_INFO    pstServingInfo = &(pstInterfaceInfo->stServingInfo);

    if(strcmp(context.name, "Status") == 0)
    {
        CellularMgr_ServingSystemInfo (pstInterfaceInfo, NULL);
        if (pstInterfaceInfo->Status == IF_UP)
        {
            rbusProperty_SetString(property, "Up");
        }
        else
        {
            rbusProperty_SetString(property, "Down");
        }
    }
    else if(strcmp(context.name, "Alias") == 0)
    {
        rbusProperty_SetString(property, pstInterfaceInfo->Alias);
    }
    else if(strcmp(context.name, "Name") == 0)
    {
        rbusProperty_SetString(property, pstInterfaceInfo->Name);
    }
    else if(strcmp(context.name, "LowerLayers") == 0)
    {
        rbusProperty_SetString(property, pstInterfaceInfo->LowerLayers);
    }
    else if(strcmp(context.name, "IMEI") == 0)
    {
        CellularMgr_GetModemIMEI(pstInterfaceInfo->Imei);
        rbusProperty_SetString(property, pstInterfaceInfo->Imei);
    }
    else if(strcmp(context.name, "SupportedAccessTechnologies") == 0)
    {
	CellularMgr_GetModemSupportedRadioTechnology(pstInterfaceInfo->SupportedAccessTechnologies); 
        rbusProperty_SetString(property, pstInterfaceInfo->SupportedAccessTechnologies);
    }
    else if(strcmp(context.name, "PreferredAccessTechnologies") == 0)
    {
	CellularMgr_GetModemPreferredRadioTechnology( pstInterfaceInfo->PreferredAccessTechnologies );
        rbusProperty_SetString(property, pstInterfaceInfo->PreferredAccessTechnologies);
    }
    else if(strcmp(context.name, "CurrentAccessTechnology") == 0)
    {
	CellularMgr_GetModemCurrentRadioTechnology( pstInterfaceInfo->CurrentAccessTechnology );
        rbusProperty_SetString(property, pstInterfaceInfo->CurrentAccessTechnology);
    }
    else if(strcmp(context.name, "X_RDK_RegisteredService") == 0)
    {
        CellularMgrGetNetworkRegisteredService(&pstInterfaceInfo->X_RDK_RegisteredService);
        if (pstInterfaceInfo->X_RDK_RegisteredService == REGISTERED_SERVICE_PS)
        {
            rbusProperty_SetString(property, "PS");
        }
        else if (pstInterfaceInfo->X_RDK_RegisteredService == REGISTERED_SERVICE_CS)
        {
            rbusProperty_SetString(property, "CS");
        }
        else if (pstInterfaceInfo->X_RDK_RegisteredService == REGISTERED_SERVICE_CS_PS)
        {
            rbusProperty_SetString(property, "PS-CS");
        }
        else
        {
            rbusProperty_SetString(property, "NONE");
        }
    }
    else if(strcmp(context.name, "X_RDK_CellId") == 0)
    {
        CellularMgr_GetServingInfo( pstServingInfo );
        rbusProperty_SetString(property, pstServingInfo->CellId);
    }
    else if(strcmp(context.name, "X_RDK_RAT") == 0)
    {
        if (pstServingInfo->Rat == RAT_INFO_GSM)
        {
            rbusProperty_SetString(property, "GSM");
        }
        else if (pstServingInfo->Rat == RAT_INFO_UMTS)
        {
            rbusProperty_SetString(property, "UMTS");
        }
        else if (pstServingInfo->Rat == RAT_INFO_LTE)
        {
            rbusProperty_SetString(property, "LTE");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "X_RDK_PlmnId") == 0)
    {
        CellularMgr_GetServingInfo( pstServingInfo );
        rbusProperty_SetString(property, pstServingInfo->PlmnId);
    }
    else if(strcmp(context.name, "X_RDK_AreaCode") == 0)
    {   
        CellularMgr_GetServingInfo( pstServingInfo );
        rbusProperty_SetString(property, pstServingInfo->AreaCode);
    }
    else if(strcmp(context.name, "X_RDK_RadioEnvConditions") == 0)
    {
        pstInterfaceInfo->RadioEnvConditions = CellularMgr_GetRadioEnvConditions( );
        if (pstInterfaceInfo->RadioEnvConditions == RADIO_ENV_CONDITION_EXCELLENT)
        {
            rbusProperty_SetString(property, "EXCELLENT");
        }
        else if (pstInterfaceInfo->RadioEnvConditions == RADIO_ENV_CONDITION_GOOD)
        {
            rbusProperty_SetString(property, "GOOD");
        }
        else if (pstInterfaceInfo->RadioEnvConditions == RADIO_ENV_CONDITION_FAIR)
        {
            rbusProperty_SetString(property, "FAIR");
        }
        else if (pstInterfaceInfo->RadioEnvConditions == RADIO_ENV_CONDITION_POOR)
        {
            rbusProperty_SetString(property, "POOR");
        }
        else if (pstInterfaceInfo->RadioEnvConditions == RADIO_ENV_CONDITION_UNAVAILABLE)
        {
            rbusProperty_SetString(property, "UNAVAILABLE");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Interface_Validate_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Interface_Commit_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Interface_Rollback_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_Cellular_Interface_Validate_Cellular_Interface_Commit_Cellular_Interface_Rollback(void* context)
{
    if(Cellular_Interface_Validate_rbus(context) == 0)
    {
        if(Cellular_Interface_Commit_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(Cellular_Interface_Rollback_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t Cellular_Interface_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Enable") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        if( pstInterfaceInfo->Enable != val )
        {
            CellularMgr_SetModemInterfaceEnable( val );
            pstInterfaceInfo->Enable = val;
        }
        /*context.userData is this objects data and val is the property's new value*/
    }
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
    else if(strcmp(context.name, "Upstream") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        pstInterfaceInfo->Upstream = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
#endif
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(opts->commit)
    {
      return do_Cellular_Interface_Validate_Cellular_Interface_Commit_Cellular_Interface_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Interface_SetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "RegistrationRetries") == 0)
    {
        uint32_t val;

        rbusValueError_t verr = rbusProperty_GetUInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

         pstInterfaceInfo->RegistrationRetries = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "MaxRegistrationRetries") == 0)
    {
        uint32_t val;

        rbusValueError_t verr = rbusProperty_GetUInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        pstInterfaceInfo->MaxRegistrationRetries = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "RegistrationRetryTimer") == 0)
    {
        uint32_t val;

        rbusValueError_t verr = rbusProperty_GetUInt32Ex(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        pstInterfaceInfo->RegistrationRetryTimer = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(opts->commit)
    {
      return do_Cellular_Interface_Validate_Cellular_Interface_Commit_Cellular_Interface_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Interface_GetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_INTERFACE_SERVING_INFO    pstServingInfo       = &(pstInterfaceInfo->stServingInfo);

    if (pstServingInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    memset(pstServingInfo, 0, sizeof(*pstServingInfo));
    if(CellularMgr_RadioSignalGetSignalInfo( pstServingInfo ) != RETURN_OK)
    {
        CcspTraceError(("%s-%d : CellularMgr_RadioSignalGetSignalInfo failed\n", __FUNCTION__, __LINE__));
    }

    if(strcmp(context.name, "RSSI") == 0)
    {
        rbusProperty_SetInt32(property, pstServingInfo->Rssi);
    }
    else if(strcmp(context.name, "X_RDK_SNR") == 0)
    {
        rbusProperty_SetInt32(property, pstServingInfo->Snr);
    }
    else if(strcmp(context.name, "RSRP") == 0)
    {
        rbusProperty_SetInt32(property, pstServingInfo->Rsrp);
    }
    else if(strcmp(context.name, "RSRQ") == 0)
    {
        rbusProperty_SetInt32(property, pstServingInfo->Rsrq);
    }
    else if(strcmp(context.name, "X_RDK_TRX") == 0)
    {
        rbusProperty_SetInt32(property, pstServingInfo->Trx);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_Interface_SetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "LowerLayers") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

         AnscCopyString(pstInterfaceInfo->LowerLayers, val);
        /*context.userData is this objects data and val is the property's new value*/
    }
    if(strcmp(context.name, "PreferredAccessTechnologies") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        if (RETURN_OK ==  CellularMgr_SetModemPreferredRadioTechnology( val))
        {
            AnscCopyString(pstInterfaceInfo->PreferredAccessTechnologies, val);
        }
	else 
            return RBUS_ERROR_INVALID_INPUT;
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(opts->commit)
    {
      return do_Cellular_Interface_Validate_Cellular_Interface_Commit_Cellular_Interface_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Identification_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Imei") == 0)
    {
        CellularMgr_GetModemIMEI(pstInterfaceInfo->Imei);
        rbusProperty_SetString(property, pstInterfaceInfo->Imei);
    }
    else if(strcmp(context.name, "Iccid") == 0)
    {
        CellularMgr_GetModemCurrentICCID(pstInterfaceInfo->Iccid);
        rbusProperty_SetString(property, pstInterfaceInfo->Iccid);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t PlmnAccess_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    if (pstPlmnInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "RoamingEnable") == 0)
    {
        CellularMgr_GetPlmnInformation(pstPlmnInfo);
        rbusProperty_SetBoolean(property, pstPlmnInfo->RoamingEnable);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t PlmnAccess_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    if (pstPlmnInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "RoamingStatus") == 0)
    {
        CellularMgr_GetPlmnInformation(pstPlmnInfo);
        if (pstPlmnInfo->RoamingStatus == ROAMING_STATUS_HOME)
        {
            rbusProperty_SetString(property, "HOME");
        }
        else if (pstPlmnInfo->RoamingStatus == ROAMING_STATUS_VISITOR)
        {
            rbusProperty_SetString(property, "VISITOR");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}


bool AvailableNetworks_IsUpdated_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_INTERFACE_INFO   pstInterfaceInfo = NULL;
    INT index = 0;

    if (pstDmlCellular == NULL)
    {
        return false;
    }

    sscanf(((HandlerContext *)ctx)->fullName, "Device.Cellular.Interface.%d.X_RDK_PlmnAccess.", &index);
    if ((pstDmlCellular->ulInterfaceNoEntries > 0) && index)
    {
        pstInterfaceInfo = &(pstDmlCellular->pstInterfaceInfo[index-1]);
    }

    if (pstInterfaceInfo == NULL)
    {
        return false;
    }


    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    if (pstPlmnInfo == NULL)
    {
        return false;
    }

    if ( ( AnscGetTickInSeconds() - pstPlmnInfo->ulAvailableNetworkListLastUpdatedTime ) < CELLULAR_AVAILABLE_NETWORK_LIST_REFRESH_THRESHOLD )
    {
        return false;
    }
    else
    {
        pstPlmnInfo->ulAvailableNetworkListLastUpdatedTime =  AnscGetTickInSeconds();
        CcspTraceError(("%s-%d :YES\n",__FUNCTION__, __LINE__));
	return true;
    }
    return true;
}

rbusError_t AvailableNetworks_Synchronize_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    PCELLULAR_INTERFACE_INFO   pstInterfaceInfo = NULL;
    INT index = 0;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    sscanf(((HandlerContext *)ctx)->fullName, "Device.Cellular.Interface.%d.X_RDK_PlmnAccess.", &index);
    if ((pstDmlCellular->ulInterfaceNoEntries > 0) && index)
    {
        pstInterfaceInfo = &(pstDmlCellular->pstInterfaceInfo[index-1]);
    }

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    if (pstPlmnInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    ULONG ulPrevCount = pstPlmnInfo->ulAvailableNetworkNoOfEntries;

    CellularMgr_GetAvailableNetworksInformation( &pstPlmnInfo->pstAvailableNetworks, &pstPlmnInfo->ulAvailableNetworkNoOfEntries );

    /**
     *  RBUS Limitation Hack:
     *  We need to unregister all rows except default 1st row and set context
     * */

    if( ulPrevCount > 0 )
    {
        char param_name[256] = {0};
        sprintf(param_name, PLMNACCESS_AVAILABLENETWORK_TABLE, index);
        for(int i = 1; i < ulPrevCount; i++)
        {
            if (Sample_UnregisterRow(param_name, (i+1)) == RBUS_ERROR_SUCCESS)
            {
                CcspTraceInfo(("%s: Unregistered Previously available network table:Inst(%s%d) \n", __FUNCTION__, param_name, (i+1)));
            }
        }
    }
    
    if( ( pstPlmnInfo->ulAvailableNetworkNoOfEntries > 0 ) &&
        ( pstPlmnInfo->pstAvailableNetworks != NULL ) )
    {
        char param_name[256] = {0};
        sprintf(param_name, PLMNACCESS_AVAILABLENETWORK_TABLE, index);
        for(int i = 0; i < pstPlmnInfo->ulAvailableNetworkNoOfEntries; i++)
        {
            PCELLULAR_PLMN_AVAILABLENETWORK_INFO *pAvailableNetworkInfo = &(pstPlmnInfo->pstAvailableNetworks[i]);
            if (Sample_RegisterRow(param_name, (i+1), NULL, pAvailableNetworkInfo) == RBUS_ERROR_SUCCESS)
            {
                CcspTraceInfo(("%s-%d : Add Available Network Table:Inst(%s%d) \n", __FUNCTION__, __LINE__, param_name, (i+1)));
            }
            else
            {
                CcspTraceInfo(("%s-%d : tableName(%s), Inst(%d) \n",__FUNCTION__, __LINE__, param_name, (i+1)));
                SetRowContext(param_name, (i+1), NULL, pAvailableNetworkInfo);
            }
        }
    }
    else
    {
        //RBUS Limitation FIX:
        if( pstPlmnInfo->ulAvailableNetworkNoOfEntries == 0 )
        {
            char param_name[256] = {0};
            sprintf(param_name, PLMNACCESS_AVAILABLENETWORK_TABLE, index);
            CcspTraceInfo(("%s-%d : tableName(%s), Inst(%d) \n",__FUNCTION__, __LINE__, param_name, 1));
            SetRowContext(param_name, 1, NULL, NULL);
            pstPlmnInfo->ulAvailableNetworkNoOfEntries = 1;
        }
    }

    return RBUS_ERROR_SUCCESS;
}

int do_AvailableNetworks_IsUpdated_AvailableNetworks_Synchronize(HandlerContext context)
{
    if(AvailableNetworks_IsUpdated_rbus(&context))
    {
        return AvailableNetworks_Synchronize_rbus(&context);
    }
    return 0;
}

static rbusError_t AvailableNetworks_GetEntryCount_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_AvailableNetworks_IsUpdated_AvailableNetworks_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO            pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;
    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    if (pstPlmnInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "AvailableNetworksNumberOfEntries") == 0)
    {
        rbusProperty_SetUInt64(property, pstPlmnInfo->ulAvailableNetworkNoOfEntries);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t AvailableNetworks_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_AvailableNetworks_IsUpdated_AvailableNetworks_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_PLMN_AVAILABLENETWORK_INFO  pstAvailableNetworks =(PCELLULAR_PLMN_AVAILABLENETWORK_INFO)context.userData;

    if (pstAvailableNetworks == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Allowed") == 0)
    {
        rbusProperty_SetBoolean(property, pstAvailableNetworks->Allowed);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}


static rbusError_t AvailableNetworks_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_AvailableNetworks_IsUpdated_AvailableNetworks_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);

    PCELLULAR_PLMN_AVAILABLENETWORK_INFO  pstAvailableNetworks =(PCELLULAR_PLMN_AVAILABLENETWORK_INFO)context.userData;
    if (pstAvailableNetworks == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Mcc") == 0)
    {
        rbusProperty_SetString(property, pstAvailableNetworks->MCC);
    }
    else if(strcmp(context.name, "Mnc") == 0)
    {
        rbusProperty_SetString(property, pstAvailableNetworks->MNC);
    }
    else if(strcmp(context.name, "Name") == 0)
    {
        rbusProperty_SetString(property, pstAvailableNetworks->Name);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t HomeNetwork_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    if (pstPlmnInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    CellularMgr_GetPlmnInformation(pstPlmnInfo);
    if(strcmp(context.name, "Mcc") == 0)
    {
        rbusProperty_SetString(property, pstPlmnInfo->HomeNetwork_MCC);
    }
    else if(strcmp(context.name, "Mnc") == 0)
    {
        rbusProperty_SetString(property, pstPlmnInfo->HomeNetwork_MNC);
    }
    else if(strcmp(context.name, "Name") == 0)
    {
        rbusProperty_SetString(property, pstPlmnInfo->HomeNetwork_Name);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t NetworkInUse_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO        pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_PLMNACCESS_INFO    pstPlmnInfo      = &(pstInterfaceInfo->stPlmnAccessInfo);
    if (pstPlmnInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    CellularMgr_GetPlmnInformation(pstPlmnInfo);
    if(strcmp(context.name, "Mcc") == 0)
    {
        rbusProperty_SetString(property, pstPlmnInfo->NetworkInUse_MCC);
    }
    else if(strcmp(context.name, "Mnc") == 0)
    {
        rbusProperty_SetString(property, pstPlmnInfo->NetworkInUse_MNC);
    }
    else if(strcmp(context.name, "Name") == 0)
    {
        rbusProperty_SetString(property, pstPlmnInfo->NetworkInUse_Name);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

bool NeighborCell_IsUpdated_rbus(void* ctx)
{
    return true;
}

rbusError_t NeighborCell_Synchronize_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

int do_NeighborCell_IsUpdated_NeighborCell_Synchronize(HandlerContext context)
{
    if(IsTimeToSyncDynamicTable(context.fullName))
    {
        if(NeighborCell_IsUpdated_rbus(&context))
        {
            return NeighborCell_Synchronize_rbus(&context);
        }
    }
    return 0;
}

static rbusError_t NeighborCell_GetEntryCount_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_NeighborCell_IsUpdated_NeighborCell_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO            pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;
    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "X_RDK_NeighborCellNumberOfEntries") == 0)
    {
        rbusProperty_SetUInt64(property, pstInterfaceInfo->ulNeighbourNoOfEntries);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}


static rbusError_t NeighborCell_GetParamIntValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_NeighborCell_IsUpdated_NeighborCell_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO   pstNeighbourInfo = (PCELLULAR_INTERFACE_NEIGHBOUR_INFO)context.userData;
    if (pstNeighbourInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "ReceivedSignal") == 0)
    {
        rbusProperty_SetInt32(property, pstNeighbourInfo->ReceivedSignal);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t NeighborCell_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);;
    rbusError_t ret;

    if((ret = do_NeighborCell_IsUpdated_NeighborCell_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO   pstNeighbourInfo = (PCELLULAR_INTERFACE_NEIGHBOUR_INFO)context.userData;
    if (pstNeighbourInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Rfcn") == 0)
    {
        rbusProperty_SetUInt32(property, pstNeighbourInfo->Rfcn);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t NeighborCell_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_NeighborCell_IsUpdated_NeighborCell_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO   pstNeighbourInfo = (PCELLULAR_INTERFACE_NEIGHBOUR_INFO)context.userData;
    if (pstNeighbourInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "CellId") == 0)
    {
        rbusProperty_SetString(property, pstNeighbourInfo->CellId);
    }
    else if(strcmp(context.name, "Rat") == 0)
    {
        if (pstNeighbourInfo->Rat == RAT_INFO_GSM)
        {
            rbusProperty_SetString(property, "GSM");
        }
        else if (pstNeighbourInfo->Rat == RAT_INFO_UMTS)
        {
            rbusProperty_SetString(property, "UMTS");
        }
        else if (pstNeighbourInfo->Rat == RAT_INFO_LTE)
        {
            rbusProperty_SetString(property, "LTE");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "PlmnId") == 0)
    {
        rbusProperty_SetString(property, pstNeighbourInfo->PlmnId);
    }
    else if(strcmp(context.name, "AreaCode") == 0)
    {
        rbusProperty_SetString(property, pstNeighbourInfo->AreaCode);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

bool ContextProfile_IsUpdated_rbus(void* ctx)
{
    return true;
}

rbusError_t ContextProfile_Synchronize_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    int i;
    int index = 0;

    if ((pstDmlCellular == NULL) || (pstDmlCellular->pstInterfaceInfo == NULL))
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    sscanf(((HandlerContext *)ctx)->fullName, "Device.Cellular.Interface.%d.X_RDK_ContextProfile", &index);
    PCELLULAR_INTERFACE_INFO   pstInterfaceInfo = &(pstDmlCellular->pstInterfaceInfo[index - 1]);

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    for( i = 0; i < pstInterfaceInfo->ulContextProfileNoOfEntries; i++ )
    {
        PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO    pstContextProfileInfo = &(pstInterfaceInfo->pstContextProfileInfo[i]);

        //Get current context information
        CellularMgrGetCurrentPDPContextStatusInformation( pstContextProfileInfo );
    }
    return RBUS_ERROR_SUCCESS;
}

int do_ContextProfile_IsUpdated_ContextProfile_Synchronize(HandlerContext context)
{
    if(ContextProfile_IsUpdated_rbus(&context))
    {
        return ContextProfile_Synchronize_rbus(&context);
    }
        
    return 0;
}

static rbusError_t ContextProfile_GetEntryCount_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_ContextProfile_IsUpdated_ContextProfile_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_INFO            pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;

    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "X_RDK_ContextProfileNumberOfEntries") == 0)
    {
        rbusProperty_SetUInt64(property, pstInterfaceInfo->ulContextProfileNoOfEntries);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t ContextProfile_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_ContextProfile_IsUpdated_ContextProfile_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO   pstContextProfileInfo = (PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO)context.userData;
    if (pstContextProfileInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "MtuSize") == 0)
    {
        rbusProperty_SetUInt32(property, pstContextProfileInfo->MtuSize);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t ContextProfile_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;
    if((ret = do_ContextProfile_IsUpdated_ContextProfile_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO   pstContextProfileInfo = (PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO)context.userData;
    if (pstContextProfileInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Status") == 0)
    {
        CellularMgr_ServingSystemInfo ( NULL, pstContextProfileInfo);
        if (pstContextProfileInfo->Status == CONTEXTPROFILE_STATUS_ACTIVE)
        {
            rbusProperty_SetString(property, "ACTIVE");
        }
        else if (pstContextProfileInfo->Status == CONTEXTPROFILE_STATUS_INACTIVE)
        {
            rbusProperty_SetString(property, "INACTIVE");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "Type") == 0)
    {
        if (pstContextProfileInfo->Type == CONTEXTPROFILE_TYPE_DEFAULT)
        {
            rbusProperty_SetString(property, "DEFAULT");
        }
        else if (pstContextProfileInfo->Type == CONTEXTPROFILE_TYPE_DEDICATED)
        {
            rbusProperty_SetString(property, "DEDICATED");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "Apn") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Apn);
    }
    else if(strcmp(context.name, "IpAddressFamily") == 0)
    {
        if (pstContextProfileInfo->IpAddressFamily == INTERFACE_PROFILE_FAMILY_IPV4)
        {
            rbusProperty_SetString(property, "IPv4");
        }
        else if (pstContextProfileInfo->IpAddressFamily == INTERFACE_PROFILE_FAMILY_IPV6)
        {
            rbusProperty_SetString(property, "IPv6");
        }
        else if (pstContextProfileInfo->IpAddressFamily == INTERFACE_PROFILE_FAMILY_IPV4_IPV6)
        {
            rbusProperty_SetString(property, "IPv4IPv6");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "Ipv4Adress") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv4Adress);
    }
    else if(strcmp(context.name, "Ipv4SubnetMask") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv4SubnetMask);
    }
    else if(strcmp(context.name, "Ipv4Gateway") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv4Gateway);
    }
    else if(strcmp(context.name, "Ipv4PrimaryDns") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv4PrimaryDns);
    }
    else if(strcmp(context.name, "Ipv4SecondaryDns") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv4SecondaryDns);
    }
    else if(strcmp(context.name, "Ipv6Address") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv6Address);
    }
    else if(strcmp(context.name, "Ipv6Gateway") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv6Gateway);
    }
    else if(strcmp(context.name, "Ipv6PrimaryDns") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv6PrimaryDns);
    }
    else if(strcmp(context.name, "Ipv6SecondaryDns") == 0)
    {
        rbusProperty_SetString(property, pstContextProfileInfo->Ipv6SecondaryDns);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t X_RDK_Statistics_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    PCELLULAR_INTERFACE_INFO           pstInterfaceInfo = (PCELLULAR_INTERFACE_INFO)context.userData;
    if (pstInterfaceInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_INTERFACE_STATS_INFO     pstStatsInfo     = &(pstInterfaceInfo->stStatsInfo);
    if (pstStatsInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    CellularMgr_GetNetworkPacketStatisticsInfo( pstStatsInfo );

    if(strcmp(context.name, "BytesSent") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->BytesSent);
    }
    else if(strcmp(context.name, "BytesReceived") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->BytesReceived);
    }
    else if(strcmp(context.name, "PacketsSent") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->PacketsSent);
    }
    else if(strcmp(context.name, "PacketsReceived") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->PacketsReceived);
    }
    else if(strcmp(context.name, "PacketsSentDrop") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->PacketsSentDrop);
    }
    else if(strcmp(context.name, "PacketsReceivedDrop") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->PacketsReceivedDrop);
    }
    else if(strcmp(context.name, "UpStreamMaxBitRate") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->UpStreamMaxBitRate);
    }
    else if(strcmp(context.name, "DownStreamMaxBitRate") == 0)
    {
        rbusProperty_SetUInt32(property, pstStatsInfo->DownStreamMaxBitRate);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

bool UiccSlot_IsUpdated_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return false;
    }

    if ( ( AnscGetTickInSeconds() - pstDmlCellular->ulUICCSlotListLastUpdatedTime ) < CELLULAR_UICCSLOT_LIST_REFRESH_THRESHOLD )
    {
        return false;
    }
    else
    {
        CcspTraceError(("%s-%d : YES \n",__FUNCTION__, __LINE__));
        pstDmlCellular->ulUICCSlotListLastUpdatedTime =  AnscGetTickInSeconds();
        return true;
    }
    return true;
}

rbusError_t UiccSlot_Synchronize_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    ULONG UICCCurrentEntriesCount = pstDmlCellular->ulUICCNoOfEntries;
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

    if ( (UICCCurrentEntriesCount != pstDmlCellular->ulUICCNoOfEntries) &&
       ( pstDmlCellular->pstUICCSlotInfo != NULL ))
    {
        for(int j = 0; j < pstDmlCellular->ulUICCNoOfEntries; j++)
        {
            PCELLULAR_UICC_SLOT_INFO *pstSlotInfo = &(pstDmlCellular->pstUICCSlotInfo[j]);
            if (Sample_RegisterRow(CELLULARMGR_UICC_TABLE, (j+1), NULL, pstSlotInfo) == RBUS_ERROR_SUCCESS)
            {
                CcspTraceInfo(("%s-%d : Add UICC Table:Inst(%s%d) \n", __FUNCTION__, __LINE__, CELLULARMGR_UICC_TABLE, (j+1)));
            }
        }
    }

    if( ( pstDmlCellular->ulUICCNoOfEntries > 0 ) && ( pstDmlCellular->pstUICCSlotInfo != NULL ) )
    {
        UINT  i;

        for( i = 0; i < pstDmlCellular->ulUICCNoOfEntries; i++ )
        {
            PCELLULAR_UICC_SLOT_INFO *pstSlotInfo = &(pstDmlCellular->pstUICCSlotInfo[i]);
            CellularMgr_GetUICCSlotInfo( i, pstSlotInfo );
            SetRowContext(CELLULARMGR_UICC_TABLE, (i+1), NULL, pstSlotInfo);
        }
    }
    return ANSC_STATUS_SUCCESS;
}

int do_UiccSlot_IsUpdated_UiccSlot_Synchronize(HandlerContext context)
{
    if(UiccSlot_IsUpdated_rbus(&context))
    {
        return UiccSlot_Synchronize_rbus(&context);
    }
    return 0;
}

static rbusError_t UiccSlot_GetEntryCount_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    rbusError_t ret;

    if((ret = do_UiccSlot_IsUpdated_UiccSlot_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "X_RDK_UiccNumberOfEntries") == 0)
    {
        rbusProperty_SetUInt64(property, pstDmlCellular->ulUICCNoOfEntries);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t UiccSlot_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_UiccSlot_IsUpdated_UiccSlot_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_UICC_SLOT_INFO       pstSlotInfo = (PCELLULAR_UICC_SLOT_INFO)context.userData;
    if (pstSlotInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Enable") == 0)
    {
        rbusProperty_SetBoolean(property, pstSlotInfo->PowerEnable);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t UiccSlot_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_UiccSlot_IsUpdated_UiccSlot_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_UICC_SLOT_INFO       pstSlotInfo = (PCELLULAR_UICC_SLOT_INFO)context.userData;
    if (pstSlotInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Status") == 0)
    {
        if (pstSlotInfo->Status == SIM_STATUS_VALID)
        {
            rbusProperty_SetString(property, "VALID");
        }
        else if (pstSlotInfo->Status == SIM_STATUS_BLOCKED)
        {
            rbusProperty_SetString(property, "BLOCKED");
        }
        else if (pstSlotInfo->Status == SIM_STATUS_ERROR)
        {
            rbusProperty_SetString(property, "ERROR");
        }
        else
        {
            rbusProperty_SetString(property, "EMPTY");
        }
    }
    else if(strcmp(context.name, "FormFactor") == 0)
    {
        if (pstSlotInfo->FormFactor == USIM_1FF)
        {
            rbusProperty_SetString(property, "1FF");
        }
        else if (pstSlotInfo->FormFactor == USIM_2FF)
        {
            rbusProperty_SetString(property, "2FF");
        }
        else if (pstSlotInfo->FormFactor == USIM_3FF)
        {
            rbusProperty_SetString(property, "3FF");
        }
        else if (pstSlotInfo->FormFactor == USIM_4FF)
        {
            rbusProperty_SetString(property, "4FF");
        }
        else if (pstSlotInfo->FormFactor == USIM_M2FF)
        {
            rbusProperty_SetString(property, "M2FF");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "Application") == 0)
    {
        if (pstSlotInfo->Application == APPLICATION_USIM)
        {
            rbusProperty_SetString(property, "USIM");
        }
        else if (pstSlotInfo->Application == APPLICATION_ESIM)
        {
            rbusProperty_SetString(property, "eSIM");
        }
        else if (pstSlotInfo->Application == APPLICATION_ISIM)
        {
            rbusProperty_SetString(property, "iSIM");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "Iccid") == 0)
    {
        rbusProperty_SetString(property, pstSlotInfo->Iccid);
    }
    else if(strcmp(context.name, "Msisdn") == 0)
    {
        rbusProperty_SetString(property, pstSlotInfo->Msisdn);
    }
    else if(strcmp(context.name, "MnoName") == 0)
    {
        rbusProperty_SetString(property, pstSlotInfo->MnoName);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t UiccSlot_Validate_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t UiccSlot_Commit_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t UiccSlot_Rollback_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_UiccSlot_Validate_UiccSlot_Commit_UiccSlot_Rollback(void* context)
{
    if(UiccSlot_Validate_rbus(context) == 0)
    {
        if(UiccSlot_Commit_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(UiccSlot_Rollback_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t UiccSlot_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_UiccSlot_IsUpdated_UiccSlot_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_UICC_SLOT_INFO       pstSlotInfo = (PCELLULAR_UICC_SLOT_INFO)context.userData;

    if (pstSlotInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Enable") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        if( RETURN_OK == CellularMgr_SetSIMPowerEnable( pstSlotInfo->uiInstanceNumber - 1, val ) )
        {
            pstSlotInfo->PowerEnable = val;
        }
        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(opts->commit)
    {
      return do_UiccSlot_Validate_UiccSlot_Commit_UiccSlot_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t EuiccSlot_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);
    if (pstSlotInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "PowerEnable") == 0)
    {
        rbusProperty_SetBoolean(property, pstSlotInfo->PowerEnable);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t EuiccSlot_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);

    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);
    if (pstSlotInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Status") == 0)
    {
        if (pstSlotInfo->Status == SIM_STATUS_VALID)
        {
            rbusProperty_SetString(property, "VALID");
        }
        else if (pstSlotInfo->Status == SIM_STATUS_BLOCKED)
        {
            rbusProperty_SetString(property, "BLOCKED");
        }
        else if (pstSlotInfo->Status == SIM_STATUS_ERROR)
        {
            rbusProperty_SetString(property, "ERROR");
        }
        else
        {
            rbusProperty_SetString(property, "EMPTY");
        }
    }
    else if(strcmp(context.name, "FormFactor") == 0)
    {
        if (pstSlotInfo->FormFactor == ESIM_M2FF)
        {
            rbusProperty_SetString(property, "M2FF");
        }
        else if (pstSlotInfo->FormFactor == ESIM_ISIM)
        {
            rbusProperty_SetString(property, "iSIM");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "Application") == 0)
    {
        if (pstSlotInfo->Application == APPLICATION_USIM)
        {
            rbusProperty_SetString(property, "USIM");
        }
        else if (pstSlotInfo->Application == APPLICATION_ESIM)
        {
            rbusProperty_SetString(property, "eSIM");
        }
        else if (pstSlotInfo->Application == APPLICATION_ISIM)
        {
            rbusProperty_SetString(property, "iSIM");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "Imei") == 0)
    {
        rbusProperty_SetString(property, pstSlotInfo->Imei);
    }
    else if(strcmp(context.name, "EIccid") == 0)
    {
        rbusProperty_SetString(property, pstSlotInfo->EIccid);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t EuiccSlot_Validate_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t EuiccSlot_Commit_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t EuiccSlot_Rollback_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_EuiccSlot_Validate_EuiccSlot_Commit_EuiccSlot_Rollback(void* context)
{
    if(EuiccSlot_Validate_rbus(context) == 0)
    {
        if(EuiccSlot_Commit_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(EuiccSlot_Rollback_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t EuiccSlot_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);
    if (pstSlotInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "PowerEnable") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

         pstSlotInfo->PowerEnable = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(opts->commit)
    {
      return do_EuiccSlot_Validate_EuiccSlot_Commit_EuiccSlot_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

bool MnoProfile_IsUpdated_rbus(void* ctx)
{
    return true;
}

rbusError_t MnoProfile_Synchronize_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

int do_MnoProfile_IsUpdated_MnoProfile_Synchronize(HandlerContext context)
{
    if(IsTimeToSyncDynamicTable(context.fullName))
    {
        if(MnoProfile_IsUpdated_rbus(context.userData))
        {
            return MnoProfile_Synchronize_rbus(context.userData);
        }
    }
    return 0;
}

static rbusError_t MnoProfile_GetEntryCount_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    rbusError_t ret;

    if((ret = do_MnoProfile_IsUpdated_MnoProfile_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_EUICC_SLOT_INFO   pstSlotInfo    = (PCELLULAR_EUICC_SLOT_INFO)&(pstDmlCellular->stEUICCSlotInfo);
    if (pstSlotInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "MnoProfileNumberOfEntries") == 0)
    {
        rbusProperty_SetUInt64(property, pstSlotInfo->ulMNOProfileNoOfEntries);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t MnoProfile_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_MnoProfile_IsUpdated_MnoProfile_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_MNO_PROFILE_INFO       pstMNPOProfileInfo = (PCELLULAR_MNO_PROFILE_INFO) context.userData;

    if (pstMNPOProfileInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "PowerEnable") == 0)
    {
        rbusProperty_SetBoolean(property, pstMNPOProfileInfo->Enable);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t MnoProfile_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_MnoProfile_IsUpdated_MnoProfile_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_MNO_PROFILE_INFO       pstMNPOProfileInfo = (PCELLULAR_MNO_PROFILE_INFO) context.userData;

    if (pstMNPOProfileInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Status") == 0)
    {
        if (pstMNPOProfileInfo->Status == SIM_STATUS_VALID)
        {
            rbusProperty_SetString(property, "VALID");
        }
        else if (pstMNPOProfileInfo->Status == SIM_STATUS_BLOCKED)
        {
            rbusProperty_SetString(property, "BLOCKED");
        }
        else if (pstMNPOProfileInfo->Status == SIM_STATUS_ERROR)
        {
            rbusProperty_SetString(property, "ERROR");
        }
        else
        {
            rbusProperty_SetString(property, "EMPTY");
        }
    }
    else if(strcmp(context.name, "Iccid") == 0)
    {
        rbusProperty_SetString(property, pstMNPOProfileInfo->Iccid);
    }
    else if(strcmp(context.name, "Msisdn") == 0)
    {
        rbusProperty_SetString(property, pstMNPOProfileInfo->Msisdn);
    }
    else if(strcmp(context.name, "Imsi") == 0)
    {
        rbusProperty_SetString(property, pstMNPOProfileInfo->Imsi);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

void* Cellular_AccessPoint_Add(void* ctx, uint32_t* instNum)
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
    pstProfileInfo[ulTotalCount - 1].X_RDK_Roaming = TRUE;

    //Find first available index
    *instNum = -1;
    for(int i = 1; i < ulTotalCount; i++)
    {
        bool indexFound = false;
        for(int j = 0 ; j < (ulTotalCount -1); j++)
        {
            if(pstProfileInfo[j].PDPContextNumber == i)
            {
                indexFound = true;
                break;
            }
        }
        if(!indexFound)
        {
            *instNum = i;
            break;
        }
    }

    if(*instNum == -1)
    {
        *instNum = ulTotalCount;
    }
    
    pstProfileInfo[ulTotalCount - 1].PDPContextNumber = *instNum;
    pstDmlCellular->pstAPInfo = pstProfileInfo;

    //Needs to update current time during add since new profile can be updated withing TTL threshold
    pstDmlCellular->ulAccessPointListLastUpdatedTime = 0;

    /* Set a unique APN value to avoid Access Point creation failure due to duplicate profile */
    snprintf(pstProfileInfo[ulTotalCount - 1].APN, sizeof(pstProfileInfo[ulTotalCount - 1].APN), "APN_%d", pstProfileInfo[ulTotalCount - 1].PDPContextNumber);
    CellularMgr_AccessPointCreateProfile( &pstProfileInfo[ulTotalCount - 1] );

    CcspTraceInfo(("%s-%d : Added AccessPoint(%d) Successfully \n",__FUNCTION__, __LINE__, *instNum));
    return ((ANSC_HANDLE)&(pstDmlCellular->pstAPInfo[pstDmlCellular->ulAccessPointNoOfEntries - 1]));
}

static rbusError_t Cellular_AccessPoint_AddEntry_rbus(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    HandlerContext context = GetTableContext(tableName);
    void* rowContext = Cellular_AccessPoint_Add(NULL, instNum);
    if(!rowContext)
    {
        CcspTraceError(("Cellular_AccessPoint_Add returned null row context"));
        return RBUS_ERROR_BUS_ERROR;
    }
    SetRowContext(context.fullName, *instNum, aliasName, rowContext);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_AccessPoint_Delete(void* ctx, void* inst)
{
    PCELLULARMGR_CELLULAR_DATA              pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    if (pMyObject == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_DML_INFO                      pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO    pstProfileInfo =  (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)inst;
    if (pstProfileInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    UINT                                    ulTotalCount;
    ANSC_STATUS                             returnStatus   = ANSC_STATUS_SUCCESS;

    //Check if this is default profile or not
    if( TRUE == pstProfileInfo->X_RDK_Default )
    {
        return -1;
    }

    //Delete Profile
    CellularMgr_AccessPointDeleteProfile( pstProfileInfo );

    pstDmlCellular->ulAccessPointListLastUpdatedTime =  0; //To resync next time after delete

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_AccessPoint_DeleteEntry_rbus(rbusHandle_t handle, char const* rowName)
{
    HandlerContext context = GetHandlerContext(rowName);
    void* rowContext = GetRowContext(context.fullName);
    int rc = Cellular_AccessPoint_Delete(&context, rowContext);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s-%d : Cellular_AccessPoint_Delete failed\n",__FUNCTION__, __LINE__));
        return RBUS_ERROR_BUS_ERROR;
    }
    RemoveRowContextByName(rowName);
    return RBUS_ERROR_SUCCESS;
}

bool Cellular_AccessPoint_IsUpdated_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if ( ( AnscGetTickInSeconds() - pstDmlCellular->ulAccessPointListLastUpdatedTime ) < CELLULAR_ACCESSPOINT_LIST_REFRESH_THRESHOLD )
    {
        return false;
    }
    else
    {
        CcspTraceError(("%s-%d : YES \n",__FUNCTION__, __LINE__));
        pstDmlCellular->ulAccessPointListLastUpdatedTime =  AnscGetTickInSeconds();
        return true;
    }
    return true;
}

rbusError_t Cellular_AccessPoint_Synchronize_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    ULONG AccessPointCurrentEntriesCount = pstDmlCellular->ulAccessPointNoOfEntries;
    CellularMgr_AccessPointGetProfileList( &(pstDmlCellular->pstAPInfo), (int*)&pstDmlCellular->ulAccessPointNoOfEntries);

    if ( pstDmlCellular->pstAPInfo != NULL )
    {
        for(int j = 0; j < pstDmlCellular->ulAccessPointNoOfEntries; j++)
        {
            CELLULAR_INTERFACE_ACCESSPOINT_INFO  *pstAPInfo = &(pstDmlCellular->pstAPInfo[j]);

            if (Sample_RegisterRow(CELLULARMGR_ACCESSPOINT_TABLE, pstAPInfo->PDPContextNumber, NULL, pstAPInfo) == RBUS_ERROR_SUCCESS)
            {
                CcspTraceInfo(("%s-%d : Add AccessPoint Table:Inst(%s%d) \n", __FUNCTION__, __LINE__, CELLULARMGR_ACCESSPOINT_TABLE,pstAPInfo->PDPContextNumber));
            }
            else
            {
                CcspTraceInfo(("%s-%d : tableName(%s), Inst(%d) \n",__FUNCTION__, __LINE__, CELLULARMGR_ACCESSPOINT_TABLE, pstAPInfo->PDPContextNumber));
                SetRowContext(CELLULARMGR_ACCESSPOINT_TABLE, pstAPInfo->PDPContextNumber, NULL, pstAPInfo);
            }
        }
    }
    return RBUS_ERROR_SUCCESS;
}

int do_Cellular_AccessPoint_IsUpdated_Cellular_AccessPoint_Synchronize(HandlerContext context)
{
    if(Cellular_AccessPoint_IsUpdated_rbus(&context))
    {
        return Cellular_AccessPoint_Synchronize_rbus(&context);
    }
    return 0;
}

static rbusError_t Cellular_AccessPoint_GetEntryCount_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
    rbusError_t ret;

    if((ret = do_Cellular_AccessPoint_IsUpdated_Cellular_AccessPoint_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "AccessPointNumberOfEntries") == 0)
    {
        rbusProperty_SetUInt64(property, pstDmlCellular->ulAccessPointNoOfEntries);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_AccessPoint_GetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_Cellular_AccessPoint_IsUpdated_Cellular_AccessPoint_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)context.userData;

    if (pstAPInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Enable") == 0)
    {
        rbusProperty_SetBoolean(property, pstAPInfo->Enable);
    }
    else if(strcmp(context.name, "X_RDK_DefaultProfile") == 0)
    {
        rbusProperty_SetBoolean(property, pstAPInfo->X_RDK_Default);
    }
    else if(strcmp(context.name, "X_RDK_Roaming") == 0)
    {
        rbusProperty_SetBoolean(property, pstAPInfo->X_RDK_Roaming);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_AccessPoint_GetParamUlongValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_Cellular_AccessPoint_IsUpdated_Cellular_AccessPoint_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)context.userData;
    if (pstAPInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "X_RDK_ProfileId") == 0)
    {
        rbusProperty_SetUInt32(property, pstAPInfo->ProfileIndex);
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

static rbusError_t Cellular_AccessPoint_GetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_Cellular_AccessPoint_IsUpdated_Cellular_AccessPoint_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)context.userData;

    if (pstAPInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Alias") == 0)
    {
        rbusProperty_SetString(property, pstAPInfo->Alias);
    }
    else if(strcmp(context.name, "Apn") == 0)
    {
        rbusProperty_SetString(property, pstAPInfo->APN);
    }
    else if(strcmp(context.name, "X_RDK_ApnAuthentication") == 0)
    {
        if (pstAPInfo->X_RDK_ApnAuthentication == CELLULAR_PDP_AUTHENTICATION_PAP)
        {
            rbusProperty_SetString(property, "PAP");
        }
        else if (pstAPInfo->X_RDK_ApnAuthentication == CELLULAR_PDP_AUTHENTICATION_CHAP)
        {
            rbusProperty_SetString(property, "CHAP");
        }
        else
        {
            rbusProperty_SetString(property, "NONE");
        }
    }
    else if(strcmp(context.name, "Username") == 0)
    {
        rbusProperty_SetString(property, pstAPInfo->Username);
    }
    else if(strcmp(context.name, "Password") == 0)
    {
        rbusProperty_SetString(property, pstAPInfo->Password);
    }
    else if(strcmp(context.name, "X_RDK_IpAddressFamily") == 0)
    {
        if (pstAPInfo->X_RDK_IpAddressFamily == INTERFACE_PROFILE_FAMILY_IPV4)
        {
            rbusProperty_SetString(property, "IPv4");
        }
        else if (pstAPInfo->X_RDK_IpAddressFamily == INTERFACE_PROFILE_FAMILY_IPV6)
        {
            rbusProperty_SetString(property, "IPv6");
        }
        else if (pstAPInfo->X_RDK_IpAddressFamily == INTERFACE_PROFILE_FAMILY_IPV4_IPV6)
        {
            rbusProperty_SetString(property, "IPv4IPv6");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else if(strcmp(context.name, "X_RDK_PdpInterfaceConfig") == 0)
    {
        if (pstAPInfo->X_RDK_PdpInterfaceConfig == CELLULAR_PDP_NETWORK_CONFIG_NAS)
        {
            rbusProperty_SetString(property, "NAS");
        }
        else if (pstAPInfo->X_RDK_PdpInterfaceConfig == CELLULAR_PDP_NETWORK_CONFIG_DHCP)
        {
            rbusProperty_SetString(property, "DHCP");
        }
        else
        {
            rbusProperty_SetString(property, "None");
        }
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_AccessPoint_Validate_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_AccessPoint_Commit_rbus(void* ctx)
{
    PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;

    PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;
    if (pstDmlCellular == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)((HandlerContext *)ctx)->userData;
    if (pstAPInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

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

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_AccessPoint_Rollback_rbus(void* ctx)
{
    return RBUS_ERROR_SUCCESS;
}

rbusError_t do_Cellular_AccessPoint_Validate_Cellular_AccessPoint_Commit_Cellular_AccessPoint_Rollback(void* context)
{
    if(Cellular_AccessPoint_Validate_rbus(context) == 0)
    {
        if(Cellular_AccessPoint_Commit_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;
    }
    else
    {
        if(Cellular_AccessPoint_Rollback_rbus(context) == 0)
            return RBUS_ERROR_SUCCESS;
        else
            return RBUS_ERROR_BUS_ERROR;  
    }
}

rbusError_t Cellular_AccessPoint_SetParamBoolValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_Cellular_AccessPoint_IsUpdated_Cellular_AccessPoint_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)context.userData;
    
    if (pstAPInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Enable") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        pstAPInfo->Enable = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "X_RDK_DefaultProfile") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

         pstAPInfo->X_RDK_Default = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "X_RDK_Roaming") == 0)
    {
        bool val;

        rbusValueError_t verr = rbusProperty_GetBooleanEx(property, &val);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

         pstAPInfo->X_RDK_Roaming = val;
        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(opts->commit)
    {
      return do_Cellular_AccessPoint_Validate_Cellular_AccessPoint_Commit_Cellular_AccessPoint_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t Cellular_AccessPoint_SetParamStringValue_rbus(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    HandlerContext context = GetPropertyContext(property);
    rbusError_t ret;

    if((ret = do_Cellular_AccessPoint_IsUpdated_Cellular_AccessPoint_Synchronize(context)) != RBUS_ERROR_SUCCESS)
        return ret;

    context = GetPropertyContext(property);
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo = (PCELLULAR_INTERFACE_ACCESSPOINT_INFO)context.userData;

    if (pstAPInfo == NULL)
    {
        return RBUS_ERROR_BUS_ERROR;
    }

    if(strcmp(context.name, "Alias") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

         AnscCopyString(pstAPInfo->Alias, val);
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "Apn") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        AnscCopyString(pstAPInfo->APN, val);
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "X_RDK_ApnAuthentication") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "Username") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        AnscCopyString(pstAPInfo->Username, val);
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "Password") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

         AnscCopyString(pstAPInfo->Password, val);
        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "X_RDK_IpAddressFamily") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else if(strcmp(context.name, "X_RDK_PdpInterfaceConfig") == 0)
    {
        const char* val;

        rbusValueError_t verr = rbusProperty_GetStringEx(property, &val, NULL);
        if(verr != RBUS_VALUE_ERROR_SUCCESS)
            return RBUS_ERROR_INVALID_INPUT;

        /*context.userData is this objects data and val is the property's new value*/
    }
    else
    {
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(opts->commit)
    {
      return do_Cellular_AccessPoint_Validate_Cellular_AccessPoint_Commit_Cellular_AccessPoint_Rollback(&context);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusError_t registerGeneratedDataElements(rbusHandle_t handle)
{
    rbusError_t rc;
    static rbusDataElement_t dataElements[] = {
        {"Device.Cellular.X_RDK_Enable", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamBoolValue_rbus, Cellular_SetParamBoolValue_rbus, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.X_RDK_Status", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.X_RDK_Model", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_HardwareRevision", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Vendor", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_ControlInterface", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_ControlInterfaceStatus", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_DataInterface", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_DataInterfaceLink", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.CellularConfig", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, Cellular_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
#if defined HYBRID_SUPPORT
        {"Device.Cellular.X_RDK_DeviceType", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_GetParamStringValue_rbus, Cellular_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
#endif
        {"Device.Cellular.X_RDK_DeviceManagement.Imei", RBUS_ELEMENT_TYPE_PROPERTY, {DeviceManagement_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_DeviceManagement.FactoryReset", RBUS_ELEMENT_TYPE_PROPERTY, {DeviceManagement_GetParamBoolValue_rbus, DeviceManagement_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.X_RDK_Firmware.CurrentImageVersion", RBUS_ELEMENT_TYPE_PROPERTY, {Firmware_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Firmware.FallbackImageVersion", RBUS_ELEMENT_TYPE_PROPERTY, {Firmware_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.InterfaceNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetEntryCount_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamBoolValue_rbus, Cellular_Interface_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}}, 
        {"Device.Cellular.Interface.{i}.Status", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.LastChange", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.LowerLayers", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, Cellular_Interface_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
        {"Device.Cellular.Interface.{i}.Upstream", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamBoolValue_rbus, Cellular_Interface_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
#endif
        {"Device.Cellular.Interface.{i}.X_RDK_RegisteredService", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
	    {"Device.Cellular.Interface.{i}.X_RDK_PhyConnectedStatus", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamBoolValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_LinkAvailableStatus", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamBoolValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.RegistrationRetries", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamUlongValue_rbus, Cellular_Interface_SetParamUlongValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.MaxRegistrationRetries", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamUlongValue_rbus, Cellular_Interface_SetParamUlongValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.RegistrationRetryTimer", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamUlongValue_rbus, Cellular_Interface_SetParamUlongValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.IMEI", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.SupportedAccessTechnologies", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.PreferredAccessTechnologies", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, Cellular_Interface_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.CurrentAccessTechnology", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_CellId", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_RAT", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_RFCN", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnId", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_AreaCode", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.RSSI", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamIntValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_SNR", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamIntValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.RSRP", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamIntValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.RSRQ", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamIntValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_TRX", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamIntValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_RadioEnvConditions", RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_Interface_GetParamStringValue_rbus, NULL, NULL, NULL, CellularMgrDmlPublishEventHandler, NULL}},

        {"Device.Cellular.Interface.{i}.X_RDK_Identification.Imei", RBUS_ELEMENT_TYPE_PROPERTY, {Identification_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Identification.Iccid", RBUS_ELEMENT_TYPE_PROPERTY, {Identification_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.RoamingEnable", RBUS_ELEMENT_TYPE_PROPERTY, {PlmnAccess_GetParamBoolValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.RoamingStatus", RBUS_ELEMENT_TYPE_PROPERTY, {PlmnAccess_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},


        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.AvailableNetworksNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {AvailableNetworks_GetEntryCount_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.AvailableNetworks.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
	{"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.AvailableNetworks.{i}.Mcc", RBUS_ELEMENT_TYPE_PROPERTY, {AvailableNetworks_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.AvailableNetworks.{i}.Mnc", RBUS_ELEMENT_TYPE_PROPERTY, {AvailableNetworks_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.AvailableNetworks.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {AvailableNetworks_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.AvailableNetworks.{i}.Allowed", RBUS_ELEMENT_TYPE_PROPERTY, {AvailableNetworks_GetParamBoolValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.HomeNetwork.Mcc", RBUS_ELEMENT_TYPE_PROPERTY, {HomeNetwork_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.HomeNetwork.Mnc", RBUS_ELEMENT_TYPE_PROPERTY, {HomeNetwork_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.HomeNetwork.Name", RBUS_ELEMENT_TYPE_PROPERTY, {HomeNetwork_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.NetworkInUse.Mcc", RBUS_ELEMENT_TYPE_PROPERTY, {NetworkInUse_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.NetworkInUse.Mnc", RBUS_ELEMENT_TYPE_PROPERTY, {NetworkInUse_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_PlmnAccess.NetworkInUse.Name", RBUS_ELEMENT_TYPE_PROPERTY, {NetworkInUse_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCellNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {NeighborCell_GetEntryCount_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.CellId", RBUS_ELEMENT_TYPE_PROPERTY, {NeighborCell_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.Rat", RBUS_ELEMENT_TYPE_PROPERTY, {NeighborCell_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.Rfcn", RBUS_ELEMENT_TYPE_PROPERTY, {NeighborCell_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.PlmnId", RBUS_ELEMENT_TYPE_PROPERTY, {NeighborCell_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.AreaCode", RBUS_ELEMENT_TYPE_PROPERTY, {NeighborCell_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_NeighborCell.{i}.ReceivedSignal", RBUS_ELEMENT_TYPE_PROPERTY, {NeighborCell_GetParamIntValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfileNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetEntryCount_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Type", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Apn", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.IpAddressFamily", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv4Adress", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv4SubnetMask", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv4Gateway", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv4PrimaryDns", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv4SecondaryDns", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv6Address", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv6Gateway", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv6PrimaryDns", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.Ipv6SecondaryDns", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_ContextProfile.{i}.MtuSize", RBUS_ELEMENT_TYPE_PROPERTY, {ContextProfile_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.BytesSent", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.BytesReceived", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.PacketsSent", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.PacketsReceived", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.PacketsSentDrop", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.PacketsReceivedDrop", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.UpStreamMaxBitRate", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.Interface.{i}.X_RDK_Statistics.DownStreamMaxBitRate", RBUS_ELEMENT_TYPE_PROPERTY, {X_RDK_Statistics_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.X_RDK_UiccNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetEntryCount_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetParamBoolValue_rbus, UiccSlot_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.FormFactor", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.Application", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.MnoName", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.Iccid", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Uicc.{i}.Msisdn", RBUS_ELEMENT_TYPE_PROPERTY, {UiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.PowerEnable", RBUS_ELEMENT_TYPE_PROPERTY, {EuiccSlot_GetParamBoolValue_rbus, EuiccSlot_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.Status", RBUS_ELEMENT_TYPE_PROPERTY, {EuiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.FormFactor", RBUS_ELEMENT_TYPE_PROPERTY, {EuiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.Application", RBUS_ELEMENT_TYPE_PROPERTY, {EuiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.Imei", RBUS_ELEMENT_TYPE_PROPERTY, {EuiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.EIccid", RBUS_ELEMENT_TYPE_PROPERTY, {EuiccSlot_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.X_RDK_Euicc.MnoProfileNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {MnoProfile_GetEntryCount_rbus, NULL, NULL, NULL, NULL, NULL}},
       	{"Device.Cellular.X_RDK_Euicc.MnoProfile.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.MnoProfile.{i}.PowerEnable", RBUS_ELEMENT_TYPE_PROPERTY, {MnoProfile_GetParamBoolValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.MnoProfile.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {MnoProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.MnoProfile.{i}.Iccid", RBUS_ELEMENT_TYPE_PROPERTY, {MnoProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.MnoProfile.{i}.Msisdn", RBUS_ELEMENT_TYPE_PROPERTY, {MnoProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.X_RDK_Euicc.MnoProfile.{i}.Imsi", RBUS_ELEMENT_TYPE_PROPERTY, {MnoProfile_GetParamStringValue_rbus, NULL, NULL, NULL, NULL, NULL}},

        {"Device.Cellular.AccessPointNumberOfEntries", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetEntryCount_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, Cellular_AccessPoint_AddEntry_rbus, Cellular_AccessPoint_DeleteEntry_rbus, NULL, NULL}},

        {"Device.Cellular.AccessPoint.{i}.Enable", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamBoolValue_rbus, Cellular_AccessPoint_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.X_RDK_Roaming", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamBoolValue_rbus, Cellular_AccessPoint_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.X_RDK_DefaultProfile", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamBoolValue_rbus, Cellular_AccessPoint_SetParamBoolValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.X_RDK_ProfileId", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamUlongValue_rbus, NULL, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamStringValue_rbus, Cellular_AccessPoint_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.Apn", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamStringValue_rbus, Cellular_AccessPoint_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.X_RDK_ApnAuthentication", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamStringValue_rbus, Cellular_AccessPoint_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.Username", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamStringValue_rbus, Cellular_AccessPoint_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.Password", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamStringValue_rbus, Cellular_AccessPoint_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.X_RDK_IpAddressFamily", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamStringValue_rbus, Cellular_AccessPoint_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}},
        {"Device.Cellular.AccessPoint.{i}.X_RDK_PdpInterfaceConfig", RBUS_ELEMENT_TYPE_PROPERTY, {Cellular_AccessPoint_GetParamStringValue_rbus, Cellular_AccessPoint_SetParamStringValue_rbus, NULL, NULL, NULL, NULL}}
    };
    rc = rbus_regDataElements(handle, ARRAY_SZ(dataElements), dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        rtLog_Error("rbus_regDataElements failed");
        CcspTraceError(("%s %d - rbus_regDataElements failed=%d \n", __FUNCTION__, __LINE__, rc));
    }
    return rc;
}

#endif /*RBUS_BUILD_FLAG_ENABLE */
