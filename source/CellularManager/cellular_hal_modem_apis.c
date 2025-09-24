/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Deutsche Telekom AG.
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
**********************************************************************/

#ifdef FEATURE_MODEM_HAL
#include "secure_wrapper.h"
#include "ansc_platform.h"
#include "ccsp_trace.h"
#include "cellular_hal_modem_apis.h"

#define MODEM_PATH_PREFIX   "/org/freedesktop/ModemManager1/Modem/"
#define SIM_PATH_PREFIX   "/org/freedesktop/ModemManager1/SIM/"
#define BEARER_PATH_PREFIX   "/org/freedesktop/ModemManager1/Bearer/"
#define DEFAULT_MTU_SIZE 1500

// Modem Manager object
static MMManager *g_mmManagerObj = NULL;

// Modem device, this can be made an array to support multiple modem devices
static Modem *g_pmodemDevice = NULL;
static bool modemPresent = false;

// variables used by for the glibc main loop
static GDBusConnection *g_dbusConnection = NULL;
static GMainLoop *g_mmManagerloop;
static GCancellable *g_mmManagerCancellable;

// global to hold the current command from state machine to be executed, on a seperate thread context
static MODEMCommand_t g_commandModem = MODEM_NO_COMMAND;
static unsigned char g_portName[16];
static CellularProfileStruct *g_pstProfileInput = NULL;
static unsigned int g_refreshRate = 0;

// command processor thread
static BOOL g_run_command_processor_thread = true;
static pthread_t g_command_processor_thread;

static pthread_t g_configure_interface_thread;
static gulong g_handler_id = 0;

// Declaration of thread condition variable
static pthread_cond_t g_condWait = PTHREAD_COND_INITIALIZER;
// Mutex to serialize acccess to state machine state
static pthread_mutex_t g_stateLock = PTHREAD_MUTEX_INITIALIZER;

// Callbacks from state machine
CellularNetworkCBStruct g_networkStatusCB = { NULL, NULL };
CellularDeviceContextCBStruct g_deviceStatusCB = { NULL, NULL };
cellular_device_profile_status_api_callback g_profileStatusCb = NULL;
cellular_device_slot_status_api_callback g_slot_status_cb = NULL;
cellular_device_registration_status_callback g_registration_status_cb = NULL;


static void state_changed (MMModem *modem, MMModemState old_state, MMModemState new_state,
               MMModemStateChangeReason reason);
static void stop_network ();

/**********************************************************************
                HELPER FUNCTION DEFINITION
**********************************************************************/
// Function to convert the subnet mask from / notation
static void convertSubnetMask(int prefix, char *value, int size)
{
    unsigned long mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
    snprintf(value, size, "%lu.%lu.%lu.%lu", mask >> 24, (mask >> 16) & 0xFF, (mask >> 8) & 0xFF, mask & 0xFF);
    return;
}

// Function to release the memory allocated to Modem and its internal objects
// and then free the modem
static void free_modem_context ()
{
    gboolean result;
    GError *error = NULL;

    CcspTraceInfo (("%s :: Entry \n", __FUNCTION__));

    // First delete the bearers and free bearer context
    for (int currindex = 0; currindex < g_pmodemDevice->numberOfBearers;
            currindex++)
    {
        g_clear_object (&(g_pmodemDevice->pBearer[currindex]->bearerProperties));
        // Free context
        g_object_unref (g_pmodemDevice->pBearer[currindex]->bearerObj);

        // Found the bearer in our list
        result = mm_modem_delete_bearer_sync (
                g_pmodemDevice->modemObj,
                g_pmodemDevice->pBearer[currindex]->dbusPath, NULL, &error);
        if (!result)
        {
            CcspTraceError (("Error: couldn't delete the bearer: '%s'\n", error ? error->message : "unknown error"));
            if (error)
                g_error_free (error);
        }

        // Now free the memory of the bearer
        free (g_pmodemDevice->pBearer[currindex]);
        g_pmodemDevice->pBearer[currindex] = NULL;
    }
    g_pmodemDevice->numberOfBearers = 0;

    // Delete the SIMs and free SIM context
    for (int currindex = 0; currindex < g_pmodemDevice->numberOfSIMs; currindex++)
    {
        // Free context
        g_object_unref (g_pmodemDevice->pSIM[currindex]->simObj);

        // Now free the memory of the SIM
        free (g_pmodemDevice->pSIM[currindex]);
        g_pmodemDevice->pSIM[currindex] = NULL;
    }
    g_pmodemDevice->numberOfSIMs = 0;

    // Free modem context
    g_object_unref (g_pmodemDevice->modemObj);

    // Free object context
    g_object_unref (g_pmodemDevice->mmObj);

    CcspTraceInfo (("%s :: Exit \n", __FUNCTION__));
}

// Function to stop the network and close the device
static void close_device ()
{
    gboolean result;
    GError *error = NULL;

    CcspTraceInfo (("%s :: Entry \n", __FUNCTION__));

    // Stop the network
    stop_network ();

    // close the modem device
    result = mm_modem_disable_sync (g_pmodemDevice->modemObj, NULL, &error);
    if (!result)
    {
        CcspTraceError(("%s :: FATAL!! Unable to enable the modem \n", __FUNCTION__));
    }

    g_clear_signal_handler(&g_handler_id,g_pmodemDevice->modemObj);
    // Now the free the contexts
    free_modem_context ();

    // This is an additional check, in case multiple modem support is enabled
    if (g_pmodemDevice)
    {
        // Free the memory
        free (g_pmodemDevice);
        g_pmodemDevice = NULL;
    }

    CcspTraceInfo (("%s :: Exit \n", __FUNCTION__));
}

// Function to enable the mode, this routine results in modem registering with the network
static void open_device ()
{
    MMModemPortInfo *port_infos;
    guint n_port_infos = 0;
    guint i;
    const char *value = NULL;
    const char **msisdn = NULL;
    gboolean result;
    GError *error = NULL;

    CcspTraceInfo (("%s :: Entry \n", __FUNCTION__));

    // Open the modem device
    result = mm_modem_enable_sync (g_pmodemDevice->modemObj, NULL, &error);
    if (!result)
    {
        CcspTraceError (("%s - FATAL!! Unable to enable the modem!! \n", __FUNCTION__));
        return;
    }

    // Listen modem signals
    g_handler_id = g_signal_connect (g_pmodemDevice->modemObj, "state-changed", G_CALLBACK (state_changed), NULL);

    // Get the modem properties
    if ((value = mm_modem_get_model (g_pmodemDevice->modemObj)) != NULL)
        strncpy (g_pmodemDevice->info.model, value, strlen (value) + 1);

    if ((value = mm_modem_get_manufacturer (g_pmodemDevice->modemObj)) != NULL)
        strncpy (g_pmodemDevice->info.vendor, value, strlen (value) + 1);

    if ((value = mm_modem_get_revision (g_pmodemDevice->modemObj)) != NULL)
        strncpy (g_pmodemDevice->info.firmwareVersion, value,
                 strlen (value) + 1);

    if ((value = mm_modem_get_primary_port (g_pmodemDevice->modemObj)) != NULL)
        strncpy (g_pmodemDevice->info.primaryPort, value, strlen (value) + 1);

    if ((value = mm_modem_get_hardware_revision (g_pmodemDevice->modemObj)) != NULL)
        strncpy (g_pmodemDevice->info.hardwareVersion, value, strlen (value) + 1);

    if ((msisdn = (const char**) mm_modem_get_own_numbers (g_pmodemDevice->modemObj)) != NULL)
        strncpy (g_pmodemDevice->info.MSISDN, *msisdn, strlen (*msisdn) + 1);

    MMModem3gpp *modem_3gpp = mm_object_get_modem_3gpp (g_pmodemDevice->mmObj);
    if (modem_3gpp)
    {
        if ((value = mm_modem_3gpp_get_imei (modem_3gpp)) != NULL)
            strncpy (g_pmodemDevice->info.IMEI, value, strlen (value) + 1);
    }
    g_object_unref (modem_3gpp);

    // Now get the port name
    mm_modem_get_ports (g_pmodemDevice->modemObj, &port_infos, &n_port_infos);
    for (i = 0; i < n_port_infos; i++)
    {
        switch (port_infos[i].type)
        {
        case MM_MODEM_PORT_TYPE_NET:
            AnscCopyString (g_portName, port_infos[i].name);
            break;
        default:
            break;
        }
        CcspTraceInfo (("%s :: Port Type :: %d,  Port Name :: %s \n", __FUNCTION__, port_infos[i].type, port_infos[i].name));
    }
    mm_modem_port_info_array_free (port_infos, n_port_infos);

    CcspTraceInfo (("%s :: Got Port Name :: %s \n", __FUNCTION__, g_portName));

    // Now update the right interface name
    if (g_deviceStatusCB.device_open_status_cb)
    {
        g_deviceStatusCB.device_open_status_cb (g_portName, g_portName,
                                                 DEVICE_OPEN_STATUS_READY,
						 CELLULAR_MODEM_SET_ONLINE);
        CcspTraceInfo (("%s :: Callback DEVICE_OPEN_STATUS_READY \n", __FUNCTION__));
    }

    CcspTraceInfo (("%s :: Exit \n", __FUNCTION__));
}

// Function to select the device slot, this method is supposed to select the SIM
// TODO:: Not implemented yet
static void select_device_slot ()
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    if (g_pmodemDevice->modemState >= MODEM_STATE_REGISTERED)
    {
        CcspTraceInfo (("%s - already registered \n", __FUNCTION__));
        if (g_slot_status_cb)
        {
            CcspTraceInfo (("%s - calling slot status ready callback \n", __FUNCTION__));
            g_slot_status_cb ("slot1", "logical_slot1", 1, DEVICE_SLOT_STATUS_READY);
        }
    }
    CcspTraceInfo (("%s :: Exit \n", __FUNCTION__));
}

// Function to register the modem with the network, in the current implementation
// this is already handled when Modem is enabled in "open_device", so just return
// the callback
static void monitor_device_registration ()
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    if (g_registration_status_cb)
    {
        g_registration_status_cb (DEVICE_NAS_STATUS_REGISTERED,
                                  DEVICE_NAS_STATUS_ROAMING_OFF,
                                  CELLULAR_MODEM_REGISTERED_SERVICE_CS_PS);
    }
    CcspTraceInfo (("%s :: Exit \n", __FUNCTION__));
}

// Function to create the bearer
static void profile_create ()
{
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMBearerProperties *properties = NULL;
    BearerAllowedAuth bearerAuth = BEARER_ALLOWED_AUTH_NONE;
    BearerIpFamily ipFamily = BEARER_IP_FAMILY_IPV4V6;
    CellularDeviceProfileSelectionStatus_t profileStatus =  DEVICE_PROFILE_STATUS_NOT_READY;
    CHAR bearerIndex[IDX_SIZE], *bearPath;

    CellularProfileStruct *pstProfileInput = g_pstProfileInput;

    properties = mm_bearer_properties_new ();
    if (properties)
    {
        if (pstProfileInput->APN != NULL)
            mm_bearer_properties_set_apn (properties, pstProfileInput->APN);

        if (pstProfileInput->PDPAuthentication
                == CELLULAR_PDP_AUTHENTICATION_PAP)
            bearerAuth = BEARER_ALLOWED_AUTH_PAP;
        else if (pstProfileInput->PDPAuthentication
                == CELLULAR_PDP_AUTHENTICATION_CHAP)
            bearerAuth = BEARER_ALLOWED_AUTH_CHAP;

        mm_bearer_properties_set_allowed_auth (properties, bearerAuth);
        if (bearerAuth != BEARER_ALLOWED_AUTH_NONE)
        {
            mm_bearer_properties_set_user (properties,
                                           pstProfileInput->Username);
            mm_bearer_properties_set_password (
                    properties, pstProfileInput->Password);
        }
        if (pstProfileInput->PDPType == CELLULAR_PDP_TYPE_IPV4)
            ipFamily = BEARER_IP_FAMILY_IPV4;
        else if (pstProfileInput->PDPType == CELLULAR_PDP_TYPE_IPV6)
            ipFamily = BEARER_IP_FAMILY_IPV6;

        mm_bearer_properties_set_ip_type (properties, ipFamily);
        mm_bearer_properties_set_allow_roaming (properties, pstProfileInput->bIsNoRoaming);

        // Allocate memory for bearer
        g_pmodemDevice->pBearer[g_pmodemDevice->numberOfBearers] = calloc (1, sizeof(Bearer));

        g_pmodemDevice->pBearer[g_pmodemDevice->numberOfBearers]->bearerObj =
                mm_modem_create_bearer_sync (g_pmodemDevice->modemObj, properties, NULL, &error);
        if (g_pmodemDevice->pBearer[g_pmodemDevice->numberOfBearers]->bearerObj  == NULL)
        {
            CcspTraceError (("Error: couldn't create new bearer: '%s'\n", error ? error->message : "unknown error"));
            if (error)
            {
                if (strstr (error->message, "already reached maximum"))
                {
                    profileStatus = DEVICE_PROFILE_STATUS_READY;
                    res = RETURN_OK;
                }
                g_error_free (error);
            }

            g_clear_object (&properties);
            // free the bearer memory
            free (g_pmodemDevice->pBearer[g_pmodemDevice->numberOfBearers]);
        }
        else
        {
            bearPath = mm_bearer_get_path (g_pmodemDevice->pBearer[g_pmodemDevice->numberOfBearers]->bearerObj);
            AnscCopyString (g_pmodemDevice->pBearer[g_pmodemDevice->numberOfBearers]->dbusPath, bearPath);
            strncpy (bearerIndex, bearPath + strlen (BEARER_PATH_PREFIX), IDX_SIZE - 1);
            pstProfileInput->ProfileID = atoi (bearerIndex);
            profileStatus = DEVICE_PROFILE_STATUS_READY;
            g_pmodemDevice->pBearer[g_pmodemDevice->numberOfBearers]->bearerProperties = properties;
            g_pmodemDevice->numberOfBearers++;
            CcspTraceInfo (("%s: Created bearer, bearer count :: %d", __FUNCTION__, g_pmodemDevice->numberOfBearers));
        }

        g_profileStatusCb ("profile", pstProfileInput->PDPType, profileStatus);
    }
}

// Function to delete the bearer
// TODO:: Fix this to take index
static void profile_delete ()
{
    GError *error = NULL;
    INT res = RETURN_ERROR;
    gboolean result;
    const gchar bearPath[64] =
        { 0 };
    const gchar **bearer_paths;

    CellularProfileStruct *pstProfileInput = g_pstProfileInput;

    if (pstProfileInput)
        snprintf (bearPath, sizeof(bearPath) - 1, "%s%d", BEARER_PATH_PREFIX,
                  pstProfileInput->ProfileID);
    else
    {
        bearer_paths = (const gchar**) mm_modem_get_bearer_paths (g_pmodemDevice->modemObj);
        if (bearer_paths && bearer_paths[0])
            strncpy (bearPath, *bearer_paths, sizeof(bearPath) - 1);
    }

    // Find the bearer in the currently opened entries
    int currindex = 0, removeindex = MAX_BEARER_COUNT;
    for (currindex = 0; currindex < g_pmodemDevice->numberOfBearers;
            currindex++)
    {
        if (AnscEqualString (bearPath, g_pmodemDevice->pBearer[currindex]->dbusPath, TRUE))
        {
            removeindex = currindex;

            // Found the bearer in our list
            result = mm_modem_delete_bearer_sync (g_pmodemDevice->modemObj, bearPath, NULL, &error);
            if (!result)
            {
                CcspTraceError (("Error: couldn't delete the bearer: '%s'\n", error ? error->message : "unknown error"));
                if (error)
                    g_error_free (error);
            }
            else
            {
                if (g_profileStatusCb)
                    g_profileStatusCb ("unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_DELETED);
            }

            // Now free the memory of the bearer
            g_clear_object (
                    &(g_pmodemDevice->pBearer[currindex]->bearerProperties));
            g_object_unref (
                    g_pmodemDevice->pBearer[currindex]->bearerObj);
            free (g_pmodemDevice->pBearer[currindex]);
            g_pmodemDevice->pBearer[currindex] = NULL;
        }

        // adjust the pointers
        if (removeindex != MAX_BEARER_COUNT)
        {
            g_pmodemDevice->pBearer[currindex] =
                    g_pmodemDevice->pBearer[currindex + 1];
        }
    }

    // This indicates we removed a bearer so, decrement count
    if (removeindex != MAX_BEARER_COUNT)
        g_pmodemDevice->numberOfBearers--;
}
// Delete the profile and re-create, this have
// the same effect as modify
// TODO:: Fix it, use index
static void profile_modify ()
{
    profile_delete ();
    profile_create ();
}

// Get the bearer info for the index provided
static bool get_bearer_info (CHAR *index)
{
    GError *error = NULL;
    INT res = RETURN_ERROR;
    MMBearerProperties *properties = NULL;
    MMBearerIpConfig *ipv4_config = NULL;
    MMBearerIpConfig *ipv6_config = NULL;
    const char *value = NULL;
    const char **Dns = NULL;

    unsigned int currindex = *index;

    // Find the bearer in the currently opened entries
    if (g_pmodemDevice->pBearer[currindex]->bearerObj)
    {
        properties = g_pmodemDevice->pBearer[currindex]->bearerProperties;
        if (properties)
        {
            g_pmodemDevice->pBearer[currindex]->info.type =
                    mm_bearer_get_bearer_type (g_pmodemDevice->pBearer[currindex]->bearerObj);
            if ((value = mm_bearer_get_interface (g_pmodemDevice->pBearer[currindex]->bearerObj)) != NULL)
                strncpy (g_pmodemDevice->pBearer[currindex]->info.NetPort,
                        value,
                        sizeof(g_pmodemDevice->pBearer[currindex]->info.NetPort) - 1);
            if ((value = mm_bearer_properties_get_apn (properties)) != NULL)
                strncpy (g_pmodemDevice->pBearer[currindex]->info.Apn,
                        value,
                        sizeof(g_pmodemDevice->pBearer[currindex]->info.Apn) - 1);
            if ((value = mm_bearer_properties_get_user (properties)) != NULL)
                strncpy (g_pmodemDevice->pBearer[currindex]->info.username,
                        value,
                        sizeof(g_pmodemDevice->pBearer[currindex]->info.username) - 1);
            if ((value = mm_bearer_properties_get_password (properties)) != NULL)
                strncpy (g_pmodemDevice->pBearer[currindex]->info.password,
                        value,
                        sizeof(g_pmodemDevice->pBearer[currindex]->info.password) - 1);

            g_pmodemDevice->pBearer[currindex]->info.Status =
                    (mm_bearer_get_connected (
                            g_pmodemDevice->pBearer[currindex]->bearerObj) ?
                            TRUE : FALSE);
            g_pmodemDevice->pBearer[currindex]->info.IpAddressFamily =
                    mm_bearer_properties_get_ip_type (properties);
            g_pmodemDevice->pBearer[currindex]->info.Roaming =
                    mm_bearer_properties_get_allow_roaming (properties);
            res = RETURN_OK;
        }

        if ((value = mm_bearer_get_interface ( g_pmodemDevice->pBearer[currindex]->bearerObj)) != NULL)
            strncpy (g_pmodemDevice->pBearer[currindex]->info.WANIFName,
                    value,
                    sizeof(g_pmodemDevice->pBearer[currindex]->info.WANIFName) - 1);

        ipv4_config = mm_bearer_get_ipv4_config (g_pmodemDevice->pBearer[currindex]->bearerObj);
        if (ipv4_config)
        {
            g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Method =  mm_bearer_ip_config_get_method (ipv4_config);
            if (g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Method != BEARER_IP_METHOD_UNKNOWN)
            {
                if ((value = mm_bearer_ip_config_get_address (ipv4_config)) != NULL)
                    strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv4Info.IpAddress,
                            value,
                            sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv4Info.IpAddress) - 1);
                if ((value = mm_bearer_ip_config_get_gateway (ipv4_config)) != NULL)
                    strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Gateway,
                            value,
                            sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Gateway) - 1);
                if ((Dns = mm_bearer_ip_config_get_dns (ipv4_config)) != NULL)
                {
                    strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv4Info.DnsPri,
                            *Dns,
                            sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv4Info.DnsPri) - 1);
                    if (*(Dns + 1) != NULL)
                        strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv4Info.DnsSec,
                                *(Dns + 1),
                                sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv4Info.DnsSec) - 1);
                }
                g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Prefix = mm_bearer_ip_config_get_prefix (ipv4_config);
                g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Mtu = mm_bearer_ip_config_get_mtu (ipv4_config);
		CcspTraceInfo(("%s %d- VALUE of g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Mtu:<%d>",__FUNCTION__,__LINE__,g_pmodemDevice->pBearer[currindex]->info.IPv4Info.Mtu));
            }
            g_clear_object (&ipv4_config);
        }

        ipv6_config = mm_bearer_get_ipv6_config (g_pmodemDevice->pBearer[currindex]->bearerObj);
        if (ipv6_config)
        {
            g_pmodemDevice->pBearer[currindex]->info.IPv6Info.Method = mm_bearer_ip_config_get_method (ipv6_config);
            if (g_pmodemDevice->pBearer[currindex]->info.IPv6Info.Method != BEARER_IP_METHOD_UNKNOWN)
            {
                if ((value = mm_bearer_ip_config_get_address (ipv6_config)) != NULL)
                    strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv6Info.IpAddress,
                            value,
                            sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv6Info.IpAddress) - 1);
                if ((value = mm_bearer_ip_config_get_gateway (ipv6_config)) != NULL)
                    strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv6Info.Gateway,
                            value,
                            sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv6Info.Gateway) - 1);
                if ((Dns = mm_bearer_ip_config_get_dns (ipv6_config)) != NULL)
                {
                    strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv6Info.DnsPri,
                            *Dns,
                            sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv6Info.DnsPri) - 1);
                    if (*(Dns + 1) != NULL)
                        strncpy (g_pmodemDevice->pBearer[currindex]->info.IPv6Info.DnsSec,
                                *(Dns + 1),
                                sizeof(g_pmodemDevice->pBearer[currindex]->info.IPv6Info.DnsSec) - 1);
                }
                g_pmodemDevice->pBearer[currindex]->info.IPv6Info.Prefix = mm_bearer_ip_config_get_prefix (ipv6_config);
                g_pmodemDevice->pBearer[currindex]->info.IPv6Info.Mtu = mm_bearer_ip_config_get_mtu (ipv6_config);
            }
            g_clear_object (&ipv6_config);
        }
    }

}

// Function to get the connected bearer info.
// TODO:: This can be potentially merged with the above function.
static unsigned int get_connected_bearer_info (MMModem *modem, CellularIPStruct *ipStruct)
{
    GError *error = NULL;
    MMBearerProperties *properties = NULL;
    MMBearerIpConfig *ipv4_config = NULL;
    MMBearerIpConfig *ipv6_config = NULL;
    const char *value = NULL;
    const char **Dns = NULL;
    BearerIpFamily ipFamily;
    CHAR ip[16] = { 0 };

    CcspTraceInfo (("%s: Finding first connected bearer, current bearer count :: %d \n", __FUNCTION__, g_pmodemDevice->numberOfBearers));

    // Find the bearer in the currently opened entries
    int currindex = 0, foundindex = MAX_BEARER_COUNT;
    for (currindex = 0; currindex < g_pmodemDevice->numberOfBearers; currindex++)
    {
        char *interfaceName = mm_bearer_get_interface (g_pmodemDevice->pBearer[currindex]->bearerObj);
        if (interfaceName != NULL)
        {
            foundindex = currindex;
            CcspTraceInfo (("%s: Found a connected bearer at index %d", __FUNCTION__, foundindex));
            break;
        }
    }

    // found a connected bearer
    if (foundindex != MAX_BEARER_COUNT)
    {
        properties = g_pmodemDevice->pBearer[foundindex]->bearerProperties;
        if (properties)
        {
            ipFamily = mm_bearer_properties_get_ip_type (properties);
            if (ipFamily == BEARER_IP_FAMILY_IPV4 || ipFamily == BEARER_IP_FAMILY_IPV4V6)
            {
                memset (ipStruct, 0, sizeof(CellularIPStruct));
                if ((value = mm_bearer_get_interface (g_pmodemDevice->pBearer[foundindex]->bearerObj)) != NULL)
                    strncpy (ipStruct->WANIFName, value, sizeof(ipStruct->WANIFName) - 1);
                ipStruct->IPType = CELLULAR_NETWORK_IP_FAMILY_IPV4;
                ipv4_config = mm_bearer_get_ipv4_config (g_pmodemDevice->pBearer[foundindex]->bearerObj);
                if (ipv4_config)
                {
                    if (mm_bearer_ip_config_get_method (ipv4_config) != BEARER_IP_METHOD_UNKNOWN)
                    {
                        if ((value = mm_bearer_ip_config_get_address (ipv4_config)) != NULL)
                            strncpy (ipStruct->IPAddress,
                                    value,
                                    sizeof(ipStruct->IPAddress) - 1);
                        if ((value = mm_bearer_ip_config_get_gateway (ipv4_config)) != NULL)
                            strncpy (ipStruct->DefaultGateWay,
                                    value,
                                    sizeof(ipStruct->DefaultGateWay) - 1);
                        if ((Dns = mm_bearer_ip_config_get_dns (ipv4_config)) != NULL)
                        {
                            strncpy (ipStruct->DNSServer1,
                                    *Dns,
                                    sizeof(ipStruct->DNSServer1) - 1);
                            if (*(Dns + 1) != NULL)
                                strncpy (ipStruct->DNSServer2,
                                        *(Dns + 1),
                                        sizeof(ipStruct->DNSServer2) - 1);
                        }
                        convertSubnetMask (mm_bearer_ip_config_get_prefix (ipv4_config), ip, 16);
                        strncpy (ipStruct->SubnetMask,
                                ip,
                                sizeof(ipStruct->SubnetMask) - 1);
                        ipStruct->MTUSize = mm_bearer_ip_config_get_mtu (ipv4_config);
			CcspTraceInfo (("%s - Entry ipStruct->MTUSize:<%d> \n", __FUNCTION__, ipStruct->MTUSize));
                    }
                    g_clear_object (&ipv4_config);
                }
            }

            if (ipFamily == BEARER_IP_FAMILY_IPV6 || ipFamily == BEARER_IP_FAMILY_IPV4V6)
            {
                memset (ipStruct, 0, sizeof(CellularIPStruct));
                if ((value = mm_bearer_get_interface (g_pmodemDevice->pBearer[foundindex]->bearerObj)) != NULL)
                    strncpy (ipStruct->WANIFName, value, sizeof(ipStruct->WANIFName) - 1);
                ipStruct->IPType = CELLULAR_NETWORK_IP_FAMILY_IPV6;
                ipv6_config = mm_bearer_get_ipv6_config (g_pmodemDevice->pBearer[foundindex]->bearerObj);
                if (ipv6_config)
                {
                    if (mm_bearer_ip_config_get_method (ipv6_config) != BEARER_IP_METHOD_UNKNOWN)
                    {
                        if ((value = mm_bearer_ip_config_get_address (ipv6_config)) != NULL)
                            strncpy (ipStruct->IPAddress,
                                    value,
                                    sizeof(ipStruct->IPAddress) - 1);
                        if ((value = mm_bearer_ip_config_get_gateway (ipv6_config)) != NULL)
                            strncpy (ipStruct->DefaultGateWay,
                                    value,
                                    sizeof(ipStruct->DefaultGateWay) - 1);
                        if ((Dns = mm_bearer_ip_config_get_dns (ipv6_config)) != NULL)
                        {
                            strncpy (ipStruct->DNSServer1,
                                    *Dns,
                                    sizeof(ipStruct->DNSServer1) - 1);
                            if (*(Dns + 1) != NULL)
                                strncpy (ipStruct->DNSServer2,
                                        *(Dns + 1),
                                        sizeof(ipStruct->DNSServer2) - 1);
                        }
                        convertSubnetMask (mm_bearer_ip_config_get_prefix (ipv6_config), ip, 16);
                        strncpy (ipStruct->SubnetMask,
                                ip,
                                sizeof(ipStruct->SubnetMask) - 1);
                        ipStruct->MTUSize =
                                mm_bearer_ip_config_get_mtu (ipv6_config);
                    }
                    g_clear_object (&ipv6_config);
                }
            }
        }
    }

    return foundindex;
}

// Function to start the network, this routine connects to the bearer
// TODO:: Currently the first index is being used, REVISIT
static void start_network ()
{
    // TODO:: REVISIT, not considering the profile input, assuming only bearer created
    mm_bearer_connect_sync (g_pmodemDevice->pBearer[0]->bearerObj, NULL, NULL);
}

// Function to stop the network, this routine dis-connects to the bearer
// TODO:: Currently the first index is being used, REVISIT
static void stop_network ()
{
    // TODO:: REVISIT, not considering the profile input, assuming only bearer created
    if ((g_pmodemDevice->modemState == MM_MODEM_STATE_CONNECTED)
            && (g_pmodemDevice->pBearer[0] != NULL)
            && (g_pmodemDevice->pBearer[0]->bearerObj != NULL))
    {


        mm_bearer_disconnect_sync (g_pmodemDevice->pBearer[0]->bearerObj, NULL, NULL);
    }
}

// Glib main loop,from this thread context glib mm functions will be executed.
static gpointer gmainloop (gpointer data)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    GMainContext *loop_context = (GMainContext*) data;
    g_mmManagerCancellable = g_cancellable_new ();
    g_mmManagerloop = g_main_loop_new (loop_context, FALSE);
    g_main_context_push_thread_default (loop_context);
    g_main_loop_run (g_mmManagerloop);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
}

// Callback called when a Modem object is detected, this function creates a new
// entry for the modem
// TODO:: add the modem to a table, if support for multiple modems is required.
static void device_added (MMManager *manager, MMObject *obj)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // Allocate memory for this modem
    g_pmodemDevice = (Modem*) calloc (1, sizeof(Modem));

    // Get MMModem
    g_pmodemDevice->mmObj = g_object_ref (obj);
    g_pmodemDevice->modemObj = mm_object_get_modem (g_pmodemDevice->mmObj);
    g_pmodemDevice->modemState = mm_modem_get_state (g_pmodemDevice->modemObj);
    AnscCopyString (g_pmodemDevice->dbusPath, mm_object_get_path (g_pmodemDevice->modemObj));

    // TODO:: Serialize with is modem present
    modemPresent = true;

    CcspTraceInfo (("%s - Added modem with path :: %s in state :: %d \n", __FUNCTION__, mm_object_get_path (
                    obj), mm_modem_get_state (g_pmodemDevice->modemObj)));
}

// Callback called when a Modem object is removed, here inform the state machine
// and do the clean up
static void device_removed (MMManager *manager, MMObject *obj)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // TODO:: Serialize with is modem present
    modemPresent = false;

    // Send the deivce removed to the state machine
    if (g_slot_status_cb)
    {
        g_slot_status_cb ("unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY);
        g_slot_status_cb = NULL;
    }

    if (g_profileStatusCb)
    {
        g_profileStatusCb ("unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_NOT_READY);
        g_profileStatusCb = NULL;
    }

    if (g_deviceStatusCB.device_open_status_cb && g_deviceStatusCB.device_remove_status_cb)
    {
        CcspTraceInfo (("%s %d - Sending device removed to State Machine.... \n", __FUNCTION__, __LINE__));
        g_deviceStatusCB.device_open_status_cb (g_portName, g_portName, DEVICE_OPEN_STATUS_NOT_READY,CELLULAR_MODEM_SET_OFFLINE);
        g_deviceStatusCB.device_remove_status_cb (g_portName, DEVICE_REMOVED);
        g_deviceStatusCB.device_open_status_cb = NULL;
        g_deviceStatusCB.device_remove_status_cb = NULL;
    }

    CcspTraceInfo (("%s - Removed modem with path :: %s in state :: %d \n", __FUNCTION__, mm_object_get_path (
                    g_pmodemDevice->mmObj), mm_modem_get_state (
                    g_pmodemDevice->modemObj)));

    // Close the device
    close_device ();

    g_main_loop_quit (g_mmManagerloop);

    // Clear pointer references.
    g_object_unref (g_mmManagerCancellable);
    g_object_unref (g_mmManagerObj);
    g_object_unref (g_dbusConnection);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
}

// Function to enable/disable air plane mode, on the modem
static unsigned int set_airplaneMode (CHAR *index, BOOLEAN enable)
{
    GError *error = NULL;
    gboolean result;
    INT res = RETURN_ERROR;

    if (enable == TRUE)
        result = mm_modem_set_power_state_sync (g_pmodemDevice->modemObj,
                                                MM_MODEM_POWER_STATE_LOW, NULL,
                                                &error);
    else
        result = mm_modem_set_power_state_sync (g_pmodemDevice->modemObj,
                                                MM_MODEM_POWER_STATE_ON, NULL,
                                                &error);

    if (!result)
    {
        CcspTraceError (("error: couldn't set new power state in the modem: '%s'\n",
                        error ? error->message : "unknown error"));
        if (error)
            g_error_free (error);
    }
    else
        res = RETURN_OK;

    return res;
}

// Helper thread to asynchronously get the bearer updated properites (ip configuration) and
// and bring up the network interface
static void mm_configure_interface_thread (MMModem *modem)
{
    CellularIPStruct ipStruct;
    unsigned char index = MAX_BEARER_COUNT;
    char subnetmask[32] = { 0 };

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    memset (&ipStruct, 0, sizeof(CellularIPStruct));
    CcspTraceInfo (("%s :: Wait infinitely for bearer to get updated \n", __FUNCTION__));
    while (index == MAX_BEARER_COUNT)
    {
        sleep (1); // This sleep is for the bearer status to change to connected
        index = get_connected_bearer_info (modem, &ipStruct);
    }

    if (index != MAX_BEARER_COUNT)
    {
            CcspTraceInfo(("%s :: Getting bearer info at index %d \n", __FUNCTION__, index));
            // This indicates we have a connected bearer details
            // TODO:: REVISIT Current logic fetches the first connected bearer.
            get_bearer_info(&index);

            convertSubnetMask(g_pmodemDevice->pBearer[index]->info.IPv4Info.Prefix, subnetmask, 16);

        if (g_networkStatusCB.device_network_ip_ready_cb)
        {
	    // Fill the ipstruct params
	    strncpy (ipStruct.WANIFName, g_pmodemDevice->pBearer[index]->info.WANIFName,
			sizeof(ipStruct.WANIFName) - 1);
            ipStruct.IPType = CELLULAR_NETWORK_IP_FAMILY_IPV4;

            g_networkStatusCB.device_network_ip_ready_cb (&ipStruct, DEVICE_NETWORK_IP_READY);
            g_networkStatusCB.packet_service_status_cb ("mm", ipStruct.IPType, DEVICE_NETWORK_STATUS_CONNECTED);
        }
    }

    //Cleanup current thread when exit
    pthread_exit (NULL);
}

// Callback, that gets the notification when the state of the Modem Manager changes
static void state_changed (MMModem *modem, MMModemState old_state, MMModemState new_state,
               MMModemStateChangeReason reason)
{
    CcspTraceInfo (("%s :: Entry \n", __FUNCTION__));

    CcspTraceInfo (
            ("%s: State changed, '%s' --> '%s'\n", mm_modem_get_path (
                    g_pmodemDevice->modemObj), mm_modem_state_get_string (
                    old_state), mm_modem_state_get_string (new_state)));

    g_pmodemDevice->modemState = new_state;

    if (new_state == old_state)
        return;

    // Handle transitions to a lower state in state machine
    if (new_state < old_state)
    {
        switch (new_state)
        {
            case MM_MODEM_STATE_ENABLED:
                if (g_deviceStatusCB.device_open_status_cb)
                    g_deviceStatusCB.device_open_status_cb (g_portName, g_portName,  DEVICE_OPEN_STATUS_NOT_READY,CELLULAR_MODEM_SET_OFFLINE);

                if (g_slot_status_cb)
                    g_slot_status_cb ("unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY);
                break;
            case MM_MODEM_STATE_REGISTERED:
                if (g_networkStatusCB.packet_service_status_cb)
                {
                    g_networkStatusCB.packet_service_status_cb ( "unknown", CELLULAR_NETWORK_IP_FAMILY_IPV4,  DEVICE_NETWORK_STATUS_DISCONNECTED);
                    g_networkStatusCB.packet_service_status_cb ( "unknown", CELLULAR_NETWORK_IP_FAMILY_IPV6,  DEVICE_NETWORK_STATUS_DISCONNECTED);
                }
                break;
            case MODEM_STATE_DISABLING:
            case MODEM_STATE_DISABLED:
            case MODEM_STATE_LOCKED:
            case MODEM_STATE_FAILED:
                if (g_slot_status_cb)
                    g_slot_status_cb ("unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY);
                if (g_profileStatusCb)
                    g_profileStatusCb ("unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_NOT_READY);
                if (g_deviceStatusCB.device_open_status_cb && g_deviceStatusCB.device_remove_status_cb)
                {
                    g_deviceStatusCB.device_open_status_cb (g_portName, g_portName, DEVICE_OPEN_STATUS_NOT_READY,CELLULAR_MODEM_SET_OFFLINE);
                    g_deviceStatusCB.device_remove_status_cb (g_portName, DEVICE_REMOVED);
                }
                break;
            default:
                CcspTraceInfo (("Unhandled State!! :: %s \n", mm_modem_state_get_string (new_state)));
                break;
        }

        if (old_state == MODEM_STATE_CONNECTED)
        {
            CellularIPStruct ipStruct = { 0 };
            if (g_networkStatusCB.device_network_ip_ready_cb)
                g_networkStatusCB.device_network_ip_ready_cb (&ipStruct, DEVICE_NETWORK_IP_NOT_READY);
        }
    }

    // Handle transitions to a higher state in state machine
    if (new_state > old_state)
    {
        switch (new_state)
        {
            case MM_MODEM_STATE_LOCKED:
                // TODO:: Implement SIM unlocking here
                break;
            case MM_MODEM_STATE_DISABLED:
                break;
            case MM_MODEM_STATE_ENABLED:
                break;
            case MM_MODEM_STATE_REGISTERED:
                if (g_slot_status_cb)
                    g_slot_status_cb ("slot1", "logical_slot1", 1, DEVICE_SLOT_STATUS_READY);
                break;
            case MM_MODEM_STATE_CONNECTED:
                // Start a thread and bring up the interface
                pthread_create (&g_configure_interface_thread, NULL, &mm_configure_interface_thread,
                                (MMModem*) modem);
                break;
            default:
                CcspTraceInfo (("Unhandled State!! :: %s \n", mm_modem_state_get_string (new_state)));
                break;
        }
    }

    CcspTraceInfo (("%s :: Exit \n", __FUNCTION__));
}

// Thread to process commands from state machine on a separate thread context
static void* modem_command_processor_thread (void *arg)
{
    int rc;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    //detach thread from caller stack
    pthread_detach (pthread_self ());

    CcspTraceInfo (("%s - Started Modem Command Processor monitoring \n", __FUNCTION__));

    while (g_run_command_processor_thread)
    {
        // acquire the lock
        pthread_mutex_lock (&g_stateLock);
        // Wait on the signal
        pthread_cond_wait (&g_condWait, &g_stateLock);

        switch (g_commandModem)
        {
            case MODEM_OPEN_DEVICE:
                open_device ();
                break;
            case MODEM_CLOSE_DEVICE:
                close_device ();
                break;
            case MODEM_MONITOR_DEVICE_REGISTRATION:
                monitor_device_registration ();
                break;
            case MODEM_PROFILE_CREATE:
                profile_create ();
                break;
            case MODEM_PROFILE_DELETE:
                profile_delete ();
                break;
            case MODEM_PROFILE_MODIFY:
                profile_modify ();
                break;
            case MODEM_SELECT_DEVICE_SLOT:
                select_device_slot ();
                break;
            case MODEM_START_NETWORK:
                start_network ();
                break;
            case MODEM_STOP_NETWORK:
                stop_network ();
                break;
        }

        // release the lock
        pthread_mutex_unlock (&g_stateLock);
    }

    //Cleanup current thread when exit
    pthread_exit (NULL);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

/**********************************************************************
                HAL IMPLEMENTATION
**********************************************************************/
int cellular_hal_modem_init (CellularContextInitInputStruct *pstCtxInputStruct)
{
    GError *error = NULL;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    g_dbusConnection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!g_dbusConnection)
    {
        CcspTraceError (("%s - FATAL!! Could not get DBUS connection \n", __FUNCTION__));
        return RETURN_ERROR;
    }

    g_mmManagerObj = mm_manager_new_sync (g_dbusConnection, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_DO_NOT_AUTO_START, NULL, &error);
    if (!g_mmManagerObj)
    {
        CcspTraceError (("%s - FATAL!! Could not get handle to Modem Manager \n", __FUNCTION__));
        g_object_unref (g_dbusConnection);
        return RETURN_ERROR;
    }

    // Create a new thread for glib
    g_thread_new ("GLib Loop Thread", gmainloop,
                  g_main_context_get_thread_default ());

    // Listen to new modems
    g_signal_connect (g_mmManagerObj, "object-added", G_CALLBACK (device_added),
                      NULL);
    // Listen to new modems
    g_signal_connect (g_mmManagerObj, "object-removed",
                      G_CALLBACK (device_removed), NULL);

    //Initialize the thread for asynchronous command processing from state machine.
    pthread_create (&g_command_processor_thread, NULL,
                    &modem_command_processor_thread, (void*) NULL);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

unsigned int cellular_hal_modem_IsModemDevicePresent (void)
{
    return modemPresent;
}

int cellular_hal_modem_open_device (CellularDeviceContextCBStruct *pstDeviceCtxCB)
{
    gboolean result;
    GError *error = NULL;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (pstDeviceCtxCB) 
    {
        g_deviceStatusCB.device_remove_status_cb = pstDeviceCtxCB->device_remove_status_cb;
        g_deviceStatusCB.device_open_status_cb   = pstDeviceCtxCB->device_open_status_cb;
    }

    // Update the command
    g_commandModem = MODEM_OPEN_DEVICE;

    CcspTraceInfo (("%s - Signal the MODEM command thread - OPEN DEVICE \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

unsigned char cellular_hal_modem_IsModemControlInterfaceOpened (void)
{
    // TODO:: REVISIT
    return TRUE;
}

int cellular_hal_modem_select_device_slot (
        cellular_device_slot_status_api_callback device_slot_status_cb)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_slot_status_cb == NULL)
    {
        g_slot_status_cb = device_slot_status_cb;
    }

    // Update the command
    g_commandModem = MODEM_SELECT_DEVICE_SLOT;

    CcspTraceInfo (("%s - Signal the MODEM command thread - SELECT DEVICE SLOT \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit 1 \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_sim_power_enable (unsigned int slot_id, unsigned char enable)
{
    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // TODO:: Implement the SIM power enable

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    // TODO:: REVISIT
    return RETURN_OK;
}

int cellular_hal_modem_get_total_no_of_uicc_slots (unsigned int *total_count)
{
    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        // TODO:: REVISIT, currently hardcoding to 1 SIM Slot
        g_pmodemDevice->numberOfSIMs = 1;
        *total_count = g_pmodemDevice->numberOfSIMs;
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    return RETURN_OK;
}

int cellular_hal_modem_get_uicc_slot_info (unsigned int slot_index,
                                       CellularUICCSlotInfoStruct *pstSlotInfo)
{
    GError *error = NULL;
    char *value = NULL;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // TODO:: REVISIT we are only handling 1 SIM now, and below conditional check ensures we dont repeated read
    if (g_pmodemDevice && g_pmodemDevice->pSIM[0] == NULL)
    {
        g_pmodemDevice->pSIM[0] = (SIM*) calloc (1, sizeof(SIM));
        // TODO :: REVISIT we are not considering the slot_index here
        g_pmodemDevice->pSIM[0]->simObj = mm_modem_get_sim_sync (g_pmodemDevice->modemObj, NULL, &error);
        if (g_pmodemDevice->pSIM[0]->simObj)
        {
            if ((value = mm_sim_get_identifier (g_pmodemDevice->pSIM[0]->simObj)) != NULL)
            {
                strncpy (pstSlotInfo->iccid, value, sizeof(pstSlotInfo->iccid) - 1);
                strncpy (g_pmodemDevice->pSIM[0]->info.ICCID,
                        value,
                        sizeof(g_pmodemDevice->pSIM[0]->info.ICCID) - 1);
            }
            if ((value = mm_sim_get_operator_name (g_pmodemDevice->pSIM[0]->simObj)) != NULL)
            {
                strncpy (pstSlotInfo->MnoName, value, sizeof(pstSlotInfo->MnoName) - 1);
                strncpy (g_pmodemDevice->pSIM[0]->info.MNOName,
                        value,
                        sizeof(g_pmodemDevice->pSIM[0]->info.MNOName) - 1);
            }
            pstSlotInfo->Status = CELLULAR_UICC_STATUS_VALID;
            pstSlotInfo->CardEnable = TRUE;
        }
        else
        {
            if (error)
            {
                CcspTraceError (("Error: couldn't get sim in modem '%s': '%s'\n", mm_modem_get_path (g_pmodemDevice->modemObj), error->message));
                g_error_free (error);
            }
        }
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_active_card_status (CellularUICCStatus_t *card_status)
{
    ModemStateFailedReason reason;

    //CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        if (g_pmodemDevice->modemState == MODEM_STATE_FAILED)
            reason = mm_modem_get_state_failed_reason (g_pmodemDevice->modemObj);
        else
            reason = MODEM_STATE_FAILED_REASON_NONE;

        if (reason == MODEM_STATE_FAILED_REASON_SIM_MISSING
                || reason == MODEM_STATE_FAILED_REASON_UNKNOWN)
            *card_status = CELLULAR_UICC_STATUS_EMPTY;
        else if (reason == MODEM_STATE_FAILED_REASON_SIM_ERROR)
            *card_status = CELLULAR_UICC_STATUS_ERROR;
        else
            *card_status = CELLULAR_UICC_STATUS_VALID;

        if (g_pmodemDevice->modemState == MODEM_STATE_LOCKED)
            *card_status = CELLULAR_UICC_STATUS_BLOCKED;
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    //CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_monitor_device_registration (
        cellular_device_registration_status_callback device_registration_status_cb)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    //Send NAS registration status if CB is not null
    if ((g_registration_status_cb == NULL)
            && (device_registration_status_cb != NULL))
    {
        g_registration_status_cb = device_registration_status_cb;
    }

    // Update the command
    g_commandModem = MODEM_MONITOR_DEVICE_REGISTRATION;

    CcspTraceInfo (("%s - Signal the MODEM command thread - MONITOR DEVICE REGISTRATION \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_profile_create (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    //Copy profile ready status CB
    if ((NULL != device_profile_status_cb) && (g_profileStatusCb == NULL))
    {
        g_profileStatusCb = device_profile_status_cb;
    }

    // Update the profile currently being processed
    g_pstProfileInput = pstProfileInput;

    // Update the command
    g_commandModem = MODEM_PROFILE_CREATE;

    CcspTraceInfo (("%s - Signal the MODEM command thread - MONITOR PROFILE CREATE \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_profile_delete (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    //Copy profile ready status CB
    if ((NULL != device_profile_status_cb) && (g_profileStatusCb == NULL))
    {
        g_profileStatusCb = device_profile_status_cb;
    }

    // Update the profile currently being processed
    g_pstProfileInput = pstProfileInput;

    // Update the command
    g_commandModem = MODEM_PROFILE_DELETE;

    CcspTraceInfo (("%s - Signal the MODEM command thread - MONITOR PROFILE DELETE \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_profile_modify (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    //Copy profile ready status CB
    if ((NULL != device_profile_status_cb) && (g_profileStatusCb == NULL))
    {
        g_profileStatusCb = device_profile_status_cb;
    }

    // Update the profile currently being processed
    g_pstProfileInput = pstProfileInput;

    // Update the command
    g_commandModem = MODEM_PROFILE_MODIFY;

    CcspTraceInfo (("%s - Signal the MODEM command thread - MONITOR PROFILE MODIFY \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_profile_list (CellularProfileStruct **pstProfileOutput,
                                     int *profile_count)
{
    MMBearerProperties *properties = NULL;
    const char *value = NULL;
    CellularProfileStruct *pfStruct = NULL;
    BearerIpFamily ipFamily;
    CHAR bearerIndex[IDX_SIZE], *bearPath;
    guint totalCount = 0, i = 0;
    GError *error = NULL;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        totalCount = g_pmodemDevice->numberOfBearers;
        pfStruct = (CellularProfileStruct*) malloc (
                sizeof(CellularProfileStruct) * totalCount);
        memset (pfStruct, 0, sizeof(CellularProfileStruct) * totalCount);
        for (i = 0; i < totalCount; i++)
        {
            MMBearer *bearer = g_pmodemDevice->pBearer[i]->bearerObj;
            properties = mm_bearer_get_properties (bearer);
            if (properties)
            {
                if (mm_bearer_get_bearer_type (bearer) == BEARER_TYPE_DEFAULT)
                    pfStruct[i].bIsThisDefaultProfile = TRUE;
                if ((value = mm_bearer_properties_get_apn (properties)) != NULL)
                    strncpy (pfStruct[i].APN, value, sizeof(pfStruct[i].APN) - 1);
                if ((value = mm_bearer_properties_get_user (properties)) != NULL)
                    strncpy (pfStruct[i].Username, value, sizeof(pfStruct[i].Username) - 1);
                if ((value = mm_bearer_properties_get_password (properties)) != NULL)
                    strncpy (pfStruct[i].Password, value, sizeof(pfStruct[i].Password) - 1);

                pfStruct[i].bIsAPNDisabled = (mm_bearer_get_connected (bearer) ? FALSE : TRUE);

                ipFamily = mm_bearer_properties_get_ip_type (properties);
                if (ipFamily == BEARER_IP_FAMILY_IPV4)
                    pfStruct[i].PDPType = CELLULAR_PDP_TYPE_IPV4;
                else if (ipFamily == BEARER_IP_FAMILY_IPV6)
                    pfStruct[i].PDPType = CELLULAR_PDP_TYPE_IPV6;
                else
                    pfStruct[i].PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;

                pfStruct[i].bIsNoRoaming = (mm_bearer_properties_get_allow_roaming (properties) ? FALSE : TRUE);

                bearPath = mm_bearer_get_path (bearer);
                strncpy (bearerIndex, bearPath + strlen (BEARER_PATH_PREFIX), IDX_SIZE - 1);
                pfStruct[i].ProfileID = atoi (bearerIndex);
                g_clear_object (&properties);
            }
        }

        *pstProfileOutput = pfStruct;
        *profile_count = totalCount;
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_start_network (CellularNetworkIPType_t ip_request_type,
                                  CellularProfileStruct *pstProfileInput,
                                  CellularNetworkCBStruct *pstCBStruct)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    memcpy (&g_networkStatusCB, pstCBStruct, sizeof(CellularNetworkCBStruct));

    // Update the command
    g_commandModem = MODEM_START_NETWORK;

    CcspTraceInfo (("%s - Signal the MODEM command thread - START NETWORK \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_stop_network (CellularNetworkIPType_t ip_request_type)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // Update the command
    g_commandModem = MODEM_STOP_NETWORK;

    CcspTraceInfo (("%s - Signal the MODEM command thread - STOP NETWORK \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_signal_info (CellularSignalInfoStruct *radioSignal)
{
    GError *error = NULL;
    MMSignal *signal;
    MMModemSignal *modem_signal;
    gdouble value;
    gboolean result;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        modem_signal = mm_object_get_modem_signal (g_pmodemDevice->mmObj);
        if (modem_signal)
        {
            if (g_refreshRate == 0)
            {
                g_refreshRate = 10;
                result = mm_modem_signal_setup_sync (modem_signal,
                                                     g_refreshRate, NULL,
                                                     &error);
                if (!result)
                {
                    CcspTraceError (("error: couldn't setup extended signal information retrieval: '%s'\n",
                                    error ? error->message : "unknown error"));
                    if (error)
                        g_error_free (error);
                    g_refreshRate = 0;
                }
            }

            if( (signal = mm_modem_signal_peek_gsm (modem_signal)) != NULL ||
                    (signal = mm_modem_signal_peek_umts (modem_signal)) != NULL )
            {
                if ((value = mm_signal_get_rssi (signal)) != MM_SIGNAL_UNKNOWN)
                radioSignal->RSSI = (int)value;
            }
            else if( (signal = mm_modem_signal_peek_lte (modem_signal)) != NULL
	#ifdef TEST5G_SUPPORT 
                || (signal = mm_modem_signal_peek_nr5g (modem_signal)) != NULL
        #endif
	       	)
            {   //Both LTE and NR use the same libmm APIs.
                if ((value = mm_signal_get_rssi (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->RSSI = (int)value;
                if ((value = mm_signal_get_rsrq (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->RSRQ = (int)value;
                if ((value = mm_signal_get_rsrp (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->RSRP = (int)value;
                if ((value = mm_signal_get_snr (signal)) != MM_SIGNAL_UNKNOWN)
                    radioSignal->SNR = (int)value;
            }
            g_clear_object (&signal);
        }

        g_object_unref (modem_signal);
    }
    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_set_modem_operating_configuration (
        CellularModemOperatingConfiguration_t modem_operating_config)
{
    INT res = RETURN_ERROR;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        if (modem_operating_config == CELLULAR_MODEM_SET_OFFLINE
                || modem_operating_config == CELLULAR_MODEM_SET_LOW_POWER_MODE)
        {
            res = set_airplaneMode (NULL, TRUE);
        }
        else if (modem_operating_config == CELLULAR_MODEM_SET_ONLINE)
        {
            res = set_airplaneMode (NULL, FALSE);
        }
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    //Modem Manager doesn't set the usb-modem to low power mode, so always returning as true. 
    return RETURN_OK;
}

int cellular_hal_modem_get_device_imei (char *imei)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        AnscCopyString (imei, g_pmodemDevice->info.IMEI);
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_device_imei_sv (char *imei_sv)
{
    // TODO:: REVISIT
    return RETURN_OK;
}

int cellular_hal_modem_get_modem_current_iccid (char *iccid)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice && g_pmodemDevice->pSIM[0])
    {
        AnscCopyString (iccid, g_pmodemDevice->pSIM[0]->info.ICCID);
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_modem_current_msisdn (char *msisdn)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        AnscCopyString (msisdn, g_pmodemDevice->info.MSISDN);
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_packet_statistics (
        CellularPacketStatsStruct *network_packet_stats)
{
    MMBearerStats *stats = NULL;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // TODO:: REVISIT Get from the first bearer
    if (g_pmodemDevice && g_pmodemDevice->pBearer[0])
    {
        stats = mm_bearer_get_stats (g_pmodemDevice->pBearer[0]->bearerObj);
        if (stats)
        {
            network_packet_stats->BytesReceived =
                    mm_bearer_stats_get_rx_bytes (stats);
            network_packet_stats->BytesSent =
                    mm_bearer_stats_get_tx_bytes (stats);
            g_clear_object (&stats);
        }
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_modem_get_current_modem_interface_status (
        CellularInterfaceStatus_t *status)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        if (g_pmodemDevice->modemState > MODEM_STATE_ENABLING)
            *status = IF_UP;
        else if (g_pmodemDevice->modemState == MODEM_STATE_UNKNOWN)
            *status = IF_UNKNOWN;
        else if (g_pmodemDevice->modemState == MODEM_STATE_FAILED)
            *status = IF_ERROR;
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_modem_set_modem_network_attach (void)
{
    //ToDo: need check with MTK provide network attach and detach, but this has not affected operation of the state machine.
    return RETURN_OK;
}

int cellular_hal_modem_set_modem_network_detach (void)
{
    //ToDo: need check with MTK provide network attach and detach, but this has not affected operation of the state machine.
    return RETURN_OK;
}

int cellular_hal_modem_get_modem_firmware_version (char *firmware_version)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        AnscCopyString (firmware_version, g_pmodemDevice->info.firmwareVersion);
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
}

int cellular_hal_modem_get_current_plmn_information (
        CellularCurrentPlmnInfoStruct *plmnInfo)
{
    GError *error = NULL;
    INT res = RETURN_ERROR;
    const char *value = NULL;
    Modem3gppRegistrationState state;
    MMLocation3gpp *location_3gpp = NULL;
    MMModem3gpp *modem_3gpp = NULL;
    MMModemLocation *modem_location = NULL;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        modem_3gpp = mm_object_get_modem_3gpp (g_pmodemDevice->mmObj);
        if (modem_3gpp)
        {
            state = mm_modem_3gpp_get_registration_state (modem_3gpp);
            if (state == MM_MODEM_3GPP_REGISTRATION_STATE_ROAMING)
                plmnInfo->roaming_enabled = TRUE;
            else
                plmnInfo->roaming_enabled = FALSE;
            if ((value = mm_modem_3gpp_get_operator_name (modem_3gpp)) != NULL)
                strncpy (plmnInfo->plmn_name, value,
                         sizeof(plmnInfo->plmn_name) - 1);
        }

        modem_location = mm_object_get_modem_location (g_pmodemDevice->mmObj);
        if (modem_location)
        {
            mm_modem_location_get_full_sync (modem_location, &location_3gpp,
                                             NULL, NULL, NULL, NULL, &error);

            if (error)
            {
                CcspTraceInfo (("error: couldn't get location from the modem: '%s'\n",
                                error ? error->message : "unknown error"));
                g_error_free (error);
            }
            else
            {
                if (location_3gpp)
                {
                    plmnInfo->MCC =
                            mm_location_3gpp_get_mobile_country_code (
                                    location_3gpp);
                    plmnInfo->MNC =
                            mm_location_3gpp_get_mobile_network_code (
                                    location_3gpp);
                    plmnInfo->area_code =
                            mm_location_3gpp_get_tracking_area_code (
                                    location_3gpp);
                    plmnInfo->cell_id = mm_location_3gpp_get_cell_id (
                            location_3gpp);
                    g_clear_object (&location_3gpp);
                    res = RETURN_OK;
                }
            }
        }

        if (modem_3gpp)
            g_object_unref (modem_3gpp);

        if (modem_location)
            g_object_unref (modem_location);
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return res;
}

int cellular_hal_modem_get_available_networks_information (
        CellularNetworkScanResultInfoStruct **network_info,
        unsigned int *total_network_count)
{
    // TODO:: REVISIT
    return RETURN_OK;
}

int cellular_hal_modem_get_current_access_technology (char *accessTech)
{
    ModemAccessTechnology modemAccessTech = MODEM_ACCESS_TECHNOLOGY_UNKNOWN;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        modemAccessTech = mm_modem_get_access_technologies (
                g_pmodemDevice->modemObj);
        if (modemAccessTech <= MODEM_ACCESS_TECHNOLOGY_EDGE)
            strcpy (accessTech, MM_MODE_2G);
        else if (modemAccessTech <= MODEM_ACCESS_TECHNOLOGY_EVDOB
                && modemAccessTech > MODEM_ACCESS_TECHNOLOGY_EDGE)
            strcpy (accessTech, MM_MODE_3G);
        else if (modemAccessTech == MODEM_ACCESS_TECHNOLOGY_LTE)
            strcpy (accessTech, MM_MODE_4G);
        else if (modemAccessTech == MODEM_ACCESS_TECHNOLOGY_5GNR)
            strcpy (accessTech, MM_MODE_5G);
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_supported_access_technologies (char *access_technology)
{
    MMModemModeCombination *modes = NULL;
    guint n_modes = 0;
    UINT i, len = 0;
    CHAR buf[32] = { 0 };

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        mm_modem_get_supported_modes (g_pmodemDevice->modemObj, &modes, &n_modes);
        for (i = 0; i < n_modes; i++)
        {
            if (modes[i].allowed & MODEM_MODE_2G)
                len += snprintf (buf + len, sizeof(buf) - 1, "%s", MM_MODE_2G);

            if (modes[i].allowed & MODEM_MODE_3G)
                len += snprintf (buf + len, sizeof(buf) - 1, ",%s", MM_MODE_3G);

            if (modes[i].allowed & MODEM_MODE_4G)
                len += snprintf (buf + len, sizeof(buf) - 1, ",%s", MM_MODE_4G);

            if (modes[i].allowed & MODEM_MODE_5G)
                len += snprintf (buf + len, sizeof(buf) - 1, ",%s", MM_MODE_5G);
        }

        if (strlen (buf))
        {
            strncpy (access_technology, buf, strlen (buf) + 1);
        }

        if (modes)
        {
            g_free (modes);
        }
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

int cellular_hal_modem_get_prefered_access_technologies (char *access_technology)
{
    MMModemMode allowed;
    MMModemMode preferred;
    UINT len = 0;
    CHAR buf[32] =  { 0 };

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        mm_modem_get_current_modes (g_pmodemDevice->modemObj, &allowed, &preferred);
        if (preferred & MODEM_MODE_2G)
            len += snprintf (buf + len, sizeof(buf) - 1, "%s", MM_MODE_2G);

        if (preferred & MODEM_MODE_3G)
            len += snprintf (buf + len, sizeof(buf) - 1, ",%s", MM_MODE_3G);

        if (preferred & MODEM_MODE_4G)
            len += snprintf (buf + len, sizeof(buf) - 1, ",%s", MM_MODE_4G);

        if (preferred & MODEM_MODE_5G)
            len += snprintf (buf + len, sizeof(buf) - 1, ",%s", MM_MODE_5G);

        if (strlen (buf))
        {
            strncpy (access_technology, buf, strlen (buf) + 1);
        }
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_modem_get_device_information (CellularDeviceInfoStruct *devInfo)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    if (g_pmodemDevice != NULL) {
        AnscCopyString (devInfo->Model, g_pmodemDevice->info.model);
        AnscCopyString (devInfo->Vendor, g_pmodemDevice->info.vendor);
        // TODO::If firmware updates are supported, then fetch this from the modem here
        AnscCopyString (devInfo->CurrentImageVersion, g_pmodemDevice->info.firmwareVersion);
        AnscCopyString (devInfo->HardwareRevision, g_pmodemDevice->info.hardwareVersion);
        AnscCopyString (devInfo->Msisdn, g_pmodemDevice->info.MSISDN);
        AnscCopyString (devInfo->Imei, g_pmodemDevice->info.IMEI);
    }

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_modem_get_data_interface (char *data_interface)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    char *pinterface = NULL;
    // acquire the lock
    //pthread_mutex_lock (&g_stateLock);

    if ((pinterface = mm_bearer_get_interface ( g_pmodemDevice->pBearer[0]->bearerObj)) != NULL)
    {

        CcspTraceInfo (("%s - if pinterface not null\n", __FUNCTION__));
        CcspTraceInfo (("%s - pinterface:<%s>\n", __FUNCTION__,pinterface));
	strncpy (g_pmodemDevice->pBearer[0]->info.WANIFName,pinterface,sizeof(g_pmodemDevice->pBearer[0]->info.WANIFName) - 1);
    }
    // Do not return the interface name before, a profile is created
    if (g_pmodemDevice && g_pmodemDevice->pBearer[0])
    {
        AnscCopyString (data_interface, g_pmodemDevice->pBearer[0]->info.WANIFName);
    }

    // release the lock
   // pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_modem_set_device_props (VOID *devInfo, int dev_status)
{
    if (dev_status == 1)
    {
        // Driver loaded for a modem, nothing to be done here
        // wait for modem manager to detect and initialize, it will send a "object add" event
    }
    else
    {
        // Driver unloaded for the modem, this is an async event, so inform the state machine
        if (g_pmodemDevice != NULL)
        {
            if (g_slot_status_cb)
                g_slot_status_cb ("unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY);
            if (g_profileStatusCb)
                g_profileStatusCb ("unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_NOT_READY);
            if (g_deviceStatusCB.device_open_status_cb && g_deviceStatusCB.device_remove_status_cb)
            {
                g_deviceStatusCB.device_open_status_cb (g_portName, g_portName, DEVICE_OPEN_STATUS_NOT_READY,CELLULAR_MODEM_SET_OFFLINE);
                g_deviceStatusCB.device_remove_status_cb (g_portName, DEVICE_REMOVED);
            }

            CcspTraceInfo (("%s - USB MODEM Device Removed \n", __FUNCTION__));
        }
    }

    return RETURN_OK;
}
#endif
