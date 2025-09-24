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
#include "cellular_hal_device_manager.h"

#ifdef FEATURE_MODEM_HAL
const DEVICE_HAL_ABSTRATCOR_ modem_device_hal =
    {       cellular_hal_modem_IsModemDevicePresent,
            cellular_hal_modem_init,
            cellular_hal_modem_open_device,
            cellular_hal_modem_IsModemControlInterfaceOpened,
            cellular_hal_modem_select_device_slot,
            cellular_hal_modem_sim_power_enable,
            cellular_hal_modem_get_total_no_of_uicc_slots,
            cellular_hal_modem_get_uicc_slot_info,
            cellular_hal_modem_get_active_card_status,
            cellular_hal_modem_monitor_device_registration,
            cellular_hal_modem_profile_create,
            cellular_hal_modem_profile_delete,
            cellular_hal_modem_profile_modify,
            cellular_hal_modem_get_profile_list,
            cellular_hal_modem_start_network,
            cellular_hal_modem_stop_network,
            cellular_hal_modem_get_signal_info,
            cellular_hal_modem_set_modem_operating_configuration,
            cellular_hal_modem_get_device_imei,
            cellular_hal_modem_get_device_imei_sv,
            cellular_hal_modem_get_modem_current_iccid,
            cellular_hal_modem_get_modem_current_msisdn,
            cellular_hal_modem_get_packet_statistics,
            cellular_hal_modem_get_current_modem_interface_status,
            cellular_hal_modem_set_modem_network_attach,
            cellular_hal_modem_set_modem_network_detach,
            cellular_hal_modem_get_modem_firmware_version,
            cellular_hal_modem_get_current_plmn_information,
            cellular_hal_modem_get_available_networks_information,
            cellular_hal_modem_get_current_access_technology,
            cellular_hal_modem_get_supported_access_technologies,
            cellular_hal_modem_get_prefered_access_technologies,
            cellular_hal_modem_get_device_information,
            cellular_hal_modem_get_data_interface,
            cellular_hal_modem_set_device_props
    };
#endif

#ifdef FEATURE_RNDIS_HAL
const DEVICE_HAL_ABSTRATCOR_ rndis_device_hal =
    {       cellular_hal_rndis_IsModemDevicePresent,
            cellular_hal_rndis_init,
            cellular_hal_rndis_open_device,
            cellular_hal_rndis_IsModemControlInterfaceOpened,
            cellular_hal_rndis_select_device_slot,
            cellular_hal_rndis_sim_power_enable,
            cellular_hal_rndis_get_total_no_of_uicc_slots,
            cellular_hal_rndis_get_uicc_slot_info,
            cellular_hal_rndis_get_active_card_status,
            cellular_hal_rndis_monitor_device_registration,
            cellular_hal_rndis_profile_create,
            cellular_hal_rndis_profile_delete,
            cellular_hal_rndis_profile_modify,
            NULL,
            cellular_hal_rndis_start_network,
            cellular_hal_rndis_stop_network,
            NULL,
            cellular_hal_rndis_set_modem_operating_configuration,
            NULL,
            NULL,
            NULL,
            NULL,
            cellular_hal_rndis_get_packet_statistics,
            cellular_hal_rndis_get_current_modem_interface_status,
            cellular_hal_rndis_set_modem_network_attach,
            cellular_hal_rndis_set_modem_network_detach,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            cellular_hal_rndis_get_device_information,
            cellular_hal_rndis_get_data_interface,
            cellular_hal_rndis_set_device_props
    };
#endif

