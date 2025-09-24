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

#include <sysevent/sysevent.h>
#include "cellular_hal.h"
#include "cellularmgr_sm.h"
#include "cellularmgr_cellular_apis.h"
#include "cellular_hal_qmi_apis.h"

#if RBUS_BUILD_FLAG_ENABLE
#include "cellularmgr_rbus_events.h"
#endif

/**********************************************************************
                CONSTANT DEFINITIONS
**********************************************************************/

#define LOOP_TIMEOUT            500000 // timeout in milliseconds. This is the state machine loop interval

#define SYSEVENT_CM_WAN_V4_IPADDR   "cellular_wan_v4_ip"
#define SYSEVENT_CM_WAN_V4_SUBNET   "cellular_wan_v4_subnet"
#define SYSEVENT_CM_WAN_V4_GWADDR   "cellular_wan_v4_gw"
#define SYSEVENT_CM_WAN_V4_DNS1     "cellular_wan_v4_dns1"
#define SYSEVENT_CM_WAN_V4_DNS2     "cellular_wan_v4_dns2"
#define SYSEVENT_CM_WAN_V4_MTU      "cellular_wan_v4_mtu"
#define SYSEVENT_CM_WAN_V6_IPADDR   "cellular_wan_v6_ip"
#define SYSEVENT_CM_WAN_V6_GWADDR   "cellular_wan_v6_gw"
#define SYSEVENT_CM_WAN_V6_DNS1     "cellular_wan_v6_dns1"
#define SYSEVENT_CM_WAN_V6_DNS2     "cellular_wan_v6_dns2"
#define SYSEVENT_CM_WAN_V6_MTU      "cellular_wan_v6_mtu"
#define SYSEVENT_IPV6_NAMESERVER    "ipv6_nameserver"
#define SYSEVENT_IPV4_NAMESERVER    "ipv4_nameserver"

#define DEFAULT_MTU_SIZE            1500
/**********************************************************************
    GLOBAL or LOCAL DEFINITIONS and STRUCTURE or ENUM DECLARATION
**********************************************************************/

/** Policy Control SM */
typedef  struct                                     
{
    CellularPolicySmState_t                   enCurrentSMState;
    unsigned char                             bRDKEnable;
    CellularDeviceDetectionStatus_t           enDeviceDetectionStatus;
    CellularDeviceOpenStatus_t                enDeviceOpenStatus;     
    char                                      acDeviceName[64];
    char                                      acWANIfName[16];
    CellularDeviceSlotStatus_t                enDeviceSlotSelectionStatus;
    unsigned int                              SelectedSlotNumber;
    CellularDeviceNASStatus_t                 enDeviceNASRegisterStatus;
    CellularDeviceNASRoamingStatus_t          enDeviceNASRoamingStatus;
    CellularModemRegisteredServiceType_t      enRegisteredService;
    CellularDeviceProfileSelectionStatus_t    enDeviceProfileSelectionStatus;
    char                                      acChoosedProfileName[64];
    CellularPDPType_t                         enPDPTypeForSelectedProfile;
    CellularProfileStruct                     stContextProfile;
    unsigned char                             bIPv4NetworkStartInProgress;
    CellularDeviceIPReadyStatus_t             enNetworkIPv4IPReadyStatus;
    CellularIPStruct                          stIPv4Info;
    CellularNetworkPacketStatus_t             enNetworkIPv4PacketServiceStatus;
    unsigned char                             bIPv4WaitingForPacketStatus;
    unsigned char                             bIPv6NetworkStartInProgress;
    CellularDeviceIPReadyStatus_t             enNetworkIPv6IPReadyStatus;
    CellularIPStruct                          stIPv6Info;
    CellularNetworkPacketStatus_t             enNetworkIPv6PacketServiceStatus;
    unsigned char                             bIPv6WaitingForPacketStatus;
    PCELLULAR_INTERFACE_INFO                  pCmIfData;
    PCELLULAR_DML_INFO                        pstDmlCellular; 

} CellularMgrPolicyCtrlSMStruct;

/* STATES */
static CellularPolicySmState_t StateDown( void );                    
static CellularPolicySmState_t StateDeactivated( void);                        
static CellularPolicySmState_t StateDeregistered( void);                     
static CellularPolicySmState_t StateRegistering( void );                              
static CellularPolicySmState_t StateRegistered( void );  
static CellularPolicySmState_t StateConnected( void ); 

/* TRANSITIONS */
static CellularPolicySmState_t TransitionDown( void );  
static CellularPolicySmState_t TransitionDeactivated( void );                                                    
static CellularPolicySmState_t TransitionDeregistered( void );                         
static CellularPolicySmState_t TransitionRegistering( void );  
static CellularPolicySmState_t TransitionRegistered( void );  
static CellularPolicySmState_t TransitionRegisteredStartNetwork( void );
static CellularPolicySmState_t TransitionConnected( void );
static CellularPolicySmState_t TransitionConnectedStopNetwork( void );

static void* CellularMgr_StateMachine_Thread( void *arg );
static void CellularMgrSMSetCurrentState( CellularPolicySmState_t smState);

static CellularMgrPolicyCtrlSMStruct      *gpstCellularPolicyCtrl = NULL;
static CellularPolicySmState_t policy_sm_state_old = CELLULAR_STATE_DOWN;

extern int sysevent_fd;
extern token_t sysevent_token;
static int iter=0;
static int max_wait = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**********************************************************************
                FUNCTION DEFINITION
**********************************************************************/

int CellularMgrDeviceRemovedStatusCBForSM(char *device_name, CellularDeviceDetectionStatus_t device_detection_status )
{   
    if( NULL == device_name )
    {
        CcspTraceInfo(("%s - Invalid params received \n", __FUNCTION__));
        return RETURN_ERROR;
    }

    CcspTraceInfo(("%s - Received Device Detection Status - DeviceName[%s] Status[%d]\n", __FUNCTION__, device_name, device_detection_status));

    if ( NULL == gpstCellularPolicyCtrl)
    {
        CcspTraceError (("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    gpstCellularPolicyCtrl->enDeviceDetectionStatus = device_detection_status;
    return RETURN_OK;
}

int CellularMgrDeviceOpenStatusCBForSM(char *device_name, char *wan_ifname, CellularDeviceOpenStatus_t device_open_status, CellularModemOperatingConfiguration_t modem_operating_mode )
{   
    if( ( NULL == device_name ) || ( NULL == wan_ifname ) )
    {
        CcspTraceInfo(("%s - Invalid params received \n", __FUNCTION__));
        return RETURN_ERROR;
    }

    if ( NULL == gpstCellularPolicyCtrl)
    {
        CcspTraceError (("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    gpstCellularPolicyCtrl->enDeviceOpenStatus = device_open_status;
    memset(gpstCellularPolicyCtrl->acDeviceName, 0, sizeof(gpstCellularPolicyCtrl->acDeviceName));
    snprintf(gpstCellularPolicyCtrl->acDeviceName, sizeof(gpstCellularPolicyCtrl->acDeviceName), "%s", device_name);

    memset(gpstCellularPolicyCtrl->acWANIfName, 0, sizeof(gpstCellularPolicyCtrl->acWANIfName));
    snprintf(gpstCellularPolicyCtrl->acWANIfName, sizeof(gpstCellularPolicyCtrl->acWANIfName), "%s", wan_ifname);

    if( CELLULAR_MODEM_SET_ONLINE == modem_operating_mode )
    {
        PCELLULAR_DML_INFO  pstDmlCellular = gpstCellularPolicyCtrl->pstDmlCellular; 

        CellularMgrSMSetCellularEnable(TRUE);
        pstDmlCellular->X_RDK_Enable = TRUE;
    }
    else
    {
        PCELLULAR_DML_INFO  pstDmlCellular = gpstCellularPolicyCtrl->pstDmlCellular; 

        CellularMgrSMSetCellularEnable(FALSE);
        pstDmlCellular->X_RDK_Enable = FALSE;
    }

    CcspTraceInfo(("%s - Received Device Open Status - DeviceName[%s] Status[%d] Wwan[%s] IsModemOnline[%d]\n", __FUNCTION__, device_name, device_open_status, wan_ifname, CellularMgrSMGetCellularEnable()));

    return RETURN_OK;
}

int CellularMgrDeviceSlotStatusCBForSM(char *slot_name, char *slot_type, int slot_num, CellularDeviceSlotStatus_t device_slot_status )
{   
    if( ( NULL == slot_name ) || ( NULL == slot_type ) )
    {
        CcspTraceInfo(("%s - Invalid params received \n", __FUNCTION__));
        return RETURN_ERROR;
    }

    CcspTraceInfo(("%s - Received Device Slot Status - SlotName[%s] SlotType[%s] SlotNum[%d] Status[%d]\n", __FUNCTION__, slot_name, slot_type, slot_num, device_slot_status));
    
    
    if ( NULL == gpstCellularPolicyCtrl)
    {
        CcspTraceError (("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus = device_slot_status;
    gpstCellularPolicyCtrl->SelectedSlotNumber = slot_num;

    return RETURN_OK;
}

int cellular_get_serving_info(int *registration_status, int *roaming_status,  int *attach_status)
{

    pthread_mutex_lock(&mutex);
    *registration_status = gpstCellularPolicyCtrl->enDeviceNASRegisterStatus;
    *roaming_status = gpstCellularPolicyCtrl->enDeviceNASRoamingStatus;
    if( (gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus == DEVICE_NETWORK_STATUS_DISCONNECTED) &&
        (gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus == DEVICE_NETWORK_STATUS_DISCONNECTED))
    {
        *attach_status = PROFILE_STATUS_INACTIVE;
    }
    else {
        *attach_status = PROFILE_STATUS_ACTIVE;
    }
    pthread_mutex_unlock(&mutex);
}

int CellularMgrDeviceRegistrationStatusCBForSM( CellularDeviceNASStatus_t device_registration_status,
                                                CellularDeviceNASRoamingStatus_t roaming_status,
                                                CellularModemRegisteredServiceType_t registered_service )
{   
    CcspTraceInfo(("%s - Received Device Registration Status - Status[%d] roaming_status : [%d] Service[%d]\n", __FUNCTION__,device_registration_status,roaming_status,registered_service));
    
    if ( NULL == gpstCellularPolicyCtrl)
    {
        CcspTraceError (("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    pthread_mutex_lock(&mutex);
    if( (gpstCellularPolicyCtrl->enDeviceNASRegisterStatus == DEVICE_NAS_STATUS_REGISTERED )
         && (device_registration_status == DEVICE_NAS_STATUS_NOT_REGISTERED))
    {
        gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = DEVICE_NAS_STATUS_REGISTERING;
    }
    else
        gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = device_registration_status;
    gpstCellularPolicyCtrl->enDeviceNASRoamingStatus = roaming_status;
    gpstCellularPolicyCtrl->enRegisteredService       = registered_service;
    pthread_mutex_unlock(&mutex);

    return RETURN_OK;
}

int CellularMgrProfileStatusCBForSM(char *profile_name, CellularPDPType_t  PDPType, CellularDeviceProfileSelectionStatus_t profile_status )
{   
    if( NULL == profile_name )
    {
        CcspTraceInfo(("%s - Invalid params received \n", __FUNCTION__));
        return RETURN_ERROR;
    }

    CcspTraceInfo(("%s - Received Profile Slot Status - ProfileName[%s] PDPType[%d] ProfileStatus[%d]\n", __FUNCTION__, profile_name, PDPType, profile_status));
   
    if ( NULL == gpstCellularPolicyCtrl)
    {
        CcspTraceError (("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    snprintf(gpstCellularPolicyCtrl->acChoosedProfileName, sizeof(gpstCellularPolicyCtrl->acChoosedProfileName), "%s", profile_name);
    gpstCellularPolicyCtrl->enDeviceProfileSelectionStatus = profile_status;
    gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile    = PDPType;

    return RETURN_OK;
}

int CellularMgrIPReadyCBForSM( CellularIPStruct *pstIPStruct, CellularDeviceIPReadyStatus_t ip_ready_status )
{   
    char str_value[32];
    char dns_buf[256] = {0};
    char *pIpSource = NULL;
    if( NULL == pstIPStruct )
    {
        CcspTraceInfo(("%s - IP Information not available\n", __FUNCTION__));
        return RETURN_ERROR;
    }
    else
    {

        CcspTraceInfo(("%s - Received IP configuration - IPType[IPv%d] Address[%s] GW[%s] Subnet[%s] NS1[%s] NS2[%s] MTU[%d]\n", 
                                                        __FUNCTION__, 
                                                        ( ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pstIPStruct->IPType ) ?  4 : 6 ) ,
                                                        pstIPStruct->IPAddress, 
                                                        pstIPStruct->DefaultGateWay,
                                                        pstIPStruct->SubnetMask,
                                                        pstIPStruct->DNSServer1,
                                                        pstIPStruct->DNSServer2,
                                                        pstIPStruct->MTUSize));
    }

    if ( NULL == gpstCellularPolicyCtrl)
    {
        CcspTraceError (("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pstIPStruct->IPType )
    {
        gpstCellularPolicyCtrl->bIPv4NetworkStartInProgress = FALSE;
        gpstCellularPolicyCtrl->enNetworkIPv4IPReadyStatus  = ip_ready_status;
        if (gpstCellularPolicyCtrl->enNetworkIPv4IPReadyStatus == DEVICE_NETWORK_IP_NOT_READY){
            gpstCellularPolicyCtrl->bIPv4WaitingForPacketStatus        = FALSE;
            gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;
        }
        else
        {
           memset(&gpstCellularPolicyCtrl->stIPv4Info, 0, sizeof(CellularIPStruct));
           memcpy(&gpstCellularPolicyCtrl->stIPv4Info, pstIPStruct, sizeof(CellularIPStruct));
        }

	//Set associated sysevents
        if(sysevent_fd >= 0)
        {
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_SUBNET, pstIPStruct->SubnetMask, 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_GWADDR, pstIPStruct->DefaultGateWay, 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_DNS1, pstIPStruct->DNSServer1, 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_DNS2, pstIPStruct->DNSServer2, 0);
            snprintf(dns_buf, sizeof(dns_buf), "%s %s",pstIPStruct->DNSServer1,pstIPStruct->DNSServer2);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_IPV4_NAMESERVER,dns_buf, 0);
            snprintf(str_value, sizeof(str_value), "%d", pstIPStruct->MTUSize);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_MTU,str_value, 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_IPADDR, pstIPStruct->IPAddress, 0);
	}

    }
    else
    {
        gpstCellularPolicyCtrl->bIPv6NetworkStartInProgress = FALSE;
        gpstCellularPolicyCtrl->enNetworkIPv6IPReadyStatus  = ip_ready_status;
        if (gpstCellularPolicyCtrl->enNetworkIPv6IPReadyStatus == DEVICE_NETWORK_IP_NOT_READY){
            gpstCellularPolicyCtrl->bIPv6WaitingForPacketStatus        = FALSE;
            gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;
        }
        else
        {
           memset(&gpstCellularPolicyCtrl->stIPv6Info, 0, sizeof(CellularIPStruct));
           memcpy(&gpstCellularPolicyCtrl->stIPv6Info, pstIPStruct, sizeof(CellularIPStruct));
        }

	//Set associated sysevents
        if(sysevent_fd >= 0)
        {
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_GWADDR, pstIPStruct->DefaultGateWay, 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_DNS1, pstIPStruct->DNSServer1, 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_DNS2, pstIPStruct->DNSServer2, 0);
            snprintf(dns_buf, sizeof(dns_buf), "%s %s",pstIPStruct->DNSServer1,pstIPStruct->DNSServer2);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_IPV6_NAMESERVER,dns_buf, 0);
            snprintf(str_value, sizeof(str_value), "%d", pstIPStruct->MTUSize);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_MTU,str_value, 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_IPADDR, pstIPStruct->IPAddress, 0);
	}
      }
#ifdef HYBRID_SUPPORT
    CellularMgrUpdateIfname ( gpstCellularPolicyCtrl->acWANIfName, pstIPStruct->WANIFName );
    CcspTraceInfo(("%s - Updating Physical Status for '%s' device '%s' interface, Status[%d]\n", __FUNCTION__, gpstCellularPolicyCtrl->acDeviceName, gpstCellularPolicyCtrl->acWANIfName, gpstCellularPolicyCtrl->enDeviceOpenStatus ));
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
    if (strlen(pstIPStruct->IPAddress) > 0)
    {
        pIpSource = DML_PARAM_VALUE_IPSOURCE_STATIC;
        if (pstIPStruct->MTUSize <= 0)
        {
            pstIPStruct->MTUSize = DEFAULT_MTU_SIZE;
        }
    }
    else
    {
        pIpSource = DML_PARAM_VALUE_IPSOURCE_DHCP;
    }
    CellularMgrUpdateIpSource ( gpstCellularPolicyCtrl->acWANIfName, pIpSource);
#endif

    if ( CellularMgrUpdatePhyStatus (gpstCellularPolicyCtrl->acWANIfName, gpstCellularPolicyCtrl->enDeviceOpenStatus) != RETURN_OK )
    {
        CcspTraceError(("%s - Failed to set Interface Phy status\n", __FUNCTION__));
        return RETURN_ERROR;
    }
    sleep(1);
    CcspTraceInfo(("%s - Updating link status to UP for this '%s' interface phy.ifname %s \n", __FUNCTION__, gpstCellularPolicyCtrl->acWANIfName,pstIPStruct->WANIFName));
    if ( CellularMgrUpdateLinkStatus (gpstCellularPolicyCtrl->acWANIfName, UP_STR) != RETURN_OK)
    {
        CcspTraceInfo(("%s %d: Failed to set LinkStatus in Interface table\n", __FUNCTION__, __LINE__));
    }
#endif
    //Need to inform all ip details to WAN manager
    if ( CellularMgr_Util_SendIPToWanMgr( pstIPStruct ) != RETURN_OK ) 
    {
        CcspTraceError(("%s - Failed to send IP info to WanManager \n", __FUNCTION__));
        return RETURN_ERROR;
    }
#ifdef HYBRID_SUPPORT
    if (ip_ready_status == DEVICE_NETWORK_IP_READY)
    {
        gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = true;
        CcspTraceInfo(("%s %d - Updated X_RDK_LinkAvailableStatus to true \n", __FUNCTION__, __LINE__));
    }
    else
    {
        gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = false;
        CcspTraceInfo(("%s %d - Updated X_RDK_LinkAvailableStatus to false \n", __FUNCTION__, __LINE__));
    }
#endif
    return RETURN_OK;
}

int CellularMgrGetCurrentPDPContextStatusInformation( PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO  pstContextProfileInfo )
{
    if( NULL == pstContextProfileInfo )
    {
        CcspTraceError(("%s - Failed to get context information because of invalid argument\n",__FUNCTION__));
        return RETURN_ERROR;
    }

    if ( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError(("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    //Reinitialize context profile information
    memset( pstContextProfileInfo, 0, sizeof(CELLULAR_INTERFACE_CONTEXTPROFILE_INFO) );

    pstContextProfileInfo->Type = CONTEXTPROFILE_TYPE_DEFAULT;

    if( CELLULAR_STATE_CONNECTED == gpstCellularPolicyCtrl->enCurrentSMState )
    {
        pstContextProfileInfo->Status = CONTEXTPROFILE_STATUS_ACTIVE;

        switch( gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile )
        {
            case CELLULAR_PDP_TYPE_IPV4:
            {
                pstContextProfileInfo->IpAddressFamily = INTERFACE_PROFILE_FAMILY_IPV4;
            }
            break;
            case CELLULAR_PDP_TYPE_IPV6:
            {
                pstContextProfileInfo->IpAddressFamily = INTERFACE_PROFILE_FAMILY_IPV6;
            }
            break;
            case CELLULAR_PDP_TYPE_IPV4_OR_IPV6:
            {
                pstContextProfileInfo->IpAddressFamily = INTERFACE_PROFILE_FAMILY_IPV4_IPV6;
            }
            break;
        }

        snprintf(pstContextProfileInfo->Apn, sizeof(pstContextProfileInfo->Apn), gpstCellularPolicyCtrl->stContextProfile.APN);
    }
    else
    {
        pstContextProfileInfo->Status = CONTEXTPROFILE_STATUS_INACTIVE;
        pstContextProfileInfo->IpAddressFamily = INTERFACE_PROFILE_FAMILY_IPV4_IPV6;
    }

    if( DEVICE_NETWORK_IP_READY == gpstCellularPolicyCtrl->enNetworkIPv4IPReadyStatus )
    {
        CellularIPStruct *pstIpInfo = &gpstCellularPolicyCtrl->stIPv4Info;

        snprintf(pstContextProfileInfo->Ipv4Adress, sizeof(pstContextProfileInfo->Ipv4Adress), "%s", pstIpInfo->IPAddress);
        snprintf(pstContextProfileInfo->Ipv4SubnetMask, sizeof(pstContextProfileInfo->Ipv4SubnetMask), "%s", pstIpInfo->SubnetMask);
        snprintf(pstContextProfileInfo->Ipv4Gateway, sizeof(pstContextProfileInfo->Ipv4Gateway), "%s", pstIpInfo->DefaultGateWay);
        snprintf(pstContextProfileInfo->Ipv4PrimaryDns, sizeof(pstContextProfileInfo->Ipv4PrimaryDns), "%s", pstIpInfo->DNSServer1);
        snprintf(pstContextProfileInfo->Ipv4SecondaryDns, sizeof(pstContextProfileInfo->Ipv4SecondaryDns), "%s", pstIpInfo->DNSServer2);
        pstContextProfileInfo->MtuSize = pstIpInfo->MTUSize;
    }

    if( DEVICE_NETWORK_IP_READY == gpstCellularPolicyCtrl->enNetworkIPv6IPReadyStatus )
    {
        CellularIPStruct *pstIpInfo = &gpstCellularPolicyCtrl->stIPv6Info;

        snprintf(pstContextProfileInfo->Ipv6Address, sizeof(pstContextProfileInfo->Ipv6Address), "%s", pstIpInfo->IPAddress);
        snprintf(pstContextProfileInfo->Ipv6Gateway, sizeof(pstContextProfileInfo->Ipv6Gateway), "%s", pstIpInfo->DefaultGateWay);
        snprintf(pstContextProfileInfo->Ipv6PrimaryDns, sizeof(pstContextProfileInfo->Ipv6PrimaryDns), "%s", pstIpInfo->DNSServer1);
        snprintf(pstContextProfileInfo->Ipv6SecondaryDns, sizeof(pstContextProfileInfo->Ipv6SecondaryDns), "%s", pstIpInfo->DNSServer2);
        pstContextProfileInfo->MtuSize = pstIpInfo->MTUSize;
    }

    return RETURN_OK;
}

int CellularMgrGetNetworkRegisteredService( CELLULAR_INTERFACE_REGISTERED_SERVICE_TYPE  *penRegisteredService )
{
    if( NULL == penRegisteredService )
    {
        CcspTraceError(("%s - Failed to get network registered information because of invalid argument\n",__FUNCTION__));
        return RETURN_ERROR;
    }

    if ( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError(("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    switch( gpstCellularPolicyCtrl->enRegisteredService )
    {
        case CELLULAR_MODEM_REGISTERED_SERVICE_NONE:
        {
            *penRegisteredService = REGISTERED_SERVICE_NONE;
        }
        break;

        case CELLULAR_MODEM_REGISTERED_SERVICE_PS:
        {
            *penRegisteredService = REGISTERED_SERVICE_PS;
        }
        break;

        case CELLULAR_MODEM_REGISTERED_SERVICE_CS:
        {
            *penRegisteredService = REGISTERED_SERVICE_CS;
        }
        break;

        case CELLULAR_MODEM_REGISTERED_SERVICE_CS_PS:
        {
            *penRegisteredService = REGISTERED_SERVICE_CS_PS;
        }
        break;

        default:
        {
            *penRegisteredService = REGISTERED_SERVICE_NONE;
        }

    }

    return RETURN_OK;
}

int CellularMgrPacketServiceStatusCBForSM( char *device_name, CellularNetworkIPType_t ip_type, CellularNetworkPacketStatus_t packet_service_status )
{   
    CcspTraceInfo(("%s - Received Packet Service Status - Device:%s IPType:%d ServiceStatus[%d]\n", __FUNCTION__, device_name, ip_type, packet_service_status));
 
    pthread_mutex_lock(&mutex);
    if ( NULL == gpstCellularPolicyCtrl)
    {
        CcspTraceError (("%s %d: sm controller object is empty\n", __FUNCTION__, __LINE__));
	pthread_mutex_unlock(&mutex);
        return RETURN_ERROR;
    }

    if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == ip_type )
    {
        gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus = packet_service_status;
        gpstCellularPolicyCtrl->bIPv4NetworkStartInProgress  = FALSE;
        gpstCellularPolicyCtrl->bIPv4WaitingForPacketStatus  = FALSE;
    }
    else if( CELLULAR_NETWORK_IP_FAMILY_IPV6 == ip_type )
    {
        gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus  = packet_service_status;
        gpstCellularPolicyCtrl->bIPv6NetworkStartInProgress  = FALSE;
        gpstCellularPolicyCtrl->bIPv6WaitingForPacketStatus  = FALSE;
    }
    pthread_mutex_unlock(&mutex);
    return RETURN_OK;
}

/* Cellular Transitions */
static CellularPolicySmState_t TransitionDown( void )
{
    CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_DOWN \n", __FUNCTION__, __LINE__));
#ifdef HYBRID_SUPPORT
    // Reset the DML
    DmlCellularResetDefaults();
#endif
    CellularMgrSMSetCurrentState(CELLULAR_STATE_DOWN);

    gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus        = DEVICE_SLOT_STATUS_NOT_READY;
    gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;
    gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;

    //PhyStatus should be down
    CcspTraceInfo(("%s - Updating physical status to DOWN for this '%s' interface\n", __FUNCTION__, gpstCellularPolicyCtrl->acWANIfName));
    if ( CellularMgrUpdatePhyStatus (gpstCellularPolicyCtrl->acWANIfName, DEVICE_OPEN_STATUS_NOT_READY) != RETURN_OK)
    {
        CcspTraceInfo(("%s %d: Failed to set PhyStatus in Interface table\n", __FUNCTION__, __LINE__));
    }
#ifdef HYBRID_SUPPORT
    gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = false;
    CcspTraceError(("%s %d - Updated X_RDK_LinkAvailableStatus to false \n", __FUNCTION__, __LINE__));
#endif
    //LinkStatus should be down
    CcspTraceInfo(("%s - Updating link status to Down for this '%s' interface \n", __FUNCTION__, gpstCellularPolicyCtrl->acWANIfName));
    if ( CellularMgrUpdateLinkStatus (gpstCellularPolicyCtrl->acWANIfName, DOWN_STR) != RETURN_OK)
    {
        CcspTraceInfo(("%s %d: Failed to set LinkStatus in Interface table\n", __FUNCTION__, __LINE__));
    }

    return CELLULAR_STATE_DOWN;
}

static CellularPolicySmState_t TransitionDeactivated( void )
{
    CellularDeviceContextCBStruct stDeviceCtxCB = {0};

    //PhyStatus should be down
    CcspTraceInfo(("%s - Updating physical status to DOWN for this '%s' interface\n", __FUNCTION__, gpstCellularPolicyCtrl->acWANIfName));
    if ( CellularMgrUpdatePhyStatus (gpstCellularPolicyCtrl->acWANIfName, DEVICE_OPEN_STATUS_NOT_READY) != RETURN_OK)
    {
        CcspTraceInfo(("%s %d: Failed to set PhyStatus in Interface table\n", __FUNCTION__, __LINE__));
    }
#ifdef HYBRID_SUPPORT
    gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = false;
    CcspTraceError(("%s %d - Updated X_RDK_LinkAvailableStatus to false \n", __FUNCTION__, __LINE__));
#endif
    //LinkStatus should be down
    CcspTraceInfo(("%s - Updating link status to Down for this '%s' interface \n", __FUNCTION__, gpstCellularPolicyCtrl->acWANIfName));
    if ( CellularMgrUpdateLinkStatus (gpstCellularPolicyCtrl->acWANIfName, DOWN_STR) != RETURN_OK)
    {
        CcspTraceInfo(("%s %d: Failed to set LinkStatus in Interface table\n", __FUNCTION__, __LINE__));
    }

    gpstCellularPolicyCtrl->enDeviceOpenStatus = DEVICE_OPEN_STATUS_INPROGRESS;

    //Request Device Open
    stDeviceCtxCB.device_remove_status_cb = CellularMgrDeviceRemovedStatusCBForSM;
    stDeviceCtxCB.device_open_status_cb   = CellularMgrDeviceOpenStatusCBForSM;

    cellular_hal_open_device( &stDeviceCtxCB );
    return CELLULAR_STATE_DEACTIVATED;
}

static CellularPolicySmState_t TransitionDeregistered( void )
{
#ifndef HYBRID_SUPPORT
    CcspTraceInfo(("%s - Updating Physical Status for '%s' device '%s' interface, Status[%d]\n", __FUNCTION__, gpstCellularPolicyCtrl->acDeviceName, gpstCellularPolicyCtrl->acWANIfName, gpstCellularPolicyCtrl->enDeviceOpenStatus ));
    if ( CellularMgrUpdatePhyStatus (gpstCellularPolicyCtrl->acWANIfName, gpstCellularPolicyCtrl->enDeviceOpenStatus) != RETURN_OK )
    {
        CcspTraceError(("%s - Failed to set Interface Phy status\n", __FUNCTION__));
        return RETURN_ERROR;
    }
#endif
    gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus = DEVICE_SLOT_STATUS_SELECTING;
    gpstCellularPolicyCtrl->enDeviceNASRegisterStatus   = DEVICE_NAS_STATUS_NOT_REGISTERED;
    gpstCellularPolicyCtrl->enRegisteredService         = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;

    //Request Device Slot Selection
    cellular_hal_select_device_slot( CellularMgrDeviceSlotStatusCBForSM );

    return CELLULAR_STATE_DEREGISTERED;
}

static CellularPolicySmState_t TransitionRegistering( void )
{
    //Validate Input
    if( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError (("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return CELLULAR_STATUS_ERROR;
    }

    gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = DEVICE_NAS_STATUS_REGISTERING;

    //Request Device NAS Registration
    cellular_hal_monitor_device_registration( CellularMgrDeviceRegistrationStatusCBForSM );

    return CELLULAR_STATE_DEREGISTERED;
}

static CellularPolicySmState_t TransitionRegistered( void )
{
    gpstCellularPolicyCtrl->bIPv4NetworkStartInProgress         = FALSE;
    gpstCellularPolicyCtrl->bIPv6NetworkStartInProgress         = FALSE;
    gpstCellularPolicyCtrl->bIPv4WaitingForPacketStatus         = FALSE;
    gpstCellularPolicyCtrl->bIPv6WaitingForPacketStatus         = FALSE;
    gpstCellularPolicyCtrl->enDeviceProfileSelectionStatus      = DEVICE_PROFILE_STATUS_CONFIGURING;
    gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus    = DEVICE_NETWORK_STATUS_DISCONNECTED;
    gpstCellularPolicyCtrl->enNetworkIPv4IPReadyStatus          = DEVICE_NETWORK_IP_NOT_READY;
    gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus    = DEVICE_NETWORK_STATUS_DISCONNECTED;
    gpstCellularPolicyCtrl->enNetworkIPv6IPReadyStatus          = DEVICE_NETWORK_IP_NOT_READY;

    //Request Device Profile Create
    cellular_hal_profile_create( &(gpstCellularPolicyCtrl->stContextProfile), CellularMgrProfileStatusCBForSM );

    return CELLULAR_STATE_REGISTERED;
}

static CellularPolicySmState_t TransitionRegisteredStartNetwork( void )
{
    CellularNetworkCBStruct stNetworkCB = {0};
#ifndef HYBRID_SUPPORT
    //LinkStatus should be Up
    CcspTraceInfo(("%s - Updating link status to UP for this '%s' interface \n", __FUNCTION__, gpstCellularPolicyCtrl->acWANIfName));
    if ( CellularMgrUpdateLinkStatus (gpstCellularPolicyCtrl->acWANIfName, UP_STR) != RETURN_OK)
    {
        CcspTraceInfo(("%s %d: Failed to set LinkStatus in Interface table\n", __FUNCTION__, __LINE__));
    }

    /*
     * Provided Delay,
     * Needs to give some delay before starting network since WanManager needs to settle in Optaining IP address 
     * state to receive lease information since WAN manager won't start DHCP client so that should be provide some delay
     * here to avoid syncrhonization issue between cellular and wan manager states
     
    
     sleep(3);*/
#endif

    //Request to start Network IPv4
    if( ( CELLULAR_PDP_TYPE_IPV4 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) || 
        ( CELLULAR_PDP_TYPE_IPV4_OR_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile )
        )
    {
        gpstCellularPolicyCtrl->bIPv4NetworkStartInProgress        = TRUE;
        gpstCellularPolicyCtrl->bIPv4WaitingForPacketStatus        = TRUE;
        gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;
        gpstCellularPolicyCtrl->enNetworkIPv4IPReadyStatus         = DEVICE_NETWORK_IP_NOT_READY;

        stNetworkCB.device_network_ip_ready_cb = CellularMgrIPReadyCBForSM;
        stNetworkCB.packet_service_status_cb   = CellularMgrPacketServiceStatusCBForSM;

        CcspTraceInfo(("%s %d - Starting IPv4 Network\n", __FUNCTION__, __LINE__));
        cellular_hal_start_network( CELLULAR_NETWORK_IP_FAMILY_IPV4, NULL, &stNetworkCB );
    }

    //Request to start Network IPv6
    if( ( CELLULAR_PDP_TYPE_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) || 
        ( CELLULAR_PDP_TYPE_IPV4_OR_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile )
        )
    {
        gpstCellularPolicyCtrl->bIPv6NetworkStartInProgress        = TRUE;
        gpstCellularPolicyCtrl->bIPv6WaitingForPacketStatus        = TRUE;
        gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;
        gpstCellularPolicyCtrl->enNetworkIPv6IPReadyStatus         = DEVICE_NETWORK_IP_NOT_READY;

        stNetworkCB.device_network_ip_ready_cb = CellularMgrIPReadyCBForSM;
        stNetworkCB.packet_service_status_cb   = CellularMgrPacketServiceStatusCBForSM;

        CcspTraceInfo(("%s %d - Starting IPv6 Network\n", __FUNCTION__, __LINE__));
        cellular_hal_start_network( CELLULAR_NETWORK_IP_FAMILY_IPV6, NULL, &stNetworkCB );
    }

    return CELLULAR_STATE_REGISTERED;
}

static CellularPolicySmState_t TransitionConnected( void )
{
    max_wait = 3;

    return CELLULAR_STATE_CONNECTED;
}

static CellularPolicySmState_t TransitionConnectedStopNetwork( void )
{
#ifdef HYBRID_SUPPORT
    gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = false;
    CcspTraceError(("%s %d - Updated X_RDK_LinkAvailableStatus to false \n", __FUNCTION__, __LINE__));
#endif
    //LinkStatus should be down
    CcspTraceInfo(("%s - Updating link status to DOWN for this '%s' interface \n", __FUNCTION__, gpstCellularPolicyCtrl->acWANIfName));
    if ( CellularMgrUpdateLinkStatus (gpstCellularPolicyCtrl->acWANIfName, DOWN_STR) != RETURN_OK)
    {
        CcspTraceInfo(("%s %d: Failed to set LinkStatus in Interface table\n", __FUNCTION__, __LINE__));
    }

    if( ( CELLULAR_PDP_TYPE_IPV4 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) || 
        ( CELLULAR_PDP_TYPE_IPV4_OR_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile )
        )
    {
        CcspTraceInfo(("%s %d - Stopping IPv4 Network\n", __FUNCTION__, __LINE__));

        //Request to stop Network v4
        cellular_hal_stop_network( CELLULAR_NETWORK_IP_FAMILY_IPV4 );

	//Reset the sysevent values
        if(sysevent_fd >= 0)
        {
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_SUBNET, "0.0.0.0", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_GWADDR, "0.0.0.0", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_DNS1, "0.0.0.0", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_DNS2, "0.0.0.0", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_IPV4_NAMESERVER, "", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_MTU, "0", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V4_IPADDR, "0.0.0.0", 0);

	}

    }

    if( ( CELLULAR_PDP_TYPE_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) || 
        ( CELLULAR_PDP_TYPE_IPV4_OR_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) 
        )
    {
        CcspTraceInfo(("%s %d - Stopping IPv6 Network\n", __FUNCTION__, __LINE__));

        //Request to stop Network v6
        cellular_hal_stop_network( CELLULAR_NETWORK_IP_FAMILY_IPV6 );

	//Reset the sysevent values
        if(sysevent_fd >= 0)
        {
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_GWADDR, "::", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_DNS1, "::" , 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_DNS2, "::" , 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_IPV6_NAMESERVER,"", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_MTU, "0", 0);
            sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_CM_WAN_V6_IPADDR, "::", 0);

	}

    }

    if( FALSE == gpstCellularPolicyCtrl->bRDKEnable )
    {
        CcspTraceInfo(("%s %d - State changed to  CELLULAR_STATE_DEACTIVATED\n", __FUNCTION__, __LINE__));
        CellularMgrSMSetCurrentState(CELLULAR_STATE_DEACTIVATED);
        return CELLULAR_STATE_DEACTIVATED;
    }
    if( FALSE == gpstCellularPolicyCtrl->pCmIfData->Enable )
    {
        gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = DEVICE_NAS_STATUS_NOT_REGISTERED;
        CellularMgrSMSetCurrentState(CELLULAR_STATE_DEREGISTERED);
        return CELLULAR_STATE_DEREGISTERED;
    }
    //check if de-registered from network.. just to monitor ..no need to call qmi nas registration as it is done already
    if(DEVICE_NAS_STATUS_REGISTERED != gpstCellularPolicyCtrl->enDeviceNASRegisterStatus )
    {
        CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_REGISTERING as event occured \n", __FUNCTION__, __LINE__));
        gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = DEVICE_NAS_STATUS_REGISTERING;
        CellularMgrSMSetCurrentState(CELLULAR_STATE_DEREGISTERED);
        return CELLULAR_STATE_DEREGISTERED;
    }
    else
    {
        CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_REGISTERED \n", __FUNCTION__, __LINE__));
        CellularMgrSMSetCurrentState(CELLULAR_STATE_REGISTERED);
    }

    return CELLULAR_STATE_REGISTERED;
}

/* Cellular States */
void CellularMgrSMCheckAndSetWWANConnectionStatus( CellularPolicySmState_t smState )
{
    unsigned char bPreviousPhyConnectedStatus = gpstCellularPolicyCtrl->pCmIfData->X_RDK_PhyConnectedStatus;
    unsigned char bPreviousLinkAvailableStatus = gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus;
    CellularInterfaceStatus_t  PrevStatus = gpstCellularPolicyCtrl->pCmIfData->Status;

    switch( smState )
    {
        case CELLULAR_STATE_DOWN:
        case CELLULAR_STATE_DEACTIVATED:
        case CELLULAR_STATE_DEREGISTERED:
        case CELLULAR_STATUS_ERROR:
            gpstCellularPolicyCtrl->pCmIfData->X_RDK_PhyConnectedStatus = FALSE;
            gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = FALSE;
            gpstCellularPolicyCtrl->pCmIfData->Status = IF_DOWN;
        break;

        case CELLULAR_STATE_REGISTERED:
            gpstCellularPolicyCtrl->pCmIfData->X_RDK_PhyConnectedStatus = TRUE;
            gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = FALSE;
            gpstCellularPolicyCtrl->pCmIfData->Status = IF_UP;
        break;

        case CELLULAR_STATE_CONNECTED:
            gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus = TRUE;
        break;
    }

#ifdef RBUS_BUILD_FLAG_ENABLE
    CellularMgr_RBUS_Events_PublishInterfaceStatus(PrevStatus,gpstCellularPolicyCtrl->pCmIfData->Status);
    CellularMgr_RBUS_Events_PublishPhyConnectionStatus(bPreviousPhyConnectedStatus,gpstCellularPolicyCtrl->pCmIfData->X_RDK_PhyConnectedStatus);
    CellularMgr_RBUS_Events_PublishLinkAvailableStatus(bPreviousLinkAvailableStatus,gpstCellularPolicyCtrl->pCmIfData->X_RDK_LinkAvailableStatus);
#endif
}

static void CellularMgrSMSetCurrentState( CellularPolicySmState_t smState)
{
    gpstCellularPolicyCtrl->enCurrentSMState = smState;
}

CellularPolicySmState_t CellularMgrSMGetCurrentState( void )
{
    return gpstCellularPolicyCtrl->enCurrentSMState;
}

void CellularMgrSMSetCellularEnable( unsigned char bRDKEnable )
{
    gpstCellularPolicyCtrl->bRDKEnable = bRDKEnable;
}

unsigned char CellularMgrSMGetCellularEnable( void )
{
    return (gpstCellularPolicyCtrl->bRDKEnable);
}

static CellularPolicySmState_t StateDown( void )
{
    //Validate Input
    if( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError (("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return CELLULAR_STATUS_ERROR;
    }

    /*
     *  In this state Modem Device not present, Modem Offline, WAN Interface Down  
     *  Transition will happen when Modem Device detected case 
     */

    gpstCellularPolicyCtrl->enDeviceDetectionStatus  = DEVICE_REMOVED;

    if( TRUE == cellular_hal_IsModemDevicePresent( ) )
    {
        gpstCellularPolicyCtrl->enDeviceDetectionStatus = DEVICE_DETECTED;
    }

    if( DEVICE_DETECTED == gpstCellularPolicyCtrl->enDeviceDetectionStatus )
    {
         return(TransitionDeactivated( ));
    }

    return CELLULAR_STATE_DOWN;
}

static CellularPolicySmState_t StateDeactivated( void )
{
    //Validate Input
    if( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError (("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return CELLULAR_STATUS_ERROR;
    }

    /*
     *  In this state Modem Device present, Modem Offline, Slot not selected, Profile not created, WAN Interface Down.
     *  Transition will happen when Modem is Online   
     */

    if( DEVICE_REMOVED == gpstCellularPolicyCtrl->enDeviceDetectionStatus )
    {
        return ( TransitionDown( ) );
    }

    if( ( TRUE == gpstCellularPolicyCtrl->bRDKEnable ) && 
        ( DEVICE_OPEN_STATUS_READY == gpstCellularPolicyCtrl->enDeviceOpenStatus ) &&
        ( gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus == DEVICE_SLOT_STATUS_NOT_READY))
    {
         TransitionDeregistered( ) ;
    }

    if ( gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus == DEVICE_SLOT_STATUS_SELECTING)
    {
        //simply move the state to DE-REGISTERED without waiting for slot status. In Deregister state wait for slot & attach
        CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_DEREGISTERED \n", __FUNCTION__, __LINE__));
        CellularMgrSMSetCurrentState(CELLULAR_STATE_DEREGISTERED);
        return CELLULAR_STATE_DEREGISTERED;
    }
    return CELLULAR_STATE_DEACTIVATED;
}

static CellularPolicySmState_t StateDeregistered( void )
{
    CELLULAR_INTERFACE_SIM_STATUS enCardStatus;

    //Validate Input
    if( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError (("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return CELLULAR_STATUS_ERROR;
    }

    /*
     *  In this state Modem Device present, Modem Online, Slot not selected, Profile not created, WAN Interface Down.
     *  Transition will happen when Valid Slot and NAS attach case   
     */

    if( DEVICE_REMOVED == gpstCellularPolicyCtrl->enDeviceDetectionStatus )
    {
        return ( TransitionDown( ) );
    }

    if( ( FALSE == gpstCellularPolicyCtrl->bRDKEnable )  ||
        ( DEVICE_OPEN_STATUS_NOT_READY == gpstCellularPolicyCtrl->enDeviceOpenStatus ) ) 
    {
        return ( TransitionDeactivated( ) );
    }

    //Continue if it is selecting valid SIM   
    if( ( DEVICE_SLOT_STATUS_SELECTING == gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus ) ||
        ( DEVICE_NAS_STATUS_REGISTERING == gpstCellularPolicyCtrl->enDeviceNASRegisterStatus ) ||
        ( gpstCellularPolicyCtrl->pCmIfData->Enable == FALSE ) ||
        ( ( RETURN_OK == CellularMgr_GetActiveCardStatus( &enCardStatus ) ) &&
          ( SIM_STATUS_VALID != enCardStatus ) ) )
    {
        return CELLULAR_STATE_DEREGISTERED;
    }

    if( ( DEVICE_SLOT_STATUS_READY == gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus ) &&
        ( gpstCellularPolicyCtrl->pCmIfData->Enable == TRUE ) &&
        ( DEVICE_NAS_STATUS_NOT_REGISTERED == gpstCellularPolicyCtrl->enDeviceNASRegisterStatus ) )
    {

         TransitionRegistering( );
    }

    if( DEVICE_NAS_STATUS_REGISTERED == gpstCellularPolicyCtrl->enDeviceNASRegisterStatus )
    {
        CellularMgrSMSetCurrentState(CELLULAR_STATE_REGISTERED);
        CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_REGISTERED \n", __FUNCTION__, __LINE__));
        return CELLULAR_STATE_REGISTERED;
    }

    return CELLULAR_STATE_DEREGISTERED;
}

static CellularPolicySmState_t StateRegistered( void )
{
    CELLULAR_INTERFACE_SIM_STATUS enCardStatus;
    
    //Validate Input
    if( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError (("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return CELLULAR_STATUS_ERROR;
    }

    /*
     *  In this state Modem Device present, Modem Online, Slot selected, NAS attached, Profile not selected, Network not started
     *  Transition will happen when Network Started Successfully    
     */

    if( DEVICE_REMOVED == gpstCellularPolicyCtrl->enDeviceDetectionStatus )
    {
        return ( TransitionDown( ) );
    }

    if( ( FALSE == gpstCellularPolicyCtrl->bRDKEnable ) ||
        ( DEVICE_OPEN_STATUS_NOT_READY == gpstCellularPolicyCtrl->enDeviceOpenStatus ) )
    {
        return ( TransitionDeactivated( ) );
    }

    if( ( DEVICE_SLOT_STATUS_NOT_READY == gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus ) ||
        ( ( RETURN_OK == CellularMgr_GetActiveCardStatus( &enCardStatus ) ) &&
          ( SIM_STATUS_VALID != enCardStatus ) ) ||
        ( gpstCellularPolicyCtrl->pCmIfData->Enable == FALSE ) )
    {
        gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = DEVICE_NAS_STATUS_NOT_REGISTERED;
        CellularMgrSMSetCurrentState(CELLULAR_STATE_DEREGISTERED);
        return CELLULAR_STATE_DEREGISTERED;
    }

    if(DEVICE_NAS_STATUS_REGISTERED != gpstCellularPolicyCtrl->enDeviceNASRegisterStatus )
    {
        CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_DEREGISTERED as deregistered \n", __FUNCTION__, __LINE__));
        gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = DEVICE_NAS_STATUS_REGISTERING;
        CellularMgrSMSetCurrentState(CELLULAR_STATE_DEREGISTERED);
        return CELLULAR_STATE_DEREGISTERED;
    }

    if(( DEVICE_NAS_STATUS_REGISTERED == gpstCellularPolicyCtrl->enDeviceNASRegisterStatus ) &&
       ( DEVICE_PROFILE_STATUS_NOT_READY == gpstCellularPolicyCtrl->enDeviceProfileSelectionStatus ) )
    {
        TransitionRegistered();
        return CELLULAR_STATE_REGISTERED;
    }

    if( ( DEVICE_PROFILE_STATUS_CONFIGURING == gpstCellularPolicyCtrl->enDeviceProfileSelectionStatus ) ||
        ( TRUE == gpstCellularPolicyCtrl->bIPv4NetworkStartInProgress ) ||
        ( TRUE == gpstCellularPolicyCtrl->bIPv6NetworkStartInProgress ) ||
        ( TRUE == gpstCellularPolicyCtrl->bIPv4WaitingForPacketStatus ) ||
        ( TRUE == gpstCellularPolicyCtrl->bIPv6WaitingForPacketStatus ) )
    {
        return ( CELLULAR_STATE_REGISTERED );
    }

    if( ( DEVICE_PROFILE_STATUS_READY == gpstCellularPolicyCtrl->enDeviceProfileSelectionStatus ) &&
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
        ( gpstCellularPolicyCtrl->pCmIfData->Upstream == TRUE ) &&
#endif
        ( ( DEVICE_NETWORK_STATUS_CONNECTED != gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus ) &&
          ( DEVICE_NETWORK_STATUS_CONNECTED != gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus ) ) )
    {
        while ( iter < max_wait)
        {
            sleep(1);
            iter++;
            if(DEVICE_NAS_STATUS_REGISTERED != gpstCellularPolicyCtrl->enDeviceNASRegisterStatus )
            {
               CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_DEREGISTERED as deregistered \n", __FUNCTION__, __LINE__));
               gpstCellularPolicyCtrl->enDeviceNASRegisterStatus = DEVICE_NAS_STATUS_REGISTERING;
               iter = 0;
               CellularMgrSMSetCurrentState(CELLULAR_STATE_DEREGISTERED);
               return CELLULAR_STATE_DEREGISTERED;
            }
        }
        max_wait = 0;
        iter = 0;
        return ( TransitionRegisteredStartNetwork() );
    }

    if( ( ( CELLULAR_PDP_TYPE_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) &&
          ( DEVICE_NETWORK_STATUS_CONNECTED == gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus )  ) || 
        ( ( CELLULAR_PDP_TYPE_IPV4 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) &&
          ( DEVICE_NETWORK_STATUS_CONNECTED == gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus )  ) ||
        ( ( CELLULAR_PDP_TYPE_IPV4_OR_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) &&
          ( DEVICE_NETWORK_STATUS_CONNECTED == gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus ) &&
          ( DEVICE_NETWORK_STATUS_CONNECTED == gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus )  ) 
      )
    {
        CcspTraceInfo(("%s %d - State changed to CELLULAR_STATE_CONNECTED \n", __FUNCTION__, __LINE__));
        CellularMgrSMSetCurrentState(CELLULAR_STATE_CONNECTED);
        TransitionConnected( );
        return CELLULAR_STATE_CONNECTED;
    }

    return CELLULAR_STATE_REGISTERED;
}

static CellularPolicySmState_t StateConnected( void )
{
    //Validate Input
    if( NULL == gpstCellularPolicyCtrl )
    {
        CcspTraceError (("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return CELLULAR_STATUS_ERROR;
    }

    if( DEVICE_REMOVED == gpstCellularPolicyCtrl->enDeviceDetectionStatus )
    {
        return ( TransitionDown( ) );
    }

    if( DEVICE_OPEN_STATUS_NOT_READY == gpstCellularPolicyCtrl->enDeviceOpenStatus )
    {        
        return ( TransitionDeactivated( ) );
    }
    
    if ( ( FALSE == gpstCellularPolicyCtrl->bRDKEnable ) ||
         ( DEVICE_SLOT_STATUS_NOT_READY       == gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus ) ||
         ( gpstCellularPolicyCtrl->pCmIfData->Enable == FALSE ) ||
#ifndef WAN_MANAGER_UNIFICATION_ENABLED
         ( gpstCellularPolicyCtrl->pCmIfData->Upstream == FALSE ) ||
#endif
         ( ( CELLULAR_PDP_TYPE_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) &&
           ( DEVICE_NETWORK_STATUS_DISCONNECTED == gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus ) ) || 
         ( ( CELLULAR_PDP_TYPE_IPV4 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) &&
           ( DEVICE_NETWORK_STATUS_DISCONNECTED == gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus ) ) ||
         ( ( CELLULAR_PDP_TYPE_IPV4_OR_IPV6 == gpstCellularPolicyCtrl->enPDPTypeForSelectedProfile ) &&
           ( ( DEVICE_NETWORK_STATUS_DISCONNECTED == gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus ) ||
             ( DEVICE_NETWORK_STATUS_DISCONNECTED == gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus ) ) )
        )
    {
        gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus = DEVICE_NETWORK_STATUS_DISCONNECTED;
        gpstCellularPolicyCtrl->enNetworkIPv4IPReadyStatus       = DEVICE_NETWORK_IP_NOT_READY;
        gpstCellularPolicyCtrl->bIPv4NetworkStartInProgress      = FALSE;
        gpstCellularPolicyCtrl->bIPv4WaitingForPacketStatus      = FALSE;
        gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus = DEVICE_NETWORK_STATUS_DISCONNECTED;
        gpstCellularPolicyCtrl->enNetworkIPv6IPReadyStatus       = DEVICE_NETWORK_IP_NOT_READY;
        gpstCellularPolicyCtrl->bIPv6NetworkStartInProgress      = FALSE;
        gpstCellularPolicyCtrl->bIPv6WaitingForPacketStatus      = FALSE;

        return ( TransitionConnectedStopNetwork( ) );
    }

    return CELLULAR_STATE_CONNECTED;
}

static ANSC_STATUS CellularMgr_ControllerInit( CellularMgrSMInputStruct    *pstInput )
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;

    if ( pstInput == NULL )
    {
        CcspTraceError(("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }

    gpstCellularPolicyCtrl  = ( CellularMgrPolicyCtrlSMStruct* ) malloc ( sizeof( CellularMgrPolicyCtrlSMStruct ) );
    if( NULL != gpstCellularPolicyCtrl )
    {
        memset(gpstCellularPolicyCtrl, 0, sizeof(CellularMgrPolicyCtrlSMStruct));

        gpstCellularPolicyCtrl->enCurrentSMState                   = CELLULAR_STATE_DOWN;
        gpstCellularPolicyCtrl->enDeviceDetectionStatus            = DEVICE_REMOVED;
        gpstCellularPolicyCtrl->enDeviceOpenStatus                 = DEVICE_OPEN_STATUS_NOT_READY;
        gpstCellularPolicyCtrl->enDeviceSlotSelectionStatus        = DEVICE_SLOT_STATUS_NOT_READY;
        gpstCellularPolicyCtrl->enDeviceNASRegisterStatus          = DEVICE_NAS_STATUS_NOT_REGISTERED;
        gpstCellularPolicyCtrl->enDeviceNASRoamingStatus           = DEVICE_NAS_STATUS_ROAMING_OFF;
        gpstCellularPolicyCtrl->enRegisteredService                = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;
        gpstCellularPolicyCtrl->enDeviceProfileSelectionStatus     = DEVICE_PROFILE_STATUS_NOT_READY;
        gpstCellularPolicyCtrl->enNetworkIPv4IPReadyStatus         = DEVICE_NETWORK_IP_NOT_READY;
        gpstCellularPolicyCtrl->enNetworkIPv4PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;
        gpstCellularPolicyCtrl->bIPv4NetworkStartInProgress        = FALSE;
        gpstCellularPolicyCtrl->bIPv4WaitingForPacketStatus        = FALSE;
        gpstCellularPolicyCtrl->enNetworkIPv6IPReadyStatus         = DEVICE_NETWORK_IP_NOT_READY;
        gpstCellularPolicyCtrl->enNetworkIPv6PacketServiceStatus   = DEVICE_NETWORK_STATUS_DISCONNECTED;
        gpstCellularPolicyCtrl->bIPv6NetworkStartInProgress        = FALSE;
        gpstCellularPolicyCtrl->bIPv6WaitingForPacketStatus        = FALSE;
        snprintf(gpstCellularPolicyCtrl->acWANIfName, sizeof(gpstCellularPolicyCtrl->acWANIfName), "%s", "wwan0");
        memcpy(&gpstCellularPolicyCtrl->stContextProfile, &pstInput->stContextProfile, sizeof(CellularProfileStruct));
        gpstCellularPolicyCtrl->bRDKEnable = pstInput->bModemEnable;
        gpstCellularPolicyCtrl->pCmIfData  = pstInput->pCmIfData;
        gpstCellularPolicyCtrl->pstDmlCellular = pstInput->pstDmlCellular;

        retStatus = ANSC_STATUS_SUCCESS;
    }

   return retStatus;
}

/* CellularMgr_StateMachine_Thread() */
static void* CellularMgr_StateMachine_Thread( void *arg )
{
    unsigned char  bRunning = TRUE;
    int             n        = 0;
    struct timeval  tv;
    CellularPolicySmState_t policy_sm_state = CELLULAR_STATE_DOWN;

    //detach thread from caller stack
    pthread_detach(pthread_self());

    CcspTraceInfo(("%s %d - Entering into SM\n", __FUNCTION__, __LINE__));
    
    policy_sm_state = TransitionDown( );

    while( bRunning ) 
    {
        /* Wait up to 500 milli seconds */
        tv.tv_sec = 0;
        tv.tv_usec = LOOP_TIMEOUT;

        n = select(0, NULL, NULL, NULL, &tv);
        if (n < 0)
        {
            /* interrupted by signal or something, continue */
            continue;
        }
        if (policy_sm_state_old != policy_sm_state) // This is just to add log
        {
           CcspTraceInfo(("%s %d - New state: %d ,Previous state :%d  \n", __FUNCTION__, __LINE__,policy_sm_state,policy_sm_state_old));
           policy_sm_state_old = policy_sm_state;
           CellularMgrSMCheckAndSetWWANConnectionStatus( policy_sm_state );
        }
        switch (policy_sm_state)
        {
            case CELLULAR_STATE_DOWN:
                policy_sm_state = StateDown( );
                break;
            case CELLULAR_STATE_DEACTIVATED:
                policy_sm_state = StateDeactivated( );
                break;
            case CELLULAR_STATE_DEREGISTERED:
                policy_sm_state = StateDeregistered( );
                break;
            case CELLULAR_STATE_REGISTERED:
                policy_sm_state = StateRegistered( );
                break;
            case CELLULAR_STATE_CONNECTED:
                policy_sm_state = StateConnected( );
                break;
            case CELLULAR_STATUS_ERROR:
            default:
                CcspTraceInfo(("%s %d - Case: default \n", __FUNCTION__, __LINE__));
                bRunning = false;
                break;
        }
    }

    free( gpstCellularPolicyCtrl );
    gpstCellularPolicyCtrl = NULL;

    CcspTraceInfo(("%s %d Exit\n", __FUNCTION__, __LINE__));

    //Cleanup current thread when exit
    pthread_exit(NULL);

    return NULL;
}

int
CellularMgr_Start_State_Machine
    (
        CellularMgrSMInputStruct    *pstInput
    )
{
    if ( pstInput == NULL )
    {
        CcspTraceError(("%s %d - invalid args\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }
    
    pthread_t Thread;

    //Initialize Controller Configuration
    if (CellularMgr_ControllerInit( pstInput ) != RETURN_OK )
    {
        CcspTraceError(("%s %d - unable to initialise sm controller object\n", __FUNCTION__, __LINE__));
        return RETURN_ERROR;
    }
    
    //Initiate the thread for cellular state  
    pthread_create( &Thread, NULL, &CellularMgr_StateMachine_Thread, (void*)NULL);
    CcspTraceInfo(("%s %d - Started QMI Device Monitor Thread\n", __FUNCTION__, __LINE__));

    return RETURN_OK;
}
