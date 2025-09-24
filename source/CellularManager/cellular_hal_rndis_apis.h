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

#ifndef __CELLULAR_HAL_RNDIS_APIS_
#define __CELLULAR_HAL_RNDIS_APIS_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <time.h>
#include <libusb.h>
#include <libudev.h>

#include "cellular_hal.h"
#include "cellular_hal_utils.h"

#define  RUN_DHCP_SCRIPT				"/usr/rdk/cellularmanager/run_dhcp.sh"
#define	 BRLAN_INTERFACE				"brlan0"

typedef enum _RNDISDeviceState_t
{
    RNDIS_DEVICE_DOWN = 1,// Default state, this indicates device not connected or device powered down
    RNDIS_DEVICE_ENABLED,// Device is enabled, in case of modems, in network registration is done. in case of RNDIS/WLANSTA TBD
    RNDIS_DEVICE_REGISTERED,	// Indicates network interface can be brought up
    RNDIS_DEVICE_CONNECTED,	// Indicates network interface is up and configured
    RNDIS_DEVICE_ERROR// All device errors, no recovery from here, only power cycle.
} RNDISDeviceState_t;

typedef enum _RNDISType_t
{
    RNDIS_NONE = 1,     				// Default type
    RNDIS_TYPE,        					// RNDIS class devices
    CDC_ETHER_TYPE       				// CDC Ether class devices
} RNDISType_t;

typedef enum _RNDISCommand_t
{
    RNDIS_NO_COMMAND = 1,     				// Default type
    RNDIS_OPEN_DEVICE,
    RNDIS_CLOSE_DEVICE,
    RNDIS_MONITOR_DEVICE_REGISTRATION,
    RNDIS_PROFILE_CREATE,
    RNDIS_PROFILE_MODIFY,
    RNDIS_PROFILE_UPDATE,
    RNDIS_SELECT_DEVICE_SLOT,
    RNDIS_START_NETWORK,
    RNDIS_STOP_NETWORK
} RNDISCommand_t;

// This structure represents the RNDIS device configuration, it contains the parameters to configure
// and also to indicate the status
typedef struct RNDISDevice_
{
    bool Enable;
    RNDISDeviceState_t Status;
    int VendorID;
    int ProductID;
    char IP[16];
    char Gateway[16];
    char PrimaryDNS[16];
    char SecondaryDNS[16];
    char InterfaceName[16];
    RNDISType_t Type;
} RNDISDevice, *PRNDISDevice;

/////////// RNDIS HAL ////////////////////////////////////
unsigned int
cellular_hal_rndis_IsModemDevicePresent (void);
int
cellular_hal_rndis_init (CellularContextInitInputStruct *pstCtxInputStruct);
int
cellular_hal_rndis_open_device (CellularDeviceContextCBStruct *pstDeviceCtxCB);
unsigned char
cellular_hal_rndis_IsModemControlInterfaceOpened (void);
int
cellular_hal_rndis_select_device_slot (
        cellular_device_slot_status_api_callback device_slot_status_cb);
int
cellular_hal_rndis_sim_power_enable (unsigned int slot_id,
                                     unsigned char enable);
int
cellular_hal_rndis_get_total_no_of_uicc_slots (unsigned int *total_count);
int
cellular_hal_rndis_get_uicc_slot_info (unsigned int slot_index,
                                       CellularUICCSlotInfoStruct *pstSlotInfo);
int
cellular_hal_rndis_get_active_card_status (CellularUICCStatus_t *card_status);
int
cellular_hal_rndis_profile_create (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int
cellular_hal_rndis_profile_delete (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int
cellular_hal_rndis_profile_modify (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int
cellular_hal_rndis_get_profile_list (CellularProfileStruct **ppstProfileOutput,
                                     int *profile_count);
int
cellular_hal_rndis_monitor_device_registration (
        cellular_device_registration_status_callback device_registration_status_cb);
int
cellular_hal_rndis_start_network (CellularNetworkIPType_t ip_request_type,
                                  CellularProfileStruct *pstProfileInput,
                                  CellularNetworkCBStruct *pstCBStruct);
int
cellular_hal_rndis_stop_network (CellularNetworkIPType_t ip_request_type);
int
cellular_hal_rndis_get_signal_info (CellularSignalInfoStruct *signal_info);
int
cellular_hal_rndis_set_modem_operating_configuration (
        CellularModemOperatingConfiguration_t modem_operating_config);
int
cellular_hal_rndis_get_device_imei (char *imei);
int
cellular_hal_rndis_get_device_imei_sv (char *imei_sv);
int
cellular_hal_rndis_get_modem_current_iccid (char *iccid);
int
cellular_hal_rndis_get_modem_current_msisdn (char *msisdn);
int
cellular_hal_rndis_get_packet_statistics (
        CellularPacketStatsStruct *network_packet_stats);
int
cellular_hal_rndis_get_current_modem_interface_status (
        CellularInterfaceStatus_t *status);
int
cellular_hal_rndis_set_modem_network_attach (void);
int
cellular_hal_rndis_set_modem_network_detach (void);
int
cellular_hal_rndis_get_modem_firmware_version (char *firmware_version);
int
cellular_hal_rndis_get_current_plmn_information (
        CellularCurrentPlmnInfoStruct *plmn_info);
int
cellular_hal_rndis_get_available_networks_information (
        CellularNetworkScanResultInfoStruct **network_info,
        unsigned int *total_network_count);
int
cellular_hal_rndis_get_current_access_technology (char *access_technology);
int
cellular_hal_rndis_get_supported_access_technologies (char *access_technology);
int
cellular_hal_rndis_get_prefered_access_technologies (char *access_technology);
int
cellular_hal_rndis_get_device_information (CellularDeviceInfoStruct *devInfo);
int
cellular_hal_rndis_get_data_interface (char *data_interface);
int
cellular_hal_rndis_set_device_props (void *interface, int dev_status);
#endif // __CELLULAR_HAL_RNDIS_APIS_
