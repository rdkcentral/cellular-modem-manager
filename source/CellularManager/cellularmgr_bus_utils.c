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

#include "cellularmgr_cellular_apis.h"

extern ANSC_HANDLE  bus_handle;
extern char         g_Subsystem[32];

static int CellularMgr_RdkBus_GetComponentInfo ( char *pCompName, char *pCompPath, char *pParamName ) 
{
    if ( NULL == pParamName )
    {
        CcspTraceError(("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    int size = 0, ret;
    char dst_pathname_cr[256] = {0};
    componentStruct_t **ppComponents = NULL;

    snprintf(dst_pathname_cr, sizeof(dst_pathname_cr) - 1, "eRT.%s", CCSP_DBUS_INTERFACE_CR);

    // Get the component name and dbus path which has the data model 
    ret = CcspBaseIf_discComponentSupportingNamespace
        (
         bus_handle,
         dst_pathname_cr,
         pParamName,
         "",
         &ppComponents,
         &size
        );

        if ( ret == CCSP_SUCCESS && size >= 1)
        {
            AnscCopyString(pCompName, ppComponents[0]->componentName);
            AnscCopyString(pCompPath, ppComponents[0]->dbusPath);
        }
        else
        {
            CcspTraceError(("Failed to get component for %s ret: %d\n",pParamName,ret));
        }

        free_componentStruct_t(bus_handle, size, ppComponents);
	
  return RETURN_OK;
}


int CellularMgr_RdkBus_SetParamValue ( char *pComponentName, char *pComponentPath, char *pParamName, char *pParamValue, enum dataType_e type, bool bCommit)
{
    if ( ( pParamName == NULL ) || ( pParamValue == NULL ) || ( NULL == pComponentName ) || ( NULL == pComponentPath ) )
    {
        CcspTraceError(("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    int ret;
    char acCompName[256] = {0}, acCompPath[256] = {0};

    snprintf(acCompName, sizeof(acCompName), "%s",pComponentName);
    snprintf(acCompPath, sizeof(acCompPath), "%s",pComponentPath);

    // query the data model from the component
    CcspTraceInfo (("%s %d: Setting dm:%s from component:%s of dbuspath:%s\n", 
                __FUNCTION__, __LINE__, pParamName, acCompName, acCompPath));

    parameterValStruct_t   param_val[1]          = { 0 };
    char                  *faultParam            = NULL;

    //Copy Name
    param_val[0].parameterName  = pParamName;
    //Copy Value
    param_val[0].parameterValue = pParamValue;
    //Copy Type
    param_val[0].type           = type;

    ret = CcspBaseIf_setParameterValues(
                                        bus_handle,
                                        acCompName,
                                        acCompPath,
                                        0,
                                        0,
                                        param_val,
                                        1,
                                        bCommit,
                                        &faultParam
                                       );

    if( ( ret != CCSP_SUCCESS ) && ( faultParam != NULL ) )
    {
        CcspTraceError(("%s-%d Failed to set %s\n",__FUNCTION__,__LINE__,pParamName));
        free( faultParam );
        return RETURN_ERROR;
    }

    return RETURN_OK;

}

int CellularMgr_RdkBus_GetParamValue ( char *pComponentName, char *pComponentPath, char *pParamName, char *pParamValue )
{
    if ( ( pParamName == NULL ) || ( pParamValue == NULL ) || ( pComponentName == NULL ) || ( pComponentPath == NULL ) )
    {
        CcspTraceError(("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    char acCompName[256] = {0}, acCompPath[256] = {0};

    snprintf(acCompName, sizeof(acCompName), "%s",pComponentName);
    snprintf(acCompPath, sizeof(acCompPath), "%s",pComponentPath);

    // query the data model from the component
    CcspTraceInfo (("%s %d: Quering dm:%s from component:%s of dbuspath:%s\n", 
                __FUNCTION__, __LINE__, pParamName, acCompName, acCompPath));

    parameterValStruct_t   **retVal = 0;
    char                    *ParamName[ 1 ];
    int                    ret               = 0,
                           nval;

    //Assign address for get parameter name
    ParamName[0] = pParamName;

    ret = CcspBaseIf_getParameterValues(
                                    bus_handle,
                                    acCompName,
                                    acCompPath,
                                    ParamName,
                                    1,
                                    &nval,
                                    &retVal);

    //Copy the value
    if( CCSP_SUCCESS == ret )
    {
        if( NULL != retVal[0]->parameterValue )
        {
            memcpy( pParamValue, retVal[0]->parameterValue, strlen( retVal[0]->parameterValue ) + 1 );
        }

        if( retVal )
        {
            free_parameterValStruct_t (bus_handle, nval, retVal);
        }

        return RETURN_OK;
    }

    if( retVal )
    {
       free_parameterValStruct_t (bus_handle, nval, retVal);
    }

    return RETURN_ERROR;
}

int CellularMgr_RdkBus_GetParamValuesFromDB( char *pParamName, char *pReturnVal, int returnValLength )
{
    int     retPsmGet     = CCSP_SUCCESS;
    CHAR   *param_value   = NULL, tmpOutput[BUFLEN_256] = {0};

    /* Input Validation */
    if( ( NULL == pParamName) || ( NULL == pReturnVal ) || ( 0 >= returnValLength ) )
    {
        CcspTraceError(("%s Invalid Input Parameters\n",__FUNCTION__));
        return CCSP_FAILURE;
    }

    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, pParamName, NULL, &param_value); 
    if (retPsmGet != CCSP_SUCCESS) 
    { 
        CcspTraceError(("%s Error %d reading %s\n", __FUNCTION__, retPsmGet, pParamName));
    } 
    else 
    { 
        /* Copy DB Value */
        snprintf(pReturnVal, returnValLength, "%s", param_value);
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    } 

   return retPsmGet;
}

int CellularMgr_RdkBus_SetParamValuesToDB( char *pParamName, char *pParamVal )
{
    int     retPsmSet  = CCSP_SUCCESS;

    /* Input Validation */
    if( ( NULL == pParamName) || ( NULL == pParamVal ) )
    {
        CcspTraceError(("%s Invalid Input Parameters\n",__FUNCTION__));
        return CCSP_FAILURE;
    }

    retPsmSet = PSM_Set_Record_Value2(bus_handle, g_Subsystem, pParamName, ccsp_string, pParamVal); 
    if (retPsmSet != CCSP_SUCCESS) 
    { 
        CcspTraceError(("%s Error %d writing %s\n", __FUNCTION__, retPsmSet, pParamName));
    } 

    return retPsmSet;
}
