/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
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

#ifdef FEATURE_RNDIS_HAL
#include <unistd.h>
#include <inttypes.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

#include <sysevent/sysevent.h>
#include "syscfg/syscfg.h"
#include "secure_wrapper.h"

#include "ansc_platform.h"
#include "ccsp_trace.h"
#include "cellular_hal_rndis_apis.h"

extern int sysevent_fd;
extern token_t sysevent_token;

// TODO:: In this file
// 1. Synchronize handling of udev and libusb events
// 2. Add exception handling for junk callback pointers

// USB Context of the connected device
static RNDISDevice g_connectedRNDIS;
static RNDISCommand_t g_commandRNDIS = RNDIS_NO_COMMAND;

static CellularDeviceContextCBStruct g_stDeviceCtxCB;
static cellular_device_registration_status_callback g_device_registration_status_cb;
static cellular_device_profile_status_api_callback g_device_profile_status_cb;
static cellular_device_slot_status_api_callback g_device_slot_status_cb;
static CellularNetworkCBStruct g_networkStatusCB;
static CellularProfileStruct g_stProfileInput;

// libusb context
static libusb_context *g_libUSBContext = NULL;
static libusb_hotplug_callback_handle g_generic_callback_handle;

// Globals to store the thread handles and run variables
static char g_wanInterfaceName[16];
static BOOL g_run_command_processor_thread = true;
static pthread_t g_command_processor_thread;

// Declaration of thread condition variable
static pthread_cond_t g_condWait = PTHREAD_COND_INITIALIZER;
// Mutex to serialize acccess to state machine state
static pthread_mutex_t g_stateLock = PTHREAD_MUTEX_INITIALIZER;

int cellular_hal_rndis_set_device_props (VOID *interface, int dev_status)
{
    if (dev_status == 1 && interface != NULL)
    {
        AnscCopyString (g_connectedRNDIS.InterfaceName, (char *)interface);
        g_connectedRNDIS.Status = RNDIS_DEVICE_ENABLED;
    }
    else
    {
        if (g_connectedRNDIS.Status != RNDIS_DEVICE_DOWN)
        {
            g_connectedRNDIS.Status = RNDIS_DEVICE_DOWN;

            v_secure_system("sh %s %s %s", RUN_DHCP_SCRIPT, "erouter0", "stop");
            // Callbacks
            if (g_device_profile_status_cb)
                g_device_profile_status_cb ("profile", 0, DEVICE_PROFILE_STATUS_NOT_READY);
            if (g_stDeviceCtxCB.device_open_status_cb)
                g_stDeviceCtxCB.device_open_status_cb ("RNDIS", "wwan0", DEVICE_OPEN_STATUS_NOT_READY,CELLULAR_MODEM_SET_OFFLINE);
            if (g_stDeviceCtxCB.device_remove_status_cb)
                g_stDeviceCtxCB.device_remove_status_cb ("wwan0", DEVICE_REMOVED);

            CcspTraceInfo (("%s - USB RNDIS Device Removed \n", __FUNCTION__));
        }
    }

    return RETURN_OK;
}

// Helper function to bring up the interface, rename it to the "erouter0", and run DHCP
// TODO::This routine should be removed once the WAN Manager takes care of bringing up the interface
static int rndis_interface_up (char *wanInterface)
{
    char command[256] =  { 0 };

    // Crude implementation for now
    // Bring down the interface
    v_secure_system("ip link set %s down", g_connectedRNDIS.InterfaceName);

    // Rename the interface
    v_secure_system("ip link set %s name %s", g_connectedRNDIS.InterfaceName, wanInterface);

    // Bring up the renamed interface
    v_secure_system("ip link set %s up", wanInterface);

    // sleep for 100msecs before running dhcp
    usleep (100 * 1000);

    // Run DHCP
    CcspTraceInfo (("%s - Running DHCP \n", __FUNCTION__));
    v_secure_system("sh %s %s %s", RUN_DHCP_SCRIPT, wanInterface, "start");

    // sleep for 100msecs before enabling routing rules
    usleep (100 * 1000);

    return RETURN_OK;
}

// Helper function to bring down the interface, and rename it to the original name
// TODO::This routine should be removed once the WAN Manager takes care of bringing down the interface
static int rndis_interface_down (char *wanInterface)
{
    char command[256] =  { 0 };

    CcspTraceInfo (("%s - Stopping DHCP \n", __FUNCTION__));
    v_secure_system("sh %s %s %s", RUN_DHCP_SCRIPT, wanInterface, "stop");

    sleep (1);
    // Crude implementation for now
    // Bring down the interface
    v_secure_system("ip link set %s down", wanInterface);

    sleep (1);
    // Rename the interface
    v_secure_system("ip link set %s name %s", wanInterface, g_connectedRNDIS.InterfaceName);

    // sleep for 100msecs before running dhcp
    usleep (100 * 1000);

    return RETURN_OK;
}

// Helper function to fetch the VID and PID of the attached/detached USB devivce
static int generic_hotplug_callback (struct libusb_context *ctx, struct libusb_device *dev,
                          libusb_hotplug_event event, void *user_data)
{
    static libusb_device_handle *device_handle = NULL;
    struct libusb_device_descriptor dev_desc;
    int rc;

    (void) libusb_get_device_descriptor (dev, &dev_desc);

    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event)
    {
        CcspTraceInfo (("%s - USB Device Attached - VID %d, PID %d\n", __FUNCTION__, dev_desc.idVendor, dev_desc.idProduct));
        g_connectedRNDIS.VendorID = dev_desc.idVendor;
        g_connectedRNDIS.ProductID = dev_desc.idProduct;
    }
    else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event)
    {
        CcspTraceInfo (("%s - USB Device Detached \n", __FUNCTION__));
        g_connectedRNDIS.VendorID = 0;
        g_connectedRNDIS.ProductID = 0;

        if (g_connectedRNDIS.Status != RNDIS_DEVICE_DOWN)
        {
            g_connectedRNDIS.Status = RNDIS_DEVICE_DOWN;

            // Callbacks
            if (g_device_profile_status_cb)
                g_device_profile_status_cb ("profile", 0, DEVICE_PROFILE_STATUS_NOT_READY);
            if (g_stDeviceCtxCB.device_open_status_cb)
                g_stDeviceCtxCB.device_open_status_cb ("RNDIS", "wwan0", DEVICE_OPEN_STATUS_NOT_READY,CELLULAR_MODEM_SET_OFFLINE);
            if (g_stDeviceCtxCB.device_remove_status_cb)
                g_stDeviceCtxCB.device_remove_status_cb ("wwan0", DEVICE_REMOVED);

            CcspTraceInfo (("%s - USB RNDIS Device Removed \n", __FUNCTION__));
        }
    }
    else
    {
        CcspTraceError (("%s - Unhandled USB Event", __FUNCTION__));
    }

    return RETURN_OK;
}

// Helper function to
// 1. Bring down the interfaces
// 2. Callback the statemachine informing network is down
static int rndis_stop_network ()
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // TODO:: Do the reverse of start network.
    if (g_connectedRNDIS.Status == RNDIS_DEVICE_CONNECTED)
    {
        // If we are coming from connect state, then bring down the
        // network interface
    }

    g_connectedRNDIS.Status = RNDIS_DEVICE_REGISTERED;

    // Callback
    if (g_networkStatusCB.packet_service_status_cb != NULL)
    {
        CcspTraceInfo (
                ("%s - Device callback on stop network..... \n", __FUNCTION__));

        g_networkStatusCB.packet_service_status_cb (
                "unknown", CELLULAR_NETWORK_IP_FAMILY_IPV4,
                DEVICE_NETWORK_STATUS_DISCONNECTED);
        g_networkStatusCB.packet_service_status_cb (
                "unknown", CELLULAR_NETWORK_IP_FAMILY_IPV6,
                DEVICE_NETWORK_STATUS_DISCONNECTED);
    }

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

// Helper function to
// 1. Bring up the interfaces
// 2. Callback the statemachine informing network is up
static int rndis_start_network ()
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    CellularIPStruct *ipStruct = NULL;

    CcspTraceInfo (("%s - Entry g_connectedRNDIS.Status:<%d>\n", __FUNCTION__,g_connectedRNDIS.Status));
    if (g_connectedRNDIS.Status == RNDIS_DEVICE_REGISTERED)
    {
        AnscCopyString (g_wanInterfaceName, "erouter0");

        ipStruct = (CellularIPStruct*) malloc (sizeof(CellularIPStruct));
        memset (ipStruct, 0, sizeof(CellularIPStruct));

        // Fill the ipstruct params
        strncpy (ipStruct->WANIFName, g_connectedRNDIS.InterfaceName,
                 sizeof(ipStruct->WANIFName) - 1);
        ipStruct->IPType = CELLULAR_NETWORK_IP_FAMILY_IPV4;

        g_connectedRNDIS.Status = RNDIS_DEVICE_CONNECTED;

        // Callback
        if ((g_networkStatusCB.device_network_ip_ready_cb != NULL)
                && (g_networkStatusCB.packet_service_status_cb))
        {
            CcspTraceInfo (("%s - Device callback on start network..... \n", __FUNCTION__));

            // TODO:: Fetch the ipstruct and send
            g_networkStatusCB.device_network_ip_ready_cb (ipStruct, DEVICE_NETWORK_IP_READY);
            g_networkStatusCB.packet_service_status_cb ("RNDIS", ipStruct->IPType, DEVICE_NETWORK_STATUS_CONNECTED);
        }

        if (ipStruct)
            free (ipStruct);
    }
    else
    {
        CcspTraceError (("%s %d: RNDIS device in Invalid state, cannot start network\n", __FUNCTION__, __LINE__));
    }

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

// Thread to process commands from state machine on a separate thread context
// All the asynchronous commands from the state machine (which have a callback)
// are handled in this thread.
// We currently use only the Synchronous APIs provided by the Modem Manager (libMM), so
// executing those APIs in this thread context.
// If we choose to use the Async APIs of the Modem Manager, then this thread can be avoided
static void* rndis_command_processor_thread (void *arg)
{
    int rc;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    //detach thread from caller stack
    pthread_detach (pthread_self ());

    CcspTraceInfo (("%s - Started RNDIS Command Processor monitoring \n", __FUNCTION__));

    while (g_run_command_processor_thread)
    {
        // acquire the lock
        pthread_mutex_lock (&g_stateLock);
        // Wait on the signal
        pthread_cond_wait (&g_condWait, &g_stateLock);

        switch (g_commandRNDIS)
            {
            case RNDIS_OPEN_DEVICE:
                // TODO:: REVISIT this
                if (cellular_hal_rndis_IsModemDevicePresent ()
                        && (g_stDeviceCtxCB.device_open_status_cb != NULL))
                {
                    CcspTraceInfo (("%s - Device callback open device..... \n", __FUNCTION__));
                    // Now update the right interface name
                    // The hardcoded name "wwan0" is being sent to allow search in WanManager static configuration, need to revisit this logic
                    g_stDeviceCtxCB.device_open_status_cb ("RNDIS", "wwan0", DEVICE_OPEN_STATUS_READY,CELLULAR_MODEM_SET_ONLINE);
                }
                break;
            case RNDIS_CLOSE_DEVICE:

                break;
            case RNDIS_MONITOR_DEVICE_REGISTRATION:
                if (g_device_registration_status_cb)
                {
                    CcspTraceInfo (("%s - Device callback NAS registered..... \n", __FUNCTION__));

                    g_connectedRNDIS.Status = RNDIS_DEVICE_REGISTERED;
                    g_device_registration_status_cb (
                            DEVICE_NAS_STATUS_REGISTERED,
                            DEVICE_NAS_STATUS_ROAMING_OFF,
                            CELLULAR_MODEM_REGISTERED_SERVICE_CS_PS);
                }
                break;
            case RNDIS_PROFILE_CREATE:
                if (g_device_profile_status_cb)
                {
                    CcspTraceInfo (("%s - Device callback profile created..... \n", __FUNCTION__));
                    g_device_profile_status_cb (
                            "rndis", g_stProfileInput.PDPType,
                            DEVICE_PROFILE_STATUS_READY);
                }
                break;
            case RNDIS_PROFILE_MODIFY:

                break;
            case RNDIS_PROFILE_UPDATE:

                break;
            case RNDIS_SELECT_DEVICE_SLOT:
                if (g_device_slot_status_cb)
                {
                    CcspTraceInfo (("%s - Device callback selected slot..... \n", __FUNCTION__));
                    g_device_slot_status_cb ("slot1", "logical_slot1",
                                             1,
                                             DEVICE_SLOT_STATUS_READY);
                }
                break;
            case RNDIS_START_NETWORK:
                rndis_start_network ();
                break;
            case RNDIS_STOP_NETWORK:
                rndis_stop_network ();
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
               HAL IMPLEMENTATATION
**********************************************************************/
unsigned int cellular_hal_rndis_IsModemDevicePresent (void)
{
    //CcspTraceInfo(("%s - Entry \n", __FUNCTION__));

    // Check if the modem RNDIS device is connected and report
    if (g_connectedRNDIS.Status == RNDIS_DEVICE_ENABLED)
    {
        CcspTraceInfo (("%s - RNDIS Modem Present \n", __FUNCTION__));
        return TRUE;
    }

    //CcspTraceInfo(("%s - Exit \n", __FUNCTION__));
    return FALSE;
}

int cellular_hal_rndis_init (CellularContextInitInputStruct *pstCtxInputStruct)
{
    int rc = 0;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    //Initiate the thread for command processing from state machine.
    pthread_create (&g_command_processor_thread, NULL,
                    &rndis_command_processor_thread, (void*) NULL);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_rndis_open_device (CellularDeviceContextCBStruct *pstDeviceCtxCB)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    // Store the pointer
    g_stDeviceCtxCB = *pstDeviceCtxCB;

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // Update the command
    g_commandRNDIS = RNDIS_OPEN_DEVICE;

    CcspTraceInfo (("%s - Signal the RNDIS command thread - OPEN DEVICE \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

unsigned char cellular_hal_rndis_IsModemControlInterfaceOpened (void)
{
    return TRUE;
}

int cellular_hal_rndis_select_device_slot (
        cellular_device_slot_status_api_callback device_slot_status_cb)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // Store the pointer
    g_device_slot_status_cb = device_slot_status_cb;

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // Update the command
    g_commandRNDIS = RNDIS_SELECT_DEVICE_SLOT;

    CcspTraceInfo (("%s - Signal the RNDIS command thread - SELECT DEVICE SLOT \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_rndis_sim_power_enable (unsigned int slot_id, unsigned char enable)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_total_no_of_uicc_slots (unsigned int *total_count)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_uicc_slot_info (unsigned int slot_index,
                                       CellularUICCSlotInfoStruct *pstSlotInfo)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_active_card_status (CellularUICCStatus_t *card_status)
{
    //CcspTraceInfo(("%s - Entry \n", __FUNCTION__));

    *card_status = CELLULAR_UICC_STATUS_VALID;

    //CcspTraceInfo(("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_rndis_profile_create (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    // Store the callback
    g_device_profile_status_cb = device_profile_status_cb;
    g_stProfileInput = *pstProfileInput;

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // Update the command
    g_commandRNDIS = RNDIS_PROFILE_CREATE;

    CcspTraceInfo (
            ("%s - Signal the RNDIS command thread - CREATE PROFILE \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    return RETURN_OK;
}

int cellular_hal_rndis_profile_delete (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    return RETURN_OK;
}

int cellular_hal_rndis_profile_modify (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_profile_list (CellularProfileStruct **ppstProfileOutput,
                                     int *profile_count)
{
    return RETURN_OK;
}

int cellular_hal_rndis_monitor_device_registration (
        cellular_device_registration_status_callback device_registration_status_cb)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    //Send NAS registration status if CB is not null
    g_device_registration_status_cb = device_registration_status_cb;

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // Update the command
    g_commandRNDIS = RNDIS_MONITOR_DEVICE_REGISTRATION;

    CcspTraceInfo (("%s - Signal the RNDIS command thread - MONITOR DEVICE REGISTRATION \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_rndis_start_network (CellularNetworkIPType_t ip_request_type,
                                  CellularProfileStruct *pstProfileInput,
                                  CellularNetworkCBStruct *pstCBStruct)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // Store the pointer
    g_networkStatusCB = *pstCBStruct;

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // Update the command
    g_commandRNDIS = RNDIS_START_NETWORK;

    CcspTraceInfo (("%s - Signal the RNDIS command thread to - START NETWORK \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_rndis_stop_network (CellularNetworkIPType_t ip_request_type)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_stateLock);

    // Update the command
    g_commandRNDIS = RNDIS_STOP_NETWORK;

    CcspTraceInfo (("%s - Signal the RNDIS command thread - STOP NETWORK \n", __FUNCTION__));

    // Signal the processor thread
    pthread_cond_signal (&g_condWait);

    // release the lock
    pthread_mutex_unlock (&g_stateLock);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

int cellular_hal_rndis_get_signal_info (CellularSignalInfoStruct *signal_info)
{
    return RETURN_OK;
}

int cellular_hal_rndis_set_modem_operating_configuration (
        CellularModemOperatingConfiguration_t modem_operating_config)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_device_imei (char *imei)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_device_imei_sv (char *imei_sv)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_modem_current_iccid (char *iccid)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_modem_current_msisdn (char *msisdn)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_packet_statistics (
        CellularPacketStatsStruct *network_packet_stats)
{
    if ((g_connectedRNDIS.Status == RNDIS_DEVICE_CONNECTED) && network_packet_stats) {
        struct rtnl_link *link;
        struct nl_sock *socket;

        socket = nl_socket_alloc();
        nl_connect(socket, NETLINK_ROUTE);

        // TODO:: REVISIT, the assumption here is erouter0 is the interface created by the
        // Cellular backup
        if (rtnl_link_get_kernel(socket, 0, "erouter0", &link) >= 0) {
            network_packet_stats->PacketsReceived = rtnl_link_get_stat(link, RTNL_LINK_RX_PACKETS);
            network_packet_stats->PacketsSent = rtnl_link_get_stat(link, RTNL_LINK_TX_PACKETS);
            network_packet_stats->BytesReceived = rtnl_link_get_stat(link, RTNL_LINK_RX_BYTES);
            network_packet_stats->BytesSent = rtnl_link_get_stat(link, RTNL_LINK_TX_BYTES);
            network_packet_stats->PacketsReceivedDrop = rtnl_link_get_stat(link, RTNL_LINK_RX_DROPPED);
            network_packet_stats->PacketsSentDrop = rtnl_link_get_stat(link, RTNL_LINK_TX_DROPPED);
            rtnl_link_put(link);

            CcspTraceInfo (("%s - RNDIS Stats Rx Packets :: %lu \n", __FUNCTION__, network_packet_stats->PacketsReceived));
        }

        nl_socket_free(socket);
    }

    return RETURN_OK;
}

int cellular_hal_rndis_get_current_modem_interface_status (
        CellularInterfaceStatus_t *status)
{
    return RETURN_OK;
}

int cellular_hal_rndis_set_modem_network_attach (void)
{
    return RETURN_OK;
}

int cellular_hal_rndis_set_modem_network_detach (void)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_modem_firmware_version (char *firmware_version)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_current_plmn_information (
        CellularCurrentPlmnInfoStruct *plmn_info)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_available_networks_information (
        CellularNetworkScanResultInfoStruct **network_info,
        unsigned int *total_network_count)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_current_access_technology (char *access_technology)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_supported_access_technologies (char *access_technology)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_prefered_access_technologies (char *access_technology)
{
    return RETURN_OK;
}

int cellular_hal_rndis_get_device_information (CellularDeviceInfoStruct *devInfo)
{
    // TODO:: Can we fetch any of the params from an RNDIS device??? till then clear it.
    memset (devInfo, 0, sizeof(CellularDeviceInfoStruct));
    return RETURN_OK;
}

int cellular_hal_rndis_get_data_interface (char *data_interface)
{
    AnscCopyString (data_interface, g_connectedRNDIS.InterfaceName);
    return RETURN_OK;
}
#endif
