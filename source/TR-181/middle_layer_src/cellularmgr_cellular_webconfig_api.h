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

#ifndef  _CELLULARMGR_CELLULAR_WEBCONFIG_API_H
#define  _CELLULARMGR_CELLULAR_WEBCONFIG_API_H

#include "webconfig_framework.h"
#include "cellularmgr_cellular_param.h"

#define SUBDOC_COUNT 1
/*
#define STR_SIZE 64
#define CELL_CACHE_SIZE 64
#define CELL_CACHE_SIZE1 64
#define CELL_CACHE_SIZE2 64

typedef struct
{
    char	MnoName[STR_SIZE];
    bool   	Enable;
    char  	Iccid[STR_SIZE];     
} mnoTable;

typedef struct
{
    bool   Enable;
    bool   RoamingEnable;
} interfaceTable;

typedef struct
{
    char 	MnoName[STR_SIZE];
    bool   	Enable;
    bool   	Roaming;
    char 	Apn[STR_SIZE];
    char 	ApnAuthentication[STR_SIZE];   
    char 	IpAddressFamily[STR_SIZE];
} accesspointTable;

typedef struct {
    int  		Tablecount;
    int  		Tablecount1;
    int  		Tablecount2;
    bool        	CellularModemEnable;
    mnoTable	 	MnoTableList[CELL_CACHE_SIZE];
    interfaceTable	InterfaceTableList[CELL_CACHE_SIZE1];
    accesspointTable	AccessPointTableList[CELL_CACHE_SIZE2];
} cell_cache;

cell_cache CELL_Data_Cache;
cell_cache CELL_tmp_bck;
*/

uint32_t getBlobVersion(char* subdoc);
int setBlobVersion(char* subdoc,uint32_t version);
void webConfigFrameworkInit() ;
pErr Process_Cellularmgr_WebConfigRequest(void *Data);
void freeResources_CELL(void *arg);

#endif
