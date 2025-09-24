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
#include <errno.h>
#include <string.h>
#include <msgpack.h>
#include <stdarg.h>
//#include "webcfg_log.h"
#include "cellularmgr_cellular_helpers.h"
#include "cellularmgr_cellular_param.h"
#include "ccsp_trace.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
enum {
    OK                       = HELPERS_OK,
    OUT_OF_MEMORY            = HELPERS_OUT_OF_MEMORY,
    INVALID_FIRST_ELEMENT    = HELPERS_INVALID_FIRST_ELEMENT,
    INVALID_OBJECT,
    INVALID_VERSION,
};
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/*
int process_cellparams( celldoc_t *e, msgpack_object_map *map );
int process_mnoparams( mnoMapping_t *e, msgpack_object_map *map );
int process_interfaceparams( interfaceMapping_t *e, msgpack_object_map *map );
int process_accessparams( accessMapping_t *e, msgpack_object_map *map );
*/
int process_cellularparams( cellularparam_t *e, msgpack_object_map *map );
int process_celldoc( celldoc_t *cd, int num, ...); 
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See celldoc.h for details. */
celldoc_t* celldoc_convert( const void *buf, size_t len )
{
	return comp_helper_convert( buf, len, sizeof(celldoc_t), "cellularconfig", 
                            MSGPACK_OBJECT_MAP, true,
                           (process_fn_t) process_celldoc,
                           (destroy_fn_t) celldoc_destroy );
}
/* See celldoc.h for details. */
void celldoc_destroy( celldoc_t *cd )
{
	//size_t i;

	if( NULL != cd )
	{
/*
		if( NULL != cd->table_param )
		{
			if( NULL != cd->table_param->entries )
			{
				for( i = 0; i < cd->table_param->entries_count; i++ )
				{
					if( NULL != cd->table_param->entries[i].mno_name )
					{
						free(cd->table_param->entries[i].mno_name);
					}
					if( NULL != cd->table_param->entries[i].mno_iccid )
					{
						free(cd->table_param->entries[i].mno_iccid);
					}
				}
				free(cd->table_param->entries);
			}
			free(cd->table_param);
		}
		if( NULL != cd->table_param2 )
		{
			if( NULL != cd->table_param2->entries )
			{
				for( i = 0; i < cd->table_param2->entries_count; i++ )
				{
					if( NULL != cd->table_param2->entries[i].access_mno_name )
					{
						free(cd->table_param2->entries[i].access_mno_name);
					}
					if( NULL != cd->table_param2->entries[i].access_apn )
					{
						free(cd->table_param2->entries[i].access_apn);
					}
					if( NULL != cd->table_param2->entries[i].access_apnauthentication )
					{
						free(cd->table_param2->entries[i].access_apnauthentication);
					}
					if( NULL != cd->table_param2->entries[i].access_ipaddressfamily )
					{
						free(cd->table_param2->entries[i].access_ipaddressfamily);
					}
				}
				free(cd->table_param2->entries);
			}
			free(cd->table_param2);
		}
*/		if( NULL != cd->param )
		{
			free( cd->param );
		}
		
		if( NULL != cd->subdoc_name )
		{
			free( cd->subdoc_name );
		}
		free( cd );
	}
}
/* See celldoc.h for details. */
const char* celldoc_strerror( int errnum )
{
    struct error_map {
        int v;
        const char *txt;
    } map[] = {
        { .v = OK,                               .txt = "No errors." },
        { .v = OUT_OF_MEMORY,                    .txt = "Out of memory." },
        { .v = INVALID_FIRST_ELEMENT,            .txt = "Invalid first element." },
        { .v = INVALID_VERSION,                 .txt = "Invalid 'version' value." },
        { .v = INVALID_OBJECT,                .txt = "Invalid 'value' array." },
        { .v = 0, .txt = NULL }
    };
    int i = 0;
    while( (map[i].v != errnum) && (NULL != map[i].txt) ) { i++; }
    if( NULL == map[i].txt )
    {
	//WebcfgDebug("----celldoc_strerror----\n");
        CcspTraceWarning(("----celldoc_strerror----\n"));
        return "Unknown error.";
    }
    return map[i].txt;
}
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  Convert the msgpack map into the dnsMapping_t structure.
 *
 *  @param e    the entry pointer
 *  @param map  the msgpack map pointer
 *
 *  @return 0 on success, error otherwise
 */
/**
int process_mnoparams( mnoMapping_t *e, msgpack_object_map *map )
{
    int left = map->size;
    uint8_t objects_left = 0x03;
    msgpack_object_kv *p;
    p = map->ptr;
    while( (0 < objects_left) && (0 < left--) )
    {
        if( MSGPACK_OBJECT_STR == p->key.type )
        {
              if(MSGPACK_OBJECT_STR == p->val.type)
              {
                 if( 0 == match(p, "MnoName") )
                 {
                     e->mno_name = strndup( p->val.via.str.ptr, p->val.via.str.size );
                     objects_left &= ~(1 << 0);
                 }
		  if( 0 == match(p, "Iccid") )
                 {
                     e->mno_iccid = strndup( p->val.via.str.ptr, p->val.via.str.size );
                     objects_left &= ~(1 << 1);
                 }
		
              }
              if( MSGPACK_OBJECT_BOOLEAN == p->val.type )
              {
                 if( 0 == match(p, "Enable") )
                 {
                     e->mno_enable = p->val.via.boolean;
                     objects_left &= ~(1 << 2);
                 }
              }
             
        }
           p++;
    }
        
    
    if( 1 & objects_left ) {
    } else {
        errno = OK;
    }
   
    return (0 == objects_left) ? 0 : -1;
}

int process_interfaceparams( interfaceMapping_t *e, msgpack_object_map *map )
{
    int left = map->size;
    uint8_t objects_left = 0x02;
    msgpack_object_kv *p;
    p = map->ptr;
    while( (0 < objects_left) && (0 < left--) )
    {
        if( MSGPACK_OBJECT_STR == p->key.type )
        {
              if( MSGPACK_OBJECT_BOOLEAN == p->val.type )
              {
                 if( 0 == match(p, "Enable") )
                 {
                     e->int_enable = p->val.via.boolean;
                     objects_left &= ~(1 << 0);
                 }
		  if( 0 == match(p, "RoamingEnable") )
                 {
                     e->int_roaming_enable = p->val.via.boolean;
                     objects_left &= ~(1 << 1);
                 }
		
              }
             
        }
           p++;
    }
        
    
    if( 1 & objects_left ) {
    } else {
        errno = OK;
    }
   
    return (0 == objects_left) ? 0 : -1;
}
int process_accessparams( accessMapping_t *e, msgpack_object_map *map )
{
    int left = map->size;
    uint8_t objects_left = 0x06;
    msgpack_object_kv *p;
    p = map->ptr;
    while( (0 < objects_left) && (0 < left--) )
    {
        if( MSGPACK_OBJECT_STR == p->key.type )
        {
              if(MSGPACK_OBJECT_STR == p->val.type)
              {
                 if( 0 == match(p, "MnoName") )
                 {
                     e->access_mno_name = strndup( p->val.via.str.ptr, p->val.via.str.size );
                     objects_left &= ~(1 << 0);
                 }
                 if( 0 == match(p, "Apn") )
                 {
                     e->access_apn = strndup( p->val.via.str.ptr, p->val.via.str.size );
                     objects_left &= ~(1 << 3);
                 }
                 if( 0 == match(p, "ApnAuthentication") )
                 {
                     e->access_apnauthentication = strndup( p->val.via.str.ptr, p->val.via.str.size );
                     objects_left &= ~(1 << 2);
                 }
                 if( 0 == match(p, "IpAddressFamily") )
                 {
                     e->access_ipaddressfamily = strndup( p->val.via.str.ptr, p->val.via.str.size );
                     objects_left &= ~(1 << 1);
                 }
		
              }
              if( MSGPACK_OBJECT_BOOLEAN == p->val.type )
              {
                 if( 0 == match(p, "Enable") )
                 {
                     e->access_enable = p->val.via.boolean;
                     objects_left &= ~(1 << 5);
                 }
		  if( 0 == match(p, "RoamingEnable") )
                 {
                     e->access_roaming_enable = p->val.via.boolean;
                     objects_left &= ~(1 << 4);
                 }
		
              }
             
        }
           p++;
    }
        
    
    if( 1 & objects_left ) {
    } else {
        errno = OK;
    }
   
    return (0 == objects_left) ? 0 : -1;
}
*/
/**
 *  Convert the msgpack map into the doc_t structure.
 *
 *  @param e    the entry pointer
 *  @param map  the msgpack map pointer
 *
 *  @return 0 on success, error otherwise
 */
/**
int process_cellparams( celldoc_t *e, msgpack_object_map *map )
{
    int left = map->size;
    size_t i =0;
    size_t j =0;
    size_t k =0;
    uint8_t objects_left = 0x05;
    msgpack_object_kv *p;
    p = map->ptr;
    while( (0 < objects_left) && (0 < left--) )
    {
        if( MSGPACK_OBJECT_STR == p->key.type )
        {
              if( MSGPACK_OBJECT_BOOLEAN == p->val.type )
              {
                 if( 0 == match(p, "CellularModemEnable") )
                 {
                     e->cellular_modem_enable = p->val.via.boolean;
                     objects_left &= ~(1 << 0);
                 }
              }
              else if( MSGPACK_OBJECT_ARRAY == p->val.type )
              {
                 if( 0 == match(p, "MnoProfileTable") )
                 {
                      e->table_param = (mnoTable_t *) malloc( sizeof(mnoTable_t) );
                      if( NULL == e->table_param )
                      {
	                  //WebcfgDebug("table_param malloc failed\n");
                          CcspTraceError(("table_param malloc failed\n"));
                          return -1;
                      }
                      memset( e->table_param, 0, sizeof(mnoTable_t));

                      e->table_param->entries_count = p->val.via.array.size;

                      e->table_param->entries = (mnoMapping_t *) malloc( sizeof(mnoMapping_t) * e->table_param->entries_count);

                      if( NULL == e->table_param->entries )
                      {
	                  //WebcfgDebug("table_param malloc failed\n");
                          CcspTraceError(("table_param malloc failed\n"));
                          e->table_param->entries_count = 0;
                          return -1;
                      }
                      memset( e->table_param->entries, 0, sizeof(mnoMapping_t) * e->table_param->entries_count);

                      for( i = 0; i < e->table_param->entries_count; i++ )
                      {
                          if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[i].type )
                          {
                              CcspTraceError(("invalid OBJECT\n"));
                              errno = INVALID_OBJECT;
                              return -1;
                          }

                          if( 0 != process_mnoparams(&e->table_param->entries[i], &p->val.via.array.ptr[i].via.map) )
                          {
                              CcspTraceError(("process_mnoparams failed\n"));
                              return -1;
                          }
           
                      }
                      objects_left &= ~(1 << 4);
                }
                 else if( 0 == match(p, "InterfaceTable") )
                 {
                      e->table_param1 = (interfaceTable_t *) malloc( sizeof(interfaceTable_t) );
                      if( NULL == e->table_param1 )
                      {
	                  //WebcfgDebug("table_param1 malloc failed\n");
                          CcspTraceError(("table_param1 malloc failed\n"));
                          return -1;
                      }
                      memset( e->table_param1, 0, sizeof(interfaceTable_t));

                      e->table_param1->entries_count = p->val.via.array.size;

                      e->table_param1->entries = (interfaceMapping_t *) malloc( sizeof(interfaceMapping_t) * e->table_param1->entries_count);

                      if( NULL == e->table_param1->entries )
                      {
	                  //WebcfgDebug("table_param1 malloc failed\n");
                          CcspTraceError(("table_param1 malloc failed\n"));
                          e->table_param1->entries_count = 0;
                          return -1;
                      }
                      memset( e->table_param1->entries, 0, sizeof(interfaceMapping_t) * e->table_param1->entries_count);

                      for( j = 0; j < e->table_param1->entries_count; j++ )
                      {
                          if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[j].type )
                          {
                              CcspTraceError(("invalid OBJECT\n"));
                              errno = INVALID_OBJECT;
                              return -1;
                          }

                          if( 0 != process_interfaceparams(&e->table_param1->entries[j], &p->val.via.array.ptr[j].via.map) )
                          {
		              CcspTraceError(("process_interfaceparams failed\n"));
                              return -1;
                          }
           
                      }
                      objects_left &= ~(1 << 3);
                }
                else if( 0 == match(p, "AccessPointProfileTable") )
                 {
                      e->table_param2 = (accesspointTable_t *) malloc( sizeof(accesspointTable_t) );
                      if( NULL == e->table_param2 )
                      {
	                  //WebcfgDebug("table_param2 malloc failed\n");
                          CcspTraceError(("table_param2 malloc failed\n"));
                          return -1;
                      }
                      memset( e->table_param2, 0, sizeof(accesspointTable_t));

                      e->table_param2->entries_count = p->val.via.array.size;

                      e->table_param2->entries = (accessMapping_t *) malloc( sizeof(accessMapping_t) * e->table_param2->entries_count);

                      if( NULL == e->table_param2->entries )
                      {
	                  //WebcfgDebug("table_param2 malloc failed\n");
                          CcspTraceError(("table_param2 malloc failed\n"));
                          e->table_param2->entries_count = 0;
                          return -1;
                      }
                      memset( e->table_param2->entries, 0, sizeof(accessMapping_t) * e->table_param2->entries_count);

                      for( k = 0; k < e->table_param2->entries_count; k++ )
                      {
                          if( MSGPACK_OBJECT_MAP != p->val.via.array.ptr[k].type )
                          {
                              CcspTraceError(("invalid OBJECT\n"));
                              errno = INVALID_OBJECT;
                              return -1;
                          }

                          if( 0 != process_accessparams(&e->table_param2->entries[k], &p->val.via.array.ptr[k].via.map) )
                          {
		              CcspTraceError(("process_accessparams failed\n"));
                              return -1;
                          }
           
                      }
                      objects_left &= ~(1 << 2);
                }
                }
        }
           p++;
    }
        
    
    if( 1 & objects_left ) {
    } else {
        errno = OK;
    }
   
    return (0 == objects_left) ? 0 : -1;
}
*/
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  Convert the msgpack map into the doc_t structure.
 *
 *  @param e    the entry pointer
 *  @param map  the msgpack map pointer
 *
 *  @return 0 on success, error otherwise
 */
int process_cellularparams( cellularparam_t *e, msgpack_object_map *map )
{
    int left = map->size;
    uint8_t objects_left = 0x01;
    msgpack_object_kv *p;
    p = map->ptr;
    while( (0 < objects_left) && (0 < left--) )
    {
        if( MSGPACK_OBJECT_STR == p->key.type )
        {
              if( MSGPACK_OBJECT_BOOLEAN == p->val.type )
              {
                if( 0 == match(p, "CellularModemEnable") )
                {
                    e->cellular_modem_enable = p->val.via.boolean;
                    objects_left &= ~(1 << 0);
                }
            }
        }
           p++;
    }
        
    
    if( 1 & objects_left ) {
    } else {
        errno = OK;
    }
   
    return (0 == objects_left) ? 0 : -1;
}

int process_celldoc( celldoc_t *cd,int num, ... )
{
//To access the variable arguments use va_list 
	va_list valist;
	va_start(valist, num);//start of variable argument loop

	msgpack_object *obj = va_arg(valist, msgpack_object *);//each usage of va_arg fn argument iterates by one time
	msgpack_object_map *mapobj = &obj->via.map;

	msgpack_object *obj1 = va_arg(valist, msgpack_object *);
	cd->subdoc_name = strndup( obj1->via.str.ptr, obj1->via.str.size );

	msgpack_object *obj2 = va_arg(valist, msgpack_object *);
	cd->version = (uint32_t) obj2->via.u64;

	msgpack_object *obj3 = va_arg(valist, msgpack_object *);
	cd->transaction_id = (uint16_t) obj3->via.u64;

	va_end(valist);//End of variable argument loop
/*
	if( 0 != process_cellparams(cd, mapobj) )
	{
		//WebcfgDebug("process_cellparams failed\n");
                CcspTraceError(("process_cellparams failed\n"));
		return -1;
	}
*/
        cd->param = (cellularparam_t *) malloc( sizeof(cellularparam_t) );
        if( NULL == cd->param )
        {
	    CcspTraceWarning(("Entries count malloc failed\n"));
            return -1;
        }
        memset( cd->param, 0, sizeof(cellularparam_t));


	if( 0 != process_cellularparams(cd->param, mapobj) )
	{
		CcspTraceWarning(("process_cellularparams failed\n"));
		return -1;
	}
  
    return 0;
}
