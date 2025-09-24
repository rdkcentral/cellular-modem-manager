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

#ifndef __CELLULARMGR_CELLULAR_PARAM_H__
#define __CELLULARMGR_CELLULAR_PARAM_H__
#include <stdint.h>
#include <stdlib.h>
#include <msgpack.h>

typedef struct
{
    bool      cellular_modem_enable;       
} cellularparam_t;

typedef struct {
    cellularparam_t  *param;       
    char *       subdoc_name;
    uint32_t     version;
    uint16_t     transaction_id;
} celldoc_t;

/*
typedef struct
{
    char * mno_name;
    bool   mno_enable;
    char * mno_iccid;     
} mnoMapping_t;

typedef struct
{
    bool   int_enable;
    bool   int_roaming_enable;
} interfaceMapping_t;

typedef struct
{
    char * access_mno_name;
    bool   access_enable;
    bool   access_roaming_enable;
    char * access_apn;
    char * access_apnauthentication;   
    char * access_ipaddressfamily;
} accessMapping_t;

typedef struct
{
    accessMapping_t * entries;
    size_t    entries_count;	 
} accesspointTable_t;

typedef struct
{
    interfaceMapping_t * entries;
    size_t    entries_count;	 
} interfaceTable_t;


typedef struct
{
    mnoMapping_t * entries;
    size_t    entries_count;	   
} mnoTable_t;


typedef struct {
    bool          cellular_modem_enable;
    char *        subdoc_name;
    uint32_t      version;
    uint16_t      transaction_id;
    mnoTable_t * table_param;
    interfaceTable_t * table_param1;
    accesspointTable_t * table_param2;
} celldoc_t;
*/
/**
 *  This function converts a msgpack buffer into an celldoc_t structure
 *  if possible.
 *
 *  @param buf the buffer to convert
 *  @param len the length of the buffer in bytes
 *
 *  @return NULL on error, success otherwise
 */
celldoc_t* celldoc_convert( const void *buf, size_t len );
/**
 *  This function destroys an celldoc_t object.
 *
 *  @param e the celldoc to destroy
 */
void celldoc_destroy( celldoc_t *d );
/**
 *  This function returns a general reason why the conversion failed.
 *
 *  @param errnum the errno value to inspect
 *
 *  @return the constant string (do not alter or free) describing the error
 */
const char* celldoc_strerror( int errnum );
#endif
