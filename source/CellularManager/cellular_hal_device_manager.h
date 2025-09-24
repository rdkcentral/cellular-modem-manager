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

#ifndef __CELLULAR_HAL_DEVICE_MANAGER_
#define __CELLULAR_HAL_DEVICE_MANAGER_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <time.h>
#include <libusb.h>
#include <libudev.h>

#include "cellular_hal.h"
#ifdef FEATURE_RNDIS_HAL
#include "cellular_hal_rndis_apis.h"
#endif
#ifdef FEATURE_MODEM_HAL
#include "cellular_hal_modem_apis.h"
#endif
#include "cellular_hal_utils.h"

typedef enum _DeviceStatus_t
{
    USB_DEVICE_REMOVED,
    USB_DEVICE_ATTACHED
} DeviceStatus_t;

typedef enum _MODEM_DEVICETYPE
{
   MODEM_UNKNOWN = 1,
   MODEM_SOC,
   MODEM_USBMODEM,
   MODEM_USBRNDIS,
}
MODEM_DEVICETYPE;

typedef unsigned int (*pIsModemDevicePresent) (void);
typedef int (*pinit) (CellularContextInitInputStruct *pstCtxInputStruct);
typedef int (*popen_device) (CellularDeviceContextCBStruct *pstDeviceCtxCB);
typedef unsigned char (*pIsModemControlInterfaceOpened) (void);
typedef int (*pselect_device_slot) (cellular_device_slot_status_api_callback device_slot_status_cb);
typedef int (*psim_power_enable) (unsigned int slot_id, unsigned char enable);
typedef int (*pget_total_no_of_uicc_slots) (unsigned int *total_count);
typedef int (*pget_uicc_slot_info) (unsigned int slot_index, CellularUICCSlotInfoStruct *pstSlotInfo);
typedef int (*pget_active_card_status) (CellularUICCStatus_t *card_status);
typedef int (*pmonitor_device_registration) (cellular_device_registration_status_callback device_registration_status_cb);
typedef int (*pprofile_create) (CellularProfileStruct *pstProfileInput, cellular_device_profile_status_api_callback device_profile_status_cb);
typedef int (*pprofile_delete) (CellularProfileStruct *pstProfileInput, cellular_device_profile_status_api_callback device_profile_status_cb);
typedef int (*pprofile_modify) (CellularProfileStruct *pstProfileInput, cellular_device_profile_status_api_callback device_profile_status_cb);
typedef int (*phal_get_profile_list) (CellularProfileStruct **ppstProfileOutput, int *profile_count);
typedef int (*pstart_network) (CellularNetworkIPType_t ip_request_type, CellularProfileStruct *pstProfileInput, CellularNetworkCBStruct *pstCBStruct);
typedef int (*pstop_network) (CellularNetworkIPType_t ip_request_type);
typedef int (*pget_signal_info) (CellularSignalInfoStruct *signal_info);
typedef int (*pset_modem_operating_configuration) (CellularModemOperatingConfiguration_t modem_operating_config);
typedef int (*pget_device_imei) (char *imei);
typedef int (*pget_device_imei_sv) (char *imei_sv);
typedef int (*pget_modem_current_iccid) (char *iccid);
typedef int (*pget_modem_current_msisdn) (char *msisdn);
typedef int (*pget_packet_statistics) (CellularPacketStatsStruct *network_packet_stats);
typedef int (*pget_current_modem_interface_status) (CellularInterfaceStatus_t *status);
typedef int (*pset_modem_network_attach) (void);
typedef int (*pset_modem_network_detach) (void);
typedef int (*pget_modem_firmware_version) (char *firmware_version);
typedef int (*pget_current_plmn_information) (CellularCurrentPlmnInfoStruct *plmn_info);
typedef int (*pget_available_networks_information) (CellularNetworkScanResultInfoStruct **network_info, unsigned int *total_network_count);
typedef int (*pget_current_access_technology) (char *access_technology);
typedef int (*pget_supported_access_technologies) (char *access_technology);
typedef int (*pget_prefered_access_technologies) (char *access_technology);
typedef int (*pget_device_information) (CellularDeviceInfoStruct *devInfo);
typedef int (*pget_data_interface) (char *data_interface);
typedef int (*pset_device_props) (void *devInfo, int dev_status);

// structure contains function pointers to HAL, this abstracts the HAL layer
// and also will help scale up this design to support multiple HALs/Devices
// simultaneously
typedef struct _DEVICE_HAL_ABSTRATCOR_
{
    pIsModemDevicePresent                       hal_IsModemDevicePresent;
    pinit                                       hal_init;
    popen_device                                hal_open_device;
    pIsModemControlInterfaceOpened              hal_IsModemControlInterfaceOpened;
    pselect_device_slot                         hal_select_device_slot;
    psim_power_enable                           hal_sim_power_enable;
    pget_total_no_of_uicc_slots                 hal_get_total_no_of_uicc_slots;
    pget_uicc_slot_info                         hal_get_uicc_slot_info;
    pget_active_card_status                     hal_get_active_card_status;
    pmonitor_device_registration                hal_monitor_device_registration;
    pprofile_create                             hal_profile_create;
    pprofile_delete                             hal_profile_delete;
    pprofile_modify                             hal_profile_modify;
    phal_get_profile_list                       hal_hal_get_profile_list;
    pstart_network                              hal_start_network;
    pstop_network                               hal_stop_network;
    pget_signal_info                            hal_get_signal_info;
    pset_modem_operating_configuration          hal_set_modem_operating_configuration;
    pget_device_imei                            hal_get_device_imei;
    pget_device_imei_sv                         hal_get_device_imei_sv;
    pget_modem_current_iccid                    hal_get_modem_current_iccid;
    pget_modem_current_msisdn                   hal_get_modem_current_msisdn;
    pget_packet_statistics                      hal_get_packet_statistics;
    pget_current_modem_interface_status         hal_get_current_modem_interface_status;
    pset_modem_network_attach                   hal_set_modem_network_attach;
    pset_modem_network_detach                   hal_set_modem_network_detach;
    pget_modem_firmware_version                 hal_get_modem_firmware_version;
    pget_current_plmn_information               hal_get_current_plmn_information;
    pget_available_networks_information         hal_get_available_networks_information;
    pget_current_access_technology              hal_get_current_access_technology;
    pget_supported_access_technologies          hal_get_supported_access_technologies;
    pget_prefered_access_technologies           hal_get_prefered_access_technologies;
    pget_device_information                     hal_get_device_information;
    pget_data_interface                         hal_get_data_interface;
    pset_device_props                           hal_set_device_props;
} DEVICE_HAL_ABSTRATCOR_;

typedef struct _USBDeviceConfig_
{
    unsigned char ModelName[64];
    unsigned int VID;
    unsigned int PID;
    unsigned char ModeString[512];
} USBDeviceConfig;

int cellular_hal_device_init (CellularContextInitInputStruct *pstCtxInputStruct);
unsigned int cellular_hal_device_IsModemDevicePresent (void);
int cellular_hal_device_set_modem_type(MODEM_DEVICETYPE devType, void *pstModemInfo);
int cellular_hal_device_get_usage_timestamps(char *pFirstUse, char *pLastUse);
MODEM_DEVICETYPE cellular_hal_device_get_modem_device_type();
unsigned char cellular_hal_device_IsModemControlInterfaceOpened (void);
int cellular_hal_device_open_device (CellularDeviceContextCBStruct *pstDeviceCtxCB);
int cellular_hal_device_select_device_slot (
        cellular_device_slot_status_api_callback device_slot_status_cb);
int cellular_hal_device_sim_power_enable (unsigned int slot_id,
                                      unsigned char enable);
int cellular_hal_device_get_total_no_of_uicc_slots (unsigned int *total_count);
int cellular_hal_device_get_uicc_slot_info (
        unsigned int slot_index, CellularUICCSlotInfoStruct *pstSlotInfo);
int cellular_hal_device_get_active_card_status (CellularUICCStatus_t *card_status);
int cellular_hal_device_monitor_device_registration (
        cellular_device_registration_status_callback device_registration_status_cb);
int cellular_hal_device_profile_create (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int cellular_hal_device_profile_delete (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int cellular_hal_device_profile_modify (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int cellular_hal_device_get_profile_list (CellularProfileStruct **pstProfileOutput,
                                      int *profile_count);
int cellular_hal_device_start_network (CellularNetworkIPType_t ip_request_type,
                                   CellularProfileStruct *pstProfileInput,
                                   CellularNetworkCBStruct *pstCBStruct);
int cellular_hal_device_stop_network (CellularNetworkIPType_t ip_request_type);
int cellular_hal_device_get_signal_info (CellularSignalInfoStruct *signal_info);
int cellular_hal_device_set_modem_operating_configuration (
        CellularModemOperatingConfiguration_t modem_operating_config);
int cellular_hal_device_get_device_imei (char *imei);
int cellular_hal_device_get_device_imei_sv (char *imei_sv);
int cellular_hal_device_get_modem_current_iccid (char *iccid);
int cellular_hal_device_get_modem_current_msisdn (char *msisdn);
int cellular_hal_device_get_packet_statistics (
        CellularPacketStatsStruct *network_packet_stats);
int cellular_hal_device_get_current_modem_interface_status (
        CellularInterfaceStatus_t *status);
int cellular_hal_device_set_modem_network_attach (void);
int cellular_hal_device_set_modem_network_detach (void);
int cellular_hal_device_get_modem_firmware_version (char *firmware_version);
int cellular_hal_device_get_current_plmn_information (
        CellularCurrentPlmnInfoStruct *plmn_info);
int cellular_hal_device_get_available_networks_information (
        CellularNetworkScanResultInfoStruct **network_info,
        unsigned int *total_network_count);
int cellular_hal_device_get_current_access_technology (char *access_technology);
int cellular_hal_device_get_supported_access_technologies (char *access_technology);
int cellular_hal_device_get_prefered_access_technologies (char *access_technology);
int cellular_hal_device_get_device_information (CellularDeviceInfoStruct *devInfo);
int cellular_hal_device_get_data_interface (char *data_interface);

#ifdef FEATURE_MODEM_HAL
const DEVICE_HAL_ABSTRATCOR_ modem_device_hal;
#endif
#ifdef FEATURE_RNDIS_HAL
const DEVICE_HAL_ABSTRATCOR_ rndis_device_hal;
#endif
#endif // __CELLULAR_HAL_DEVICE_MANAGER_
