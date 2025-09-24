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

#ifndef _CELLULARMGR_CELLULAR_APIS_H_
#define _CELLULARMGR_CELLULAR_APIS_H_

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include "ansc_platform.h"

#include "cellular_hal.h"
#include "cellularmgr_bus_utils.h"

/**********************************************************************
                STRUCTURE AND CONSTANT DEFINITIONS
**********************************************************************/
/*
*  This struct is for cellular object.
*/

/* Accesspoint list should be populated for below TTL interval */
#define CELLULAR_ACCESSPOINT_LIST_REFRESH_THRESHOLD      (120)
#define CELLULAR_UICCSLOT_LIST_REFRESH_THRESHOLD         (30)
#define CELLULAR_AVAILABLE_NETWORK_LIST_REFRESH_THRESHOLD (60)
#define CELLULAR_HAL_FETCH_INTERVAL                      (30)

#define CELLULAR_RADIO_ENV_EXCELLENT_THRESHOLD           (-85)
#define CELLULAR_RADIO_ENV_GOOD_THRESHOLD_HIGH           (-85)
#define CELLULAR_RADIO_ENV_GOOD_THRESHOLD_LOW            (-95)
#define CELLULAR_RADIO_ENV_FAIR_THRESHOLD_HIGH           (-95)
#define CELLULAR_RADIO_ENV_FAIR_THRESHOLD_LOW            (-105)
#define CELLULAR_RADIO_ENV_POOR_THRESHOLD_HIGH           (-105)
#define CELLULAR_RADIO_ENV_POOR_THRESHOLD_LOW            (-115)

#define CELLULAR_ICCID_MAX_LENGTH                        (21)

typedef enum _CELLULAR_RDK_STATUS
{
   RDK_STATUS_DOWN = 1,
   RDK_STATUS_DEACTIVATED,
   RDK_STATUS_DEREGISTERED,
   RDK_STATUS_REGISTERED,
   RDK_STATUS_CONNECTED
} 
CELLULAR_RDK_STATUS;

typedef enum _CELLULAR_CONTROL_INTERFACE_STATUS
{
   CONTROL_STATUS_OPENED = 1,
   CONTROL_STATUS_CLOSED
} 
CELLULAR_CONTROL_INTERFACE_STATUS;

typedef enum _CELLULAR_DATA_INTERFACE_LINK
{
   DATA_INTERFACE_LINK_RAW_IP = 1,
   DATA_INTERFACE_LINK_802P3
} 
CELLULAR_DATA_INTERFACE_LINK;

typedef enum _CELLULAR_RADIO_ENV_CONDITIONS
{
   RADIO_ENV_CONDITION_EXCELLENT = 1,
   RADIO_ENV_CONDITION_GOOD,
   RADIO_ENV_CONDITION_FAIR,
   RADIO_ENV_CONDITION_POOR,
   RADIO_ENV_CONDITION_UNAVAILABLE
} 
CELLULAR_RADIO_ENV_CONDITIONS;

typedef enum _CELLULAR_INTERFACE_ROAMING_STATUS
{
   ROAMING_STATUS_HOME = 1,
   ROAMING_STATUS_VISITOR
} 
CELLULAR_INTERFACE_ROAMING_STATUS;

typedef enum _CELLULAR_INTERFACE_REGISTERED_SERVICE_TYPE
{
   REGISTERED_SERVICE_NONE = 0,
   REGISTERED_SERVICE_PS,
   REGISTERED_SERVICE_CS,
   REGISTERED_SERVICE_CS_PS
} 
CELLULAR_INTERFACE_REGISTERED_SERVICE_TYPE;

typedef enum _CELLULAR_INTERFACE_SIM_STATUS
{
   SIM_STATUS_VALID = 1,
   SIM_STATUS_BLOCKED,
   SIM_STATUS_ERROR,
   SIM_STATUS_EMPTY
} 
CELLULAR_INTERFACE_SIM_STATUS;

typedef enum _CELLULAR_INTERFACE_SIM_FORMFACTOR
{
   USIM_1FF = 1,
   USIM_2FF,
   USIM_3FF,
   USIM_4FF,
   USIM_M2FF
} 
CELLULAR_INTERFACE_SIM_FORMFACTOR;

typedef enum _CELLULAR_INTERFACE_ESIM_FORMFACTOR
{
   ESIM_M2FF = 1,
   ESIM_ISIM
}
CELLULAR_INTERFACE_ESIM_FORMFACTOR;

typedef enum _CELLULAR_INTERFACE_SIM_APPLICATION
{
   APPLICATION_USIM = 1,
   APPLICATION_ESIM,
   APPLICATION_ISIM
} 
CELLULAR_INTERFACE_SIM_APPLICATION;

typedef enum _CELLULAR_INTERFACE_USIM_PINCHECK
{
   USIM_PINCHECK_ON_NETWORK_ACCESS = 1,
   USIM_PINCHECK_AFTER_REBOOT,
   USIM_PINCHECK_DISABLE
} 
CELLULAR_INTERFACE_USIM_PINCHECK;

typedef enum _CELLULAR_INTERFACE_PROFILE_FAMILY
{
   INTERFACE_PROFILE_FAMILY_IPV4 = 1,
   INTERFACE_PROFILE_FAMILY_IPV6,
   INTERFACE_PROFILE_FAMILY_IPV4_IPV6
} 
CELLULAR_INTERFACE_PROFILE_FAMILY;

typedef  struct
_CELLULAR_INTERFACE_ACCESSPOINT_INFO                                      
{
    BOOLEAN                               bIsThisNewlyAddedRecord;
    BOOLEAN                               Enable;
    BOOLEAN                               X_RDK_Default;
    CHAR                                  Alias[64];
    CHAR                                  APN[64];
    CellularPDPAuthentication_t           X_RDK_ApnAuthentication;
    CHAR                                  Username[256];
    CHAR                                  Password[256];
    CELLULAR_INTERFACE_PROFILE_FAMILY     X_RDK_IpAddressFamily;
    CellularPDPNetworkConfig_t            X_RDK_PdpInterfaceConfig;
    UINT                                  ProfileIndex;
    UINT                                  PDPContextNumber;
    BOOLEAN                               X_RDK_Roaming;
}
CELLULAR_INTERFACE_ACCESSPOINT_INFO,  *PCELLULAR_INTERFACE_ACCESSPOINT_INFO;

typedef  struct
_CELLULAR_INTERFACE_STATS_INFO                                         
{
    ULONG               BytesSent;
    ULONG               BytesReceived;
    ULONG               PacketsSent;
    ULONG               PacketsReceived;
    ULONG               PacketsSentDrop;
    ULONG               PacketsReceivedDrop;
    ULONG               UpStreamMaxBitRate;
    ULONG               DownStreamMaxBitRate;
}
CELLULAR_INTERFACE_STATS_INFO,  *PCELLULAR_INTERFACE_STATS_INFO;

typedef enum _CELLULAR_INTERFACE_RAT_INFO
{
   RAT_INFO_GSM = 1,
   RAT_INFO_UMTS,
   RAT_INFO_LTE
} 
CELLULAR_INTERFACE_RAT_INFO;

typedef  struct
_CELLULAR_INTERFACE_SERVING_INFO                                         
{
   CHAR                                   CellId[64];
   CELLULAR_INTERFACE_RAT_INFO            Rat;
   UINT                                   Rfcn;
   CHAR                                   PlmnId[64];
   CHAR                                   AreaCode[64];
   INT                                    Rssi;
   INT                                    Snr;
   INT                                    Rsrp;
   INT                                    Rsrq;
   INT                                    Trx;
   CELLULAR_RADIO_ENV_CONDITIONS          RadioEnvConditions;
}
CELLULAR_INTERFACE_SERVING_INFO,  *PCELLULAR_INTERFACE_SERVING_INFO;

typedef  struct
_CELLULAR_INTERFACE_NEIGHBOUR_INFO                                         
{
   CHAR                                   CellId[64];
   CELLULAR_INTERFACE_RAT_INFO            Rat;
   UINT                                   Rfcn;
   CHAR                                   PlmnId[64];
   CHAR                                   AreaCode[64];
   INT                                    ReceivedSignal;
}
CELLULAR_INTERFACE_NEIGHBOUR_INFO,  *PCELLULAR_INTERFACE_NEIGHBOUR_INFO;

typedef enum _CELLULAR_INTERFACE_CONTEXTPROFILE_STATUS
{
   CONTEXTPROFILE_STATUS_ACTIVE = 1,
   CONTEXTPROFILE_STATUS_INACTIVE
} 
CELLULAR_INTERFACE_CONTEXTPROFILE_STATUS;

typedef enum _CELLULAR_INTERFACE_CONTEXTPROFILE_TYPE
{
   CONTEXTPROFILE_TYPE_DEFAULT = 1,
   CONTEXTPROFILE_TYPE_DEDICATED
} 
CELLULAR_INTERFACE_CONTEXTPROFILE_TYPE;

typedef  struct
_CELLULAR_INTERFACE_CONTEXTPROFILE_INFO                                         
{
   CELLULAR_INTERFACE_CONTEXTPROFILE_STATUS      Status;
   CELLULAR_INTERFACE_CONTEXTPROFILE_TYPE        Type;
   CHAR                                          Apn[64];
   CELLULAR_INTERFACE_PROFILE_FAMILY             IpAddressFamily;
   CHAR                                          Ipv4Adress[16];
   CHAR                                          Ipv4SubnetMask[16];
   CHAR                                          Ipv4Gateway[16];
   CHAR                                          Ipv4PrimaryDns[16];
   CHAR                                          Ipv4SecondaryDns[16];
   CHAR                                          Ipv6Address[128];
   CHAR                                          Ipv6Gateway[128];
   CHAR                                          Ipv6PrimaryDns[128];
   CHAR                                          Ipv6SecondaryDns[128];
   UINT                                          MtuSize;
}
CELLULAR_INTERFACE_CONTEXTPROFILE_INFO,  *PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO;

typedef  struct
_CELLULAR_MNO_PROFILE_INFO                                         
{
    BOOLEAN                             Enable;
    CELLULAR_INTERFACE_SIM_STATUS       Status;
    CHAR                                Iccid[CELLULAR_ICCID_MAX_LENGTH];
    CHAR                                Msisdn[20];
    CHAR                                Imsi[20];
}
CELLULAR_MNO_PROFILE_INFO,  *PCELLULAR_MNO_PROFILE_INFO;

typedef  struct
_CELLULAR_EUICC_SLOT_INFO                                         
{
    BOOLEAN                             PowerEnable;
    CELLULAR_INTERFACE_SIM_STATUS       Status;
    CHAR                                Imei[16];
    CELLULAR_INTERFACE_ESIM_FORMFACTOR  FormFactor;
    CELLULAR_INTERFACE_SIM_APPLICATION  Application;
    CHAR                                EIccid[CELLULAR_ICCID_MAX_LENGTH];
    ULONG                               ulMNOProfileNoOfEntries;
    PCELLULAR_MNO_PROFILE_INFO          *pstMNPOProfileInfo;
}
CELLULAR_EUICC_SLOT_INFO,  *PCELLULAR_EUICC_SLOT_INFO;

typedef  struct
_CELLULAR_UICC_SLOT_INFO                                         
{
    UINT                                uiInstanceNumber;
    BOOLEAN                             PowerEnable;
    CELLULAR_INTERFACE_SIM_STATUS       Status;
    CELLULAR_INTERFACE_SIM_FORMFACTOR   FormFactor;
    CELLULAR_INTERFACE_SIM_APPLICATION  Application;
    CHAR                                MnoName[32];
    CHAR                                Iccid[CELLULAR_ICCID_MAX_LENGTH];
    CHAR                                Msisdn[20];
}
CELLULAR_UICC_SLOT_INFO,  *PCELLULAR_UICC_SLOT_INFO;

typedef  struct
_CELLULAR_PLMN_AVAILABLENETWORK_INFO                                         
{
    CHAR                                        MCC[8];
    CHAR                                        MNC[8];
    CHAR                                        Name[16];
    BOOLEAN                                     Allowed;
}
CELLULAR_PLMN_AVAILABLENETWORK_INFO,  *PCELLULAR_PLMN_AVAILABLENETWORK_INFO;

typedef  struct
_CELLULAR_PLMNACCESS_INFO                                         
{
    BOOLEAN                                     RoamingEnable;
    CELLULAR_INTERFACE_ROAMING_STATUS           RoamingStatus;
    CHAR                                        HomeNetwork_MCC[8];
    CHAR                                        HomeNetwork_MNC[8];
    CHAR                                        HomeNetwork_Name[16];
    CHAR                                        NetworkInUse_MCC[8];
    CHAR                                        NetworkInUse_MNC[8];
    CHAR                                        NetworkInUse_Name[16];
    ULONG                                       ulAvailableNetworkListLastUpdatedTime;
    UINT                                        ulAvailableNetworkNoOfEntries;
    PCELLULAR_PLMN_AVAILABLENETWORK_INFO        pstAvailableNetworks;
}
CELLULAR_PLMNACCESS_INFO,  *PCELLULAR_PLMNACCESS_INFO;

typedef  struct
_CELLULAR_INTERFACE_INFO                                         
{
    BOOLEAN                                     Enable;
    CellularInterfaceStatus_t                   Status;
    CHAR                                        Alias[64];
    CHAR                                        Name[64];
    ULONG                                       LastChange;
    CHAR                                        LowerLayers[1024];
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
    BOOLEAN                                     Upstream;
#endif
    CELLULAR_INTERFACE_REGISTERED_SERVICE_TYPE  X_RDK_RegisteredService;
    BOOLEAN                                     X_RDK_PhyConnectedStatus;
    BOOLEAN                                     X_RDK_LinkAvailableStatus;
    UINT                                        RegistrationRetries;
    UINT                                        MaxRegistrationRetries;
    UINT                                        RegistrationRetryTimer;
    CHAR                                        Imei[16];
    CHAR                                        Iccid[CELLULAR_ICCID_MAX_LENGTH];
    CHAR                                        SupportedAccessTechnologies[256];
    CHAR                                        PreferredAccessTechnologies[256];
    CHAR                                        CurrentAccessTechnology[256];
    CELLULAR_PLMNACCESS_INFO                    stPlmnAccessInfo;
    CELLULAR_INTERFACE_SERVING_INFO             stServingInfo;
    ULONG                                       ulNeighbourNoOfEntries;
    PCELLULAR_INTERFACE_NEIGHBOUR_INFO          pstNeighbourInfo;
    ULONG                                       ulContextProfileNoOfEntries;
    PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO     pstContextProfileInfo; 
    CELLULAR_INTERFACE_STATS_INFO               stStatsInfo;
    CELLULAR_RADIO_ENV_CONDITIONS               RadioEnvConditions;
}
CELLULAR_INTERFACE_INFO,  *PCELLULAR_INTERFACE_INFO;
typedef  struct
_MODEM_INTERFACE_INFO
{
    CHAR                                        Name[64];
    CHAR                                        Gateway[64];
    CHAR                                        IPv4Address[64];
    CHAR                                        SubnetMask[64];
}
MODEM_INTERFACE_INFO,  *PMODEM_INTERFACE_INFO;

typedef  struct
_CELLULAR_DML_INFO                                              
{
    CELLULAR_RDK_DEVICETYPE                X_RDK_DeviceType;
    MODEM_INTERFACE_INFO                   ModemInterfaceInfo;
    BOOLEAN                                X_RDK_Enable;
    CELLULAR_RDK_STATUS                    X_RDK_Status;
    CHAR                                   X_RDK_Model[20];
    CHAR                                   X_RDK_HardwareRevision[20];
    CHAR                                   X_RDK_Vendor[20];
    CHAR                                   CurrentImageVersion[128];
    CHAR                                   FallbackImageVersion[128];
    CHAR                                   X_RDK_ControlInterface[10];
    CELLULAR_CONTROL_INTERFACE_STATUS      X_RDK_ControlInterfaceStatus;
    CHAR                                   X_RDK_DataInterface[10];
    CELLULAR_DATA_INTERFACE_LINK           X_RDK_DataInterfaceLink;
    CHAR                                   X_RDK_Imei[16];
    ULONG                                  deviceInfoLastUpdatedTime;
    ULONG                                  ulInterfaceNoEntries;
    PCELLULAR_INTERFACE_INFO               pstInterfaceInfo;
    ULONG                                  ulUICCNoOfEntries;
    ULONG                                  ulUICCSlotListLastUpdatedTime;
    PCELLULAR_UICC_SLOT_INFO               pstUICCSlotInfo;
    CELLULAR_EUICC_SLOT_INFO               stEUICCSlotInfo;
    ULONG                                  ulAccessPointListLastUpdatedTime;
    ULONG                                  ulAccessPointNoOfEntries;
    PCELLULAR_INTERFACE_ACCESSPOINT_INFO   pstAPInfo;
}
CELLULAR_DML_INFO,  *PCELLULAR_DML_INFO;

/**********************************************************************
                FUNCTION PROTOTYPES
**********************************************************************/

ANSC_STATUS
DmlCellularInitialize
    (
        ANSC_HANDLE     hDml
    );
ANSC_STATUS DmlCellularResetDefaults(); // This route to be used only for resetting the DML to defaults
int CellularMgrUpdateLinkStatus ( char *wan_ifname, char *status );
int CellularMgrUpdateIfname ( char *wan_ifname, char *pCellularIfName );
int CellularMgrUpdateIpSource ( char *wan_ifname, char *pIpSource );
int CellularMgrGetLTEIfaceIndex ( char *wan_ifname );

int CellularMgrUpdatePhyStatus ( char *wan_ifname, CellularDeviceOpenStatus_t device_open_status );

int CellularMgr_AccessPointGetProfileList( CELLULAR_INTERFACE_ACCESSPOINT_INFO  **ppstAPInfo, int *profile_count );

int CellularMgr_AccessPointCreateProfile( PCELLULAR_INTERFACE_ACCESSPOINT_INFO pstAPInfo );

int CellularMgr_AccessPointDeleteProfile( PCELLULAR_INTERFACE_ACCESSPOINT_INFO pstAPInfo );

int CellularMgr_AccessPointModifyProfile( PCELLULAR_INTERFACE_ACCESSPOINT_INFO pstAPInfo );

int CellularMgr_RadioSignalGetSignalInfo( CELLULAR_INTERFACE_SERVING_INFO *pstServingInfo );

int CellularMgr_ServingSystemInfo( CELLULAR_INTERFACE_INFO  *pstInterfaceInfo, CELLULAR_INTERFACE_CONTEXTPROFILE_INFO *pstContextProfileInfo);

CELLULAR_RADIO_ENV_CONDITIONS CellularMgr_GetRadioEnvConditions( void );

int CellularMgr_SetModemEnable( BOOLEAN bEnable );

int CellularMgr_GetModemIMEI( char *pcIMEI );

int CellularMgr_GetModemIMEISoftwareVersion( char *pcIMEI_SV );

int CellularMgr_GetModemCurrentICCID( char *pcICCID );

int CellularMgr_GetNetworkPacketStatisticsInfo( PCELLULAR_INTERFACE_STATS_INFO pstStatsInfo );

CELLULAR_CONTROL_INTERFACE_STATUS CellularMgr_GetModemControlInterfaceStatus( void );

int CellularMgr_GetModemInterfaceStatus( CellularInterfaceStatus_t *interface_status );

int CellularMgr_SetModemInterfaceEnable( BOOLEAN bEnable );

int CellularMgr_SetSIMPowerEnable( UINT uiSlotID, BOOLEAN bEnable );

int CellularMgr_GetTotalUICCSlots( UINT *puiTotalSlots );

int CellularMgr_GetUICCSlotInfo(UINT uiSlotID, PCELLULAR_UICC_SLOT_INFO  pstUICCSlotInfo);

int CellularMgr_GetActiveCardStatus( CELLULAR_INTERFACE_SIM_STATUS *enCardStatus );

int CellularMgr_GetModemFirmwareVersion(char *pcFirmwareVersion);

int CellularMgr_GetPlmnInformation( PCELLULAR_PLMNACCESS_INFO pstPlmnAccessInfo);

int CellularMgr_GetAvailableNetworksInformation( PCELLULAR_PLMN_AVAILABLENETWORK_INFO *ppAvailableNetworkInfo, unsigned int *puiTotalCount );

BOOL CellularMgr_BlobUnpack(char* blob);

int CellularMgr_GetModemSupportedRadioTechnology( char *pSupportedRAT );

int CellularMgr_GetModemPreferredRadioTechnology( char *pPreferredRAT );

int CellularMgr_SetModemPreferredRadioTechnology( char *pPreferredRAT );

int CellularMgr_GetModemCurrentRadioTechnology( char *pCurrentRAT );

void CellularMgr_FactoryReset(void);
int CellularMgr_GetSupportedAccessTechnologies(char *pSupportedAccessTechnologies);

int CellularMgr_GetPreferedAccessTechnologies(char *pPreferedAccessTechnologies);

int CellularMgr_GetCurrentAccessTechnology(char *pCurrentAccessTechnology);

CELLULAR_RDK_DEVICETYPE CellularMgr_GetModemDeviceType();
int CellularMgr_GetServingInfo( CELLULAR_INTERFACE_SERVING_INFO *pstServingInfo );

int CellularMgr_GetDeviceInfo( PCELLULAR_DML_INFO pstDeviceInfo );

int CellularMgr_GetDataInterface(char *pDataInterface);

#endif //_CELLULARMGR_CELLULAR_APIS_H_
