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
#include "poam_irepfo_interface.h"
#include "sys_definitions.h"

extern void * g_pDslhDmlAgent;

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        CellularMgr_CellularCreate
            (
            );

    description:

        This function constructs cosa nat object and return handle.

    argument:  

    return:     newly created nat object.

**********************************************************************/

ANSC_HANDLE CellularMgr_CellularCreate ( VOID )
{
    ANSC_STATUS                 returnStatus  =  ANSC_STATUS_SUCCESS;
	PCELLULARMGR_CELLULAR_DATA  pMyObject     =  (PCELLULARMGR_CELLULAR_DATA) NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
    */
    pMyObject = (PCELLULARMGR_CELLULAR_DATA)AnscAllocateMemory(sizeof(CELLULARMGR_CELLULAR_DATA));
    if ( pMyObject == NULL )
    {
        return  (ANSC_HANDLE)NULL;
    }
    /*
     * Initialize the common variables and functions for a container object.
    */

    AnscZeroMemory(pMyObject, sizeof(CELLULARMGR_CELLULAR_DATA));

    pMyObject->Oid               = CELLULARMGR_CELLULAR_DATA_OID;
    pMyObject->Create            = CellularMgr_CellularCreate;
    pMyObject->Remove            = CellularMgr_CellularRemove;
    pMyObject->Initialize        = CellularMgr_CellularInitialize;

    pMyObject->Initialize((ANSC_HANDLE)pMyObject);

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CellularMgr_CellularInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa nat object and return handle.

    argument:    ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS CellularMgr_CellularInitialize ( ANSC_HANDLE hThisObject )
{
    ANSC_STATUS                 returnStatus  = ANSC_STATUS_SUCCESS;
    PCELLULARMGR_CELLULAR_DATA  pMyObject     =  (PCELLULARMGR_CELLULAR_DATA) hThisObject;

    if (pMyObject == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Initiation all functions */
    DmlCellularInitialize(pMyObject);

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        CellularMgr_CellularRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa nat object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS CellularMgr_CellularRemove ( ANSC_HANDLE hThisObject )
{
    ANSC_STATUS                 returnStatus  = ANSC_STATUS_SUCCESS;
    PCELLULARMGR_CELLULAR_DATA  pMyObject     =  (PCELLULARMGR_CELLULAR_DATA) hThisObject;
    PCELLULAR_DML_INFO          pstDmlCellular =  NULL;

    /* Remove necessary resource */
    if (pMyObject != NULL)
    {
        AnscFreeMemory(pMyObject);
        pMyObject = NULL;
    }

    pstDmlCellular = pMyObject->pstDmlCellular;
    if (pstDmlCellular != NULL)
    {
        if( pstDmlCellular->pstInterfaceInfo != NULL ) 
        {
            AnscFreeMemory(pstDmlCellular->pstInterfaceInfo);
            pstDmlCellular->pstInterfaceInfo = NULL;
        }

        if( pstDmlCellular->pstAPInfo != NULL ) 
        {
            AnscFreeMemory(pstDmlCellular->pstAPInfo);
            pstDmlCellular->pstAPInfo = NULL;
        }
        AnscFreeMemory(pstDmlCellular);
        pstDmlCellular = NULL;
    }

    if (pMyObject != NULL)
    {
        AnscFreeMemory(pMyObject);
        pMyObject = NULL;
    }

    return returnStatus;
}


