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

#ifndef  _CELLULAR_INTERNAL_H
#define  _CELLULAR_INTERNAL_H

#include "cellularmgr_cellular_apis.h"
#include "cellularmgr_plugin_main_apis.h"

/*
*  This struct is for cellular DML object.
*/
#define  CELLULARMGR_CELLULAR_DATA_CLASS_CONTENT                                    \
    /* duplication of the base object class content */                              \
    BASE_CONTENT                                                                    \
    PCELLULAR_DML_INFO                     pstDmlCellular;                          

typedef  struct
_CELLULARMGR_CELLULAR_DATA                                              
{
    CELLULARMGR_CELLULAR_DATA_CLASS_CONTENT
}
CELLULARMGR_CELLULAR_DATA,  *PCELLULARMGR_CELLULAR_DATA;

/*
    Function declaration 
*/ 

ANSC_HANDLE
CellularMgr_CellularCreate
    (
        VOID
    );

ANSC_STATUS
CellularMgr_CellularInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
CellularMgr_CellularRemove
    (
        ANSC_HANDLE                 hThisObject
    );

#endif


