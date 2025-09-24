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

#ifndef __ARC_CELLULAR_API_H__
#define __ARC_CELLULAR_API_H__

#include "cellular_hal.h"
#include "cellular_hal_utils.h"
/**********************************************************************
               CONSTANT DEFINITIONS
**********************************************************************/

#ifndef CHAR
#define CHAR  char
#endif

#ifndef UCHAR
#define UCHAR unsigned char
#endif

#ifndef BOOLEAN
#define BOOLEAN  unsigned char
#endif

#ifndef USHORT
#define USHORT  unsigned short
#endif

#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef INT
#define INT   int
#endif

#ifndef UINT
#define UINT  unsigned int
#endif

#ifndef LONG
#define LONG	long
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif

#ifndef TRUE
#define TRUE     1
#endif

#ifndef FALSE
#define FALSE    0
#endif

#ifndef ENABLE
#define ENABLE   1
#endif

#define IDX_SIZE 8
#define MM_MODE_2G "GSM"
#define MM_MODE_3G "UMTS"
#define MM_MODE_4G "LTE"
#define MM_MODE_5G "NR"
/****************************************************************
			ENUM DECLARATIONS
*****************************************************************/

typedef enum {
    MODEM_STATE_FAILED        = -1,
    MODEM_STATE_UNKNOWN       = 0,
    MODEM_STATE_INITIALIZING  = 1,
    MODEM_STATE_LOCKED        = 2,
    MODEM_STATE_DISABLED      = 3,
    MODEM_STATE_DISABLING     = 4,
    MODEM_STATE_ENABLING      = 5,
    MODEM_STATE_ENABLED       = 6,
    MODEM_STATE_SEARCHING     = 7,
    MODEM_STATE_REGISTERED    = 8,
    MODEM_STATE_DISCONNECTING = 9,
    MODEM_STATE_CONNECTING    = 10,
    MODEM_STATE_CONNECTED     = 11
} ModemState;

typedef enum {
    BEARER_TYPE_UNKNOWN        = 0,
    BEARER_TYPE_DEFAULT        = 1,
    BEARER_TYPE_DEFAULT_ATTACH = 2,
    BEARER_TYPE_DEDICATED      = 3,
} BearerType;

typedef enum {
    BEARER_IP_METHOD_UNKNOWN = 0,
    BEARER_IP_METHOD_PPP     = 1,
    BEARER_IP_METHOD_STATIC  = 2,
    BEARER_IP_METHOD_DHCP    = 3,
} BearerIpMethod;

typedef enum {
    BEARER_IP_FAMILY_NONE    = 0,
    BEARER_IP_FAMILY_IPV4    = 1 << 0,
    BEARER_IP_FAMILY_IPV6    = 1 << 1,
    BEARER_IP_FAMILY_IPV4V6  = 1 << 2,
    BEARER_IP_FAMILY_ANY     = 0xFFFFFFFF
} BearerIpFamily;

typedef enum {
    BEARER_ALLOWED_AUTH_UNKNOWN  = 0,
    BEARER_ALLOWED_AUTH_NONE     = 1 << 0,
    BEARER_ALLOWED_AUTH_PAP      = 1 << 1,
    BEARER_ALLOWED_AUTH_CHAP     = 1 << 2,
    BEARER_ALLOWED_AUTH_MSCHAP   = 1 << 3,
    BEARER_ALLOWED_AUTH_MSCHAPV2 = 1 << 4,
    BEARER_ALLOWED_AUTH_EAP      = 1 << 5,
} BearerAllowedAuth;


typedef enum {
    MODEM_3GPP_REGISTRATION_STATE_IDLE                       = 0,
    MODEM_3GPP_REGISTRATION_STATE_HOME                       = 1,
    MODEM_3GPP_REGISTRATION_STATE_SEARCHING                  = 2,
    MODEM_3GPP_REGISTRATION_STATE_DENIED                     = 3,
    MODEM_3GPP_REGISTRATION_STATE_UNKNOWN                    = 4,
    MODEM_3GPP_REGISTRATION_STATE_ROAMING                    = 5,
    MODEM_3GPP_REGISTRATION_STATE_HOME_SMS_ONLY              = 6,
    MODEM_3GPP_REGISTRATION_STATE_ROAMING_SMS_ONLY           = 7,
    MODEM_3GPP_REGISTRATION_STATE_EMERGENCY_ONLY             = 8,
    MODEM_3GPP_REGISTRATION_STATE_HOME_CSFB_NOT_PREFERRED    = 9,
    MODEM_3GPP_REGISTRATION_STATE_ROAMING_CSFB_NOT_PREFERRED = 10,
} Modem3gppRegistrationState;

typedef enum {
    MODEM_MODE_NONE = 0,
    MODEM_MODE_CS   = 1 << 0,
    MODEM_MODE_2G   = 1 << 1,
    MODEM_MODE_3G   = 1 << 2,
    MODEM_MODE_4G   = 1 << 3,
    MODEM_MODE_5G   = 1 << 4,
    MODEM_MODE_ANY  = 0xFFFFFFFF
} ModemMode;

typedef enum {
    MODEM_STATE_FAILED_REASON_NONE        = 0,
    MODEM_STATE_FAILED_REASON_UNKNOWN     = 1,
    MODEM_STATE_FAILED_REASON_SIM_MISSING = 2,
    MODEM_STATE_FAILED_REASON_SIM_ERROR   = 3,
} ModemStateFailedReason;

typedef enum {
    MODEM_ACCESS_TECHNOLOGY_UNKNOWN     = 0,
    MODEM_ACCESS_TECHNOLOGY_POTS        = 1 << 0,
    MODEM_ACCESS_TECHNOLOGY_GSM         = 1 << 1,
    MODEM_ACCESS_TECHNOLOGY_GSM_COMPACT = 1 << 2,
    MODEM_ACCESS_TECHNOLOGY_GPRS        = 1 << 3,
    MODEM_ACCESS_TECHNOLOGY_EDGE        = 1 << 4,
    MODEM_ACCESS_TECHNOLOGY_UMTS        = 1 << 5,
    MODEM_ACCESS_TECHNOLOGY_HSDPA       = 1 << 6,
    MODEM_ACCESS_TECHNOLOGY_HSUPA       = 1 << 7,
    MODEM_ACCESS_TECHNOLOGY_HSPA        = 1 << 8,
    MODEM_ACCESS_TECHNOLOGY_HSPA_PLUS   = 1 << 9,
    MODEM_ACCESS_TECHNOLOGY_1XRTT       = 1 << 10,
    MODEM_ACCESS_TECHNOLOGY_EVDO0       = 1 << 11,
    MODEM_ACCESS_TECHNOLOGY_EVDOA       = 1 << 12,
    MODEM_ACCESS_TECHNOLOGY_EVDOB       = 1 << 13,
    MODEM_ACCESS_TECHNOLOGY_LTE         = 1 << 14,
    MODEM_ACCESS_TECHNOLOGY_5GNR        = 1 << 15,
    MODEM_ACCESS_TECHNOLOGY_ANY         = 0xFFFFFFFF,
} ModemAccessTechnology;

enum {
    DEVICE_MODEL = 1,
    DEVICE_VENDOR,
    DEVICE_IMAGE_VERSION,
    DEVICE_CONTROL_INTERFACE,
    DEVICE_HARDWARE_REVISION,
    DEVICE_MSISDN,
    DEVICE_IMEI
};

enum {
    SIM_ICCID = 1,
    SIM_MNO_NAME
};

/*********************************************************
		STRUCTURE DEFINITIONS
**********************************************************/

typedef struct
_DEVICE_MANAGEMENT
{
    CHAR                          Model[32];
    CHAR                          HardwareRevision[32];
    CHAR                          Vendor[64];
    CHAR                          ControlInterface[32];
    CHAR                          DataInterface[32];
    CHAR                          Imei[64];
    CHAR                          CurrentImageVersion[32];
    CHAR                          Msisdn[64];
    CHAR                          NetPort[32];
}
DEVICE_MANAGEMENT;

typedef struct 
_ModemModeCombination{
    ModemMode allowed;
    ModemMode preferred;
}ModemModeCombination;

typedef struct
_ACCESS_TECH{
    ModemModeCombination Support;
    ModemModeCombination Current;
}ACCESS_TECH;

typedef struct
_PLMN_ACCESS
{
    Modem3gppRegistrationState    State;
    CHAR                          Operator_code[16];
    CHAR                          Name[64];
}
PLMN_ACCESS;

typedef struct
_SIM_INFORMATION
{
    CHAR                          Imsi[64];
    CHAR                          Iccid[64];
    CHAR                          Operator_id[64];
    CHAR                          Operator_name[64];
}
SIM_INFORMATION;

typedef struct
_RADIO_SIGNAL
{
    INT                           Rssi;
    INT                           Rsrq;
    INT                           Rsrp;
    INT                           Snr;
}
RADIO_SIGNAL;

typedef struct
_SERVING_CELL
{
    ULONG                         CellId;
    UINT                          PlmnId;
    UINT                          Rfcn;
    ULONG                         LocationAreaCode;
    ULONG                         TrackingAreaCode;
}
SERVING_CELL;

typedef struct
_BEARER_PROPERTIES
{
    BearerType                    type;
    BOOLEAN                       Status;
    CHAR                          Apn[64];
    BearerAllowedAuth             ApnAuth;
    BearerIpFamily                IpAddressFamily;
    CHAR                          username[256];
    CHAR                          password[256];
    BOOLEAN                       Roaming;
    CHAR                          NetPort[32];
}
BEARER_PROPERTIES;

typedef struct
_BEARER_IPV4_CONFIG
{
    BearerIpMethod                Method;
    UINT                          Prefix;
    CHAR                          IpAddress[16];
    CHAR                          Gateway[16];
    CHAR                          DnsPri[16];
    CHAR                          DnsSec[16];
    UINT                          Mtu; 
}
BEARER_IPV4_CONFIG;

typedef struct
_BEARER_IPV6_CONFIG
{
    BearerIpMethod                Method;
    UINT                          Prefix;
    CHAR                          IpAddress[64];
    CHAR                          Gateway[64];
    CHAR                          DnsPri[64];
    CHAR                          DnsSec[64];
    UINT                          Mtu;
}
BEARER_IPV6_CONFIG;

typedef struct
_BEARER_STATS
{
    ULONG                         BytesSent;
    ULONG                         BytesReceived;
}
BEARER_STATS;

typedef struct
_DEVICE_INFO
{
    CHAR                         deviceName[16];
    CHAR                         wanInterfaceName[16];
}
DEVICE_INFO;

/*******************************************************************
		           FUNCTION PROTOTYPES
********************************************************************/

/*********************** ARC CELLULAR DECLARATIONS *****************/

INT cellular_hal_mm_get_modem_state (CHAR *index, ModemState *State, ModemStateFailedReason *reason);
INT cellular_hal_mm_get_device_management (CHAR *index, CellularDeviceInfoStruct *devMngt);
INT cellular_hal_mm_get_device_data (CHAR *index, INT type, CHAR *data);
INT cellular_hal_mm_get_supported_access_technologies (CHAR *index, CHAR *accessTech);
INT cellular_hal_mm_get_prefered_access_technologies (CHAR *index, CHAR *accessTech);
INT cellular_hal_mm_get_current_access_technology (CHAR *index, CHAR *accessTech);
INT cellular_hal_mm_get_plmn (CHAR *index, CellularCurrentPlmnInfoStruct *plmnInfo);
INT cellular_hal_mm_get_sim_info (CHAR *index, CellularUICCSlotInfoStruct *pstSlotInfo);
INT cellular_hal_mm_get_sim_data (CHAR *index, INT type, CHAR *data);
INT cellular_hal_mm_set_signal_refresh_rate (CHAR *index, UINT rate);
INT cellular_hal_mm_get_signal (CHAR *index, CellularSignalInfoStruct *radioSignal);
INT cellular_hal_mm_get_bearer_properties (CHAR *index, CellularProfileStruct **pstProfileOutput);
INT cellular_hal_mm_get_bearer_properties_list (CHAR *modemIndex, CellularProfileStruct **pstProfileOutput, int *profile_count);
INT cellular_hal_mm_get_bearer_ipv4_config (CHAR *index, BEARER_IPV4_CONFIG *bearerIpv4Config);
INT cellular_hal_mm_get_bearer_ipv6_config (CHAR *index, BEARER_IPV6_CONFIG *bearerIpv6Config);
INT cellular_hal_mm_get_bearer_stats (CHAR *index, CellularPacketStatsStruct *network_packet_stats);
INT cellular_hal_mm_enable_modem (CHAR *index, BOOLEAN enable, cellular_device_slot_status_api_callback device_slot_status_cb);
INT cellular_hal_mm_create_bearer_properties (CHAR *index, CellularProfileStruct *pstProfileInput);
INT cellular_hal_mm_connect_bearer (CHAR *index, BOOLEAN connect);
INT cellular_hal_mm_set_access_technology (CHAR *index, ModemModeCombination *ModemMode);
void cellular_hal_mm_main_loop_init(void);
INT cellular_hal_mm_get_modem_index (CHAR *modemIndex);
INT cellular_hal_mm_get_sim_index_by_modem (CHAR *index, CHAR *simIndex);
INT cellular_hal_mm_get_bearer_index_by_modem (CHAR *index, CHAR *bearerIndex);
INT cellular_hal_mm_delete_bearer(CHAR *modemIndex, CellularProfileStruct *pstProfileInput);
INT cellular_hal_mm_enable_pin (CHAR *index, CHAR *pin_code, BOOLEAN enable);
INT cellular_hal_mm_enable_airplaneMode (CHAR *index, BOOLEAN enable);
INT cellular_hal_mm_get_bearer_info (CHAR *index, BEARER_PROPERTIES *bearerProperties, BEARER_IPV4_CONFIG *bearerIpv4Config, BEARER_IPV6_CONFIG *bearerIpv6Config);
INT cellular_hal_mm_get_current_modem_interface_status( CellularInterfaceStatus_t *status );
BOOLEAN cellular_hal_mm_is_modem_ready (CHAR *index);
INT cellular_hal_mm_set_modem_operating_configuration(CellularModemOperatingConfiguration_t modem_operating_config);
INT cellular_hal_mm_get_active_card_status( CellularUICCStatus_t *card_status );
void cellular_hal_mm_get_uicc_slots(CHAR *index, unsigned int *total_count);
void cellular_hal_mm_send_device_registration_status(cellular_device_registration_status_callback device_registration_status_cb);
void cellular_hal_mm_send_device_profile_status(cellular_device_profile_status_api_callback device_profile_status_cb);
void cellular_hal_mm_send_device_status(CellularDeviceContextCBStruct *device_status_cb);
void cellular_hal_mm_send_network_status(CellularNetworkCBStruct *network_status_cb);
INT cellular_hal_mm_get_bearer_data_interface(CHAR *index, CHAR* dataInterface);
#endif

/**
 * @}
 */


