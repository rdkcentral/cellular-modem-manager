/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Arcadyan Technology Corporation.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <glib.h>
#include <gio/gio.h>
#include <time.h>
#include <stdbool.h>

#include "libmm-glib.h"
#include "cellular_hal_mm_apis.h"
#include "mm-unlock-retries.h"

#define MODEM_PATH_PREFIX   "/org/freedesktop/ModemManager1/Modem/"
#define SIM_PATH_PREFIX   "/org/freedesktop/ModemManager1/SIM/"
#define BEARER_PATH_PREFIX   "/org/freedesktop/ModemManager1/Bearer/"
#define WAN_INTERFACE_NAME "wwan0"
#define WAN_DEVICE_NAME "wan device"

typedef struct {
    MMObject *object;
    MMModem *modem;
    MMModem3gpp *modem_3gpp;
    MMSim *sim;
    MMModemSignal *modem_signal;
    MMBearer *bearer;
    MMModemLocation *modem_location;
    MMSms *sms;
} Context;

GMutex  manager_mutex;
GDBusConnection *connection = NULL;
MMManager *manager = NULL;
ModemState mmState = MODEM_STATE_UNKNOWN;
gulong state_id = 0;
GHashTable *hashTable;
DEVICE_INFO deviceInfo;

CellularNetworkCBStruct *networkStatusCB = NULL;
CellularDeviceContextCBStruct *deviceStatusCB = NULL;
cellular_device_profile_status_api_callback profileStatusCb = NULL;
cellular_device_slot_status_api_callback slot_status_cb = NULL;
cellular_device_registration_status_callback registration_status_cb = NULL;

static void cellular_hal_mm_callBack_bearer_info (MMModem *modem);
static void send_bearer_info (MMBearer *bearer);
static void context_free (Context *ctx);
static void cellular_hal_mm_send_device_slot_status(cellular_device_slot_status_api_callback device_slot_status_cb);
MMObject *get_modem_context (const gchar *modem_str);
MMSim *get_sim_context (CHAR *modemIndex);

void convertSubnetMask(int prefix, char *value, int size)
{
    unsigned long mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
    snprintf(value, size, "%lu.%lu.%lu.%lu", mask >> 24, (mask >> 16) & 0xFF, (mask >> 8) & 0xFF, mask & 0xFF);
    return;
}

INT get_manager_context (GDBusConnection *connection)
{
    gchar *name_owner;
    GError *error = NULL;

    manager = mm_manager_new_sync (connection,
                                   G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START,
                                   NULL,
                                   &error);

    if ( manager )
    {
        name_owner = g_dbus_object_manager_client_get_name_owner (G_DBUS_OBJECT_MANAGER_CLIENT (manager));
        if (!name_owner) 
        {
            CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't find the name owner\n", __FUNCTION__);
            g_clear_object(&manager);
            return RETURN_ERROR;
        }
        g_free (name_owner);
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't find manager: %s\n",
                                __FUNCTION__, error ? error->message : "unknown error");
        if(error)
            g_error_free (error);
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

static gchar *
get_dBus_path (const gchar *path_or_index,
               const gchar *path_prefix)
{
    if (!path_or_index) {
        return NULL;
    }
    gchar *dBus_path = NULL;
    UINT full_path_len;

    if (g_str_has_prefix (path_or_index, path_prefix)) {
        full_path_len = strlen(path_or_index) + 1;
        dBus_path = g_malloc0(full_path_len);
        snprintf(dBus_path, full_path_len, "%s", path_or_index);
    } else if (g_ascii_isdigit (path_or_index[0])) {
        full_path_len = strlen(path_prefix) + strlen(path_or_index) + 1;
        dBus_path = g_malloc0(full_path_len);
        snprintf(dBus_path, full_path_len, "%s%s", path_prefix, path_or_index);
    }

    return dBus_path;
}

static MMObject *
lookfor_modem (MMManager   *manager,
               const gchar *modemPath)
{
    GList *list, *modems = NULL;
    MMObject *found = NULL, *object = NULL;
    MMModem  *modem = NULL;

    if( !manager )
        return NULL;
    g_mutex_lock(&manager_mutex);
    modems = g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (manager));
    g_mutex_unlock(&manager_mutex);
    if (!modems) {
        CELLULAR_HAL_DBG_PRINT ("%s - error: ModemManger is not ready\n" ,__FUNCTION__);
        return NULL;
    }

    for (list = modems; list; list = g_list_next (list)) {
        object = MM_OBJECT (list->data);
        modem  = MM_MODEM (mm_object_get_modem (object));

        if (modemPath && g_str_equal (mm_object_get_path (object), modemPath)) {
            found = g_object_ref (object);
            g_clear_object(&modem);
            break;
        }

        g_clear_object(&modem);
    }
    g_list_free_full (modems, g_object_unref);

    if (!found) {
        if (modemPath)
        {    
            CELLULAR_HAL_DBG_PRINT("%s - error: Invalid modem path '%s'\n", __FUNCTION__, modemPath);
        }
    }

    return found;
}

static MMObject *
get_object (void)
{
    GList *list, *modems = NULL;
    MMObject *found = NULL, *object = NULL;
    
    if( !manager )
        return NULL;

    g_mutex_lock(&manager_mutex);
    modems = g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (manager));

    if (!modems) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s - error: ModemManger is not ready\n", __FUNCTION__);
    }
    else
    {
        for (list = modems; list; list = g_list_next (list)) {
            object = MM_OBJECT (list->data);
            if( mm_object_get_path(object) != NULL )
            {
                found = g_object_ref (object);
                break;
            }
        }
        g_list_free_full (modems, g_object_unref);
    }
    g_mutex_unlock(&manager_mutex);
    return found;
}

MMObject *
get_modem_context (const gchar *modemIndex)
{
    MMObject *found;
    gchar *modemPath = NULL;

    if( modemIndex == NULL )
    {
        // Assign modem index to NULL, if multiple modem is unsupported.
        found = get_object();
    }
    else
    {
        modemPath = get_dBus_path(modemIndex, MODEM_PATH_PREFIX);
        found = lookfor_modem (manager, modemPath);
    }

    if(modemPath)
        g_free (modemPath);

    return found;
}

#if MM_CHECK_VERSION(1,18,8)
MMSim *
get_sim_context (CHAR *simIndex)
{
    GList *list, *modems = NULL;
    MMSim *found = NULL, *sim = NULL;
    gchar *sim_path = NULL;
    GError *error = NULL;
    MMObject *object = NULL;
    MMModem *modem = NULL;
    g_autoptr(GPtrArray) sim_slots = NULL;
    INT i;

    sim_path = get_dBus_path(simIndex, SIM_PATH_PREFIX);

    modems = g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (manager));
    if (!modems) {
        CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't find sim at '%s'\n",
                                __FUNCTION__, sim_path);
        if(sim_path)
            g_free (sim_path);
        return NULL;
    }

    for (list = modems; !found && list; list = g_list_next (list)) 
    {
        object = MM_OBJECT (list->data);
        modem = mm_object_get_modem (object);
        if(!sim_path)
        {
            found = mm_modem_get_sim_sync (modem, NULL, &error);
            if (error) 
                g_error_free (error); 
        }
        else
        {
            if (g_str_equal (sim_path, mm_modem_get_sim_path (modem))) 
            {
                found = mm_modem_get_sim_sync (modem, NULL, &error);
                if (error) 
                    g_error_free (error);
            }
            else
            {
                sim_slots = mm_modem_list_sim_slots_sync (modem, NULL, &error);
                if (error) 
                {
                    CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't get SIM slots in modem '%s': '%s'\n",
                                            __FUNCTION__, mm_modem_get_path (modem),error->message);
                    g_error_free (error);
                }
                else
                {
                    for (i = 0; i < sim_slots->len; i++) 
                    {
                        sim = MM_SIM (g_ptr_array_index (sim_slots, i));
                        if (sim && g_str_equal (sim_path, mm_sim_get_path (sim))) 
                            found = g_object_ref (sim);
                    }
                }   
            }
        }
        g_clear_object (&modem);
    }
    g_list_free_full (modems, g_object_unref);
    
    if(sim_path)
        g_free (sim_path);
    
    if (!found) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't find sim\n", __FUNCTION__);
    }
    return found;
}
#else
MMSim *
get_sim_context (CHAR *modemIndex)
{
    MMSim *found = NULL;
    MMObject *object = NULL;
    MMModem *modem = NULL;
    GError *error = NULL;
    
    if( modemIndex )
        object = get_modem_context (modemIndex);
    else
        object = get_object();

    if(!object)
        return NULL;

    modem = mm_object_get_modem (object);
    found = mm_modem_get_sim_sync (modem, NULL, &error);
    if (!found)
    {
        if (error)
        {
            CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't get sim'%s'\n",
                                    __FUNCTION__, error->message);
            g_error_free (error);
        }
    }

    g_clear_object(&modem);
    g_clear_object(&object);
    return found;
}
#endif
static MMBearer *
lookfor_bearer (GList       *bearer_list,
                const gchar *bearer_path)
{
    GList *list;
    MMBearer *bearer = NULL;

    for (list = bearer_list; list; list = g_list_next (list)) {
        bearer = MM_BEARER (list->data);
        if( bearer_path == NULL )
        {
            if( mm_bearer_get_path (bearer) != NULL )
                return g_object_ref (bearer);
        } else {
            if (!g_strcmp0 (mm_bearer_get_path (bearer), bearer_path)) {
                return g_object_ref (bearer);
            }
        }

        return NULL;
    }
}

MMBearer *
get_bearer_context (const gchar *path_or_index)
{
    GList *list, *modems = NULL, *bearers = NULL;
    MMBearer *found = NULL;
    gchar *bearer_path = NULL;
    GError *error = NULL;
    MMObject *object;
    MMModem *modem = NULL;

    bearer_path = get_dBus_path(path_or_index, BEARER_PATH_PREFIX);

    if(!manager)
        return NULL;
    g_mutex_lock (&manager_mutex);
    modems = g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (manager));
    g_mutex_unlock (&manager_mutex);
    if (!modems) {
        CELLULAR_HAL_DBG_PRINT ("%s- error: couldn't find bearer\n", __FUNCTION__);
        if(bearer_path)
            g_free (bearer_path);
        return NULL;
    }

    for (list = modems; !found && list; list = g_list_next (list)) {
        object = MM_OBJECT (list->data);
        modem = mm_object_get_modem (object);

        bearers = mm_modem_list_bearers_sync (modem, NULL, &error);
        if (!bearers) {
            CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't list bearer '%s'\n",
                                    __FUNCTION__, error ? error->message : "unknown error");
            if(error)
                g_error_free (error);
        }
        else
        {
            found = lookfor_bearer (bearers, bearer_path);
            g_list_free_full (bearers, g_object_unref);
        }
        g_clear_object (&modem);
    }
    g_list_free_full (modems, g_object_unref);
    if(bearer_path)
        g_free (bearer_path);

    if (!found) {
        CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't find bearer\n", __FUNCTION__);
        return NULL;
    }

    return found;
}

static void
state_changed (MMModem                  *modem,
               ModemState              old_state,
               ModemState              new_state)
{
    CHAR *modemPath = mm_modem_get_path (modem);
    gulong stateId = 0;
    CELLULAR_HAL_DBG_PRINT ("\t%s: State changed, '%s' --> '%s'\n",
                            modemPath,
                            mm_modem_state_get_string (old_state),
                            mm_modem_state_get_string (new_state));

    if( new_state < old_state )
    {
        if( new_state <= MODEM_STATE_DISABLING )
        {
            stateId = (gulong)GPOINTER_TO_SIZE(g_hash_table_lookup(hashTable, modemPath));
            g_clear_signal_handler(&stateId, modem);
            g_hash_table_remove(hashTable, modemPath);
            
            if( deviceStatusCB )
            {
                deviceStatusCB->device_open_status_cb( deviceInfo.deviceName, deviceInfo.wanInterfaceName, DEVICE_OPEN_STATUS_NOT_READY, CELLULAR_MODEM_SET_OFFLINE );
                deviceStatusCB->device_remove_status_cb( deviceInfo.deviceName, DEVICE_REMOVED );
            }
            if( slot_status_cb )
                slot_status_cb( "unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY );
            if( profileStatusCb )
                profileStatusCb( "unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_NOT_READY );
        }
        
        if( new_state == MODEM_STATE_ENABLED )
        {
            if( deviceStatusCB )
                deviceStatusCB->device_open_status_cb( deviceInfo.deviceName, deviceInfo.wanInterfaceName, DEVICE_OPEN_STATUS_NOT_READY, CELLULAR_MODEM_SET_OFFLINE );
            
            if( slot_status_cb )
                slot_status_cb( "unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY );    
        }

        if( new_state == MODEM_STATE_REGISTERED )
        {
            if( networkStatusCB )
            {
                networkStatusCB->packet_service_status_cb("unknown", CELLULAR_NETWORK_IP_FAMILY_IPV4, DEVICE_NETWORK_STATUS_DISCONNECTED);
                networkStatusCB->packet_service_status_cb("unknown", CELLULAR_NETWORK_IP_FAMILY_IPV6, DEVICE_NETWORK_STATUS_DISCONNECTED);
            }
        }

        if( old_state == MODEM_STATE_CONNECTED )
        {
            CellularIPStruct ipStruct = {0};
            if( networkStatusCB )
                networkStatusCB->device_network_ip_ready_cb(&ipStruct, DEVICE_NETWORK_IP_NOT_READY);
        }
    }

    if( new_state > old_state )
    {
        if( new_state == MODEM_STATE_REGISTERED )
        {
            if( slot_status_cb )
                slot_status_cb( "slot1", "logical_slot1", 1, DEVICE_SLOT_STATUS_READY );
        }
        else if( new_state == MODEM_STATE_CONNECTED )
        {
            if( networkStatusCB != NULL )
            {
                cellular_hal_mm_callBack_bearer_info(modem);
            }
        }
    }
    
}

static void
context_free (Context *ctx)
{
    if (!ctx)
        return;

    if (ctx->modem)
        g_object_unref(ctx->modem);

    if (ctx->modem_3gpp)
        g_object_unref(ctx->modem_3gpp);

    if (ctx->modem_location)
        g_object_unref(ctx->modem_location);

    if (ctx->bearer)
        g_object_unref(ctx->bearer);

    if (ctx->modem_signal)
        g_object_unref(ctx->modem_signal);

    if (ctx->sim)
        g_object_unref(ctx->sim);

    if (ctx->object)
        g_object_unref(ctx->object);

    g_free (ctx);
}

static gpointer gmainloop(gpointer data)
{
    GError *error = NULL;
    INT res;

    do
    {
        connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
        if( connection )
            break;
        else
            sleep(1);
    } while ( 1 );

    do
    {
        res = get_manager_context (connection);
        if( res == RETURN_OK )
            break;
        else
            sleep(1);
    } while ( 1 );

    hashTable = g_hash_table_new(g_str_hash, g_str_equal);
    
    g_mutex_init(&manager_mutex);
    GMainContext* loop_context = (GMainContext*) data;
    GMainLoop* loop = g_main_loop_new (loop_context, FALSE);
    g_main_context_push_thread_default(loop_context);
    g_main_loop_run (loop);
}

void cellular_hal_mm_main_loop_init(void)
{
    g_thread_new("Arc GLib Loop Thread", gmainloop, g_main_context_get_thread_default());
}

/*============================get index================================*/
INT cellular_hal_mm_get_modem_index (CHAR *index)
{
    GError *error = NULL;
    GList *modems = NULL;
    GList *list;
    gchar *modem_path = NULL;
    INT res = RETURN_ERROR;
    MMObject *object = NULL;

    modems = g_dbus_object_manager_get_objects (G_DBUS_OBJECT_MANAGER (manager));
    if (!modems) {
        return RETURN_ERROR;
    }

    for (list = modems; list; list = g_list_next (list))
    {
        object = MM_OBJECT (list->data);
        if( (modem_path = mm_object_get_path (object)) != NULL )
        {
            strncpy(index, modem_path+strlen(MODEM_PATH_PREFIX), IDX_SIZE-1);
            res = RETURN_OK;
            break;
        }
    }

    g_list_free_full (modems, g_object_unref);
    return res;
}

INT cellular_hal_mm_get_sim_index_by_modem (CHAR *modemIndex, CHAR *simIndex)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    CHAR *sim_path;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (modemIndex);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if(ctx->modem)
        {
            if( (sim_path = mm_modem_get_sim_path (ctx->modem)) != NULL )
            {
                strncpy(simIndex, sim_path+strlen(SIM_PATH_PREFIX), IDX_SIZE-1);
                res = RETURN_OK;
            }
        }
    }

    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_bearer_index_by_modem (CHAR *modemIndex, CHAR *bearerIndex)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    const char **bearer_path;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (modemIndex);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if(ctx->modem)
        {
            if( (bearer_path = (const char **)mm_modem_get_bearer_paths (ctx->modem)) != NULL )
            {
                strncpy(bearerIndex, (*bearer_path)+strlen(BEARER_PATH_PREFIX), IDX_SIZE-1);
                res = RETURN_OK;
            }
        }
    }

    context_free(ctx);
    return res;
}

static void register_signal(MMModem *modem)
{
    gulong stateId = 0;
    CHAR *modemPath;
    if(modem)
    {   
        modemPath = mm_modem_get_path (modem);
        stateId = (gulong) GPOINTER_TO_SIZE(g_hash_table_lookup(hashTable, modemPath));
        if( !stateId )
        {
            stateId = g_signal_connect (modem,
                                         "state-changed",
                                         G_CALLBACK (state_changed),
                                         NULL);
            g_hash_table_insert(hashTable, (gpointer) modemPath, GSIZE_TO_POINTER(stateId));
        }
       
    }
    return;
}

static void cellular_hal_mm_get_modem_port (MMModem *modem)
{
    MMModemPortInfo *ports = NULL;
    guint n_ports = 0, i;

    snprintf(deviceInfo.wanInterfaceName, sizeof(deviceInfo.wanInterfaceName), "%s", WAN_INTERFACE_NAME);
    snprintf(deviceInfo.deviceName, sizeof(deviceInfo.deviceName), "%s", WAN_DEVICE_NAME);
    if (modem)
    {
        mm_modem_get_ports(modem, &ports, &n_ports);
        if( n_ports )
        {
            for( i = 0; i < n_ports; i++ )
            {
                if( ports[i].type == MM_MODEM_PORT_TYPE_NET )
                {
                    snprintf(deviceInfo.wanInterfaceName, sizeof(deviceInfo.wanInterfaceName), "%s", ports[i].name);
                    break;
                }
            }
            mm_modem_port_info_array_free (ports, n_ports);
        }
        snprintf(deviceInfo.deviceName, sizeof(deviceInfo.deviceName), "%s", mm_modem_get_primary_port (modem));
    }
}

INT cellular_hal_mm_enable_modem (CHAR *index, BOOLEAN enable, cellular_device_slot_status_api_callback device_slot_status_cb)
{
    Context *ctx;
    INT res = RETURN_ERROR;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if (ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if( ctx->modem )
        {
            register_signal(ctx->modem);
            cellular_hal_mm_send_device_slot_status(device_slot_status_cb);
            if( mmState >= MODEM_STATE_REGISTERED && enable == TRUE )
            {
            	if( slot_status_cb )
                	slot_status_cb( "slot1", "logical_slot1", 1, DEVICE_SLOT_STATUS_READY );
            }
            else
            {
                if( enable == TRUE )
                    mm_modem_enable(ctx->modem, NULL, NULL, NULL);
                else
                    mm_modem_disable(ctx->modem, NULL, NULL, NULL);
            }
            res = RETURN_OK;
        }
    }

    context_free(ctx);
    return res;
}

/*======================== get modem ===================================*/
BOOLEAN cellular_hal_mm_is_modem_ready (CHAR *index)
{
    Context *ctx;
    BOOLEAN res = FALSE;
    ModemState modemState = MODEM_STATE_UNKNOWN;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if(ctx->modem)
        {
            modemState = mm_modem_get_state(ctx->modem);
            if( modemState != MODEM_STATE_DISABLING && modemState >= MODEM_STATE_DISABLED )
                res = TRUE;

            if( mmState == MODEM_STATE_UNKNOWN )
                mmState = modemState;

            cellular_hal_mm_get_modem_port(ctx->modem);
        }
    }
    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_modem_state (CHAR *index, ModemState *State, ModemStateFailedReason *reason)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if(ctx->modem)
        {
            *State = mm_modem_get_state(ctx->modem);
            if( *State == MODEM_STATE_FAILED )
                *reason = mm_modem_get_state_failed_reason(ctx->modem);
            else
                *reason = MODEM_STATE_FAILED_REASON_NONE;
            res = RETURN_OK;
        }
    }

    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_device_management (CHAR *index, CellularDeviceInfoStruct *devMngt)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    const char *value = NULL;
    const char **msisdn = NULL;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);

    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if (ctx->modem)
        {
            if( (value = mm_modem_get_model (ctx->modem)) != NULL )
                strncpy (devMngt->Model, value, sizeof(devMngt->Model)-1);
            if( (value = mm_modem_get_manufacturer (ctx->modem)) != NULL )
                strncpy (devMngt->Vendor, value, sizeof(devMngt->Vendor)-1);
            if( (value = mm_modem_get_revision (ctx->modem)) != NULL )
                strncpy (devMngt->CurrentImageVersion, value, sizeof(devMngt->CurrentImageVersion)-1);
            if( (value = mm_modem_get_primary_port (ctx->modem)) != NULL )
                strncpy (devMngt->ControlInterface, value, sizeof(devMngt->ControlInterface)-1);
            if( (value = mm_modem_get_hardware_revision (ctx->modem)) != NULL )
                strncpy (devMngt->HardwareRevision, value, sizeof(devMngt->HardwareRevision)-1);
            if( (msisdn = (const char **)mm_modem_get_own_numbers (ctx->modem)) != NULL )
                strncpy (devMngt->Msisdn, *msisdn, sizeof(devMngt->Msisdn)-1);
            ctx->modem_3gpp = mm_object_get_modem_3gpp (ctx->object);
            if( ctx->modem_3gpp )
            {
                if( (value = mm_modem_3gpp_get_imei (ctx->modem_3gpp)) != NULL )
                    strncpy (devMngt->Imei, value, sizeof(devMngt->Imei)-1);
            }
            res = RETURN_OK;
        }
    }
    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_device_data (CHAR *index, INT type, CHAR *data)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    const char *value = NULL;
    const char **msisdn = NULL;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);

    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if (ctx->modem)
        {
            res = RETURN_OK;
            switch ( type )
            {
                case DEVICE_MODEL:
                {
                    if( (value = mm_modem_get_model (ctx->modem)) != NULL )
                        strncpy (data, value, strlen(value)+1);
                    break;
                }
                case DEVICE_VENDOR:
                {
                    if( (value = mm_modem_get_manufacturer (ctx->modem)) != NULL )
                        strncpy (data, value, strlen(value)+1);
                    break;
                }
                case DEVICE_IMAGE_VERSION:
                {
                    if( (value = mm_modem_get_revision (ctx->modem)) != NULL )
                        strncpy (data, value, strlen(value)+1);
                    break;
                }
                case DEVICE_CONTROL_INTERFACE:
                {
                    if( (value = mm_modem_get_primary_port (ctx->modem)) != NULL )
                        strncpy (data, value, strlen(value)+1);
                    break;
                }
                case DEVICE_HARDWARE_REVISION:
                {
                    if( (value = mm_modem_get_hardware_revision (ctx->modem)) != NULL )
                        strncpy (data, value, strlen(value)+1);
                    break;
                }
                case DEVICE_MSISDN:
                {
                    if( (msisdn = (const char **)mm_modem_get_own_numbers (ctx->modem)) != NULL )
                        strncpy (data, *msisdn, strlen(*msisdn)+1);
                    break;
                }
                case DEVICE_IMEI:
                {
                    ctx->modem_3gpp = mm_object_get_modem_3gpp (ctx->object);
                    if( ctx->modem_3gpp )
                    {
                        if( (value = mm_modem_3gpp_get_imei (ctx->modem_3gpp)) != NULL )
                            strncpy (data, value, strlen(value)+1);
                    }
                    break;
                }
                default:
                    res = RETURN_ERROR;
            }
        }
    }
    context_free(ctx);
    return res;
} 

INT cellular_hal_mm_get_supported_access_technologies (CHAR *index, CHAR *accessTech)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    MMModemModeCombination *modes = NULL;
    guint n_modes = 0;
    UINT i, len = 0;
    CHAR buf[32] = {0};
    BOOLEAN support_2g = FALSE, support_3g = FALSE, support_4g = FALSE, support_5g = FALSE;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if (ctx->modem)
        {
            mm_modem_get_supported_modes (ctx->modem, &modes, &n_modes);
            for( i = 0; i < n_modes; i++)
            {
                if( modes[i].allowed & MODEM_MODE_2G && support_2g == FALSE )
                {
                    len += snprintf(buf+len, sizeof(buf)-len, "%s%s", len ? ",":"", MM_MODE_2G);
                    support_2g = TRUE;
                }

                if( modes[i].allowed & MODEM_MODE_3G && support_3g == FALSE )
                {
                    len += snprintf(buf+len, sizeof(buf)-len, "%s%s", len ? ",":"", MM_MODE_3G);
                    support_3g = TRUE;
                }

                if( modes[i].allowed & MODEM_MODE_4G && support_4g == FALSE )
                {
                    len += snprintf(buf+len, sizeof(buf)-len, "%s%s", len ? ",":"", MM_MODE_4G);
                    support_4g = TRUE;
                }
                
                if( modes[i].allowed & MODEM_MODE_5G && support_5g == FALSE )
                {
                    len += snprintf(buf+len, sizeof(buf)-len, "%s%s", len ? ",":"", MM_MODE_5G);
                    support_5g = TRUE;
                }
            }
            if(len)
            {
                strncpy(accessTech, buf, len+1);
                res = RETURN_OK;
            }
        }
    }
    if(modes)
        g_free(modes);
    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_prefered_access_technologies (CHAR *index, CHAR *accessTech)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    MMModemMode allowed;
    MMModemMode preferred;
    UINT len = 0; 
    CHAR buf[32] = {0};

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if (ctx->modem)
        {
            mm_modem_get_current_modes (ctx->modem, &allowed, &preferred);
            if( preferred & MODEM_MODE_2G )
                len += snprintf(buf+len, sizeof(buf)-len, "%s", MM_MODE_2G);

            if( preferred & MODEM_MODE_3G )
                len += snprintf(buf+len, sizeof(buf)-len, ",%s", MM_MODE_3G);

            if( preferred & MODEM_MODE_4G )
                len += snprintf(buf+len, sizeof(buf)-len, ",%s", MM_MODE_4G);

            if( preferred & MODEM_MODE_5G )
                len += snprintf(buf+len, sizeof(buf)-len, ",%s", MM_MODE_5G);

            if(len)
            {
                strncpy(accessTech, buf, len+1);
                res = RETURN_OK;
            }
        }
    }
    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_current_access_technology (CHAR *index, CHAR *accessTech)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    ModemAccessTechnology modemAccessTech = MODEM_ACCESS_TECHNOLOGY_UNKNOWN;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if (ctx->modem)
        {
            modemAccessTech = mm_modem_get_access_technologies(ctx->modem);
            if( modemAccessTech <= MODEM_ACCESS_TECHNOLOGY_EDGE )
                strcpy(accessTech, MM_MODE_2G);
            else if( modemAccessTech <= MODEM_ACCESS_TECHNOLOGY_EVDOB && modemAccessTech > MODEM_ACCESS_TECHNOLOGY_EDGE )
                strcpy(accessTech, MM_MODE_3G);
            else if( modemAccessTech == MODEM_ACCESS_TECHNOLOGY_LTE )
                strcpy(accessTech, MM_MODE_4G);
            else if( modemAccessTech == MODEM_ACCESS_TECHNOLOGY_5GNR )
                strcpy(accessTech, MM_MODE_5G); 
            res = RETURN_OK;
        }
    }

    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_plmn (CHAR *index, CellularCurrentPlmnInfoStruct *plmnInfo)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    const char *value = NULL;
    Modem3gppRegistrationState state;
    MMLocation3gpp *location_3gpp = NULL;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem_3gpp = mm_object_get_modem_3gpp (ctx->object);
        if (ctx->modem_3gpp)
        {
            state = mm_modem_3gpp_get_registration_state (ctx->modem_3gpp);
            if( state == MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING )
                plmnInfo->roaming_enabled = TRUE;
            else
                plmnInfo->roaming_enabled = FALSE;
            if( (value = mm_modem_3gpp_get_operator_name(ctx->modem_3gpp)) != NULL )
                strncpy (plmnInfo->plmn_name, value, sizeof(plmnInfo->plmn_name)-1);
        }
        
        ctx->modem_location = mm_object_get_modem_location (ctx->object);
        if( ctx->modem_location )
        {
            mm_modem_location_get_full_sync (ctx->modem_location,
                                             &location_3gpp,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &error);

            if (error)
            {
                CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't get location: '%s'\n",
                                        __FUNCTION__, error ? error->message : "unknown error");
                g_error_free (error);
            }
            else
            {
                if (location_3gpp)
                {
                    plmnInfo->MCC = mm_location_3gpp_get_mobile_country_code (location_3gpp);
                    plmnInfo->MNC = mm_location_3gpp_get_mobile_network_code (location_3gpp);
                    plmnInfo->area_code = mm_location_3gpp_get_tracking_area_code (location_3gpp);
                    plmnInfo->cell_id = mm_location_3gpp_get_cell_id (location_3gpp);
                    g_clear_object (&location_3gpp);
                }
            }
        }
        res = RETURN_OK;
    }

    context_free(ctx);
    return res;
}

/*======================== get sim ===================================*/

INT cellular_hal_mm_get_sim_info (CHAR *index, CellularUICCSlotInfoStruct *pstSlotInfo)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    GError *error = NULL;
    const char *value = NULL;

    ctx = g_new0 (Context, 1);
    ctx->sim = get_sim_context (index);
    if( ctx->sim )
    {
        if( (value = mm_sim_get_identifier (ctx->sim)) != NULL )
            strncpy (pstSlotInfo->iccid, value, sizeof(pstSlotInfo->iccid)-1);
        if( (value = mm_sim_get_operator_name (ctx->sim)) != NULL )
            strncpy (pstSlotInfo->MnoName, value, sizeof(pstSlotInfo->MnoName)-1);
        pstSlotInfo->Status = CELLULAR_UICC_STATUS_VALID;
        pstSlotInfo->CardEnable = TRUE;
        res = RETURN_OK;
    }

    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_sim_data (CHAR *index, INT type, CHAR *data)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    GError *error = NULL;
    const char *value = NULL;

    ctx = g_new0 (Context, 1);
    ctx->sim = get_sim_context (index);
    if( ctx->sim )
    {
        res = RETURN_OK;
        switch ( type )
        {
            case SIM_ICCID:
            {
                if( (value = mm_sim_get_identifier (ctx->sim)) != NULL )
                    strncpy (data, value, strlen(value)+1);
                break;
            }
            case SIM_MNO_NAME:
            {
                if( (value = mm_sim_get_operator_name (ctx->sim)) != NULL )
                    strncpy (data, value, strlen(value)+1);
                break;
            }
            default:
                res = RETURN_ERROR;
        }
    }

    context_free(ctx);
    return res;
}

/*======================== get signal ===================================*/

INT cellular_hal_mm_set_signal_refresh_rate (CHAR *index, UINT rate)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    gboolean result;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem_signal = mm_object_get_modem_signal (ctx->object);
        if(ctx->modem_signal)
        {
            result = mm_modem_signal_setup_sync (ctx->modem_signal, rate, NULL, &error);
            if( !result )
            {
                CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't setup signal refresh rate: '%s'\n",
                                        __FUNCTION__, error ? error->message : "unknown error");
                if(error)
                    g_error_free (error);
            }
            else
                res = RETURN_OK;
        }
    }

    context_free(ctx);
    return res;
}

static UINT refreshRate = 0;
INT cellular_hal_mm_get_signal (CHAR *index, CellularSignalInfoStruct *radioSignal)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMSignal *signal;
    gdouble   value;
    gboolean result;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if( ctx->object )
    {
        ctx->modem_signal = mm_object_get_modem_signal (ctx->object);
        if (ctx->modem_signal)
        {
            if( refreshRate == 0 )
            {
                refreshRate = 10;
                result = mm_modem_signal_setup_sync (ctx->modem_signal, refreshRate, NULL, &error);
                if( !result )
                {
                    CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't setup signal refresh rate: '%s'\n",
                                           __FUNCTION__, error ? error->message : "unknown error");
                    if(error)
                        g_error_free (error);
                    refreshRate = 0;
                }
            }

            if( (signal = mm_modem_signal_peek_gsm (ctx->modem_signal)) != NULL ||
                (signal = mm_modem_signal_peek_umts (ctx->modem_signal)) != NULL )
            {
                if ((value = mm_signal_get_rssi (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->RSSI = (int)value;
                res = RETURN_OK;
            }
            else if( (signal = mm_modem_signal_peek_lte (ctx->modem_signal)) != NULL || 
                     (signal = mm_modem_signal_peek_nr5g (ctx->modem_signal)) != NULL )
            {   //Both LTE and NR use the same libmm APIs.
                if ((value = mm_signal_get_rssi (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->RSSI = (int)value;
                if ((value = mm_signal_get_rsrq (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->RSRQ = (int)value;
                if ((value = mm_signal_get_rsrp (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->RSRP = (int)value;
                if ((value = mm_signal_get_snr (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->SNR = (int)value;
                CELLULAR_HAL_DBG_PRINT("%s - RSSI: %d, RSRQ: %d, RSRP: %d, SNR: %d\n", __FUNCTION__, radioSignal->RSSI, radioSignal->RSRQ , radioSignal->RSRP, radioSignal->SNR);

                if( radioSignal->RSSI != 0 )
                    res = RETURN_OK;
            }
            g_clear_object (&signal);
        }
    }
    context_free(ctx);
    return res;
}

/*======================== get bearer properties===================================*/

INT cellular_hal_mm_get_bearer_properties (CHAR *index, CellularProfileStruct **pstProfileOutput)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    MMBearerProperties *properties = NULL;
    const char *value = NULL;
    CellularProfileStruct *pfStruct = NULL;
    BearerIpFamily ipFamily;
    CHAR bearerIndex[IDX_SIZE], *bearPath;

    ctx = g_new0 (Context, 1);
    ctx->bearer = get_bearer_context (index);
    if (ctx->bearer)
    {
        properties  = mm_bearer_get_properties (ctx->bearer);
        if( properties )
        {
            pfStruct = (CellularProfileStruct*)malloc(sizeof(CellularProfileStruct));
            memset(pfStruct, 0, sizeof(CellularProfileStruct));
            if( mm_bearer_get_bearer_type (ctx->bearer) == BEARER_TYPE_DEFAULT )
                pfStruct->bIsThisDefaultProfile = TRUE;
            if( (value = mm_bearer_properties_get_apn (properties)) != NULL )
                strncpy (pfStruct->APN, value, sizeof(pfStruct->APN)-1);
            if( (value = mm_bearer_properties_get_user (properties)) != NULL )
                strncpy (pfStruct->Username, value, sizeof(pfStruct->Username)-1);
            if( (value = mm_bearer_properties_get_password (properties)) != NULL )
                strncpy (pfStruct->Password, value, sizeof(pfStruct->Password)-1);
            pfStruct->bIsAPNDisabled = (mm_bearer_get_connected (ctx->bearer) ? FALSE : TRUE);
            ipFamily = mm_bearer_properties_get_ip_type (properties);
            if( ipFamily == BEARER_IP_FAMILY_IPV4 )
                pfStruct->PDPType = CELLULAR_PDP_TYPE_IPV4;
            else if( ipFamily == BEARER_IP_FAMILY_IPV6 )
                pfStruct->PDPType = CELLULAR_PDP_TYPE_IPV6;
            else
                pfStruct->PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
            pfStruct->bIsNoRoaming = (mm_bearer_properties_get_allow_roaming(properties) ? FALSE : TRUE);
            bearPath = mm_bearer_get_path (ctx->bearer);
            strncpy(bearerIndex, bearPath+strlen(BEARER_PATH_PREFIX), IDX_SIZE-1);
            pfStruct->ProfileID = atoi(bearerIndex);
            *pstProfileOutput = pfStruct;
            res = RETURN_OK;
            g_clear_object (&properties);
        }
    }

    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_bearer_properties_list (CHAR *modemIndex, CellularProfileStruct **pstProfileOutput, int *profile_count)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    MMBearerProperties *properties = NULL;
    const char *value = NULL;
    CellularProfileStruct *pfStruct = NULL;
    BearerIpFamily ipFamily;
    CHAR bearerIndex[IDX_SIZE], *bearPath;
    GList *bearers = NULL, *list;
    guint totalCount = 0, i = 0;
    GError *error = NULL;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (modemIndex);
    ctx->modem = mm_object_get_modem (ctx->object);
    if (ctx->modem)
    {
        bearers = mm_modem_list_bearers_sync (ctx->modem, NULL, &error);
        if (!bearers) {
            CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't get bearers at '%s': '%s'\n",
                                    __FUNCTION__, mm_modem_get_path (ctx->modem), error ? error->message : "unknown error");
            if(error)
                g_error_free (error);
        }
        else
        {
            totalCount = g_list_length(bearers);
            pfStruct = (CellularProfileStruct*)malloc(sizeof(CellularProfileStruct)*totalCount);
            memset(pfStruct, 0, sizeof(CellularProfileStruct)*totalCount);
            for (list = bearers, i = 0; list, i < totalCount; list = g_list_next (list), i++)
            {
                MMBearer *bearer = MM_BEARER (list->data);
                properties  = mm_bearer_get_properties (bearer);
                if( properties )
                {
                    if( mm_bearer_get_bearer_type (bearer) == BEARER_TYPE_DEFAULT )
                        pfStruct[i].bIsThisDefaultProfile = TRUE;
                    if( (value = mm_bearer_properties_get_apn (properties)) != NULL )
                        strncpy (pfStruct[i].APN, value, sizeof(pfStruct[i].APN)-1);
                    if( (value = mm_bearer_properties_get_user (properties)) != NULL )
                        strncpy (pfStruct[i].Username, value, sizeof(pfStruct[i].Username)-1);
                    if( (value = mm_bearer_properties_get_password (properties)) != NULL )
                        strncpy (pfStruct[i].Password, value, sizeof(pfStruct[i].Password)-1);
                    pfStruct[i].bIsAPNDisabled = (mm_bearer_get_connected (bearer) ? FALSE : TRUE);
                    ipFamily = mm_bearer_properties_get_ip_type (properties);
                    if( ipFamily == BEARER_IP_FAMILY_IPV4 )
                        pfStruct[i].PDPType = CELLULAR_PDP_TYPE_IPV4;
                    else if( ipFamily == BEARER_IP_FAMILY_IPV6 )
                        pfStruct[i].PDPType = CELLULAR_PDP_TYPE_IPV6;
                    else
                        pfStruct[i].PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
                    pfStruct[i].bIsNoRoaming = (mm_bearer_properties_get_allow_roaming(properties) ? FALSE : TRUE);
                    bearPath = mm_bearer_get_path (bearer);
                    strncpy(bearerIndex, bearPath+strlen(BEARER_PATH_PREFIX), IDX_SIZE-1);
                    pfStruct[i].ProfileID = atoi(bearerIndex);
                    g_clear_object (&properties);
                }
            }
            *pstProfileOutput = pfStruct;
            *profile_count = totalCount;
            res = RETURN_OK;
            g_list_free_full (bearers, g_object_unref);
        }
    }

    context_free(ctx);
    return res;
}
/*======================== get bearer ipv4===================================*/

INT cellular_hal_mm_get_bearer_ipv4_config (CHAR *index, BEARER_IPV4_CONFIG *bearerIpv4Config)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMBearerIpConfig   *ipv4_config = NULL;
    const char *value = NULL;
    const char **Dns = NULL;

    ctx = g_new0 (Context, 1);
    ctx->bearer = get_bearer_context (index);

    if (ctx->bearer)
    {
        ipv4_config = mm_bearer_get_ipv4_config(ctx->bearer);
        if( ipv4_config )
        {
            bearerIpv4Config->Method = mm_bearer_ip_config_get_method (ipv4_config);
            if (bearerIpv4Config->Method != BEARER_IP_METHOD_UNKNOWN)
            {
                if( (value = mm_bearer_ip_config_get_address (ipv4_config)) != NULL )
                    strncpy (bearerIpv4Config->IpAddress, value, sizeof(bearerIpv4Config->IpAddress)-1);
                if( (value = mm_bearer_ip_config_get_gateway (ipv4_config)) != NULL )
                    strncpy (bearerIpv4Config->Gateway, value, sizeof(bearerIpv4Config->Gateway)-1);
                if( (Dns = mm_bearer_ip_config_get_dns (ipv4_config)) != NULL )
                {
                    strncpy (bearerIpv4Config->DnsPri, *Dns, sizeof(bearerIpv4Config->DnsPri)-1);
                    if( *(Dns+1) != NULL )
                        strncpy (bearerIpv4Config->DnsSec, *(Dns+1), sizeof(bearerIpv4Config->DnsSec)-1);
                }
                bearerIpv4Config->Prefix = mm_bearer_ip_config_get_prefix (ipv4_config);
                bearerIpv4Config->Mtu = mm_bearer_ip_config_get_mtu (ipv4_config);
            }
            res = RETURN_OK;
            g_clear_object (&ipv4_config);
        }
    }

    context_free(ctx);
    return res;
}

/*======================== get bearer ipv6===================================*/

INT cellular_hal_mm_get_bearer_ipv6_config (CHAR *index, BEARER_IPV6_CONFIG *bearerIpv6Config)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMBearerIpConfig   *ipv6_config = NULL;
    const char *value = NULL;
    const char **Dns = NULL;

    ctx = g_new0 (Context, 1);
    ctx->bearer = get_bearer_context (index);
    if (ctx->bearer)
    {
        ipv6_config = mm_bearer_get_ipv6_config(ctx->bearer);
        if( ipv6_config )
        {
            bearerIpv6Config->Method = mm_bearer_ip_config_get_method (ipv6_config);
            if (bearerIpv6Config->Method != BEARER_IP_METHOD_UNKNOWN)
            {
                if( (value = mm_bearer_ip_config_get_address (ipv6_config)) != NULL )
                    strncpy (bearerIpv6Config->IpAddress, value, sizeof(bearerIpv6Config->IpAddress)-1);
                if( (value = mm_bearer_ip_config_get_gateway (ipv6_config)) != NULL )
                    strncpy (bearerIpv6Config->Gateway, value, sizeof(bearerIpv6Config->Gateway)-1);
                if( (Dns = mm_bearer_ip_config_get_dns (ipv6_config)) != NULL )
                {
                    strncpy (bearerIpv6Config->DnsPri, *Dns, sizeof(bearerIpv6Config->DnsPri)-1);
                    if( *(Dns+1) != NULL )
                        strncpy (bearerIpv6Config->DnsSec, *(Dns+1), sizeof(bearerIpv6Config->DnsSec)-1);
                }
                bearerIpv6Config->Prefix = mm_bearer_ip_config_get_prefix (ipv6_config);
                bearerIpv6Config->Mtu = mm_bearer_ip_config_get_mtu (ipv6_config);
            }
            res = RETURN_OK;
            g_clear_object (&ipv6_config);
        }
    }

    context_free(ctx);
    return res;
}

/*======================== get bearer stats===================================*/

INT cellular_hal_mm_get_bearer_stats (CHAR *index, CellularPacketStatsStruct *network_packet_stats)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMBearerStats *stats = NULL;

    ctx = g_new0 (Context, 1);
    ctx->bearer = get_bearer_context (index);
    if (ctx->bearer)
    {
        stats = mm_bearer_get_stats (ctx->bearer);
        if( stats )
        {
            network_packet_stats->BytesReceived = mm_bearer_stats_get_rx_bytes (stats);
            network_packet_stats->BytesSent = mm_bearer_stats_get_tx_bytes (stats);
            res = RETURN_OK;
            g_clear_object (&stats);
        }
    }

    context_free(ctx);
    return res;
}

/*======================== create bearer properties===================================*/

INT cellular_hal_mm_create_bearer_properties (CHAR *index, CellularProfileStruct *pstProfileInput)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMBearerProperties *properties = NULL;
    MMBearer *bearer = NULL;
    BearerAllowedAuth bearerAuth = BEARER_ALLOWED_AUTH_NONE;
    BearerIpFamily ipFamily = BEARER_IP_FAMILY_IPV4V6;
    CellularDeviceProfileSelectionStatus_t profileStatus = DEVICE_PROFILE_STATUS_NOT_READY;
    CHAR bearerIndex[IDX_SIZE], *bearPath;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    ctx->modem = mm_object_get_modem (ctx->object);

    properties = mm_bearer_properties_new ();
    if( properties )
    {
        if( pstProfileInput->APN != NULL )
            mm_bearer_properties_set_apn (properties, pstProfileInput->APN);
        
        if( pstProfileInput->PDPAuthentication == CELLULAR_PDP_AUTHENTICATION_PAP )
            bearerAuth = BEARER_ALLOWED_AUTH_PAP;
        else if( pstProfileInput->PDPAuthentication == CELLULAR_PDP_AUTHENTICATION_CHAP )    
            bearerAuth = BEARER_ALLOWED_AUTH_CHAP;
        
        mm_bearer_properties_set_allowed_auth (properties, bearerAuth);
        if( bearerAuth != BEARER_ALLOWED_AUTH_NONE )
        {
            mm_bearer_properties_set_user (properties, pstProfileInput->Username);
            mm_bearer_properties_set_password (properties, pstProfileInput->Password);
        }
        if( pstProfileInput->PDPType == CELLULAR_PDP_TYPE_IPV4 )
            ipFamily = BEARER_IP_FAMILY_IPV4;
        else if( pstProfileInput->PDPType == CELLULAR_PDP_TYPE_IPV6 )
            ipFamily = BEARER_IP_FAMILY_IPV6;
        
        mm_bearer_properties_set_ip_type(properties, ipFamily);
        mm_bearer_properties_set_allow_roaming(properties, pstProfileInput->bIsNoRoaming);

        bearer = mm_modem_create_bearer_sync (ctx->modem, properties, NULL, &error);

        if( bearer == NULL )
        {
            CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't create new bearer: '%s'\n",
                                    __FUNCTION__, error ? error->message : "unknown error");

            if( error )
            {
                if(strstr(error->message, "already reached maximum"))
                {   
                    profileStatus = DEVICE_PROFILE_STATUS_READY;
                    res = RETURN_OK;
                }
                g_error_free (error);
            }
        }
        else
        {
            bearPath = mm_bearer_get_path (bearer);
            strncpy(bearerIndex, bearPath+strlen(BEARER_PATH_PREFIX), IDX_SIZE-1);
            pstProfileInput->ProfileID = atoi(bearerIndex);
            profileStatus = DEVICE_PROFILE_STATUS_READY;
            res = RETURN_OK;
            g_clear_object (&bearer);
        }
        profileStatusCb( "profile", pstProfileInput->PDPType, profileStatus );
        g_clear_object (&properties);
    }

    context_free(ctx);
    return res;
}

/*======================== connect bearer ===================================*/

INT cellular_hal_mm_connect_bearer (CHAR *index, BOOLEAN connect)
{
    Context *ctx;
    INT res = RETURN_ERROR;

    ctx = g_new0 (Context, 1);
    ctx->bearer = get_bearer_context (index);

    if( ctx->bearer )
    {
        if( mmState == MODEM_STATE_CONNECTED && connect )
        {
            if( networkStatusCB )
                send_bearer_info(ctx->bearer);
        }
        else
        {
            if( connect )
                mm_bearer_connect(ctx->bearer, NULL, NULL, NULL);
            else
                mm_bearer_disconnect(ctx->bearer, NULL, NULL, NULL);
        }
        res = RETURN_OK;
    }
    context_free(ctx);
    return res;
}

/*======================== delete bearer ===================================*/

INT cellular_hal_mm_delete_bearer(CHAR *modemIndex, CellularProfileStruct *pstProfileInput)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    gboolean result;
    const gchar bearPath[64] = {0};
    const gchar **bearer_paths;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (modemIndex);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if( ctx->modem )
        {
            
            if( pstProfileInput )
                snprintf(bearPath, sizeof(bearPath)-1, "%s%d", BEARER_PATH_PREFIX, pstProfileInput->ProfileID);
            else
            {
                bearer_paths = (const gchar **) mm_modem_get_bearer_paths (ctx->modem);
                if(bearer_paths && bearer_paths[0])
                    strncpy(bearPath, *bearer_paths, sizeof(bearPath)-1);
                else
                {
                    context_free(ctx);
                    return RETURN_ERROR;
                }
                CELLULAR_HAL_DBG_PRINT ("%s - delete the bearer: '%s'\n", __FUNCTION__, bearPath);
            }
            result = mm_modem_delete_bearer_sync (ctx->modem,
                                              bearPath,
                                              NULL,
                                              &error);

            if( !result )
            {
                CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't delete the bearer: '%s'\n",
                                        __FUNCTION__, error ? error->message : "unknown error");
                if(error)
                    g_error_free (error);
            }
            else
            {
                if( profileStatusCb )
                    profileStatusCb( "unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_DELETED );
                res = RETURN_OK;
            }
        }
    }
    context_free(ctx);
    return res;
}

/*======================== set access tech ===================================*/

INT cellular_hal_mm_set_access_technology (CHAR *index, ModemModeCombination *ModemMode)
{
    Context *ctx;
    GError *error = NULL;
    gboolean result;
    INT res = RETURN_ERROR;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if( ctx->modem )
        {
            result = mm_modem_set_current_modes_sync (ctx->modem, ModemMode->allowed, ModemMode->preferred, NULL, &error);
            if( !result )
            {
                if(error)
                    g_error_free (error);
            }
            else
                res = RETURN_OK;
        }
    }

    context_free(ctx);
    return res;
}

INT cellular_hal_mm_get_bearer_data_interface(CHAR *index, CHAR *dataInterface)
{
    Context *ctx;
    INT res = RETURN_ERROR;
    MMBearerProperties *properties = NULL;
    const char *value = NULL;

    ctx = g_new0 (Context, 1);
    ctx->bearer = get_bearer_context (index);
    if (ctx->bearer)
    {
        properties  = mm_bearer_get_properties (ctx->bearer);
        if( properties )
        {
            if( (value = mm_bearer_get_interface (ctx->bearer)) != NULL )
            {
                strncpy (dataInterface, value, strlen(value)+1);
                res = RETURN_OK;
            }
            g_clear_object (&properties);
        }
    }
    context_free(ctx);
    return res; 
    
}

INT cellular_hal_mm_get_bearer_info (CHAR *index, BEARER_PROPERTIES *bearerProperties,
                                     BEARER_IPV4_CONFIG *bearerIpv4Config, BEARER_IPV6_CONFIG *bearerIpv6Config)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMBearerProperties *properties = NULL;
    MMBearerIpConfig   *ipv4_config = NULL;
    MMBearerIpConfig   *ipv6_config = NULL;
    const char *value = NULL;
    const char **Dns = NULL;

    ctx = g_new0 (Context, 1);
    ctx->bearer = get_bearer_context (index);
    if (ctx->bearer)
    {
        properties  = mm_bearer_get_properties (ctx->bearer);
        if( properties )
        {
            bearerProperties->type = mm_bearer_get_bearer_type (ctx->bearer);
            if( (value = mm_bearer_get_interface (ctx->bearer)) != NULL )
                strncpy (bearerProperties->NetPort, value, sizeof(bearerProperties->NetPort)-1);
            if( (value = mm_bearer_properties_get_apn (properties)) != NULL )
                strncpy (bearerProperties->Apn, value, sizeof(bearerProperties->Apn)-1);
            if( (value = mm_bearer_properties_get_user (properties)) != NULL )
                strncpy (bearerProperties->username, value, sizeof(bearerProperties->username)-1);
            if( (value = mm_bearer_properties_get_password (properties)) != NULL )
                strncpy (bearerProperties->password, value, sizeof(bearerProperties->password)-1);
            bearerProperties->Status = (mm_bearer_get_connected (ctx->bearer) ? TRUE : FALSE);
            bearerProperties->IpAddressFamily = mm_bearer_properties_get_ip_type (properties);
            bearerProperties->Roaming = mm_bearer_properties_get_allow_roaming (properties);
            res = RETURN_OK;
            g_clear_object (&properties);
        }

        ipv4_config = mm_bearer_get_ipv4_config(ctx->bearer);
        if( ipv4_config )
        {
            bearerIpv4Config->Method = mm_bearer_ip_config_get_method (ipv4_config);
            if (bearerIpv4Config->Method != BEARER_IP_METHOD_UNKNOWN)
            {
                if( (value = mm_bearer_ip_config_get_address (ipv4_config)) != NULL )
                    strncpy (bearerIpv4Config->IpAddress, value, sizeof(bearerIpv4Config->IpAddress)-1);
                if( (value = mm_bearer_ip_config_get_gateway (ipv4_config)) != NULL )
                    strncpy (bearerIpv4Config->Gateway, value, sizeof(bearerIpv4Config->Gateway)-1);
                if( (Dns = mm_bearer_ip_config_get_dns (ipv4_config)) != NULL )
                {
                    strncpy (bearerIpv4Config->DnsPri, *Dns, sizeof(bearerIpv4Config->DnsPri)-1);
                    if( *(Dns+1) != NULL )
                        strncpy (bearerIpv4Config->DnsSec, *(Dns+1), sizeof(bearerIpv4Config->DnsSec)-1);
                }
                bearerIpv4Config->Prefix = mm_bearer_ip_config_get_prefix (ipv4_config);
                bearerIpv4Config->Mtu = mm_bearer_ip_config_get_mtu (ipv4_config);
            }
            g_clear_object (&ipv4_config);
        }

        ipv6_config = mm_bearer_get_ipv6_config(ctx->bearer);
        if( ipv6_config )
        {
            bearerIpv6Config->Method = mm_bearer_ip_config_get_method (ipv6_config);
            if (bearerIpv6Config->Method != BEARER_IP_METHOD_UNKNOWN)
            {
                if( (value = mm_bearer_ip_config_get_address (ipv6_config)) != NULL )
                    strncpy (bearerIpv6Config->IpAddress, value, sizeof(bearerIpv6Config->IpAddress)-1);
                if( (value = mm_bearer_ip_config_get_gateway (ipv6_config)) != NULL )
                    strncpy (bearerIpv6Config->Gateway, value, sizeof(bearerIpv6Config->Gateway)-1);
                if( (Dns = mm_bearer_ip_config_get_dns (ipv6_config)) != NULL )
                {
                    strncpy (bearerIpv6Config->DnsPri, *Dns, sizeof(bearerIpv6Config->DnsPri)-1);
                    if( *(Dns+1) != NULL )
                        strncpy (bearerIpv6Config->DnsSec, *(Dns+1), sizeof(bearerIpv6Config->DnsSec)-1);
                }
                bearerIpv6Config->Prefix = mm_bearer_ip_config_get_prefix (ipv6_config);
                bearerIpv6Config->Mtu = mm_bearer_ip_config_get_mtu (ipv6_config);
            }
            g_clear_object (&ipv6_config);
        }
    }

    context_free(ctx);
    return res;
}

static void cellular_hal_mm_send_device_slot_status(
    cellular_device_slot_status_api_callback device_slot_status_cb)
{
    //Send ready status if CB is not null
    if( slot_status_cb == NULL )
    {
        if( device_slot_status_cb != NULL)
            slot_status_cb = device_slot_status_cb;
    }
}

void cellular_hal_mm_send_device_registration_status(
    cellular_device_registration_status_callback device_registration_status_cb)
{
    //Send NAS registration status if CB is not null
    if( registration_status_cb == NULL )
    {
        if( device_registration_status_cb != NULL)
        {    
            registration_status_cb = device_registration_status_cb;
        }
    }
    
    registration_status_cb( DEVICE_NAS_STATUS_REGISTERED, DEVICE_NAS_STATUS_ROAMING_OFF, CELLULAR_MODEM_REGISTERED_SERVICE_CS_PS );
}

void cellular_hal_mm_get_uicc_slots(CHAR *index, unsigned int *total_count)
{
#if MM_CHECK_VERSION(1,18,8)
    Context *ctx;
    INT i;
    gchar **sim_slot_paths = NULL;
    *total_count = 0;
    g_autoptr(GPtrArray) sim_slots = NULL;
    GError *error = NULL;
    MMSim *sim;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if( ctx->object )
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if( ctx->modem )
        {
            sim_slots = mm_modem_list_sim_slots_sync (ctx->modem, NULL, &error);
            if( error )
                g_error_free(error);
            else
            {
                for (i = 0; i < sim_slots->len; i++) 
                {
                    sim = MM_SIM (g_ptr_array_index (sim_slots, i));
                    if(sim)
                       (*total_count)++;
                }
            }
        }
    }
    
    context_free(ctx);  
#else
    *total_count = 1;
#endif
    return;
}

void cellular_hal_mm_send_device_status(CellularDeviceContextCBStruct *device_status_cb)
{
    if( deviceStatusCB == NULL )
    {
        deviceStatusCB = g_new0 (CellularDeviceContextCBStruct, 1);
        memcpy(deviceStatusCB, device_status_cb, sizeof(CellularDeviceContextCBStruct));
    }
    
    if( mmState >= MODEM_STATE_DISABLED && mmState != MODEM_STATE_DISABLING )
        deviceStatusCB->device_open_status_cb( deviceInfo.deviceName, deviceInfo.wanInterfaceName, DEVICE_OPEN_STATUS_READY, CELLULAR_MODEM_SET_ONLINE );
}

void cellular_hal_mm_send_network_status(CellularNetworkCBStruct *network_status_cb)
{
    if( networkStatusCB == NULL )
    {
        networkStatusCB = g_new0 (CellularNetworkCBStruct, 1);
        memcpy(networkStatusCB, network_status_cb, sizeof(CellularNetworkCBStruct));
    }
}

void cellular_hal_mm_send_device_profile_status(
    cellular_device_profile_status_api_callback device_profile_status_cb)
{
    //Send Profile ready status if CB is not null
    if( NULL != device_profile_status_cb )
    {
        if(profileStatusCb == NULL)
        {
            profileStatusCb = device_profile_status_cb;
        }
    }
}

static void send_bearer_info (MMBearer *bearer)
{
    MMBearerProperties *properties = NULL;
    MMBearerIpConfig   *ipv4_config = NULL;
    MMBearerIpConfig   *ipv6_config = NULL;
    const char *value = NULL;
    const char **Dns = NULL;
    CellularIPStruct *ipStruct = NULL;
    BearerIpFamily ipFamily;
    CHAR ip[16] = {0};

    ipStruct = (CellularIPStruct*)malloc(sizeof(CellularIPStruct));
    properties  = mm_bearer_get_properties (bearer);
    if( properties )
    {
        ipFamily = mm_bearer_properties_get_ip_type (properties);
        if( ipFamily == BEARER_IP_FAMILY_IPV4 || ipFamily == BEARER_IP_FAMILY_IPV4V6 )
        {
            memset(ipStruct, 0, sizeof(CellularIPStruct));
            if( (value = mm_bearer_get_interface (bearer)) != NULL )
                strncpy (ipStruct->WANIFName, value, sizeof(ipStruct->WANIFName)-1);
            ipStruct->IPType = CELLULAR_NETWORK_IP_FAMILY_IPV4;
            ipv4_config = mm_bearer_get_ipv4_config(bearer);
            if( ipv4_config )
            {
                if ( mm_bearer_ip_config_get_method (ipv4_config) != BEARER_IP_METHOD_UNKNOWN )
                {
                    if( (value = mm_bearer_ip_config_get_address (ipv4_config)) != NULL )
                        strncpy (ipStruct->IPAddress, value, sizeof(ipStruct->IPAddress)-1);
                    if( (value = mm_bearer_ip_config_get_gateway (ipv4_config)) != NULL )
                        strncpy (ipStruct->DefaultGateWay, value, sizeof(ipStruct->DefaultGateWay)-1);
                    if( (Dns = mm_bearer_ip_config_get_dns (ipv4_config)) != NULL )
                    {
                        strncpy (ipStruct->DNSServer1, *Dns, sizeof(ipStruct->DNSServer1)-1);
                        if( *(Dns+1) != NULL )
                            strncpy (ipStruct->DNSServer2, *(Dns+1), sizeof(ipStruct->DNSServer2)-1);
                    }
                    convertSubnetMask(mm_bearer_ip_config_get_prefix (ipv4_config), ip, 16);
                    strncpy (ipStruct->SubnetMask, ip, sizeof(ipStruct->SubnetMask)-1);
                    ipStruct->MTUSize = mm_bearer_ip_config_get_mtu (ipv4_config);
                }
                g_clear_object (&ipv4_config);
                networkStatusCB->device_network_ip_ready_cb(ipStruct, DEVICE_NETWORK_IP_READY);
                networkStatusCB->packet_service_status_cb("mm", ipStruct->IPType, DEVICE_NETWORK_STATUS_CONNECTED);
            }
        }

        if( ipFamily == BEARER_IP_FAMILY_IPV6 || ipFamily == BEARER_IP_FAMILY_IPV4V6 )
        {
            memset(ipStruct, 0, sizeof(CellularIPStruct));
            if( (value = mm_bearer_get_interface (bearer)) != NULL )
                strncpy (ipStruct->WANIFName, value, sizeof(ipStruct->WANIFName)-1);
            ipStruct->IPType = CELLULAR_NETWORK_IP_FAMILY_IPV6;
            ipv6_config = mm_bearer_get_ipv6_config(bearer);
            if( ipv6_config )
            {
                if ( mm_bearer_ip_config_get_method (ipv6_config) != BEARER_IP_METHOD_UNKNOWN )
                {
                    if( (value = mm_bearer_ip_config_get_address (ipv6_config)) != NULL )
                        strncpy (ipStruct->IPAddress, value, sizeof(ipStruct->IPAddress)-1);
                    if( (value = mm_bearer_ip_config_get_gateway (ipv6_config)) != NULL )
                        strncpy (ipStruct->DefaultGateWay, value, sizeof(ipStruct->DefaultGateWay)-1);
                    if( (Dns = mm_bearer_ip_config_get_dns (ipv6_config)) != NULL )
                    {
                        strncpy (ipStruct->DNSServer1, *Dns, sizeof(ipStruct->DNSServer1)-1);
                        if( *(Dns+1) != NULL )
                            strncpy (ipStruct->DNSServer2, *(Dns+1), sizeof(ipStruct->DNSServer2)-1);
                    }
                    ipStruct->MTUSize = mm_bearer_ip_config_get_mtu (ipv6_config);
                }
                g_clear_object (&ipv6_config);
                networkStatusCB->device_network_ip_ready_cb(ipStruct, DEVICE_NETWORK_IP_READY);
                networkStatusCB->packet_service_status_cb("mm", ipStruct->IPType, DEVICE_NETWORK_STATUS_CONNECTED);
            }
        }
        g_clear_object (&properties);
    }

    if(ipStruct)
        free(ipStruct);
    return;
}

static void cellular_hal_mm_callBack_bearer_info (MMModem *modem)
{
    GList *bearers = NULL, *l= NULL;
    GError *error = NULL;
    MMBearerProperties *properties = NULL;
    MMBearerIpConfig   *ipv4_config = NULL;
    MMBearerIpConfig   *ipv6_config = NULL;
    const char *value = NULL;
    const char **Dns = NULL;
    CellularIPStruct *ipStruct = NULL;
    MMBearer *found = NULL;
    BearerIpFamily ipFamily;
    CHAR ip[16] = {0};

    bearers = mm_modem_list_bearers_sync (modem, NULL, &error);
    if (!bearers)
    {
        CELLULAR_HAL_DBG_PRINT ("error: couldn't list bearers at '%s': '%s'\n",
                                mm_modem_get_path (modem),
                                error->message);
        if(error)
            g_error_free (error);
    }
    else
    {
        for (l = bearers; l; l = g_list_next (l))
        {
            MMBearer *bearer = MM_BEARER (l->data);
            if (mm_bearer_get_connected (bearer))
            {
                found = g_object_ref (bearer);
                break;
            }
        }
        g_list_free_full (bearers, g_object_unref);

        if( found )
        {
            ipStruct = (CellularIPStruct*)malloc(sizeof(CellularIPStruct));
            properties  = mm_bearer_get_properties (found);
            if( properties )
            {
                ipFamily = mm_bearer_properties_get_ip_type (properties);
                if( ipFamily == BEARER_IP_FAMILY_IPV4 || ipFamily == BEARER_IP_FAMILY_IPV4V6 )
                {
                    memset(ipStruct, 0, sizeof(CellularIPStruct));
                    if( (value = mm_bearer_get_interface (found)) != NULL )
                        strncpy (ipStruct->WANIFName, value, sizeof(ipStruct->WANIFName)-1);
                    ipStruct->IPType = CELLULAR_NETWORK_IP_FAMILY_IPV4;
                    ipv4_config = mm_bearer_get_ipv4_config(found);
                    if( ipv4_config )
                    {
                        if ( mm_bearer_ip_config_get_method (ipv4_config) != BEARER_IP_METHOD_UNKNOWN )
                        {
                            if( (value = mm_bearer_ip_config_get_address (ipv4_config)) != NULL )
                                strncpy (ipStruct->IPAddress, value, sizeof(ipStruct->IPAddress)-1);
                            if( (value = mm_bearer_ip_config_get_gateway (ipv4_config)) != NULL )
                                strncpy (ipStruct->DefaultGateWay, value, sizeof(ipStruct->DefaultGateWay)-1);
                            if( (Dns = mm_bearer_ip_config_get_dns (ipv4_config)) != NULL )
                            {
                                strncpy (ipStruct->DNSServer1, *Dns, sizeof(ipStruct->DNSServer1)-1);
                                if( *(Dns+1) != NULL )
                                    strncpy (ipStruct->DNSServer2, *(Dns+1), sizeof(ipStruct->DNSServer2)-1);
                            }
                            convertSubnetMask(mm_bearer_ip_config_get_prefix (ipv4_config), ip, 16);
                            strncpy (ipStruct->SubnetMask, ip, sizeof(ipStruct->SubnetMask)-1);
                            ipStruct->MTUSize = mm_bearer_ip_config_get_mtu (ipv4_config);
                        }
                        g_clear_object (&ipv4_config);
                        networkStatusCB->device_network_ip_ready_cb(ipStruct, DEVICE_NETWORK_IP_READY);
                        networkStatusCB->packet_service_status_cb("mm", ipStruct->IPType, DEVICE_NETWORK_STATUS_CONNECTED);
                    }
                }

                if( ipFamily == BEARER_IP_FAMILY_IPV6 || ipFamily == BEARER_IP_FAMILY_IPV4V6 )
                {
                    memset(ipStruct, 0, sizeof(CellularIPStruct));
                    if( (value = mm_bearer_get_interface (found)) != NULL )
                        strncpy (ipStruct->WANIFName, value, sizeof(ipStruct->WANIFName)-1);
                    ipStruct->IPType = CELLULAR_NETWORK_IP_FAMILY_IPV6;
                    ipv6_config = mm_bearer_get_ipv6_config(found);
                    if( ipv6_config )
                    {
                        if ( mm_bearer_ip_config_get_method (ipv6_config) != BEARER_IP_METHOD_UNKNOWN )
                        {
                            if( (value = mm_bearer_ip_config_get_address (ipv6_config)) != NULL )
                                strncpy (ipStruct->IPAddress, value, sizeof(ipStruct->IPAddress)-1);
                            if( (value = mm_bearer_ip_config_get_gateway (ipv6_config)) != NULL )
                                strncpy (ipStruct->DefaultGateWay, value, sizeof(ipStruct->DefaultGateWay)-1);
                            if( (Dns = mm_bearer_ip_config_get_dns (ipv6_config)) != NULL )
                            {
                                strncpy (ipStruct->DNSServer1, *Dns, sizeof(ipStruct->DNSServer1)-1);
                                if( *(Dns+1) != NULL )
                                    strncpy (ipStruct->DNSServer2, *(Dns+1), sizeof(ipStruct->DNSServer2)-1);
                            }
                            ipStruct->MTUSize = mm_bearer_ip_config_get_mtu (ipv6_config);
                        }
                        g_clear_object (&ipv6_config);
                        networkStatusCB->device_network_ip_ready_cb(ipStruct, DEVICE_NETWORK_IP_READY);
                        networkStatusCB->packet_service_status_cb("mm", ipStruct->IPType, DEVICE_NETWORK_STATUS_CONNECTED);
                    }
                }
                g_clear_object (&properties);
            }
            g_object_unref(found);
        }

    }

    if(ipStruct)
        free(ipStruct);
    return;
}

INT cellular_hal_mm_get_active_card_status( CellularUICCStatus_t *card_status )
{
    INT res = RETURN_ERROR;
    ModemStateFailedReason reason;
    ModemState State;

    res = cellular_hal_mm_get_modem_state(NULL, &State, &reason);
    if( res == RETURN_OK )
    {
        if( reason == MODEM_STATE_FAILED_REASON_SIM_MISSING || reason == MODEM_STATE_FAILED_REASON_UNKNOWN )
            *card_status = CELLULAR_UICC_STATUS_EMPTY;
        else if( reason == MODEM_STATE_FAILED_REASON_SIM_ERROR )
            *card_status = CELLULAR_UICC_STATUS_ERROR;
        else
            *card_status = CELLULAR_UICC_STATUS_VALID;
        
        if( State == MODEM_STATE_LOCKED )
            *card_status = CELLULAR_UICC_STATUS_BLOCKED;
    }
	else
		*card_status = CELLULAR_UICC_STATUS_EMPTY;
    
    return res;
}

static void
disable_ready (MMModem      *modem,
               GAsyncResult *result,
               gpointer      nothing)
{
    gboolean operation_result;
    GError *error = NULL;

    operation_result = mm_modem_disable_finish (modem, result, &error);
    if (!operation_result) {
        CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't disable the modem: '%s'\n",
                                __FUNCTION__, error ? error->message : "unknown error");

        if(error)
            g_error_free(error);
    }
    else
    {
        operation_result = mm_modem_set_power_state_sync (modem, MM_MODEM_POWER_STATE_LOW, NULL, &error);
        if( !operation_result )
        {
            CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't set new power state in the modem: '%s'\n",
                                    __FUNCTION__, error ? error->message : "unknown error");
            if(error)
                g_error_free (error);
        }
    }
}

INT cellular_hal_mm_enable_airplaneMode (CHAR *index, BOOLEAN enable)
{
    Context *ctx;
    GError *error = NULL;
    gboolean result;
    INT res = RETURN_ERROR;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);

    if(ctx->object)
    {
        ctx->modem = mm_object_get_modem (ctx->object);
        if (ctx->modem)
        {
            if( enable == TRUE )
            {
                mm_modem_disable(ctx->modem, NULL, (GAsyncReadyCallback)disable_ready, NULL);
                res = RETURN_OK;
            }
            else
            {
                result = mm_modem_set_power_state_sync (ctx->modem, MM_MODEM_POWER_STATE_ON, NULL, &error);

                if( !result )
                {
                    CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't set new power state in the modem: '%s'\n",
                                            __FUNCTION__, error ? error->message : "unknown error");
                    if(error)
                        g_error_free (error);
                }
                else
                    res = RETURN_OK;
            }
        }
    }

    context_free(ctx);
    return res;
}

INT cellular_hal_mm_set_modem_operating_configuration(CellularModemOperatingConfiguration_t modem_operating_config)
{
    INT res = RETURN_ERROR;
    if( modem_operating_config == CELLULAR_MODEM_SET_OFFLINE || 
        modem_operating_config == CELLULAR_MODEM_SET_LOW_POWER_MODE )
    {
        res = cellular_hal_mm_enable_airplaneMode(NULL, TRUE);
    }
    else if( modem_operating_config == CELLULAR_MODEM_SET_ONLINE )
    {
        res = cellular_hal_mm_enable_airplaneMode(NULL, FALSE);
    }
    return res;
}

INT cellular_hal_mm_get_current_modem_interface_status( CellularInterfaceStatus_t *status )
{
    INT res = RETURN_ERROR;
    ModemStateFailedReason reason = MODEM_STATE_FAILED_REASON_NONE;
    ModemState State;
    *status = IF_DOWN;
    
    res = cellular_hal_mm_get_modem_state(NULL, &State, &reason);
    if( res == RETURN_OK )
    {
        if( State > MODEM_STATE_UNKNOWN )
            *status = IF_UP;
        else if( State == MODEM_STATE_UNKNOWN )
            *status = IF_UNKNOWN;
        else if( State == MODEM_STATE_FAILED )
            *status = IF_ERROR;
    }    
    return res;
}

/*======================== get servingCell ===================================*/
INT cellular_hal_mm_get_servingCell (CHAR *index, SERVING_CELL *servingCell)
{
    Context *ctx;
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMLocation3gpp *location_3gpp = NULL;
    UINT  mcc, mnc;

    ctx = g_new0 (Context, 1);
    ctx->object = get_modem_context (index);
    if(ctx->object)
    {
        ctx->modem_location = mm_object_get_modem_location (ctx->object);
        if( ctx->modem_location )
        {
            mm_modem_location_get_full_sync (ctx->modem_location,
                                             &location_3gpp,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &error);

            if (error)
            {
                CELLULAR_HAL_DBG_PRINT ("%s - error: couldn't get location '%s'\n",
                                        __FUNCTION__, error ? error->message : "unknown error");
                g_error_free (error);
            }
            else
            {
                if (location_3gpp)
                {
                    mcc = mm_location_3gpp_get_mobile_country_code (location_3gpp);
                    mnc = mm_location_3gpp_get_mobile_network_code (location_3gpp);
                    servingCell->PlmnId = mcc*1000 + mnc;
                    servingCell->LocationAreaCode = mm_location_3gpp_get_location_area_code (location_3gpp);
                    servingCell->TrackingAreaCode = mm_location_3gpp_get_tracking_area_code (location_3gpp);
                    servingCell->CellId = mm_location_3gpp_get_cell_id (location_3gpp);
                    g_clear_object (&location_3gpp);
                }
                res = RETURN_OK;
            }
        }
    }

    context_free(ctx);
    return res;
}
