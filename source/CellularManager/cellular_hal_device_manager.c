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

#include "ansc_platform.h"
#include "ccsp_trace.h"
#include "cellular_hal_device_manager.h"
#include "cellularmgr_bus_utils.h"

// List of supported USB Dongles, add new dongles and their mode switch strings here
// Note that the current implementation is using mode switch, so the "ModeString" should always be the usb_modeswitch string
// TODO:: Future plans to make it script configurable
static USBDeviceConfig SupportedUSBDeviceTable[] = {
        { "Huawei E353", 0x12d1, 0x1f01, " -v 12d1 -p 1f01 -V 12d1 -P 14db -J" },
        { "Alcatel IK40V", 0x1bbb, 0xf000, " -v 1bbb -p f000 -V 1bbb -P 0195 -M 55534243123456788000000080000606f50402527000000000000000000000" },
        { "Huawei E3372-607", 0x12d1, 0x14fe, " -v 12d1 -p 14fe -V 12d1 -P 14db -J" }
};

static MODEM_DEVICETYPE g_modem_device_type = MODEM_UNKNOWN;

// This holds the currently enabled HAL
DEVICE_HAL_ABSTRATCOR_ g_device_hal;
DeviceStatus_t g_device_status = USB_DEVICE_REMOVED;

// TODO:: In this file
// 1. Synchronize handling of udev and libusb events, to ensure no calls from state machine happen while HAL is being updated.
// 2. Add exception handling for junk callback pointers

CellularContextInitInputStruct g_pstCtxInputStruct;

// Mutex to serialize access to HAL, this mutex will ensure that a HAL update is not done while a HAL call in progress
// With USB Plug-n-Play, it is very much possible that while a HAL call is in progress the device might be yanked out,
// this mutex ensures the call completes before HAL updates
static pthread_mutex_t g_halAccessSerializer = PTHREAD_MUTEX_INITIALIZER;

// libusb context
static libusb_context *g_libUSBContext = NULL;
static libusb_hotplug_callback_handle g_generic_callback_handle;

#define PSM_CELLULARMANAGER_FIRST_USE_TIMESTAMP                   "dmsb.cellularmanager.firstusetimestamp"
#define PSM_CELLULARMANAGER_LAST_USE_TIMESTAMP                    "dmsb.cellularmanager.lastusetimestamp"
static unsigned char g_firstUseTimeStamp[64] = "NA";
static unsigned char g_lastUseTimeStamp[64];

/**********************************************************************
                HELPER FUNCTIONS
**********************************************************************/
// Function to update the usage time stamps
static void updateUsageTimeStamps() {
    struct  timeval  tv;
    struct  tm       *tm;
    char    fmt[ 64 ], buf [64] = {0};
    int     retPsmSet  = CCSP_SUCCESS;

    gettimeofday( &tv, NULL );

    if( (tm = localtime( &tv.tv_sec ) ) != NULL)
    {
        strftime( fmt, sizeof( fmt ), "%x - %I:%M%p", tm );
        snprintf( buf, sizeof( buf ), fmt, " UTC");
    }

    if(AnscEqualString(g_firstUseTimeStamp, "NA", TRUE))
    {
        snprintf( g_firstUseTimeStamp, sizeof(buf)-1, "%s", buf);
        // Persist this time only once
        CellularMgr_RdkBus_SetParamValuesToDB(PSM_CELLULARMANAGER_FIRST_USE_TIMESTAMP , g_firstUseTimeStamp);
    }
    snprintf( g_lastUseTimeStamp, sizeof(buf)-1, "%s", buf);
    CellularMgr_RdkBus_SetParamValuesToDB(PSM_CELLULARMANAGER_LAST_USE_TIMESTAMP, g_lastUseTimeStamp);
}

#if defined(FEATURE_RNDIS_HAL) || defined(FEATURE_MODEM_HAL)
// Helper function to switch the mode of the USB dongle attached
// This currently uses the utility usb_modeswitch to switch the configuration
static int configure_usb_dongle (int vid, int pid)
{
    unsigned int numEntries = sizeof(SupportedUSBDeviceTable)/sizeof(SupportedUSBDeviceTable[0]);
    CcspTraceInfo (("%s - Configurations available for %d Devices \n", __FUNCTION__, numEntries));

    // This routine will switch the USB dongle to the required configuration
    for (unsigned int i = 0; i < numEntries; i++)
    {
        if (vid == SupportedUSBDeviceTable[i].VID && pid == SupportedUSBDeviceTable[i].PID)
        {
            CcspTraceInfo (("%s - Configuring %s Dongle \n", __FUNCTION__, SupportedUSBDeviceTable[i].ModelName));
            v_secure_system("usb_modeswitch %s", SupportedUSBDeviceTable[i].ModeString);

            return RETURN_OK;
        }
    }

    CcspTraceWarning (("%s - No Mode Switch USB Dongle VID - 0x%x, PID - 0x%x \n", __FUNCTION__, vid, pid));
    return RETURN_OK;
}

// Helper function to fetch the VID and PID of the attached/detached USB devivce
static int generic_hotplug_callback (struct libusb_context *ctx, struct libusb_device *dev,
                          libusb_hotplug_event event, void *user_data)
{
    static libusb_device_handle *device_handle = NULL;
    struct libusb_device_descriptor dev_desc;

    (void) libusb_get_device_descriptor (dev, &dev_desc);

    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event)
    {
        CcspTraceInfo (("%s - USB Device Attached - VID %d, PID %d\n", __FUNCTION__, dev_desc.idVendor, dev_desc.idProduct));
        configure_usb_dongle (dev_desc.idVendor, dev_desc.idProduct);
    }
    else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event)
    {
        CcspTraceInfo (("%s - USB Device Detached \n", __FUNCTION__));
    }
    else
    {
        CcspTraceError (("%s - Unhandled USB Event", __FUNCTION__));
    }

    return RETURN_OK;
}
#endif

#if defined(FEATURE_RNDIS_HAL) || defined(FEATURE_MODEM_HAL)
// Globals to store the thread handles and run variables
static pthread_t g_usb_eventhandler_thread;
static BOOL g_run_usb_eventhandler_thread = true;
static pthread_t g_udev_eventhandler_thread;
static BOOL g_run_udev_eventhandler_thread = true;
// Helper function to handle all the USB plug and play events
static void* usb_eventhandler_thread (void *arg)
{
    int rc;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    //detach thread from caller stack
    pthread_detach (pthread_self ());

    CcspTraceInfo (("%s - Started USB monitoring \n", __FUNCTION__));

    if (libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG) != 0)
    {
        CcspTraceInfo (("%s - USB Hot plug detection supported \n", __FUNCTION__));
    }
    else
    {
        CcspTraceError (("%s - USB Hot plug detection NOT supported", __FUNCTION__));
    }

    rc = libusb_hotplug_register_callback (
            NULL,
            LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED
                    | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
            LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY,
            LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            generic_hotplug_callback, NULL, &g_generic_callback_handle);
    if (LIBUSB_SUCCESS != rc)
    {
        CcspTraceError (
                ("%s - USB Error creating a hotplug callback", __FUNCTION__));
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    while (g_run_usb_eventhandler_thread)
    {
        libusb_handle_events_completed (NULL, NULL);
        nanosleep (&(struct timespec) { 0, 10000000UL }, NULL);
    }

    libusb_hotplug_deregister_callback (NULL, g_generic_callback_handle);

    libusb_exit (g_libUSBContext);

    //Cleanup current thread when exit
    pthread_exit (NULL);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return RETURN_OK;
}

// Helper thread to handle the udev events
static void* udev_eventhandler_thread (void *arg)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    //detach thread from caller stack
    pthread_detach (pthread_self ());

    struct udev *udev;
    struct udev_device *dev;
    struct udev_monitor *mon;
    int fd;

    CcspTraceInfo (("%s - Started UDEV monitoring \n", __FUNCTION__));

    /* create udev object */
    udev = udev_new ();
    if (!udev)
    {
        CcspTraceError (("%s - FATAL Failed getting handle to UDEV \n", __FUNCTION__));
        return -1;
    }

    mon = udev_monitor_new_from_netlink (udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype (mon, "net", NULL);
    udev_monitor_enable_receiving (mon);
    fd = udev_monitor_get_fd (mon);

    while (g_run_udev_eventhandler_thread)
    {
        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO (&fds);
        FD_SET (fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        ret = select (fd + 1, &fds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET (fd, &fds))
        {
            dev = udev_monitor_receive_device (mon);
            if (dev)
            {
                if (AnscEqualString (udev_device_get_action (dev),
                                     "add", TRUE))
                {
                    char driver_name[64];
                    AnscCopyString (driver_name, udev_device_get_property_value (dev, "ID_USB_DRIVER"));
                    if (((AnscEqualString (driver_name, "ipheth", TRUE))
                            || (AnscEqualString (driver_name, "cdc_ether", TRUE))
                            || (AnscEqualString (driver_name, "rndis_host", TRUE)))
                            && (g_device_status == USB_DEVICE_REMOVED)) // Ensure we only support one modem at a time
                    {
                        #ifdef FEATURE_RNDIS_HAL
                        CcspTraceInfo (("%s - USB RNDIS Device Initalized \n", __FUNCTION__));

                        g_modem_device_type = MODEM_USBRNDIS;

                        // acquire the lock
                        pthread_mutex_lock (&g_halAccessSerializer);

                        //Enable USB RNDIS HAL
                        memcpy (&g_device_hal, &rndis_device_hal, sizeof(rndis_device_hal));
                        g_device_hal.hal_init (&g_pstCtxInputStruct);
                        g_device_hal.hal_set_device_props (udev_device_get_property_value (dev, "INTERFACE"), 1);

                        g_device_status = USB_DEVICE_ATTACHED;
                        // release the lock
                        pthread_mutex_unlock (&g_halAccessSerializer);
                        #endif
                    }
                    else if (((AnscEqualString (driver_name, "huawei_cdc_ncm", TRUE)) // TODO:: Add support for other drivers
                            || (AnscEqualString (driver_name, "cdc_ncm", TRUE))
                            || (AnscEqualString (driver_name, "qmi_wwan", TRUE))
                            || (AnscEqualString (driver_name, "cdc_mbim", TRUE)))
                            && (g_device_status == USB_DEVICE_REMOVED))
                    {
                        #ifdef FEATURE_MODEM_HAL
                        CcspTraceInfo (("%s - USB MODEM Device Initalized \n", __FUNCTION__));

                        g_modem_device_type = MODEM_USBMODEM;

                        // acquire the lock
                        pthread_mutex_lock (&g_halAccessSerializer);

                        //Enable USB MODEM HAL
                        memcpy (&g_device_hal, &modem_device_hal, sizeof(modem_device_hal));
                        g_device_hal.hal_init (&g_pstCtxInputStruct);
                        g_device_hal.hal_set_device_props ("", 1);

                        g_device_status = USB_DEVICE_ATTACHED;
                        // release the lock
                        pthread_mutex_unlock (&g_halAccessSerializer);
                        #endif
                    }
                }

                if (AnscEqualString (udev_device_get_action (dev), "remove", TRUE))
                    {
                        char driver_name[64];
                        AnscCopyString (driver_name, udev_device_get_property_value (dev, "ID_USB_DRIVER"));
                        if (((AnscEqualString (driver_name, "ipheth", TRUE))
                                || (AnscEqualString (driver_name, "cdc_ether", TRUE))
                                || (AnscEqualString (driver_name, "rndis_host", TRUE)))
                                && (g_device_status == USB_DEVICE_ATTACHED))
                            {
                                #ifdef FEATURE_RNDIS_HAL
                                CcspTraceInfo (("%s - USB RNDIS Device Removed, clearing HAL \n", __FUNCTION__));

                                g_modem_device_type = MODEM_UNKNOWN;

                                // acquire the lock
                                pthread_mutex_lock (&g_halAccessSerializer);

                                g_device_hal.hal_set_device_props ("", 0);
                                memset (&g_device_hal, 0, sizeof(rndis_device_hal));

                                g_device_status = USB_DEVICE_REMOVED;

                                // release the lock
                                pthread_mutex_unlock (&g_halAccessSerializer);
                                #endif      
                            }
                        else if (((AnscEqualString (driver_name, "huawei_cdc_ncm", TRUE)) // TODO:: Add support for other drivers
                                || (AnscEqualString (driver_name, "cdc_ncm", TRUE))
                                || (AnscEqualString (driver_name, "qmi_wwan", TRUE))
                                || (AnscEqualString (driver_name, "cdc_mbim", TRUE)))
                                && (g_device_status == USB_DEVICE_ATTACHED))
                            {
                                #ifdef FEATURE_MODEM_HAL
                                CcspTraceInfo (("%s - USB MODEM Device Removed, clearing HAL \n", __FUNCTION__));

                                g_modem_device_type = MODEM_UNKNOWN;

                                // acquire the lock
                                pthread_mutex_lock (&g_halAccessSerializer);

                                // Modem Manager currently has two issues,
                                // 1. When a USB modem is unplugged and re-plugged, it is not able to open /dev/ttyUSB0
                                // 2. When a USB modem is unplugged Modem Manager is not sending the "device-removed" notification
                                // to address the above two issues, we are currently restarting the modem manager when a USB modem is unplugged.
                                v_secure_system("systemctl restart ModemManager");
                                sleep(2);

                                g_device_hal.hal_set_device_props ("", 0);
                                memset (&g_device_hal, 0, sizeof(modem_device_hal));

                                g_device_status = USB_DEVICE_REMOVED;

                                // release the lock
                                pthread_mutex_unlock (&g_halAccessSerializer);
                                #endif
                            }
                    }

                /* free dev */
                udev_device_unref (dev);
            }
        }
        /* 500 milliseconds */
        usleep (500 * 1000);
    }

    /* free udev */
    udev_unref (udev);

    //Cleanup current thread when exit
    pthread_exit (NULL);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}

// Helper function to reset the USB port, this will be required for error handling
static int usb_helper_dev_reset (void)
{
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // Check if any devices are already connected during boot and reset
    // this is required for multiple reasons
    // 1. To detach the cellular connection if already active on the dongle
    // 2. To control the USB device configuration
    // 3. To enable/block specific USB devices
    libusb_device **list = NULL;
    int rc = 0;
    ssize_t count = 0;

    // Get the list of devices connected
    count = libusb_get_device_list (g_libUSBContext, &list);
    if (count > 0)
    {
        for (size_t idx = 0; idx < count; ++idx)
        {
            libusb_device *device = list[idx];
            struct libusb_device_descriptor dev_desc = { 0 };

            rc = libusb_get_device_descriptor (device, &dev_desc);
            if (rc == 0)
            {
                // Got the device descriptor, now check if it is a Communication Device Class (CDC)
                // and reset it
                CcspTraceInfo (("%s - Vendor:Device = %04x:%04x \n", __FUNCTION__, dev_desc.idVendor, dev_desc.idProduct));
                CcspTraceInfo (("%s - DeviceClass:SubClass:Protocol = %04x:%04x:%04x \n", __FUNCTION__, dev_desc.bDeviceClass, dev_desc.bDeviceSubClass, dev_desc.bDeviceProtocol));
                if (dev_desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE || dev_desc.bDeviceClass == LIBUSB_CLASS_COMM)
                { // reset any device connected, avoid hubs
                  // Open the device handle from device
                    libusb_device_handle *devh;
                    if (libusb_open (device, &devh) == 0)
                    {
                        libusb_reset_device (devh);
                        libusb_close (devh);
                        CcspTraceInfo (("%s - SUCCESS!!Successfully reset the device Vendor:Device = %04x:%04x \n", __FUNCTION__, dev_desc.idVendor, dev_desc.idProduct));
                    }
                    else
                    {
                        CcspTraceError (("%s - FATAL!! Could not open the device handle!!\n", __FUNCTION__));
                    }
                }
            }
        }
    }
    else
    {
        CcspTraceInfo (("%s - No USB devices connected during boot!!  \n", __FUNCTION__));
    }

    libusb_free_device_list (list, count);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return RETURN_OK;
}
#endif

int cellular_hal_device_init (CellularContextInitInputStruct *pstCtxInputStruct)
{
    int rc = 0, ret = RETURN_OK;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // Read the PSM default value
    int retPsmGet = CCSP_SUCCESS;
    char param_value[256];
    char param_name[512];
    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_FIRST_USE_TIMESTAMP);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
       AnscCopyString(g_firstUseTimeStamp, param_value);
       CcspTraceInfo (("%s - First use timestamp read from PSM : %s \n", __FUNCTION__, g_firstUseTimeStamp));
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_CELLULARMANAGER_LAST_USE_TIMESTAMP);
    retPsmGet = CellularMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
       AnscCopyString(g_lastUseTimeStamp, param_value);
       CcspTraceInfo (("%s - Last use timestamp read from PSM : %s \n", __FUNCTION__, g_lastUseTimeStamp));
    }

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    // Initialize the HAL to NULL
    memset (&g_device_hal, 0, sizeof(DEVICE_HAL_ABSTRATCOR_));

    memcpy (&g_pstCtxInputStruct, pstCtxInputStruct,
            sizeof(g_pstCtxInputStruct));

    rc = libusb_init (&g_libUSBContext);
    if (rc == 0)
    {

#if defined(FEATURE_RNDIS_HAL) || defined(FEATURE_MODEM_HAL)
        //Initiate the thread for USB Event handling
        pthread_create (&g_usb_eventhandler_thread, NULL,
                        &usb_eventhandler_thread, (void*) NULL);

        //Initiate the thread for UDEV Event handling
        pthread_create (&g_udev_eventhandler_thread, NULL,
                        &udev_eventhandler_thread, (void*) NULL);

        // Now that listeners are setup, reset the USB port, this will bring the device to a known state
        // and our listeners will catch the device.
        usb_helper_dev_reset ();
#endif
    }
    else
    {
        CcspTraceError (("%s - FATAL!!USB Event Notifications not supported \n", __FUNCTION__));
        ret = RETURN_ERROR;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return ret;
}

unsigned int cellular_hal_device_IsModemDevicePresent (void)
{
    unsigned int ret = FALSE;
    //CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_IsModemDevicePresent)
        ret = g_device_hal.hal_IsModemDevicePresent ();

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    //CcspTraceInfo (("%s - Exit \n", __FUNCTION__));
    return ret;
}

unsigned char cellular_hal_device_IsModemControlInterfaceOpened (void)
{
    unsigned char ret = FALSE;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_IsModemControlInterfaceOpened)
        ret = g_device_hal.hal_IsModemControlInterfaceOpened ();

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_open_device (CellularDeviceContextCBStruct *pstDeviceCtxCB)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_open_device)
        ret = g_device_hal.hal_open_device (pstDeviceCtxCB);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_select_device_slot (
        cellular_device_slot_status_api_callback device_slot_status_cb)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_select_device_slot)
        ret = g_device_hal.hal_select_device_slot (device_slot_status_cb);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_sim_power_enable (unsigned int slot_id,
                                      unsigned char enable)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_sim_power_enable)
        ret = g_device_hal.hal_sim_power_enable (slot_id, enable);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_total_no_of_uicc_slots (unsigned int *total_count)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_total_no_of_uicc_slots)
    {
        ret = g_device_hal.hal_get_total_no_of_uicc_slots (total_count);
    }
    else
    {
        // Set defaults
        *total_count = 0;
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_uicc_slot_info (unsigned int slot_index,
                                        CellularUICCSlotInfoStruct *pstSlotInfo)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_uicc_slot_info)
    {
        ret = g_device_hal.hal_get_uicc_slot_info (slot_index, pstSlotInfo);
    }
    else
    {
        // Set defaults
        memset (pstSlotInfo, 0, sizeof(CellularUICCSlotInfoStruct));
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_active_card_status (CellularUICCStatus_t *card_status)
{
    int ret = RETURN_ERROR;
    //CcspTraceInfo(("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_active_card_status)
    {
        ret = g_device_hal.hal_get_active_card_status (card_status);
    }
    else
    {
        // Set defaults
        *card_status = CELLULAR_UICC_STATUS_EMPTY;
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_monitor_device_registration (
        cellular_device_registration_status_callback device_registration_status_cb)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_monitor_device_registration)
        ret = g_device_hal.hal_monitor_device_registration (
                device_registration_status_cb);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_profile_create (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_profile_create)
        ret = g_device_hal.hal_profile_create (pstProfileInput,
                                               device_profile_status_cb);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_profile_delete (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_profile_delete)
        ret = g_device_hal.hal_profile_delete (pstProfileInput,
                                               device_profile_status_cb);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_profile_modify (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_profile_modify)
        ret = g_device_hal.hal_profile_modify (pstProfileInput,
                                               device_profile_status_cb);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_profile_list (CellularProfileStruct **pstProfileOutput,
                                      int *profile_count)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_hal_get_profile_list)
    {
        ret = g_device_hal.hal_hal_get_profile_list (pstProfileOutput,
                                                     profile_count);
    }
    else
    {
        // Set to defaults
        *pstProfileOutput = NULL;
        *profile_count = 0;
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_start_network (CellularNetworkIPType_t ip_request_type,
                                   CellularProfileStruct *pstProfileInput,
                                   CellularNetworkCBStruct *pstCBStruct)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);
    
    // Update the timestamps
    updateUsageTimeStamps();

    if (g_device_hal.hal_start_network)
    {
        ret = g_device_hal.hal_start_network (ip_request_type,
                                              pstProfileInput, pstCBStruct);
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_stop_network (CellularNetworkIPType_t ip_request_type)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_stop_network)
        ret = g_device_hal.hal_stop_network (ip_request_type);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_signal_info (CellularSignalInfoStruct *signal_info)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_signal_info)
        ret = g_device_hal.hal_get_signal_info (signal_info);
    else
    {
        // Set to defaults
        memset (signal_info, 0, sizeof(CellularSignalInfoStruct));
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_set_modem_operating_configuration (
        CellularModemOperatingConfiguration_t modem_operating_config)
{
    int ret = RETURN_OK;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_set_modem_operating_configuration)
        ret = g_device_hal.hal_set_modem_operating_configuration (
                modem_operating_config);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_device_imei (char *imei)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_device_imei)
        ret = g_device_hal.hal_get_device_imei (imei);
    else
    {
        // Set to defaults
        memset (imei, 0, 16);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_device_imei_sv (char *imei_sv)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_device_imei_sv)
        ret = g_device_hal.hal_get_device_imei_sv (imei_sv);
    else
    {
        // Set to defaults
        memset (imei_sv, 0, 16);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_modem_current_iccid (char *iccid)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_modem_current_iccid)
        ret = g_device_hal.hal_get_modem_current_iccid (iccid);
    else
    {
        // Set to defaults
        memset (iccid, 0, 21);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_modem_current_msisdn (char *msisdn)
{
    int ret = RETURN_ERROR;

    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));
    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_modem_current_msisdn)
        ret = g_device_hal.hal_get_modem_current_msisdn (msisdn);
    else
    {
        // Set to defaults
        memset (msisdn, 0, 20);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_packet_statistics (
        CellularPacketStatsStruct *network_packet_stats)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_packet_statistics)
        ret = g_device_hal.hal_get_packet_statistics (network_packet_stats);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_current_modem_interface_status (
        CellularInterfaceStatus_t *status)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_current_modem_interface_status)
        ret = g_device_hal.hal_get_current_modem_interface_status (status);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_set_modem_network_attach (void)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_set_modem_network_attach)
        ret = g_device_hal.hal_set_modem_network_attach ();

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_set_modem_network_detach (void)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_set_modem_network_detach)
        ret = g_device_hal.hal_set_modem_network_detach ();

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_modem_firmware_version (char *firmware_version)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_modem_firmware_version)
        ret = g_device_hal.hal_get_modem_firmware_version (firmware_version);
    else
    {
        // Set to defaults
        memset (firmware_version, 0, 26);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_current_plmn_information (
        CellularCurrentPlmnInfoStruct *plmn_info)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_current_plmn_information)
        ret = g_device_hal.hal_get_current_plmn_information (plmn_info);
    else
    {
        // Set to defaults
        memset (plmn_info, 0, sizeof(CellularCurrentPlmnInfoStruct));
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_available_networks_information (
        CellularNetworkScanResultInfoStruct **network_info,
        unsigned int *total_network_count)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_available_networks_information)
        ret = g_device_hal.hal_get_available_networks_information (
                network_info, total_network_count);
    else
    {
        *total_network_count = 0;
        CcspTraceInfo (("%s - No networks!! \n", __FUNCTION__));
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_current_access_technology (char *access_technology)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_current_access_technology)
        ret = g_device_hal.hal_get_current_access_technology (
                access_technology);
    else
    {
        // Set to defaults
        memset (access_technology, 0, 256);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_supported_access_technologies (char *access_technology)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_supported_access_technologies)
        ret = g_device_hal.hal_get_supported_access_technologies (
                access_technology);
    else
    {
        // Set to defaults
        memset (access_technology, 0, 256);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_prefered_access_technologies (char *access_technology)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_prefered_access_technologies)
        ret = g_device_hal.hal_get_prefered_access_technologies (
                access_technology);
    else
    {
        // Set to defaults
        memset (access_technology, 0, 256);
        ret = RETURN_OK;
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_device_information (CellularDeviceInfoStruct *devInfo)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_device_information)
        ret = g_device_hal.hal_get_device_information (devInfo);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_data_interface (char *data_interface)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (g_device_hal.hal_get_data_interface)
        ret = g_device_hal.hal_get_data_interface (data_interface);

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);

    return ret;
}

int cellular_hal_device_get_usage_timestamps(char *pFirstUse, char *pLastUse)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    // acquire the lock
    pthread_mutex_lock (&g_halAccessSerializer);

    if (pFirstUse != NULL)
    {
        AnscCopyString(pFirstUse, g_firstUseTimeStamp);
    }

    if (pLastUse != NULL)
    {
        AnscCopyString(pLastUse, g_lastUseTimeStamp);
    }

    // release the lock
    pthread_mutex_unlock (&g_halAccessSerializer);
    
    CcspTraceInfo (("%s - Exit \n", __FUNCTION__));

    return ret;
}

int cellular_hal_device_set_modem_type(MODEM_DEVICETYPE devType, PVOID pstModemInfo)
{
    int ret = RETURN_ERROR;
    CcspTraceInfo (("%s - Entry \n", __FUNCTION__));

    return ret;
}

MODEM_DEVICETYPE cellular_hal_device_get_modem_device_type()
{
    return g_modem_device_type;
}

