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


#include "cellularmgr_plugin_main_apis.h"
#include "cellularmgr_cellular_internal.h"
#include "cellularmgr_global.h"
#include "cellularmgr_utils.h"
#include "syscfg/syscfg.h"
#include <sysevent/sysevent.h>
#if RBUS_BUILD_FLAG_ENABLE
#include "cellularmgr_rbus_dml.h"
#endif
#define SE_IP_ADDR      "127.0.0.1"
#define SE_PROG_NAME    "CellularManager"

int sysevent_fd = -1;
token_t sysevent_token;

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        BackEndManagerCreate
            (
            );

    description:

        This function constructs datamodel object and return handle.

    argument:

    return:     newly created qos object.

**********************************************************************/

ANSC_HANDLE
BackEndManagerCreate
    (
        VOID
    )
{
    ANSC_STATUS                returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT    pMyObject    = (PBACKEND_MANAGER_OBJECT)NULL;

    /*
        * We create object by first allocating memory for holding the variables and member functions.
        */
    pMyObject = (PBACKEND_MANAGER_OBJECT) AnscAllocateMemory(sizeof(BACKEND_MANAGER_OBJECT));

    if (!pMyObject)
    {
        return  (ANSC_HANDLE)NULL;
    }

    /*
     * Initialize the common variables and functions for a container object.
     */
    pMyObject->Oid               = DATAMODEL_BASE_OID;
    pMyObject->Create            = BackEndManagerCreate;
    pMyObject->Remove            = BackEndManagerRemove;
    pMyObject->Initialize        = BackEndManagerInitialize;

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        BackEndManagerRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function remove manager object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
BackEndManagerRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT         pMyObject    = (PBACKEND_MANAGER_OBJECT)hThisObject;

    if (pMyObject == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
#if RBUS_BUILD_FLAG_ENABLE
    if(cellularmgr_Unload() != RBUS_ERROR_SUCCESS)
    {
         CcspTraceError(("%s %d - Rbus Term failed !\n", __FUNCTION__, __LINE__ ));
    }
#endif
    /* * Remove Cellular */
    if( NULL != pMyObject->hCellular )
    {
        AnscFreeMemory((ANSC_HANDLE)pMyObject->hCellular);
        pMyObject->hCellular = NULL;
    }
    //close sysevent fd
    if(sysevent_fd >= 0)
    {
        sysevent_close(sysevent_fd, sysevent_token);
        sysevent_fd = -1;
    }

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        BackEndManagerInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate manager object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
BackEndManagerInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{

    ANSC_STATUS              returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT  pMyObject    = (PBACKEND_MANAGER_OBJECT)hThisObject;

    if (pMyObject == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Initialize Syscfg */
    if ( syscfg_init() != 0 )
    {
        AnscTraceError(("%s:%d:: syscfg_init failed \n", __FUNCTION__, __LINE__));
    }

    sysevent_fd =  sysevent_open(SE_IP_ADDR, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, SE_PROG_NAME, &sysevent_token);
    if (sysevent_fd < 0)
    {
        CcspTraceError(("sysevent_open failed\n"));
    } else {
        CcspTraceInfo(("sysevent_open success\n"));
    }

    pMyObject->hCellular    = (ANSC_HANDLE)CellularMgr_CellularCreate();
    if (pMyObject->hCellular == NULL)
    {
        AnscTraceError(("%s:%d:: CellularMgr_CellularCreate Failed \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    AnscTraceInfo((" CellularMgr_CellularCreate done!\n"));
	
    CcspTraceInfo(("Initializing WebConfig Framework!\n"));
    webConfigFrameworkInit();
    CcspTraceInfo(("Initializing WebConfig Framework done!\n"));
    
    //Starts the Rbus Initialize
#if RBUS_BUILD_FLAG_ENABLE
    if(cellularmgr_Init() != RBUS_ERROR_SUCCESS)
    {
         CcspTraceError(("%s %d - Rbus Init failed !\n", __FUNCTION__, __LINE__ ));
    }
#endif
    return ANSC_STATUS_SUCCESS;
}
