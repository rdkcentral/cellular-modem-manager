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

#ifndef _CELLULAR_HAL_QMI_APIS_H_
#define _CELLULAR_HAL_QMI_APIS_H_

#include "cellular_hal_utils.h"

/**********************************************************************
                STRUCTURE AND CONSTANT DEFINITIONS
**********************************************************************/

#define QMI_DEVICE_PATH        "/dev/cdc-wdm0"
#define QMI_DEVICE_NAME        "cdc-wdm0"

typedef enum {
    WDS_PROFILE_OPERATION_CREATE                  = 1,
    WDS_PROFILE_OPERATION_DELETE,
    WDS_PROFILE_OPERATION_MODIFY,
    WDS_PROFILE_OPERATION_LIST
} WDSProfileOperation_t;

typedef struct {
    CellularProfileStruct *pstProfileInput;
    cellular_device_profile_status_api_callback device_profile_status_cb;
} QMIHALProfileCreateDeleteModifyStruct;

typedef struct {
    CellularProfileStruct **ppstProfileOutput;
    int *profile_count;
} QMIHALProfileGetListStruct;

typedef union {
    QMIHALProfileCreateDeleteModifyStruct stProfileCreateDeleteModify;
    QMIHALProfileGetListStruct            stProfileGetList;
} QMIHALProfileOperationUnion;

typedef struct {
    WDSProfileOperation_t           enProfileOperationInput;
    QMIHALProfileOperationUnion     unProfileOperation;
} QMIHALProfileOperationInputStruct;

typedef enum {
    NAS_NETWORK_ATTACH                  = 1,
    NAS_NETWORK_DETACH
} NASAttachDetachOperation_t;

/**********************************************************************
                FUNCTION PROTOTYPES
**********************************************************************/

int cellular_hal_qmi_init (CellularContextInitInputStruct *pstCtxInputStruct);
int cellular_hal_qmi_open_device(CellularDeviceContextCBStruct *pstDeviceCtxCB);
unsigned char cellular_hal_qmi_IsModemControlInterfaceOpened( void );
int cellular_hal_qmi_select_device_slot(cellular_device_slot_status_api_callback device_slot_status_cb);
int cellular_hal_qmi_sim_power_enable(unsigned int slot_id, unsigned char enable);
int cellular_hal_qmi_get_total_no_of_uicc_slots(unsigned int *total_count);
int cellular_hal_qmi_get_uicc_slot_info(unsigned int slot_id, CellularUICCSlotInfoStruct *pstSlotInfo);
int cellular_hal_qmi_get_active_card_status(CellularUICCStatus_t *card_status);
int cellular_hal_qmi_monitor_device_registration(cellular_device_registration_status_callback device_registration_status_cb);
int cellular_hal_qmi_profile_operation(QMIHALProfileOperationInputStruct *pstProfileOperationInput);
int cellular_hal_qmi_get_imei( char *imei );
int cellular_hal_qmi_get_imei_softwareversion( char *imei_sv );
int cellular_hal_qmi_get_iccid_information( char *iccid );
int cellular_hal_qmi_get_msisdn_information ( char *msisdn );
int cellular_hal_qmi_get_profile_list(CellularProfileStruct **ppstProfileOutput, int *profile_count);
int cellular_hal_qmi_start_network(CellularNetworkIPType_t ip_request_type, CellularProfileStruct *pstProfileInput, CellularNetworkCBStruct *pstCBStruct);
int cellular_hal_qmi_stop_network(CellularNetworkIPType_t ip_request_type);
int cellular_hal_qmi_get_signal_info(CellularSignalInfoStruct *signal_info);
int cellular_hal_qmi_set_modem_operating_configuration(CellularModemOperatingConfiguration_t modem_operating_config);
int cellular_hal_qmi_get_packet_statistics( CellularPacketStatsStruct *packet_stats );
int cellular_hal_qmi_get_current_modem_interface_status( CellularInterfaceStatus_t *status );
int cellular_hal_qmi_set_modem_network_operation( NASAttachDetachOperation_t network_operation);
int cellular_hal_qmi_get_modem_firmware_version(char *firmware_version);
int cellular_hal_qmi_get_current_plmn_information(CellularCurrentPlmnInfoStruct *plmn_info);
int cellular_hal_qmi_get_available_networks_information(CellularNetworkScanResultInfoStruct **network_info, unsigned int *total_network_count);
int cellular_hal_qmi_get_supported_radio_technology(char *supported_rat);
int cellular_hal_qmi_get_preferred_radio_technology( char *preferred_rat);
int cellular_hal_qmi_set_preferred_radio_technology( char *preferred_rat);
int cellular_hal_qmi_get_current_radio_technology( char *current_rat);
#endif //_CELLULAR_HAL_QMI_APIS_H_
