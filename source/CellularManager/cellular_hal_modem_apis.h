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

#ifndef __CELLULAR_HAL_MODEM_APIS_
#define __CELLULAR_HAL_MODEM_APIS_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "libmm-glib.h"
#include "mm-unlock-retries.h"
#include "cellular_hal.h"
#include "cellular_hal_utils.h"

#define MAX_SIM_COUNT		16
#define MAX_BEARER_COUNT	16

#define IDX_SIZE 8
#define MM_MODE_2G "GSM"
#define MM_MODE_3G "UMTS"
#define MM_MODE_4G "LTE"
#define MM_MODE_5G "NR"
/****************************************************************
 ENUM DECLARATIONS
 *****************************************************************/
typedef enum
{ /*< underscore_name=mm_modem_state >*/
    MODEM_STATE_FAILED = -1,
    MODEM_STATE_UNKNOWN = 0,
    MODEM_STATE_INITIALIZING = 1,
    MODEM_STATE_LOCKED = 2,
    MODEM_STATE_DISABLED = 3,
    MODEM_STATE_DISABLING = 4,
    MODEM_STATE_ENABLING = 5,
    MODEM_STATE_ENABLED = 6,
    MODEM_STATE_SEARCHING = 7,
    MODEM_STATE_REGISTERED = 8,
    MODEM_STATE_DISCONNECTING = 9,
    MODEM_STATE_CONNECTING = 10,
    MODEM_STATE_CONNECTED = 11
} ModemState;

typedef enum
{ /*< underscore_name=mm_bearer_type >*/
    BEARER_TYPE_UNKNOWN = 0,
    BEARER_TYPE_DEFAULT = 1,
    BEARER_TYPE_DEFAULT_ATTACH = 2,
    BEARER_TYPE_DEDICATED = 3,
} BearerType;

typedef enum
{ /*< underscore_name=mm_bearer_ip_method >*/
    BEARER_IP_METHOD_UNKNOWN = 0,
    BEARER_IP_METHOD_PPP = 1,
    BEARER_IP_METHOD_STATIC = 2,
    BEARER_IP_METHOD_DHCP = 3,
} BearerIpMethod;

typedef enum
{ /*< underscore_name=mm_bearer_ip_family >*/
    BEARER_IP_FAMILY_NONE = 0,
    BEARER_IP_FAMILY_IPV4 = 1 << 0,
    BEARER_IP_FAMILY_IPV6 = 1 << 1,
    BEARER_IP_FAMILY_IPV4V6 = 1 << 2,
    BEARER_IP_FAMILY_ANY = 0xFFFFFFFF
} BearerIpFamily;

typedef enum
{ /*< underscore_name=mm_bearer_allowed_auth >*/
    BEARER_ALLOWED_AUTH_UNKNOWN = 0,
    /* bits 0..4 order match Ericsson device bitmap */
    BEARER_ALLOWED_AUTH_NONE = 1 << 0,
    BEARER_ALLOWED_AUTH_PAP = 1 << 1,
    BEARER_ALLOWED_AUTH_CHAP = 1 << 2,
    BEARER_ALLOWED_AUTH_MSCHAP = 1 << 3,
    BEARER_ALLOWED_AUTH_MSCHAPV2 = 1 << 4,
    BEARER_ALLOWED_AUTH_EAP = 1 << 5,
} BearerAllowedAuth;

typedef enum
{ /*< underscore_name=mm_modem_3gpp_registration_state >*/
    MODEM_3GPP_REGISTRATION_STATE_IDLE = 0,
    MODEM_3GPP_REGISTRATION_STATE_HOME = 1,
    MODEM_3GPP_REGISTRATION_STATE_SEARCHING = 2,
    MODEM_3GPP_REGISTRATION_STATE_DENIED = 3,
    MODEM_3GPP_REGISTRATION_STATE_UNKNOWN = 4,
    MODEM_3GPP_REGISTRATION_STATE_ROAMING = 5,
    MODEM_3GPP_REGISTRATION_STATE_HOME_SMS_ONLY = 6,
    MODEM_3GPP_REGISTRATION_STATE_ROAMING_SMS_ONLY = 7,
    MODEM_3GPP_REGISTRATION_STATE_EMERGENCY_ONLY = 8,
    MODEM_3GPP_REGISTRATION_STATE_HOME_CSFB_NOT_PREFERRED = 9,
    MODEM_3GPP_REGISTRATION_STATE_ROAMING_CSFB_NOT_PREFERRED = 10,
} Modem3gppRegistrationState;

/**
 * MMModemMode:
 * @MM_MODEM_MODE_NONE: None.
 * @MM_MODEM_MODE_CS: CSD, GSM, and other circuit-switched technologies.
 * @MM_MODEM_MODE_2G: GPRS, EDGE.
 * @MM_MODEM_MODE_3G: UMTS, HSxPA.
 * @MM_MODEM_MODE_4G: LTE.
 * @MM_MODEM_MODE_ANY: Any mode can be used (only this value allowed for POTS modems).
 *
 * Bitfield to indicate which access modes are supported, allowed or
 * preferred in a given device.
 *
 * Since: 1.0
 */
typedef enum
{ /*< underscore_name=mm_modem_mode >*/
    MODEM_MODE_NONE = 0,
    MODEM_MODE_CS = 1 << 0,
    MODEM_MODE_2G = 1 << 1,
    MODEM_MODE_3G = 1 << 2,
    MODEM_MODE_4G = 1 << 3,
    MODEM_MODE_5G = 1 << 4,
    MODEM_MODE_ANY = 0xFFFFFFFF
} ModemMode;

/**
 * ModemStateFailedReason:
 * @MODEM_STATE_FAILED_REASON_NONE: No error.
 * @MODEM_STATE_FAILED_REASON_UNKNOWN: Unknown error.
 * @MODEM_STATE_FAILED_REASON_SIM_MISSING: SIM is required but missing.
 * @MODEM_STATE_FAILED_REASON_SIM_ERROR: SIM is available, but unusable (e.g. permanently locked).
 *
 * Enumeration of possible errors when the modem is in @MODEM_STATE_FAILED.
 *
 * Since: 1.0
 */
typedef enum
{ /*< underscore_name=mm_modem_state_failed_reason >*/
    MODEM_STATE_FAILED_REASON_NONE = 0,
    MODEM_STATE_FAILED_REASON_UNKNOWN = 1,
    MODEM_STATE_FAILED_REASON_SIM_MISSING = 2,
    MODEM_STATE_FAILED_REASON_SIM_ERROR = 3,
} ModemStateFailedReason;

typedef enum
{ /*< underscore_name=mm_modem_access_technology >*/
    MODEM_ACCESS_TECHNOLOGY_UNKNOWN = 0,
    MODEM_ACCESS_TECHNOLOGY_POTS = 1 << 0,
    MODEM_ACCESS_TECHNOLOGY_GSM = 1 << 1,
    MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT = 1 << 2,
    MODEM_ACCESS_TECHNOLOGY_GPRS = 1 << 3,
    MODEM_ACCESS_TECHNOLOGY_EDGE = 1 << 4,
    MODEM_ACCESS_TECHNOLOGY_UMTS = 1 << 5,
    MODEM_ACCESS_TECHNOLOGY_HSDPA = 1 << 6,
    MODEM_ACCESS_TECHNOLOGY_HSUPA = 1 << 7,
    MODEM_ACCESS_TECHNOLOGY_HSPA = 1 << 8,
    MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS = 1 << 9,
    MODEM_ACCESS_TECHNOLOGY_1XRTT = 1 << 10,
    MODEM_ACCESS_TECHNOLOGY_EVDO0 = 1 << 11,
    MODEM_ACCESS_TECHNOLOGY_EVDOA = 1 << 12,
    MODEM_ACCESS_TECHNOLOGY_EVDOB = 1 << 13,
    MODEM_ACCESS_TECHNOLOGY_LTE = 1 << 14,
    MODEM_ACCESS_TECHNOLOGY_5GNR = 1 << 15,
    MODEM_ACCESS_TECHNOLOGY_ANY = 0xFFFFFFFF,
} ModemAccessTechnology;

enum
{
    DEVICE_MODEL = 1,
    DEVICE_VENDOR,
    DEVICE_IMAGE_VERSION,
    DEVICE_CONTROL_INTERFACE,
    DEVICE_HARDWARE_REVISION,
    DEVICE_MSISDN,
    DEVICE_IMEI
};

enum
{
    SIM_ICCID = 1, SIM_MNO_NAME
};

typedef struct _IPv4Config_
{
    BearerIpMethod Method;
    unsigned int Prefix;
    unsigned char IpAddress[16];
    unsigned char Gateway[16];
    unsigned char DnsPri[16];
    unsigned char DnsSec[16];
    unsigned int Mtu;
} IPv4Config;

typedef struct _IPv6Config_
{
    BearerIpMethod Method;
    unsigned int Prefix;
    unsigned char IpAddress[64];
    unsigned char Gateway[64];
    unsigned char DnsPri[64];
    unsigned char DnsSec[64];
    unsigned int Mtu;
} IPv6Config;

typedef struct _Bearer_
{
    MMBearer *bearerObj;
    MMBearerProperties *bearerProperties;
    unsigned char dbusPath[1024];
    struct
    {
        BearerType type;
        bool Status;
        unsigned char Apn[64];
        BearerAllowedAuth ApnAuth;
        BearerIpFamily IpAddressFamily;
        unsigned char username[256];
        unsigned char password[256];
        bool Roaming; //allow_roaming
        unsigned char NetPort[32];
        unsigned char WANIFName[16];
        IPv4Config IPv4Info;
        IPv6Config IPv6Info;
    } info;
} Bearer;

typedef struct _SIM_
{
    MMSim *simObj;
    unsigned char dbusPath[1024];
    struct
    {
        unsigned char IMSI[64];
        unsigned char ICCID[64];
        unsigned char MNOID[64];
        unsigned char MNOName[64];
    } info;
} SIM;

// This can be scaled up to support multiple modem devices, by converting the below into an array of devices.
typedef struct _Modem_
{
    MMObject *mmObj;
    MMModem *modemObj;
    MMModemState modemState;
    unsigned char dbusPath[1024];
    struct
    {
        unsigned char vendor[64];
        unsigned char model[32];
        unsigned char controlInterface[32];
        unsigned char dataInterface[32];
        unsigned char IMEI[64];
        unsigned char firmwareVersion[32];
        unsigned char hardwareVersion[32];
        unsigned char MSISDN[64];
        unsigned char primaryPort[32];
        unsigned char ports[32];
    } info;
    unsigned char numberOfSIMs;
    SIM *pSIM[MAX_SIM_COUNT]; // Array of pointers
    unsigned char numberOfBearers;
    Bearer *pBearer[MAX_BEARER_COUNT]; // Array of bearers
} Modem;

// These are the set of commands that need to be executed asynchronously
typedef enum _MODEMCommand_t
{
    MODEM_NO_COMMAND = 1,     				// Default type
    MODEM_OPEN_DEVICE,
    MODEM_CLOSE_DEVICE,
    MODEM_MONITOR_DEVICE_REGISTRATION,
    MODEM_PROFILE_CREATE,
    MODEM_PROFILE_MODIFY,
    MODEM_PROFILE_DELETE,
    MODEM_SELECT_DEVICE_SLOT,
    MODEM_START_NETWORK,
    MODEM_STOP_NETWORK
} MODEMCommand_t;

/////////// MODEM HAL ////////////////////////////////////
unsigned int
cellular_hal_modem_IsModemDevicePresent (void);
int
cellular_hal_modem_init (CellularContextInitInputStruct *pstCtxInputStruct);
int
cellular_hal_modem_open_device (CellularDeviceContextCBStruct *pstDeviceCtxCB);
unsigned char
cellular_hal_modem_IsModemControlInterfaceOpened (void);
int
cellular_hal_modem_select_device_slot (
        cellular_device_slot_status_api_callback device_slot_status_cb);
int
cellular_hal_modem_sim_power_enable (unsigned int slot_id,
                                     unsigned char enable);
int
cellular_hal_modem_get_total_no_of_uicc_slots (unsigned int *total_count);
int
cellular_hal_modem_get_uicc_slot_info (unsigned int slot_index,
                                       CellularUICCSlotInfoStruct *pstSlotInfo);
int
cellular_hal_modem_get_active_card_status (CellularUICCStatus_t *card_status);
int
cellular_hal_modem_monitor_device_registration (
        cellular_device_registration_status_callback device_registration_status_cb);
int
cellular_hal_modem_profile_create (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int
cellular_hal_modem_profile_delete (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int
cellular_hal_modem_profile_modify (
        CellularProfileStruct *pstProfileInput,
        cellular_device_profile_status_api_callback device_profile_status_cb);
int
cellular_hal_modem_get_profile_list (CellularProfileStruct **ppstProfileOutput,
                                     int *profile_count);
int
cellular_hal_modem_start_network (CellularNetworkIPType_t ip_request_type,
                                  CellularProfileStruct *pstProfileInput,
                                  CellularNetworkCBStruct *pstCBStruct);
int
cellular_hal_modem_stop_network (CellularNetworkIPType_t ip_request_type);
int
cellular_hal_modem_get_signal_info (CellularSignalInfoStruct *signal_info);
int
cellular_hal_modem_set_modem_operating_configuration (
        CellularModemOperatingConfiguration_t modem_operating_config);
int
cellular_hal_modem_get_device_imei (char *imei);
int
cellular_hal_modem_get_device_imei_sv (char *imei_sv);
int
cellular_hal_modem_get_modem_current_iccid (char *iccid);
int
cellular_hal_modem_get_modem_current_msisdn (char *msisdn);
int
cellular_hal_modem_get_packet_statistics (
        CellularPacketStatsStruct *network_packet_stats);
int
cellular_hal_modem_get_current_modem_interface_status (
        CellularInterfaceStatus_t *status);
int
cellular_hal_modem_set_modem_network_attach (void);
int
cellular_hal_modem_set_modem_network_detach (void);
int
cellular_hal_modem_get_modem_firmware_version (char *firmware_version);
int
cellular_hal_modem_get_current_plmn_information (
        CellularCurrentPlmnInfoStruct *plmn_info);
int
cellular_hal_modem_get_available_networks_information (
        CellularNetworkScanResultInfoStruct **network_info,
        unsigned int *total_network_count);
int
cellular_hal_modem_get_current_access_technology (char *access_technology);
int
cellular_hal_modem_get_supported_access_technologies (char *access_technology);
int
cellular_hal_modem_get_prefered_access_technologies (char *access_technology);
int
cellular_hal_modem_get_device_information (CellularDeviceInfoStruct *devInfo);
int
cellular_hal_modem_get_data_interface (char *data_interface);
int
cellular_hal_modem_set_device_props (void *devInfo, int dev_status);
#endif // __CELLULAR_HAL_MODEM_APIS_
