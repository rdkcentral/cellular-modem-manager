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

#include <syscfg/syscfg.h>
#include "cellularmgr_cellular_webconfig_api.h"
#include "cellularmgr_cellular_apis.h"
#include "cellularmgr_plugin_main_apis.h"
#include "cellularmgr_cellular_internal.h"

#include "webconfig_framework.h"
#include "ccsp_trace.h"

extern PBACKEND_MANAGER_OBJECT               g_pBEManager;

/* API to get the subdoc version */
uint32_t getBlobVersion(char* subdoc)
{

        char subdoc_ver[64] = {0}, buf[72] = {0};
        snprintf(buf,sizeof(buf),"%s_version",subdoc);
        if ( syscfg_get( NULL, buf, subdoc_ver, sizeof(subdoc_ver)) == 0 )
        {
                int version = atoi(subdoc_ver);
                //  uint32_t version = strtoul(subdoc_ver, NULL, 10) ; 
                return (uint32_t)version;
        }
        return 0;
}

/* API to update the subdoc version */
int setBlobVersion(char* subdoc,uint32_t version)
{

        char subdoc_ver[64] = {0}, buf[72] = {0};
        snprintf(subdoc_ver,sizeof(subdoc_ver),"%u",version);
        snprintf(buf,sizeof(buf),"%s_version",subdoc);
        if(syscfg_set_commit(NULL,buf,subdoc_ver) != 0)
        {
                CcspTraceError(("syscfg_set failed\n"));
                return -1;
        }
        return 0;
}

/* API to register all the supported subdocs , versionGet and versionSet are callback functions to get and set the subdoc versions in db */
void webConfigFrameworkInit()
{
        char *sub_docs[SUBDOC_COUNT+1]= {"cellularconfig",(char *) 0 };

        blobRegInfo *blobData;

        blobData = (blobRegInfo*) malloc(SUBDOC_COUNT * sizeof(blobRegInfo));

        int i;
        memset(blobData, 0, SUBDOC_COUNT * sizeof(blobRegInfo));

        blobRegInfo *blobDataPointer = blobData;


        for (i=0 ; i < SUBDOC_COUNT ; i++ )
        {
                strncpy( blobDataPointer->subdoc_name, sub_docs[i], sizeof(blobDataPointer->subdoc_name)-1);

                blobDataPointer++;
        }

        blobDataPointer = blobData ;

        getVersion versionGet = getBlobVersion;

        setVersion versionSet = setBlobVersion;

        register_sub_docs(blobData,SUBDOC_COUNT,versionGet,versionSet);

}

/* Read blob entries */
int set_cell_conf( celldoc_t* cd )
{
        CcspTraceWarning(("%s: set_cell_conf called \n", __FUNCTION__));
        PCELLULARMGR_CELLULAR_DATA  pMyObject      =  (PCELLULARMGR_CELLULAR_DATA) g_pBEManager->hCellular;
        PCELLULAR_DML_INFO          pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

        if( pstDmlCellular->X_RDK_Enable != cd->param->cellular_modem_enable )
        {
          if( RETURN_OK == CellularMgr_SetModemEnable( cd->param->cellular_modem_enable ) )
          {
                pstDmlCellular->X_RDK_Enable = cd->param->cellular_modem_enable;
                CcspTraceInfo(("%s: set_cell_confg doc value : %d and X_RDK_Enable: %d\n",__FUNCTION__,cd->param->cellular_modem_enable,pstDmlCellular->X_RDK_Enable));              
          }
          else
          {
                CcspTraceError(("%s: set_cell_confg failed while applying\n", __FUNCTION__));
                return BLOB_EXEC_FAILURE;
          }
        }
        else
        {
          CcspTraceInfo(("%s: doc value: %d and X_RDK_Enable: %d are same, no update needed\n",__FUNCTION__,cd->param->cellular_modem_enable,pstDmlCellular->X_RDK_Enable));
        }
        return 0;
}

/* CallBack API to execute CELLULAR Blob request */
pErr Process_Cellularmgr_WebConfigRequest(void *Data)
{
        CcspTraceInfo(("Entering %s \n",__FUNCTION__)); 
        pErr execRetVal = NULL;

        execRetVal = (pErr) malloc (sizeof(Err));
        if (execRetVal == NULL )
        {
	    CcspTraceError(("%s : malloc failed\n",__FUNCTION__));
            return execRetVal;
        }

        memset(execRetVal,0,sizeof(Err));

        execRetVal->ErrorCode = BLOB_EXEC_SUCCESS;

        celldoc_t *cd = (celldoc_t *) Data ;

        CcspTraceInfo(("%s : cellular configurartion recieved\n",__FUNCTION__));
        int ret1  = set_cell_conf( cd );
        if ( 0 != ret1 )
        {
            if ( BLOB_EXEC_FAILURE == ret1 )
            {
                execRetVal->ErrorCode = BLOB_EXEC_FAILURE;
                strncpy(execRetVal->ErrorMsg,"BLOB_EXEC_FAILURE while applying cellular configuration\n",sizeof(execRetVal->ErrorMsg)-1);

            }
            return execRetVal;
        }

    return execRetVal;
}

void freeResources_CELL(void *arg)
{
	CcspTraceInfo(("Entering %s \n",__FUNCTION__)); 
	execData *blob_exec_data  = (execData*) arg;
  
	celldoc_t *cd = (celldoc_t *) blob_exec_data->user_data ;
	if ( cd != NULL )
	{
		celldoc_destroy( cd );
		cd = NULL;
    	}
    	if ( blob_exec_data != NULL )
    	{
        	free(blob_exec_data);
        	blob_exec_data = NULL ;
    	}
        CcspTraceInfo(("Exiting %s \n",__FUNCTION__));
}
