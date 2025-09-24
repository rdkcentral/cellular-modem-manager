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

/* 
 * Refer to the libqmi -glib reference manual at <https://www.freedesktop.org/software/libqmi/libqmi-glib/latest/> 
 * for details of the libqmi APIs.
*/

#include "cellular_hal_qmi_apis.h"
#include <unistd.h>
#include "secure_wrapper.h"

/**********************************************************************
                CONSTANT DEFINITIONS
**********************************************************************/

#define  CELLULAR_QMI_EID_LENGTH                  ( 16 )
#define  CELLULAR_QMI_VALIDATE_UNKNOWN(str)       (str ? str : "unknown")
#define  CELLULAR_QMI_VALIDATE_MASK_NONE(str)     (str ? str : "none")
#define  CELLULAR_QMI_INVALID_EVENT_INDICATION    ( 0 )

#define  CELLULAR_QMI_INVALID_PACKET_STATS_VALUE  (0xFFFFFFFF)

#define  CELLULAR_QMI_PROFILELIST_VIA_DML_MAX_WAITIME           ( 30 )
#define  CELLULAR_QMI_GETIDS_VIA_DML_MAX_WAITIME                ( 10 )
#define  CELLULAR_QMI_GETPKTSTATS_VIA_DML_MAX_WAITIME           ( 10 )
#define  CELLULAR_QMI_GETPKTSTATS_VIA_DML_TTL_FOR_STATS_INFO    ( 30 )
#define  CELLULAR_QMI_GETREG_STATUS_VIA_DML_MAX_WAITIME         ( 10 )
#define  CELLULAR_QMI_GETNETWORK_VIA_DML_MAX_WAITIME            ( 10 )
#define  CELLULAR_QMI_GETNETWORK_VIA_DML_MAX_TTL                ( 60 )
#define  CELLULAR_QMI_GET_CAPS_MAX_WAITIME                      ( 10 )

#define  CELLULAR_QMI_ICCID_MAX_LENGTH                          ( 21 )
#define  CELLULAR_QMI_OPERATOR_LENGTH                           ( 32 )
#define  CELLULAR_QMI_NETWORKSCAN_MAX_LENGTH                    ( 128 )

#define  CELLULAR_QMI_NETWORKSCAN_COLLECTION_PERIODIC_INTERVAL  ( 120 )

/**********************************************************************
    GLOBAL or LOCAL DEFINITIONS and STRUCTURE or ENUM DECLARATION
**********************************************************************/

/* Global Declaration */

typedef enum {
    MODEM_OPEN_STATE_BEGIN                  = 1,
    MODEM_OPEN_STATE_DEVICE_OPEN,
    MODEM_OPEN_STATE_DMS_OPEN,
    MODEM_OPEN_STATE_GET_REVISION,
    MODEM_OPEN_STATE_GET_VERSION,
    MODEM_OPEN_STATE_GET_IMEI,
    MODEM_OPEN_STATE_GET_OPERATING_MODE,
    MODEM_OPEN_STATE_WDS_OPEN,
    MODEM_OPEN_STATE_GET_CAPABILITIES,
    MODEM_OPEN_STATE_NAS_OPEN,
    MODEM_OPEN_STATE_NAS_REGISTER_SIGNAL_INDICATIONS,
    MODEM_OPEN_STATE_NAS_NETWORK_SCAN_INIT,
    MODEM_OPEN_STATE_NOTIFY,
    MODEM_OPEN_STATE_END
} ModemOpenState_t;

typedef struct 
{
    ModemOpenState_t                    uiCurrentStep; 
    CellularDeviceContextCBStruct       stDeviceCtxCB;
    CellularDeviceOpenStatus_t          enOpenStatus;
    CellularModemOperatingConfiguration_t modem_operating_mode;
    void*                               vpPrivateData;                        
} ContextDeviceOpen;

typedef struct 
{
    CellularModemOperatingConfiguration_t    uiRequestedOperatingConfig; 
    void*                                    vpPrivateData;                        
} ContextModemConfiguration;

typedef enum {
    NAS_MODEM_REGISTRATION_MONITOR_VIA_SM    = 1,
    NAS_MODEM_REGISTRATION_GET_VIA_DML
} NASModemRegistrationQuerySource_t;

typedef enum {
    NAS_MODEM_REGISTRATION_STATUS_BEGIN                  = 1,
    NAS_MODEM_GET_REGISTRATION_STATUS,
    NAS_MODEM_NOTIFY_REGISTRATION_STATUS,
    NAS_MODEM_REGISTRATION_STATUS_END
} NASModemRegistrationState_t;

typedef struct 
{
    NASModemRegistrationQuerySource_t   enQuerySource;
    guint                               uiCurrentStep; 
    CellularDeviceNASStatus_t           enNASRegistrationStatus;
    CellularDeviceNASRoamingStatus_t    enNASRoamingStatus;
    CellularModemRegisteredServiceType_t registered_service;
    cellular_device_registration_status_callback device_registration_status_cb;
    void*                               vpPrivateData;
    guint8                              bIsModemRegTaskDMLGetCompleted;
} ContextModemRegistration;

typedef struct 
{
    void*                               vpPrivateData;
} ContextNetworkScan;

typedef enum {
    NAS_GET_NETWORK_INFO_BEGIN                  = 1,
    NAS_GET_NETWORK_INFO_COLLECT_SERVING_SYSTEM,
    NAS_GET_NETWORK_INFO_COLLECT_SYSTEM_INFO,
    NAS_GET_NETWORK_INFO_END
} NASGetNetworkInfoState_t;

typedef struct 
{
    guint                               uiCurrentStep; 
    void*                               vpPrivateData;
} ContextGetNetworkInfo;

typedef enum {
    WDS_PROFILE_CREATE_BEGIN                  = 1,
    WDS_PROFILE_CREATE_ALLOCATE_WDS_CLIENT,
    WDS_PROFILE_CREATE_READ_DEFAULT_SETTINGS,
    WDS_PROFILE_CREATE_READ_PROFILE_LIST,
    WDS_PROFILE_CREATE_VERIFY_PROFILE_PRESENT,
    WDS_PROFILE_CREATE,
    WDS_PROFILE_CREATE_SET_DEFAULT_PROFILE,
    WDS_PROFILE_CREATE_VERIFY_AND_SET_LTE_PDN_ATTACH,
    WDS_PROFILE_CREATE_NOTIFY,
    WDS_PROFILE_CREATE_END
} WDSProfileCreateState_t;

typedef enum {
    WDS_PROFILE_DELETE_BEGIN                  = WDS_PROFILE_CREATE_END + 1,
    WDS_PROFILE_DELETE_ALLOCATE_WDS_CLIENT,
    WDS_PROFILE_DELETE,
    WDS_PROFILE_DELETE_RESYNC_PROFILE_LIST,
    WDS_PROFILE_DELETE_NOTIFY,
    WDS_PROFILE_DELETE_END
} WDSProfileDeleteState_t;

typedef enum {
    WDS_PROFILE_MODIFY_BEGIN                  = WDS_PROFILE_DELETE_END + 1,
    WDS_PROFILE_MODIFY_ALLOCATE_WDS_CLIENT,
    WDS_PROFILE_MODIFY_VERIFY_PROFILE_PRESENT,
    WDS_PROFILE_MODIFY,
    WDS_PROFILE_MODIFY_SET_DEFAULT_PROFILE,
    WDS_PROFILE_MODIFY_VERIFY_AND_SET_LTE_PDN_ATTACH,
    WDS_PROFILE_MODIFY_NOTIFY,
    WDS_PROFILE_MODIFY_END
} WDSProfileModifyState_t;

typedef enum {
    WDS_PROFILE_GETLIST_BEGIN                  = WDS_PROFILE_MODIFY_END + 1,
    WDS_PROFILE_GETLIST_ALLOCATE_WDS_CLIENT,
    WDS_PROFILE_GETLIST_READ_DEFAULT_SETTINGS,
    WDS_PROFILE_GETLIST,
    WDS_PROFILE_GETLIST_END
} WDSProfileGetListState_t;

typedef struct 
{
    WDSProfileOperation_t           enProfileOperationRequest;
    guint                           uiCurrentStep; 
    guint                           uiLastStep; 
    CellularProfileStruct           stProfileInput;
    guint8                          bIsProfileAlreadyPresent;
    cellular_device_profile_status_api_callback device_profile_status_cb;
    void*                           vpPrivateData;                        
} ContextProfileOperation;

typedef enum {
    WDS_NW_START_BEGIN                  = 1,
    WDS_NW_START_ALLOCATE_WDS_CLIENT,
    WDS_NW_START_CONFIGURE_IP_FAMILY,
    WDS_NW_START_REGISTER_EVENTS,
    WDS_NW_START_START_NETWORK,
    WDS_NW_START_GET_LEASE_SETTINGS,
    WDS_NW_START_GET_DATA_RAT,
    WDS_NW_START_END
} WDSNWStartState_t;

typedef struct 
{
    WDSNWStartState_t               uiCurrentStep; 
    CellularNetworkIPType_t         ip_request_type;
    CellularProfileStruct           stProfileInput;
    CellularNetworkCBStruct         stNetworkCB; 
    void*                           vpPrivateData;                        
} ContextNWStart;

typedef enum {
    WDS_NW_STOP_BEGIN                  = 1,
    WDS_NW_STOP_DEREGISTER_EVENTS,
    WDS_NW_STOP_STOP_NETWORK,
    WDS_NW_STOP_RELEASE_WDS_CLIENT,
    WDS_NW_STOP_END
} WDSNWStopState_t;

typedef struct 
{
    WDSNWStopState_t                uiCurrentStep; 
    CellularNetworkIPType_t         ip_request_type;
    void*                           vpPrivateData;                        
} ContextNWStop;

typedef struct 
{
    guint8                          bIsValidDMSClient;                   //Ensure DMS client is valid or not valid
    QmiClient                       *dmsClient;                          //DMS client 
    gchar                           IMEI[16];
    gchar                           IMEI_SV[16];
    gchar                           ICCID[CELLULAR_QMI_ICCID_MAX_LENGTH];
    gchar                           MSISDN[20];
    gchar                           FirmwareRevision[128];
    gchar                           FirmwareVersion[128];
    gchar                           SupportedRAT[128];

} ContextDMSInfo;

typedef struct 
{
    guint16                         mcc;
    guint16                         mnc;
    gchar                           name[32];
    gint8                           network_allowed_flag;

} NASAvailableNetworkInfo;

typedef struct 
{
    guint8                          bIsValidNASClient;                   //Ensure NAS client is valid or not valid
    QmiClient                       *nasClient;                          //NAS client
    gint8                           lte_rssi;
    gint8                           lte_rsrq;
    gint16                          lte_rsrp;
    gint16                          lte_snr;
    gint32                          tx_power;
    guint8                          bIsSignalInfoCollectionDone;
    CellularDeviceNASStatus_t       enNASRegistrationStatus;
    guint8                          bIsModemRegTaskStillRunning;
    GCancellable                    *cancellableForModemRegTask;
    guint8                          serving_system_id;
    guint8                          bIsModemRegDone;
    guint8                          bIsNetworkScanTaskStillRunning;
    pthread_cond_t                  idwCondForSCAN;
    pthread_mutex_t                 idwMutexForSCAN;
    pthread_mutex_t                 idwMutexForSCANData;
    guint8                          bIsGetNetworkInfoCollectionDone;
    guint8                          bIsNeed2StopNetworkInfoCollection;
    gint16                          iTotalNoofNetworkInfo;
    NASAvailableNetworkInfo         astNetworkInfo[CELLULAR_QMI_NETWORKSCAN_MAX_LENGTH];
    unsigned long                   LastCollectedTimeForPlmnInfo;
    CellularCurrentPlmnInfoStruct   stPlmnInfo;
    gchar                           operator_name[CELLULAR_QMI_OPERATOR_LENGTH];
    gchar                           preferredRAT[128];

} ContextNASInfo;

typedef struct 
{
    guint8                          bSlotState;
    guint8                          bCardPresentState;
    guint8                          bCardEnable;
    gchar                           iccid[CELLULAR_QMI_ICCID_MAX_LENGTH];
    gchar                           msisdn[20];
    guint8                          beUICCAvailable;
    gchar                           eid[20];

} ContextSlotInfo;

typedef struct 
{
    guint8                          bIsValiduimClient;                   //Ensure UIM client is valid or not valid
    QmiClient                       *uimClient;                         //UIM client 
    guint                           slot_status_indication_id;          //Slot event registration ID
    gint                            selected_slot_number;               //Selected slot number
    gint                            total_no_of_slots;                  //Total number of slots
    ContextSlotInfo                 *ctxSlotInfo;

} ContextUIMInfo;

typedef struct  
{
    guint8                          bIsValidwdsClient;                  //Ensure WDS client is valid or not valid
    guint8                          bNeedToSelectDefaultProfile;        //Needs to choose default profile
    QmiClient                       *wdsClient;                         //WDS client
    CellularProfileStruct           stDefaultProfileFromDB;             //Default Profile copy from HAL init
    guint8                          bIsDefaultProfileFromModemCached;   //Confirmation of valid default profile read from modem
    CellularProfileStruct           stDefaultProfileFromModem;          //Default Profile copy from Modem Default settings
    guint                           ui8ProfileCount;
    CellularProfileStruct           *pstProfileList;
    guint8                          bIsProfileListCollectionDone;

    QmiClient                       *client_ipv4;
    guint                           packet_service_status_ipv4_id;
    guint                           event_report_ipv4_id;
    guint32                         packet_data_handle_ipv4;
    GError                          *error_ipv4;
    guint8                          bIsStartNetworkIPv4InProgress;
    guint8                          bIsStopNetworkIPv4InProgress;

    QmiClient                       *client_ipv6;
    guint                           packet_service_status_ipv6_id;
    guint                           event_report_ipv6_id;
    guint32                         packet_data_handle_ipv6;
    GError                          *error_ipv6;
    guint8                          bIsStartNetworkIPv6InProgress;
    guint8                          bIsStopNetworkIPv6InProgress;

    CellularPacketStatsStruct       stPacketStats;
    guint32                         last_packet_stats_updated_time;
    guint8                          IsPacketStatsCollectionDone;
    gchar                           currentRAT[128];

} ContextWDSInfo;

typedef  struct                                     
{
    guint8                          bQMIDeviceCurrentState;
    GCancellable                    *cancellable;
    QmiDevice                       *qmiDevice;
    guint                           qmi_device_indication_id;
    guint                           qmi_device_removed_id;
    gchar                           wwan_iface[16];
    gchar                           modem_device_name[16];
    gchar                           modem_device_path[32];
    guint8                          IsDeviceRemovedSignalReceived;
    ContextDMSInfo                  dmsCtx;                            //DMS context
    ContextNASInfo                  nasCtx;                            //NAS context
    ContextUIMInfo                  uimCtx;                            //UIM context
    ContextWDSInfo                  wdsCtx;                            //WDS context

    CellularContextInitInputStruct  stCellularHALInput;

} QMIContextStructPrivate;

typedef enum {
    WDS_PROFILE_GETLIST_VIA_DEVICE_OPEN    = 1,
    WDS_PROFILE_GETLIST_VIA_DML,
    WDS_PROFILE_GETLIST_VIA_PROFILE_CREATE,
    WDS_PROFILE_GETLIST_VIA_PROFILE_DELETE
} WDSProfileGetListQuerySource_t;

typedef struct {
    guint                            iteration;
    WDSProfileGetListQuerySource_t   enQuerySource;
    GArray                           *list;
    ContextWDSInfo                   *wdsCtx;
    void*                            vpPrivateData;  
} ContextProfileList;

typedef enum {
    DMS_GET_IDS_VIA_DEVICE_OPEN    = 1,
    DMS_GET_IDS_VIA_DML,
    DMS_GET_ICCID_VIA_DML,
    DMS_GET_MSISDN_VIA_DML
} DMSGetIDsQuerySource_t;

typedef struct {
    DMSGetIDsQuerySource_t           enQuerySource;
    guint                            IsQueryDone;
    void*                            vpPrivateData;  
} ContextGetIDs;

typedef struct {
    guint                            IsQueryDone;
    void*                            vpPrivateData;
} ContextNASQuery;

typedef enum {
    WDS_NW_PACKET_STATS_BEGIN                  = 1,
    WDS_NW_PACKET_STATS_GET_PACKET_STATISTICS,
    WDS_NW_PACKET_STATS_GET_CHANNEL_RATES,
    WDS_NW_PACKET_STATS_END
} WDSNWPacketState_t;

typedef struct 
{
    WDSNWPacketState_t              uiCurrentStep; 
    void*                           vpPrivateData;                       
} ContextNWPacketStats;

/* Global Definition */
static QMIContextStructPrivate     *gpstQMIContext = NULL;
static GMainLoop                   *gLoop          = NULL;
static pthread_t                    gloopThread;
static const gchar bcd_chars[] = "0123456789\0\0\0\0\0\0";
static QmiNasRegistrationState registration_state_previous = QMI_NAS_REGISTRATION_STATE_NOT_REGISTERED;
static QmiNasRoamingIndicatorStatus roaming_state_previous =  QMI_NAS_ROAMING_INDICATOR_STATUS_OFF;
static int retry_get_serving_system = 0;
static pthread_cond_t idwCondForSCAN = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t idwMutexForSCAN = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t idwMutexForSCANData = PTHREAD_MUTEX_INITIALIZER;

static void cellular_hal_qmi_device_open_step( GTask *task );
static void cellular_hal_qmi_start_network_connection_step( GTask *task );
static void cellular_hal_qmi_stop_network_connection_step( GTask *task );
static void cellular_hal_qmi_profile_operation_step( GTask *task );
static void cellular_hal_qmi_monitor_device_registration_step( GTask *task );
static void cellular_hal_qmi_monitor_device_registration_context_free( ContextModemRegistration *pDeviceRegCtx );
static void cellular_hal_qmi_get_next_profile_settings (ContextProfileList *inner_ctx);
static void cellular_hal_qmi_get_network_packet_stats_step( GTask *task );
static void cellular_hal_qmi_get_current_plmn_information_step( GTask *task );
static int cellular_hal_qmi_network_scan_data_collection_task( void );
static void cellular_hal_qmi_network_scan_data_collection_step( GTask *task );

static void cellular_hal_qmi_get_profile_list_cb (QmiClientWds *wdsClient,
                                                     GAsyncResult *result,
                                                     gpointer  user_data);
/**********************************************************************
                FUNCTION DEFINITION
**********************************************************************/

static void* CellularHAL_QMI_GLoop_Thread( void *arg )
{
    //detach thread from caller stack
    pthread_detach(pthread_self());

    CELLULAR_HAL_DBG_PRINT("%s %d - Entry\n",__FUNCTION__,__LINE__);

    gLoop = g_main_loop_new (NULL, FALSE);

    g_main_loop_run (gLoop);

    CELLULAR_HAL_DBG_PRINT("%s %d - Entering into Loop Run\n",__FUNCTION__,__LINE__);

    g_main_loop_unref (gLoop);

    CELLULAR_HAL_DBG_PRINT("%s %d - Exit\n",__FUNCTION__,__LINE__);

    //Cleanup current thread when exit
    pthread_exit(NULL);

    return RETURN_OK;
}

int
cellular_hal_qmi_init
    (
        CellularContextInitInputStruct *pstCtxInputStruct
    )
{
    CELLULAR_HAL_DBG_PRINT ("%s %d Entry\n", __FUNCTION__, __LINE__);

    //Initiate the thread for cellular policy  
    pthread_create( &gloopThread, NULL, &CellularHAL_QMI_GLoop_Thread, (void*)NULL);
    CELLULAR_HAL_DBG_PRINT("%s %d - GLib Loop Thread\n", __FUNCTION__, __LINE__);

    //Initialize QMI context
    gpstQMIContext = ( QMIContextStructPrivate *) malloc(sizeof(QMIContextStructPrivate));

    if( NULL != gpstQMIContext )
    {
        //Initialize to default
        memset(gpstQMIContext, 0, sizeof(QMIContextStructPrivate));

        snprintf(gpstQMIContext->modem_device_name, sizeof(gpstQMIContext->modem_device_name), "%s", QMI_DEVICE_NAME);
        snprintf(gpstQMIContext->modem_device_path, sizeof(gpstQMIContext->modem_device_path), "%s", QMI_DEVICE_PATH);

        gpstQMIContext->IsDeviceRemovedSignalReceived = FALSE;

        //UIM
        gpstQMIContext->uimCtx.total_no_of_slots                 = 0;
        gpstQMIContext->uimCtx.selected_slot_number              = CELLULAR_SLOT_ID_UNKNOWN;
        gpstQMIContext->uimCtx.ctxSlotInfo                       = NULL;

        //NAS
        gpstQMIContext->nasCtx.bIsModemRegTaskStillRunning       = FALSE;
        gpstQMIContext->nasCtx.LastCollectedTimeForPlmnInfo      = 0;
        gpstQMIContext->nasCtx.bIsNetworkScanTaskStillRunning    = FALSE;
        gpstQMIContext->nasCtx.bIsNeed2StopNetworkInfoCollection = FALSE;
        gpstQMIContext->nasCtx.enNASRegistrationStatus           = DEVICE_NAS_STATUS_NOT_REGISTERED;
        gpstQMIContext->nasCtx.idwCondForSCAN                    = idwCondForSCAN;
        gpstQMIContext->nasCtx.idwMutexForSCAN                   = idwMutexForSCAN; 
        gpstQMIContext->nasCtx.idwMutexForSCANData               = idwMutexForSCANData;

        //WDS
        gpstQMIContext->wdsCtx.ui8ProfileCount                   = 0;
        gpstQMIContext->wdsCtx.pstProfileList                    = NULL;

        gpstQMIContext->wdsCtx.client_ipv4                       = NULL;
        gpstQMIContext->wdsCtx.packet_data_handle_ipv4           = CELLULAR_PACKET_DATA_INVALID_HANDLE;
        gpstQMIContext->wdsCtx.packet_service_status_ipv4_id     = CELLULAR_QMI_INVALID_EVENT_INDICATION;
        gpstQMIContext->wdsCtx.event_report_ipv4_id              = CELLULAR_QMI_INVALID_EVENT_INDICATION;

        gpstQMIContext->wdsCtx.client_ipv6                       = NULL;
        gpstQMIContext->wdsCtx.packet_data_handle_ipv6           = CELLULAR_PACKET_DATA_INVALID_HANDLE;
        gpstQMIContext->wdsCtx.packet_service_status_ipv6_id     = CELLULAR_QMI_INVALID_EVENT_INDICATION;
        gpstQMIContext->wdsCtx.event_report_ipv6_id              = CELLULAR_QMI_INVALID_EVENT_INDICATION;

        //Default Profile
        gpstQMIContext->wdsCtx.bIsDefaultProfileFromModemCached        = FALSE;
        gpstQMIContext->wdsCtx.stDefaultProfileFromDB.ProfileID        = CELLULAR_PROFILE_ID_UNKNOWN;
        gpstQMIContext->wdsCtx.stDefaultProfileFromDB.PDPContextNumber = CELLULAR_PDP_CONTEXT_UNKNOWN;
        gpstQMIContext->wdsCtx.stDefaultProfileFromDB.PDPType          = CELLULAR_PDP_TYPE_IPV4;

        //Copy HAL Input
        if( NULL != pstCtxInputStruct )
        {
            CellularProfileStruct  *pstDefaultProfile = NULL;

            memcpy( &(gpstQMIContext->stCellularHALInput), pstCtxInputStruct, sizeof(CellularContextInitInputStruct) );

            //Copy Default profile from upper layer
            pstDefaultProfile = &(gpstQMIContext->wdsCtx.stDefaultProfileFromDB);
            memcpy( pstDefaultProfile, &(pstCtxInputStruct->stIfInput), sizeof(CellularProfileStruct) );
        }

        CELLULAR_HAL_DBG_PRINT ("%s Done\n", __FUNCTION__);

        return RETURN_OK;
    }

    CELLULAR_HAL_DBG_PRINT ("%s %d Error Exit\n", __FUNCTION__, __LINE__);

    return RETURN_ERROR;
}

static void cellular_hal_qmi_reinitialize_context ( void )
{
    if( NULL != gpstQMIContext )
    {
        CellularContextInitInputStruct  stCtxInitBackUp    = {0};

        //Backup context HAL input
        memcpy( &stCtxInitBackUp, &(gpstQMIContext->stCellularHALInput), sizeof(CellularContextInitInputStruct) );

        //Reinit
        memset(gpstQMIContext, 0, sizeof(QMIContextStructPrivate));

        snprintf(gpstQMIContext->modem_device_name, sizeof(gpstQMIContext->modem_device_name), "%s", QMI_DEVICE_NAME);
        snprintf(gpstQMIContext->modem_device_path, sizeof(gpstQMIContext->modem_device_path), "%s", QMI_DEVICE_PATH);
        
	gpstQMIContext->IsDeviceRemovedSignalReceived = FALSE;

        //UIM
        gpstQMIContext->uimCtx.total_no_of_slots                 = 0;
        gpstQMIContext->uimCtx.selected_slot_number              = CELLULAR_SLOT_ID_UNKNOWN;
        gpstQMIContext->uimCtx.ctxSlotInfo                       = NULL;

        //NAS
        gpstQMIContext->nasCtx.bIsModemRegTaskStillRunning       = FALSE;
        gpstQMIContext->nasCtx.LastCollectedTimeForPlmnInfo      = 0;
        gpstQMIContext->nasCtx.bIsNetworkScanTaskStillRunning    = FALSE;
        gpstQMIContext->nasCtx.bIsNeed2StopNetworkInfoCollection = FALSE;
        gpstQMIContext->nasCtx.enNASRegistrationStatus           = DEVICE_NAS_STATUS_NOT_REGISTERED;
        gpstQMIContext->nasCtx.idwCondForSCAN                    = idwCondForSCAN;
        gpstQMIContext->nasCtx.idwMutexForSCAN                   = idwMutexForSCAN;    
        gpstQMIContext->nasCtx.idwMutexForSCANData               = idwMutexForSCANData;

        //WDS
        gpstQMIContext->wdsCtx.ui8ProfileCount                   = 0;
        gpstQMIContext->wdsCtx.pstProfileList                    = NULL;

        gpstQMIContext->wdsCtx.client_ipv4                       = NULL;
        gpstQMIContext->wdsCtx.packet_data_handle_ipv4           = CELLULAR_PACKET_DATA_INVALID_HANDLE;
        gpstQMIContext->wdsCtx.packet_service_status_ipv4_id     = CELLULAR_QMI_INVALID_EVENT_INDICATION;
        gpstQMIContext->wdsCtx.event_report_ipv4_id              = CELLULAR_QMI_INVALID_EVENT_INDICATION;

        gpstQMIContext->wdsCtx.client_ipv6                       = NULL;
        gpstQMIContext->wdsCtx.packet_data_handle_ipv6           = CELLULAR_PACKET_DATA_INVALID_HANDLE;
        gpstQMIContext->wdsCtx.packet_service_status_ipv6_id     = CELLULAR_QMI_INVALID_EVENT_INDICATION;
        gpstQMIContext->wdsCtx.event_report_ipv6_id              = CELLULAR_QMI_INVALID_EVENT_INDICATION;

        //Default Profile
        gpstQMIContext->wdsCtx.bIsDefaultProfileFromModemCached        = FALSE;
        gpstQMIContext->wdsCtx.stDefaultProfileFromDB.ProfileID        = CELLULAR_PROFILE_ID_UNKNOWN;
        gpstQMIContext->wdsCtx.stDefaultProfileFromDB.PDPContextNumber = CELLULAR_PDP_CONTEXT_UNKNOWN;
        gpstQMIContext->wdsCtx.stDefaultProfileFromDB.PDPType          = CELLULAR_PDP_TYPE_IPV4;

        //Copy context init HAL input from backup
        memcpy( &(gpstQMIContext->stCellularHALInput), &stCtxInitBackUp, sizeof(CellularContextInitInputStruct) );

        //Copy Default profile from HAL input
        memcpy( &(gpstQMIContext->wdsCtx.stDefaultProfileFromDB), &(stCtxInitBackUp.stIfInput), sizeof(CellularProfileStruct) );
    }
}

/* QMIDevice */
static void  cellular_hal_qmi_device_open (QmiDevice *device, GAsyncResult *result, gpointer user_data)
{
    GError *error = NULL;
    const gchar *wwan_ifacename;
    CellularDeviceOpenStatus_t enOpenStatus;
    GTask                      *task           = (GTask*)user_data;
    QMIContextStructPrivate    *pstQMIContext  = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx = NULL;
    CellularDeviceContextCBStruct *pstDeviceCtxCB = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    pstDeviceCtxCB  = &(pDeviceOpenCtx->stDeviceCtxCB);

    if (!qmi_device_open_finish (device, result, &error)) {
        CELLULAR_HAL_DBG_PRINT ("%s %d Failed open the Device: %s\n", __FUNCTION__, __LINE__, error->message);
        pDeviceOpenCtx->enOpenStatus  = DEVICE_OPEN_STATUS_NOT_READY;
        pDeviceOpenCtx->uiCurrentStep = MODEM_OPEN_STATE_NOTIFY;
        cellular_hal_qmi_device_open_step(task);
        return;
    }
    
    //Configuring RAW IP kernel format
    qmi_device_set_expected_data_format(device, QMI_DEVICE_EXPECTED_DATA_FORMAT_RAW_IP, &error);

    wwan_ifacename = qmi_device_get_wwan_iface (device);
    snprintf(pstQMIContext->wwan_iface, sizeof(pstQMIContext->wwan_iface), "%s", wwan_ifacename);
    pDeviceOpenCtx->enOpenStatus = DEVICE_OPEN_STATUS_READY;
    CELLULAR_HAL_DBG_PRINT ("%s %d QMI Device at '%s' ready, wwan iface '%s'\n", __FUNCTION__, __LINE__, qmi_device_get_path_display (device), gpstQMIContext->wwan_iface);

    pDeviceOpenCtx->uiCurrentStep++;
    cellular_hal_qmi_device_open_step(task);
}

static void cellular_hal_qmi_device_removed_cb (QmiDevice *device, gpointer  user_data)
{
    cellular_device_removed_status_api_callback device_removed_status_cb = (cellular_device_removed_status_api_callback)user_data;
    ContextUIMInfo       *uimCtx = &(gpstQMIContext->uimCtx); 
    ContextNASInfo       *nasCtx = &(gpstQMIContext->nasCtx); 
    ContextWDSInfo       *wdsCtx = &(gpstQMIContext->wdsCtx); 
    ContextDMSInfo       *dmsCtx = &(gpstQMIContext->dmsCtx); 

    /* QMI device went offline */
    CELLULAR_HAL_DBG_PRINT("%s - Connection to qmi-proxy for %s lost\n", __FUNCTION__, qmi_device_get_path_display (gpstQMIContext->qmiDevice));
    
    gpstQMIContext->IsDeviceRemovedSignalReceived = TRUE;

    g_signal_handler_disconnect (gpstQMIContext->qmiDevice, gpstQMIContext->qmi_device_removed_id);
    gpstQMIContext->qmi_device_removed_id = CELLULAR_QMI_INVALID_EVENT_INDICATION;

    /* Needs to clear all context memories */
    g_signal_handler_disconnect (uimCtx->uimClient, uimCtx->slot_status_indication_id);
    uimCtx->slot_status_indication_id = CELLULAR_QMI_INVALID_EVENT_INDICATION;

    //Release NAS, WDS, UIM, DMS clients
    if( NULL != nasCtx->nasClient )
    {
        //If the task still running then needs to stop
        if ( TRUE == nasCtx->bIsNetworkScanTaskStillRunning )
        {
            nasCtx->bIsNeed2StopNetworkInfoCollection = TRUE;

            pthread_mutex_lock(&(nasCtx->idwMutexForSCAN));
            pthread_cond_signal(&(nasCtx->idwCondForSCAN));
            pthread_mutex_unlock(&(nasCtx->idwMutexForSCAN));

            nasCtx->iTotalNoofNetworkInfo = 0;
            memset( nasCtx->astNetworkInfo, 0, sizeof( nasCtx->astNetworkInfo ) );
        }

        //If the task still running then needs to stop
        if ( TRUE == nasCtx->bIsModemRegTaskStillRunning )
        {
            g_cancellable_cancel (nasCtx->cancellableForModemRegTask);
            sleep(2);
            g_object_unref (nasCtx->cancellableForModemRegTask);
            nasCtx->cancellableForModemRegTask = NULL;
        }

        qmi_device_release_client ( gpstQMIContext->qmiDevice,
                                    nasCtx->nasClient,
                                    QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                    3, NULL, NULL, NULL);

        g_object_unref (nasCtx->nasClient);
        nasCtx->nasClient = NULL;
        nasCtx->bIsValidNASClient = FALSE;
        nasCtx->serving_system_id = 0;
    }

    if( NULL != uimCtx->uimClient )
    {
        qmi_device_release_client ( gpstQMIContext->qmiDevice,
                                    uimCtx->uimClient,
                                    QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                    3, NULL, NULL, NULL);

        g_object_unref (uimCtx->uimClient);
        uimCtx->uimClient = NULL;
        uimCtx->bIsValiduimClient = FALSE;
    }

    if( NULL != wdsCtx->wdsClient )
    {
        qmi_device_release_client ( gpstQMIContext->qmiDevice,
                                    wdsCtx->wdsClient,
                                    QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                    3, NULL, NULL, NULL);

        g_object_unref (wdsCtx->wdsClient);
        wdsCtx->wdsClient = NULL;
        wdsCtx->bIsValidwdsClient = FALSE;
    }

    if( NULL != dmsCtx->dmsClient )
    {
        qmi_device_release_client ( gpstQMIContext->qmiDevice,
                                    dmsCtx->dmsClient,
                                    QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                    3, NULL, NULL, NULL);

        g_object_unref (dmsCtx->dmsClient);
        dmsCtx->dmsClient = NULL;
        dmsCtx->bIsValidDMSClient = FALSE;
    }

    //Stop Network
    cellular_hal_qmi_stop_network(CELLULAR_NETWORK_IP_FAMILY_IPV4);
    cellular_hal_qmi_stop_network(CELLULAR_NETWORK_IP_FAMILY_IPV6);
    sleep(5); //To be optimize

    //Free existing resource
    if( NULL != gpstQMIContext->wdsCtx.pstProfileList )
    {
        free(gpstQMIContext->wdsCtx.pstProfileList);
        gpstQMIContext->wdsCtx.pstProfileList = NULL;
    }

    //Needs to close the QMI device
    qmi_device_close_async (gpstQMIContext->qmiDevice,
                            5,
                            NULL,
                            NULL,
                            NULL);
    g_object_unref(gpstQMIContext->qmiDevice);
    gpstQMIContext->qmiDevice = NULL;

    //Reinitialize all members
    cellular_hal_qmi_reinitialize_context();

    //Send QMI not ready status if CB is not null
    if( NULL != device_removed_status_cb )
    {
        CELLULAR_HAL_DBG_PRINT("%s - (%s) Modem Removed and sending status via CB\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        device_removed_status_cb( gpstQMIContext->modem_device_name, DEVICE_REMOVED );
    }
}

static void cellular_hal_qmi_device_indication_cb (QmiDevice *device, gpointer  user_data)
{
    cellular_device_open_status_api_callback device_open_status_cb = (cellular_device_open_status_api_callback)user_data;

    /* QMI device went offline */
    CELLULAR_HAL_DBG_PRINT("%s - Connection to qmi-proxy for %s lost\n", __FUNCTION__, qmi_device_get_path_display (gpstQMIContext->qmiDevice));
    
    g_signal_handler_disconnect (NULL, gpstQMIContext->qmi_device_indication_id);
    gpstQMIContext->qmi_device_indication_id = CELLULAR_QMI_INVALID_EVENT_INDICATION;

    //Send QMI not ready status if CB is not null
    if( NULL != device_open_status_cb )
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) ready and sending status via CB\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        device_open_status_cb( gpstQMIContext->modem_device_name, gpstQMIContext->wwan_iface, DEVICE_OPEN_STATUS_NOT_READY, CELLULAR_MODEM_SET_OFFLINE );
    }
}

static void cellular_hal_qmi_device_new (GObject *unused,  GAsyncResult *result, gpointer  user_data)
{
    GError                     *error = NULL;
    QmiDeviceOpenFlags         open_flags;
    g_autofree gchar           *open_flags_str = NULL;
    QmiDevice                  *device;
    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate    *pstQMIContext  = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx = NULL;
    CellularDeviceContextCBStruct *pstDeviceCtxCB = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    pstDeviceCtxCB  = &(pDeviceOpenCtx->stDeviceCtxCB);

    device = qmi_device_new_finish (result, &error);
    if (!device) {
        CELLULAR_HAL_DBG_PRINT ("%s %d Failed to create Device: %s\n", __FUNCTION__,__LINE__,error->message);
        
        pDeviceOpenCtx->enOpenStatus  = DEVICE_OPEN_STATUS_NOT_READY;
        pDeviceOpenCtx->uiCurrentStep = MODEM_OPEN_STATE_NOTIFY;
        cellular_hal_qmi_device_open_step(task);
        return;
    }

    pstQMIContext->qmiDevice = device;
    CELLULAR_HAL_DBG_PRINT("%s %d - QMI Device Allocation is success. So Opening a Device\n",__FUNCTION__,__LINE__);

    /* Now open the QMI device without any data format CTL flag */
    open_flags = (QMI_DEVICE_OPEN_FLAGS_NET_NO_QOS_HEADER | QMI_DEVICE_OPEN_FLAGS_NET_RAW_IP | QMI_DEVICE_OPEN_FLAGS_VERSION_INFO | QMI_DEVICE_OPEN_FLAGS_PROXY);
    open_flags_str = qmi_device_open_flags_build_string_from_mask (open_flags);
    CELLULAR_HAL_DBG_PRINT ("%s %d Opening device with flags: %s...\n", __FUNCTION__, __LINE__, open_flags_str);

    /* Register Signal Disconnect Event */
    pstQMIContext->qmi_device_removed_id = g_signal_connect (  pstQMIContext->qmiDevice,
                                                                QMI_DEVICE_SIGNAL_REMOVED,
                                                                G_CALLBACK (cellular_hal_qmi_device_removed_cb),
                                                                pstDeviceCtxCB->device_remove_status_cb);
    /* Open the device */
    qmi_device_open (pstQMIContext->qmiDevice,
                     open_flags,
                     30,
                     NULL,
                     (GAsyncReadyCallback)cellular_hal_qmi_device_open,
                     task);
}

static void cellular_hal_qmi_client_device_open_wds (QmiDevice *device, GAsyncResult *result, gpointer  user_data)
{
    GError       *error = NULL;
    GTask        *task  = (GTask *)user_data;
    QmiClient    *wdsclient;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx  = NULL;
    ContextWDSInfo             *wdsCtx          = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    wdsCtx          = &(pstQMIContext->wdsCtx);

    wdsclient = qmi_device_allocate_client_finish (device, result, &error);
    if (!wdsclient) {
        CELLULAR_HAL_DBG_PRINT ("%s Failed to create client for the '%s' service: %s\n",
                                __FUNCTION__,
                                qmi_service_get_string (QMI_SERVICE_WDS),
                                error->message);

        pDeviceOpenCtx->uiCurrentStep = MODEM_OPEN_STATE_NOTIFY;
        cellular_hal_qmi_device_open_step(task);
        return;
    }

    //Store WDS context 
    wdsCtx->wdsClient         = wdsclient;
    wdsCtx->bIsValidwdsClient = TRUE;

    CELLULAR_HAL_DBG_PRINT("%s - WDS client allocation success\n",__FUNCTION__);

    pDeviceOpenCtx->uiCurrentStep++;
    cellular_hal_qmi_device_open_step(task);
}

static void cellular_hal_qmi_get_transmit_receive_info ( QmiClientNas *nasClient,
                                                               GAsyncResult *result,
                                                               gpointer user_data )
{
    ContextNASInfo *nasCtx = (ContextNASInfo*)user_data;
    QmiMessageNasGetTxRxInfoOutput *output;
    GError *error = NULL;
    gboolean is_in_traffic;
    gint32 power;
    
    output = qmi_client_nas_get_tx_rx_info_finish (nasClient, result, &error);
    if (!output) 
    { 
        CELLULAR_HAL_DBG_PRINT("%s TX/RD info get functionality failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        nasCtx->bIsSignalInfoCollectionDone  = TRUE;
        return;
    }

    if (!qmi_message_nas_get_tx_rx_info_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT("%s failed to get TX/RX info: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_nas_get_tx_rx_info_output_unref (output);
        nasCtx->bIsSignalInfoCollectionDone  = TRUE;
        return;
    }

    /* TX Channel */
    if (qmi_message_nas_get_tx_rx_info_output_get_tx_info (
            output,
            &is_in_traffic,
            &power,
            NULL)) 
    {
        nasCtx->tx_power = (is_in_traffic) ? (0.1) * ((gdouble)power) : 0;
    }

    qmi_message_nas_get_tx_rx_info_output_unref (output);
    nasCtx->bIsSignalInfoCollectionDone  = TRUE;
}

static void cellular_hal_qmi_get_network_signal_information_cb (QmiClientNas *nasClient, GAsyncResult *result, gpointer  user_data)
{
    ContextNASInfo *nasCtx = (ContextNASInfo*)user_data;
    QmiMessageNasGetSignalInfoOutput *output;
    GError *error = NULL;
    gint8  iRSRQ, iRSSI;
    gint16 iRSRP, iSNR;

    output = qmi_client_nas_get_signal_info_finish (nasClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s signal get info failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        nasCtx->bIsSignalInfoCollectionDone  = TRUE;
        return;
    }

    if (!qmi_message_nas_get_signal_info_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s %d signal get info failed: %s\n",__FUNCTION__,__LINE__,error->message);
        g_error_free (error);
        qmi_message_nas_get_signal_info_output_unref (output);
        nasCtx->bIsSignalInfoCollectionDone  = TRUE;
        return;
    }

    /* LTE... */
    nasCtx->lte_rssi = 0;
    nasCtx->lte_rsrq = 0;
    nasCtx->lte_rsrp = 0;
    nasCtx->lte_snr  = 0;

    if (qmi_message_nas_get_signal_info_output_get_lte_signal_strength (output,
                                                                        &iRSSI,
                                                                        &iRSRQ,
                                                                        &iRSRP,
                                                                        &iSNR,
                                                                        NULL)) 
    {
        nasCtx->lte_rssi = iRSSI;
        nasCtx->lte_rsrq = iRSRQ;
        nasCtx->lte_rsrp = iRSRP;
        nasCtx->lte_snr  = (0.1) * ((gdouble)iSNR);
    }

    nasCtx->tx_power = 0;

    /* Collect TX Power Level */
    {
        QmiMessageNasGetTxRxInfoInput *input;
        GError *error = NULL;

        input = qmi_message_nas_get_tx_rx_info_input_new ();
        if (!qmi_message_nas_get_tx_rx_info_input_set_radio_interface ( input,
                                                                        QMI_NAS_RADIO_INTERFACE_LTE,
                                                                        &error)) 
        {
            CELLULAR_HAL_DBG_PRINT("%s error: couldn't create input data bundle: '%s'\n",
                                    __FUNCTION__,
                                    error->message);
            g_error_free (error);
            qmi_message_nas_get_tx_rx_info_input_unref (input);
            nasCtx->bIsSignalInfoCollectionDone = TRUE;
            return;
        }

        qmi_client_nas_get_tx_rx_info(QMI_CLIENT_NAS(nasCtx->nasClient),
                                      input,
                                      10,
                                      NULL,
                                      (GAsyncReadyCallback)cellular_hal_qmi_get_transmit_receive_info,
                                      user_data);
        qmi_message_nas_get_tx_rx_info_input_unref (input);
    }
}

int cellular_hal_qmi_get_network_signal_information(CellularSignalInfoStruct *signal_info)
{
    //Input Validation
    if( NULL == signal_info ) 
    {
        return RETURN_ERROR;
    }

    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNASInfo    *nasCtx = &(gpstQMIContext->nasCtx);
    
        if( NULL != nasCtx->nasClient )
        {
            nasCtx->bIsSignalInfoCollectionDone = FALSE;

            qmi_client_nas_get_signal_info (QMI_CLIENT_NAS(nasCtx->nasClient),
                                            NULL,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback)cellular_hal_qmi_get_network_signal_information_cb,
                                            nasCtx);

            //Wait here till signal info
            while ( 1 )
            {
                if( TRUE == nasCtx->bIsSignalInfoCollectionDone )
                {
                    nasCtx->bIsSignalInfoCollectionDone = FALSE;

                    memset( signal_info, 0, sizeof(CellularSignalInfoStruct) );
                    signal_info->RSSI = nasCtx->lte_rssi;
                    signal_info->RSRQ = nasCtx->lte_rsrq;
                    signal_info->RSRP = nasCtx->lte_rsrp;
                    signal_info->SNR  = nasCtx->lte_snr;
                    signal_info->TXPower = nasCtx->tx_power;
                    break;
                }

                sleep(1);
            }

            return RETURN_OK;
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s - NAS client is not ready so signal info can't be populated\n", __FUNCTION__);
            return RETURN_ERROR;
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI Device is not ready  so signal info can't be populated\n", __FUNCTION__);
    }
}

static void cellular_hal_qmi_get_modem_identification (QmiClientDms *dmsClient,
                                                             GAsyncResult *result,
                                                             gpointer  user_data)
{
    const gchar *imei = NULL;
    const gchar *imei_software_version = NULL;
    QmiMessageDmsGetIdsOutput *output;
    GError *error = NULL;
    ContextGetIDs *pstCtxGetIDs = (ContextGetIDs*)user_data;
    GTask        *task  = NULL;
    ContextDeviceOpen  *pDeviceOpenCtx  = NULL;
    ContextDMSInfo     *dmsCtx          = NULL;

    if( DMS_GET_IDS_VIA_DEVICE_OPEN == pstCtxGetIDs->enQuerySource )
    {
        task  = (GTask *)pstCtxGetIDs->vpPrivateData;
        QMIContextStructPrivate    *pstQMIContext   = NULL;

        pDeviceOpenCtx  = g_task_get_task_data (task);
        pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
        dmsCtx          = &(pstQMIContext->dmsCtx);
    }
    else if( DMS_GET_IDS_VIA_DML == pstCtxGetIDs->enQuerySource )
    {
        dmsCtx = (ContextDMSInfo*)pstCtxGetIDs->vpPrivateData;
    }

    output = qmi_client_dms_get_ids_finish (dmsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Get Identif: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        goto NEXTSTEP;
    }

    if (!qmi_message_dms_get_ids_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Failed to get IDs: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_dms_get_ids_output_unref (output);
        goto NEXTSTEP;
    }

    qmi_message_dms_get_ids_output_get_imei (output, &imei, NULL);
    snprintf(dmsCtx->IMEI, sizeof(dmsCtx->IMEI), "%s", imei);
    CELLULAR_HAL_DBG_PRINT("%s Device IMEI retrieved: '%s'\n", __FUNCTION__,dmsCtx->IMEI);

    if (qmi_message_dms_get_ids_output_get_imei_software_version (output,
                                                                  &imei_software_version,
                                                                  NULL)) 
    {
        snprintf(dmsCtx->IMEI_SV, sizeof(dmsCtx->IMEI_SV), "%s", imei_software_version);
        CELLULAR_HAL_DBG_PRINT("%s IMEI SV retrieved: '%s'\n",__FUNCTION__,dmsCtx->IMEI_SV);
    }

    qmi_message_dms_get_ids_output_unref (output);
    
NEXTSTEP:
    if( DMS_GET_IDS_VIA_DEVICE_OPEN == pstCtxGetIDs->enQuerySource )
    {
        pstCtxGetIDs->IsQueryDone = TRUE;

        if( NULL != pstCtxGetIDs )
        {
            g_slice_free(ContextGetIDs, pstCtxGetIDs);
            pstCtxGetIDs  = NULL;
        }

        pDeviceOpenCtx->uiCurrentStep++;
        cellular_hal_qmi_device_open_step(task);
    }
    else if( DMS_GET_IDS_VIA_DML == pstCtxGetIDs->enQuerySource )
    {
        pstCtxGetIDs->IsQueryDone = TRUE;
    }
}

static void cellular_hal_qmi_get_modem_identification_info( ContextGetIDs *pstCtxGetIDs )
{
    if( DMS_GET_IDS_VIA_DEVICE_OPEN == pstCtxGetIDs->enQuerySource )
    {
        GTask        *task  = (GTask *)pstCtxGetIDs->vpPrivateData;
        QMIContextStructPrivate    *pstQMIContext   = NULL;
        ContextDeviceOpen          *pDeviceOpenCtx  = NULL;
        ContextDMSInfo             *dmsCtx          = NULL;

        pDeviceOpenCtx  = g_task_get_task_data (task);
        pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
        dmsCtx          = &(pstQMIContext->dmsCtx);

        qmi_client_dms_get_ids (QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                NULL,
                                10,
                                NULL,
                                (GAsyncReadyCallback)cellular_hal_qmi_get_modem_identification,
                                pstCtxGetIDs);
    }
    else if( DMS_GET_IDS_VIA_DML == pstCtxGetIDs->enQuerySource )
    {
        ContextDMSInfo   *dmsCtx = (ContextDMSInfo*)pstCtxGetIDs->vpPrivateData;

        qmi_client_dms_get_ids (QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                NULL,
                                10,
                                NULL,
                                (GAsyncReadyCallback)cellular_hal_qmi_get_modem_identification,
                                pstCtxGetIDs);
    }
}

static void cellular_hal_qmi_client_device_open_dms (QmiDevice *device, GAsyncResult *result, gpointer  user_data)
{
    GError       *error = NULL;
    GTask        *task  = (GTask *)user_data;
    QmiClient    *dmsclient;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx  = NULL;
    ContextDMSInfo             *dmsCtx          = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    dmsCtx          = &(pstQMIContext->dmsCtx);

    dmsclient = qmi_device_allocate_client_finish (device, result, &error);
    if (!dmsclient) {
        CELLULAR_HAL_DBG_PRINT ("%s Failed to create client for the '%s' service: %s\n",
                                __FUNCTION__,
                                qmi_service_get_string (QMI_SERVICE_DMS),
                                error->message);

        pDeviceOpenCtx->uiCurrentStep = MODEM_OPEN_STATE_NOTIFY;
        cellular_hal_qmi_device_open_step(task);
        return;
    }

    //Store DMS context 
    dmsCtx->dmsClient         = dmsclient;
    dmsCtx->bIsValidDMSClient = TRUE;

    pDeviceOpenCtx->uiCurrentStep++;
    cellular_hal_qmi_device_open_step(task);
}

static void cellular_hal_qmi_client_device_open_nas (QmiDevice *device, GAsyncResult *result, gpointer  user_data)
{
    GError       *error = NULL;
    GTask        *task  = (GTask *)user_data;
    QmiClient    *nasclient;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx  = NULL;
    ContextNASInfo             *nasCtx          = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    nasCtx          = &(pstQMIContext->nasCtx);

    nasclient = qmi_device_allocate_client_finish (device, result, &error);
    if (!nasclient) {
        CELLULAR_HAL_DBG_PRINT ("%s Failed to create client for the '%s' service: %s\n",
                                __FUNCTION__,
                                qmi_service_get_string (QMI_SERVICE_NAS),
                                error->message);

        pDeviceOpenCtx->uiCurrentStep = MODEM_OPEN_STATE_NOTIFY;
        cellular_hal_qmi_device_open_step(task);
        return;
    }

    //Store NAS context 
    nasCtx->nasClient         = nasclient;
    nasCtx->bIsValidNASClient = TRUE;
    nasCtx->serving_system_id = 0;

    pDeviceOpenCtx->uiCurrentStep++;
    cellular_hal_qmi_device_open_step(task);
}

static void
cellular_hal_qmi_get_revision (QmiClientDms *client,
                                     GAsyncResult *res,
                                     gpointer  user_data)
{
    const gchar *str = NULL;
    QmiMessageDmsGetRevisionOutput *output;
    GError *error = NULL;
    GTask        *task  = (GTask *)user_data;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx  = NULL;
    ContextDMSInfo             *dmsCtx          = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    dmsCtx          = &(pstQMIContext->dmsCtx);

    output = qmi_client_dms_get_revision_finish (client, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("Failed to get revision: %s\n", error->message);
        g_error_free (error);
        goto NEXTSTEP;
    }

    if (!qmi_message_dms_get_revision_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("Unable to get revision: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_revision_output_unref (output);
        goto NEXTSTEP;
    }

    qmi_message_dms_get_revision_output_get_revision (output, &str, NULL);

    CELLULAR_HAL_DBG_PRINT ("[%s] Device revision retrieved:\n"
                                    "\tRevision: '%s'\n",
                                    qmi_device_get_path_display (pstQMIContext->qmiDevice),
                                    VALIDATE_UNKNOWN (str));

    snprintf(dmsCtx->FirmwareRevision, sizeof(dmsCtx->FirmwareRevision), "%s", str);

    qmi_message_dms_get_revision_output_unref (output);

NEXTSTEP:
    pDeviceOpenCtx->uiCurrentStep++;
    cellular_hal_qmi_device_open_step(task);
}

static void
cellular_hal_qmi_get_software_version (QmiClientDms *client,
                                       GAsyncResult *result,
                                       gpointer  user_data)
{
    QmiMessageDmsGetSoftwareVersionOutput *output;
    GError *error = NULL;
    const gchar *version;
    GTask        *task  = (GTask *)user_data;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx  = NULL;
    ContextDMSInfo             *dmsCtx          = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    dmsCtx          = &(pstQMIContext->dmsCtx);

    output = qmi_client_dms_get_software_version_finish (client, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("Failed to fetch software version: %s\n", error->message);
        g_error_free (error);
        goto NEXTSTEP;
    }

    if (!qmi_message_dms_get_software_version_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("Unable to get image download mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_software_version_output_unref (output);
        goto NEXTSTEP;
    }

    qmi_message_dms_get_software_version_output_get_version (output, &version, NULL);

    CELLULAR_HAL_DBG_PRINT("[%s] Software version: %s\n",
                                            qmi_device_get_path_display (pstQMIContext->qmiDevice),
                                            version);

    snprintf(dmsCtx->FirmwareVersion, sizeof(dmsCtx->FirmwareVersion), "%s", version);

    qmi_message_dms_get_software_version_output_unref (output);

NEXTSTEP:
    pDeviceOpenCtx->uiCurrentStep++;
    cellular_hal_qmi_device_open_step(task);
}

static void cellular_hal_qmi_get_operating_mode_configuration_cb (QmiClientDms *dmsClient,
                                                                  GAsyncResult *result,
                                                                  gpointer  user_data)
{
    QmiMessageDmsGetOperatingModeOutput *output;
    QmiDmsOperatingMode operating_mode;
    GError *error = NULL;
    GTask        *task  = (GTask *)user_data;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextDeviceOpen          *pDeviceOpenCtx  = NULL;
    ContextDMSInfo             *dmsCtx          = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    dmsCtx          = &(pstQMIContext->dmsCtx);

    pDeviceOpenCtx->modem_operating_mode = QMI_DMS_OPERATING_MODE_OFFLINE;

    output = qmi_client_dms_get_operating_mode_finish (dmsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Get Modem Operating Mode Operation is failed %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        goto NEXTSTEP;
    }

    if (!qmi_message_dms_get_operating_mode_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Failed to get Modem Mode %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_dms_get_operating_mode_output_unref (output);
        goto NEXTSTEP;
    }

    qmi_message_dms_get_operating_mode_output_get_mode (output, &operating_mode, NULL);
    pDeviceOpenCtx->modem_operating_mode = operating_mode;

    CELLULAR_HAL_DBG_PRINT("[%s] Operating mode is '%s'\n", qmi_device_get_path_display (pstQMIContext->qmiDevice), qmi_dms_operating_mode_get_string (operating_mode));

    qmi_message_dms_get_operating_mode_output_unref (output);

NEXTSTEP:
    pDeviceOpenCtx->uiCurrentStep++;
    cellular_hal_qmi_device_open_step(task);
}

#ifdef _WNXL11BWL_PRODUCT_REQ_
static int cellular_hal_qmi_get_software_version_via_ATCmd( char *pOutput, int iOutputSize )
{
    FILE   *FilePtr            = NULL;
    char   bufContent[ 512 ]  = { 0 };

    if ( ( NULL == pOutput ) || ( 0 == iOutputSize ) )
    {
        return -1;
    }

    FilePtr = v_secure_popen( "r", "echo -e \"AT+QGMR\r\" | microcom -t 100 /dev/ttyUSB3 | tail +2 | head -n 1" );
    if (FilePtr == NULL)
    {
        return -1;
    }

    char *pos;

    fgets( bufContent, 512, FilePtr );
    // Remove line \n charecter from string
    if ( ( pos = strchr( bufContent, '\n' ) ) != NULL )
    {
        *pos = '\0';
    }

    snprintf( pOutput, iOutputSize, "%s", bufContent );
    v_secure_pclose( FilePtr );
    FilePtr = NULL;

    return 0;
}
#endif

static void cellular_hal_qmi_device_open_step( GTask *task )
{
    QMIContextStructPrivate   *pstQMIContext  = NULL;
    ContextDeviceOpen         *pDeviceOpenCtx = NULL;

    pDeviceOpenCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceOpenCtx->vpPrivateData;
    
    switch ( pDeviceOpenCtx->uiCurrentStep ) 
    {
        case MODEM_OPEN_STATE_BEGIN:
        {
            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_DEVICE_OPEN:
        {
            if( ( NULL != pstQMIContext ) && \
                ( NULL != pstQMIContext->qmiDevice ) && \
                ( TRUE == qmi_device_is_open( pstQMIContext->qmiDevice ) ) )
            {
                pDeviceOpenCtx->enOpenStatus = DEVICE_OPEN_STATUS_READY;
                CELLULAR_HAL_DBG_PRINT("%s - QMI device is valid/opened so proceeding further\n",__FUNCTION__);
            }
            else
            {
                GFile  *file = NULL;
                
                CELLULAR_HAL_DBG_PRINT("%s - QMI device is not opened so opening now\n",__FUNCTION__);
                file = g_file_new_for_path ( QMI_DEVICE_PATH );
                qmi_device_new (file,
                                NULL,
                                (GAsyncReadyCallback)cellular_hal_qmi_device_new,
                                task);

                g_object_unref(file);
                return;
            }

            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_DMS_OPEN:
        {
            ContextDMSInfo  *dmsCtx = &(pstQMIContext->dmsCtx);

            //Check whether DMS client is valid or not. If not then needs to create new client for nas info
            if ( FALSE == dmsCtx->bIsValidDMSClient )
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - Allocating DMS client\n",__FUNCTION__,__LINE__);
                qmi_device_allocate_client (pstQMIContext->qmiDevice,
                                            QMI_SERVICE_DMS,
                                            QMI_CID_NONE,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback) cellular_hal_qmi_client_device_open_dms,
                                            task);
                return;
            }
            else
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - DMS Client is valid so proceeding further\n",__FUNCTION__,__LINE__);
            }

            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_GET_REVISION:
        {
            ContextDMSInfo  *dmsCtx  = &(pstQMIContext->dmsCtx);

            qmi_client_dms_get_revision (QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                         NULL,
                                         10,
                                         NULL,
                                         (GAsyncReadyCallback)cellular_hal_qmi_get_revision,
                                         task);
            return;
        }
        case MODEM_OPEN_STATE_GET_VERSION:
        {
            ContextDMSInfo  *dmsCtx  = &(pstQMIContext->dmsCtx);

//Needs to get firmware version via AT commands since QMI not providing full version information for quectel
#ifdef _WNXL11BWL_PRODUCT_REQ_
            cellular_hal_qmi_get_software_version_via_ATCmd( dmsCtx->FirmwareVersion, sizeof(dmsCtx->FirmwareVersion) );
            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
#else
            qmi_client_dms_get_software_version (QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                                 NULL,
                                                 10,
                                                 NULL,
                                                 (GAsyncReadyCallback)cellular_hal_qmi_get_software_version,
                                                 task);
            return;
#endif
        }
        case MODEM_OPEN_STATE_GET_IMEI:
        {
            ContextGetIDs *pstCtxGetIDs = NULL;

            pstCtxGetIDs = (ContextGetIDs*)g_slice_new0 (ContextGetIDs);
            pstCtxGetIDs->enQuerySource = DMS_GET_IDS_VIA_DEVICE_OPEN;
            pstCtxGetIDs->IsQueryDone = FALSE;
            pstCtxGetIDs->vpPrivateData = (void*)task;

            cellular_hal_qmi_get_modem_identification_info( pstCtxGetIDs );
            return;
        }
        case MODEM_OPEN_STATE_GET_OPERATING_MODE:
        {
	    ContextDMSInfo  *dmsCtx  = &(pstQMIContext->dmsCtx);

            qmi_client_dms_get_operating_mode ( QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                                NULL,
                                                10,
                                                NULL,
                                                (GAsyncReadyCallback)cellular_hal_qmi_get_operating_mode_configuration_cb,
                                                task);
            return;
        }
        case MODEM_OPEN_STATE_WDS_OPEN:
        {
            ContextWDSInfo  *wdsCtx = &(pstQMIContext->wdsCtx);

            //Check whether WDS client is valid or not. If not then needs to create new client for profile
            if ( FALSE == wdsCtx->bIsValidwdsClient )
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - Allocating WDS client\n",__FUNCTION__,__LINE__);
                qmi_device_allocate_client (pstQMIContext->qmiDevice,
                                            QMI_SERVICE_WDS,
                                            QMI_CID_NONE,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback) cellular_hal_qmi_client_device_open_wds,
                                            task);
                return;
            }
            else
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - WDS client is valid so proceeding further\n",__FUNCTION__,__LINE__);
            }

            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_GET_CAPABILITIES:
        {
            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_NAS_OPEN:
        {
            ContextNASInfo  *nasCtx = &(pstQMIContext->nasCtx);

            //Check whether NAS client is valid or not. If not then needs to create new client for nas info
            if ( FALSE == nasCtx->bIsValidNASClient )
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - Allocating NAS client\n",__FUNCTION__,__LINE__);
                qmi_device_allocate_client (pstQMIContext->qmiDevice,
                                            QMI_SERVICE_NAS,
                                            QMI_CID_NONE,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback) cellular_hal_qmi_client_device_open_nas,
                                            task);
                return;
            }
            else
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - NAS Client is valid so proceeding further\n",__FUNCTION__,__LINE__);
            }

            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_NAS_REGISTER_SIGNAL_INDICATIONS:
        {
            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_NAS_NETWORK_SCAN_INIT:
        {
            cellular_hal_qmi_network_scan_data_collection_task();
            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_NOTIFY:
        {
            CellularDeviceContextCBStruct *pstDeviceCtxCB = NULL;

            pstDeviceCtxCB  = &(pDeviceOpenCtx->stDeviceCtxCB);

            //Send QMI ready status if CB is not null
            if( NULL != pstDeviceCtxCB->device_open_status_cb )
            {
                CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) %s and sending status via CB\n", __FUNCTION__, pstQMIContext->modem_device_name, ( pDeviceOpenCtx->enOpenStatus == DEVICE_OPEN_STATUS_READY ) ? "ready" : "not ready");
                pstDeviceCtxCB->device_open_status_cb( pstQMIContext->modem_device_name, 
                                                       ( DEVICE_OPEN_STATUS_READY == pDeviceOpenCtx->enOpenStatus ) ?  pstQMIContext->wwan_iface : "Unknown", pDeviceOpenCtx->enOpenStatus,
                                                       ( QMI_DMS_OPERATING_MODE_ONLINE == pDeviceOpenCtx->modem_operating_mode ) ? CELLULAR_MODEM_SET_ONLINE : CELLULAR_MODEM_SET_OFFLINE );
            }

            pDeviceOpenCtx->uiCurrentStep++;
            /* fall through */
        }
        case MODEM_OPEN_STATE_END:
        default:
        {
            g_object_unref (task);
        }
    }
}

static void cellular_hal_qmi_device_open_context_free( ContextDeviceOpen *pDeviceOpenCtx )
{
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources\n",__FUNCTION__);

    if( NULL != pDeviceOpenCtx)
    {
        g_slice_free(ContextDeviceOpen, pDeviceOpenCtx);
        pDeviceOpenCtx  = NULL;
    }
}

/* cellular_hal_qmi_open_device() */
int cellular_hal_qmi_open_device(CellularDeviceContextCBStruct *pstDeviceCtxCB)
{
    //Check whether device file exists or not
    if( TRUE == cellular_hal_util_IsDeviceFileExists( gpstQMIContext->modem_device_name ) )
    {
        ContextDeviceOpen  *pDeviceOpenCtx = NULL;
        GTask              *task;

        pDeviceOpenCtx                  = (ContextDeviceOpen*)g_slice_new0 (ContextDeviceOpen);
        pDeviceOpenCtx->uiCurrentStep   = MODEM_OPEN_STATE_BEGIN;
        pDeviceOpenCtx->vpPrivateData   = (void*) gpstQMIContext;
        if( NULL != pstDeviceCtxCB )
        memcpy(&(pDeviceOpenCtx->stDeviceCtxCB), pstDeviceCtxCB, sizeof(CellularDeviceContextCBStruct));

        task = g_task_new (NULL, NULL, NULL, NULL);
        g_task_set_task_data (task, (gpointer)pDeviceOpenCtx, (GDestroyNotify)cellular_hal_qmi_device_open_context_free);

        CELLULAR_HAL_DBG_PRINT("%s - Opening QMI Device Task:0x%p\n",__FUNCTION__,task);
        cellular_hal_qmi_device_open_step(task);
    }
    else
    {
        /* Register Signal Connect Event */
        gpstQMIContext->qmi_device_indication_id = g_signal_connect (  NULL,
                                                                       QMI_DEVICE_SIGNAL_INDICATION,
                                                                       G_CALLBACK (cellular_hal_qmi_device_indication_cb),
                                                                       pstDeviceCtxCB->device_open_status_cb);

        CELLULAR_HAL_DBG_PRINT("%s - QMI device is not ready Event Indication ID:%d\n",__FUNCTION__,gpstQMIContext->qmi_device_indication_id);

        //Send QMI not ready status if CB is not null
        if( ( NULL != pstDeviceCtxCB ) && (NULL != pstDeviceCtxCB->device_open_status_cb ) )
        {
            CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready and sending status via CB\n", __FUNCTION__, gpstQMIContext->modem_device_name);
            pstDeviceCtxCB->device_open_status_cb( gpstQMIContext->modem_device_name, "unknown", DEVICE_OPEN_STATUS_NOT_READY, CELLULAR_MODEM_SET_OFFLINE );
        }
    }

    return RETURN_OK;
}

unsigned char cellular_hal_qmi_IsModemControlInterfaceOpened( void )
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        return TRUE;
    }

    return FALSE;
}

int cellular_hal_qmi_get_modem_firmware_version(char *firmware_version)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextDMSInfo  *dmsCtx  = &(gpstQMIContext->dmsCtx);

        if( dmsCtx->FirmwareVersion[0] != '\0' )
        {
            memcpy(firmware_version, dmsCtx->FirmwareVersion, strlen(dmsCtx->FirmwareVersion) + 1);
        }
        else
        {
            strcpy(firmware_version, "");
        }

        return RETURN_OK;
    }

    return RETURN_ERROR;
}

int cellular_hal_qmi_get_current_modem_interface_status( CellularInterfaceStatus_t *status )
{
    //Validate input
    if( NULL == status )
    {
        return RETURN_ERROR;
    } 
    
    *status = IF_DOWN;

    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);

        if( NULL != nasCtx->nasClient )
        {
            ContextModemRegistration  *pDeviceRegCtx = NULL;
            GTask                     *task;
            int iLapsedSeconds = 0;

            pDeviceRegCtx                  = (ContextModemRegistration*)g_slice_new0 (ContextModemRegistration);
            pDeviceRegCtx->uiCurrentStep   = NAS_MODEM_REGISTRATION_STATUS_BEGIN;
            pDeviceRegCtx->vpPrivateData   = (void*) gpstQMIContext;
            pDeviceRegCtx->device_registration_status_cb = NULL;
            pDeviceRegCtx->enQuerySource   = NAS_MODEM_REGISTRATION_GET_VIA_DML;
            pDeviceRegCtx->bIsModemRegTaskDMLGetCompleted = FALSE;

            task = g_task_new (NULL, NULL, NULL, NULL);
            g_task_set_task_data (task, (gpointer)pDeviceRegCtx, (GDestroyNotify)cellular_hal_qmi_monitor_device_registration_context_free);

            CELLULAR_HAL_DBG_PRINT("%s - Query Modem Device Registration Status Task:0x%p\n",__FUNCTION__,task);
            cellular_hal_qmi_monitor_device_registration_step(task);

            //Wait here till Registration Status Query 
            while ( 1 )
            {
                if( TRUE == pDeviceRegCtx->bIsModemRegTaskDMLGetCompleted )
                {
                    if( DEVICE_NAS_STATUS_REGISTERED == pDeviceRegCtx->enNASRegistrationStatus )
                    {
                        *status = IF_UP;
                    }
                    else
                    {
                        *status = IF_DOWN;
                    }
                    break;
                }
                else if( iLapsedSeconds >= CELLULAR_QMI_GETREG_STATUS_VIA_DML_MAX_WAITIME )  
                {
                    CELLULAR_HAL_DBG_PRINT("%s - Timeout during Reg Query\n",__FUNCTION__);
                    *status = IF_DOWN;
                    break;
                }

                sleep(1);
                iLapsedSeconds++;
            }

            //Free allocated resource
            if( NULL != pDeviceRegCtx)
            {
                g_slice_free(ContextModemRegistration, pDeviceRegCtx);
                pDeviceRegCtx  = NULL;
            }

            return RETURN_OK;
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s - NAS client is not ready so can't be query registration status\n", __FUNCTION__);
            return RETURN_ERROR;
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't be query registration status\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

static void cellular_hal_qmi_set_operating_mode ( QmiClientDms *dmsClient,
                                                        GAsyncResult *result,
                                                        gpointer  user_data)
{
    ContextModemConfiguration  *pDeviceModemConfigCtx = (ContextModemConfiguration*)user_data;
    QmiMessageDmsSetOperatingModeOutput *output;
    GError *error = NULL;
    QMIContextStructPrivate    *pstQMIContext   = NULL;

    pstQMIContext   = (QMIContextStructPrivate*)pDeviceModemConfigCtx->vpPrivateData;

    output = qmi_client_dms_set_operating_mode_finish (dmsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Set Operation Failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        goto LAST_STEP;
    }

    if (!qmi_message_dms_set_operating_mode_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Failed to set operating mode: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_dms_set_operating_mode_output_unref (output);
        goto LAST_STEP;
    }

    CELLULAR_HAL_DBG_PRINT("%s - Successfully set operating mode. Value '%d'\n",
                                            __FUNCTION__,
                                            pDeviceModemConfigCtx->uiRequestedOperatingConfig);

    qmi_message_dms_set_operating_mode_output_unref (output);

LAST_STEP:
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources\n",__FUNCTION__);
    if( NULL != pDeviceModemConfigCtx )
    {
        g_slice_free(ContextModemConfiguration, pDeviceModemConfigCtx);
        pDeviceModemConfigCtx  = NULL;
    }
}

int cellular_hal_qmi_set_modem_operating_configuration(CellularModemOperatingConfiguration_t modem_operating_config)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        QmiMessageDmsSetOperatingModeInput *input = NULL;
        ContextModemConfiguration  *pDeviceModemConfigCtx = NULL;
        QmiDmsOperatingMode mode;
        ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);
        GError *error = NULL;
        
        switch( modem_operating_config )
        {
            case CELLULAR_MODEM_SET_ONLINE:
            {
                mode = QMI_DMS_OPERATING_MODE_RESET; //After OFFLINE we should reset the modem to come online
            }
            break;
            case CELLULAR_MODEM_SET_OFFLINE:
            {
                mode = QMI_DMS_OPERATING_MODE_OFFLINE;
            }
            break;
            case CELLULAR_MODEM_SET_RESET:
            {
                mode = QMI_DMS_OPERATING_MODE_RESET;
            }
            break;
            default:
            {
                CELLULAR_HAL_DBG_PRINT("%s - Invalid modem configure state request : %d\n", __FUNCTION__,modem_operating_config);
                return RETURN_ERROR;
            }
        }

        CELLULAR_HAL_DBG_PRINT("%s - Modem configure state request:%d\n", __FUNCTION__,mode);

        if( FALSE == dmsCtx->bIsValidDMSClient )
        {
            CELLULAR_HAL_DBG_PRINT("%s - DMS client is not valid. so unable to proceed modem configuration\n", __FUNCTION__);
            return RETURN_ERROR;
        }

        input = qmi_message_dms_set_operating_mode_input_new ();
        if (!qmi_message_dms_set_operating_mode_input_set_mode (input,
                                                                mode,
                                                                &error)) 
        {
            CELLULAR_HAL_DBG_PRINT("%s Failed to create input data bundle: '%s'\n",
                                                        __FUNCTION__,
                                                        error->message);
            g_error_free (error);
            qmi_message_dms_set_operating_mode_input_unref (input);
            return RETURN_ERROR;
        }

        //Needs to stop network scan thread when modem is going offline since no network is there 
        if( mode == QMI_DMS_OPERATING_MODE_OFFLINE )
        {
            ContextNASInfo *nasCtx = &(gpstQMIContext->nasCtx);

            //If the task still running then needs to stop
            if ( TRUE == nasCtx->bIsNetworkScanTaskStillRunning )
            {
                nasCtx->bIsNeed2StopNetworkInfoCollection = TRUE;
                
                pthread_mutex_lock(&(nasCtx->idwMutexForSCAN));
                pthread_cond_signal(&(nasCtx->idwCondForSCAN));
                pthread_mutex_unlock(&(nasCtx->idwMutexForSCAN));

                nasCtx->iTotalNoofNetworkInfo = 0;
                memset( nasCtx->astNetworkInfo, 0, sizeof( nasCtx->astNetworkInfo ) );
                nasCtx->bIsNetworkScanTaskStillRunning = FALSE;
            }
        }

        pDeviceModemConfigCtx  = (ContextModemConfiguration*)g_slice_new0 (ContextModemConfiguration);
        pDeviceModemConfigCtx->vpPrivateData = (void*)gpstQMIContext;
        pDeviceModemConfigCtx->uiRequestedOperatingConfig = modem_operating_config;

        qmi_client_dms_set_operating_mode ( QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                            input,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback)cellular_hal_qmi_set_operating_mode,
                                            pDeviceModemConfigCtx);
        qmi_message_dms_set_operating_mode_input_unref (input);
        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't configure modem state\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

int cellular_hal_qmi_get_imei( gchar *imei )
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);

        if( '\0' != dmsCtx->IMEI[0] )  
        {
            memcpy( imei, dmsCtx->IMEI, strlen(dmsCtx->IMEI) + 1 );
            CELLULAR_HAL_DBG_PRINT("%s - IMEI '%s' Retrived via DML\n", __FUNCTION__,imei);
        }
        else
        {
            ContextGetIDs *pstCtxGetIDs = NULL;
            int iLapsedSeconds = 0;

            pstCtxGetIDs = (ContextGetIDs*)g_slice_new0 (ContextGetIDs);
            pstCtxGetIDs->enQuerySource = DMS_GET_IDS_VIA_DML;
            pstCtxGetIDs->IsQueryDone = FALSE;
            pstCtxGetIDs->vpPrivateData = (void*)dmsCtx;

            cellular_hal_qmi_get_modem_identification_info( pstCtxGetIDs );

            //Wait here till IDs query 
            while ( 1 )
            {
                if( TRUE == pstCtxGetIDs->IsQueryDone )
                {
                    memcpy( imei, dmsCtx->IMEI, strlen(dmsCtx->IMEI) + 1 );
                    CELLULAR_HAL_DBG_PRINT("%s - IMEI '%s' Retrived via DML\n", __FUNCTION__,imei);
                    break;
                }
                else if( iLapsedSeconds >= CELLULAR_QMI_GETIDS_VIA_DML_MAX_WAITIME )  
                {
                    CELLULAR_HAL_DBG_PRINT("%s - Timeout during IDs Query so sending empty IMEI\n",__FUNCTION__);
                    break;
                }

                sleep(1);
                iLapsedSeconds++;
            }

            //Free Resources
            if( NULL != pstCtxGetIDs )
            {
                g_slice_free(ContextGetIDs, pstCtxGetIDs);
                pstCtxGetIDs  = NULL;
            }
        }

        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't get imei\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

int cellular_hal_qmi_get_imei_softwareversion( char *imei_sv )
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);

        if( '\0' != dmsCtx->IMEI_SV[0] )  
        {
            memcpy( imei_sv, dmsCtx->IMEI_SV, strlen(dmsCtx->IMEI_SV) + 1 );
            CELLULAR_HAL_DBG_PRINT("%s - IMEI SV '%s' Retrived via DML\n", __FUNCTION__,imei_sv);
        }
        else
        {
            ContextGetIDs *pstCtxGetIDs = NULL;
            int iLapsedSeconds = 0;

            pstCtxGetIDs = (ContextGetIDs*)g_slice_new0 (ContextGetIDs);
            pstCtxGetIDs->enQuerySource = DMS_GET_IDS_VIA_DML;
            pstCtxGetIDs->IsQueryDone = FALSE;
            pstCtxGetIDs->vpPrivateData = (void*)dmsCtx;

            cellular_hal_qmi_get_modem_identification_info( pstCtxGetIDs );

            //Wait here till IDs query
            while ( 1 )
            {
                if( TRUE == pstCtxGetIDs->IsQueryDone )
                {
                    memcpy( imei_sv, dmsCtx->IMEI_SV, strlen(dmsCtx->IMEI_SV) + 1 );
                    CELLULAR_HAL_DBG_PRINT("%s - IMEI SV '%s' Retrived via DML\n", __FUNCTION__,imei_sv);
                    break;
                }
                else if( iLapsedSeconds >= CELLULAR_QMI_GETIDS_VIA_DML_MAX_WAITIME )  
                {
                    CELLULAR_HAL_DBG_PRINT("%s - Timeout during IDs Query so sending empty IMEI SV\n",__FUNCTION__);
                    break;
                }

                sleep(1);
                iLapsedSeconds++;
            }

            //Free Resources
            if( NULL != pstCtxGetIDs )
            {
                g_slice_free(ContextGetIDs, pstCtxGetIDs);
                pstCtxGetIDs  = NULL;
            }
        }

        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't get imei sv\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

static void  cellular_hal_qmi_get_ccid (QmiClientDms *dmsClient,
                                              GAsyncResult *result,
                                              gpointer  user_data)
{
    const gchar *str = NULL;
    QmiMessageDmsUimGetIccidOutput *output;
    GError *error = NULL;
    ContextGetIDs *pstCtxGetIDs = (ContextGetIDs*)user_data;
    ContextDMSInfo *dmsCtx      = NULL;

    dmsCtx  = (ContextDMSInfo*)pstCtxGetIDs->vpPrivateData;
    memset(dmsCtx->ICCID, 0, sizeof(dmsCtx->ICCID));

    output = qmi_client_dms_uim_get_iccid_finish (dmsClient, result, &error);
    if (!output) 
    {
        CELLULAR_HAL_DBG_PRINT("%s failed to get ccid: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        pstCtxGetIDs->IsQueryDone = TRUE;
        return;
    }

    if (!qmi_message_dms_uim_get_iccid_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s failed to fetch ICCID: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_iccid_output_unref (output);
        pstCtxGetIDs->IsQueryDone = TRUE;
        return;
    }

    qmi_message_dms_uim_get_iccid_output_get_iccid (output, &str, NULL);
    snprintf(dmsCtx->ICCID, sizeof(dmsCtx->ICCID), "%s", str);
    CELLULAR_HAL_DBG_PRINT("%s Succssfully fetched ICCID '%s'\n",__FUNCTION__,dmsCtx->ICCID);

    qmi_message_dms_uim_get_iccid_output_unref (output);

    pstCtxGetIDs->IsQueryDone = TRUE;
}

int cellular_hal_qmi_get_iccid_information( char *iccid )
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);
        ContextUIMInfo  *uimCtx = &(gpstQMIContext->uimCtx);

        if( ( CELLULAR_SLOT_ID_UNKNOWN != uimCtx->selected_slot_number ) &&
            ( NULL != uimCtx->ctxSlotInfo ) &&
            ( '\0' != uimCtx->ctxSlotInfo[ uimCtx->selected_slot_number - 1 ].iccid[0] ) )

        {
            ContextSlotInfo *pstSlot = &(uimCtx->ctxSlotInfo[ uimCtx->selected_slot_number - 1 ]);

            memcpy( iccid, pstSlot->iccid, strlen(pstSlot->iccid) + 1 );
            CELLULAR_HAL_DBG_PRINT("%s - ICCID '%s' Retrived\n", __FUNCTION__,iccid);
        }
        else if( '\0' != dmsCtx->ICCID[0] )  
        {
            memcpy( iccid, dmsCtx->ICCID, strlen(dmsCtx->ICCID) + 1 );
            CELLULAR_HAL_DBG_PRINT("%s - ICCID '%s' Retrived\n", __FUNCTION__,iccid);
        }
        else
        {
            ContextGetIDs *pstCtxGetIDs = NULL;
            int iLapsedSeconds = 0;

            pstCtxGetIDs = (ContextGetIDs*)g_slice_new0 (ContextGetIDs);
            pstCtxGetIDs->enQuerySource = DMS_GET_ICCID_VIA_DML;
            pstCtxGetIDs->IsQueryDone = FALSE;
            pstCtxGetIDs->vpPrivateData = (void*)(dmsCtx);

            qmi_client_dms_uim_get_iccid ( QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                           NULL,
                                           10,
                                           NULL,
                                           (GAsyncReadyCallback)cellular_hal_qmi_get_ccid,
                                           pstCtxGetIDs);

            //Wait here till IDs query
            while ( 1 )
            {
                if( TRUE == pstCtxGetIDs->IsQueryDone )
                {
                    memcpy( iccid, dmsCtx->ICCID, strlen(dmsCtx->ICCID) + 1 );
                    CELLULAR_HAL_DBG_PRINT("%s - ICCID '%s' Retrived\n", __FUNCTION__,iccid);
                    break;
                }
                else if( iLapsedSeconds >= CELLULAR_QMI_GETIDS_VIA_DML_MAX_WAITIME )  
                {
                    CELLULAR_HAL_DBG_PRINT("%s - Timeout during IDs Query so sending empty ICCID\n",__FUNCTION__);
                    break;
                }

                sleep(1);
                iLapsedSeconds++;
            }

            //Free Resources
            if( NULL != pstCtxGetIDs )
            {
                g_slice_free(ContextGetIDs, pstCtxGetIDs);
                pstCtxGetIDs  = NULL;
            }
        }

        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't get iccid\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

static void cellular_hal_qmi_get_get_msisdn (QmiClientDms *client,
                                                   GAsyncResult *res,
                                                   gpointer  user_data)
{
    const gchar *str = NULL;
    QmiMessageDmsGetMsisdnOutput *output;
    GError *error = NULL;
    ContextGetIDs *pstCtxGetIDs = (ContextGetIDs*)user_data;
    ContextDMSInfo *dmsCtx      = NULL;

    dmsCtx  = (ContextDMSInfo*)pstCtxGetIDs->vpPrivateData;
    memset(dmsCtx->MSISDN, 0, sizeof(dmsCtx->MSISDN));

    output = qmi_client_dms_get_msisdn_finish (client, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Failed to get msisdn information: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        pstCtxGetIDs->IsQueryDone = TRUE;
        return;
    }

    if (!qmi_message_dms_get_msisdn_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Unable to get MSISDN: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_dms_get_msisdn_output_unref (output);
        pstCtxGetIDs->IsQueryDone = TRUE;
        return;
    }

    qmi_message_dms_get_msisdn_output_get_msisdn (output, &str, NULL);

    snprintf(dmsCtx->MSISDN, sizeof(dmsCtx->MSISDN), "%s", str);
    CELLULAR_HAL_DBG_PRINT("%s MSISDN retrieved: '%s'\n",__FUNCTION__,dmsCtx->MSISDN);

    qmi_message_dms_get_msisdn_output_unref (output);

    pstCtxGetIDs->IsQueryDone = TRUE;
}

int cellular_hal_qmi_get_msisdn_information( char *msisdn )
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);

        if( '\0' != dmsCtx->MSISDN[0] )  
        {
            memcpy( msisdn, dmsCtx->MSISDN, strlen(dmsCtx->MSISDN) + 1 );
            CELLULAR_HAL_DBG_PRINT("%s - MSISDN '%s' Retrived\n", __FUNCTION__,msisdn);
        }
        else
        {
            ContextGetIDs *pstCtxGetIDs = NULL;
            int iLapsedSeconds = 0;

            pstCtxGetIDs = (ContextGetIDs*)g_slice_new0 (ContextGetIDs);
            pstCtxGetIDs->enQuerySource = DMS_GET_MSISDN_VIA_DML;
            pstCtxGetIDs->IsQueryDone = FALSE;
            pstCtxGetIDs->vpPrivateData = (void*)(dmsCtx);

            qmi_client_dms_get_msisdn ( QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                        NULL,
                                        10,
                                        NULL,
                                        (GAsyncReadyCallback)cellular_hal_qmi_get_get_msisdn,
                                        pstCtxGetIDs);

            //Wait here till IDs query
            while ( 1 )
            {
                if( TRUE == pstCtxGetIDs->IsQueryDone )
                {
                    memcpy( msisdn, dmsCtx->MSISDN, strlen(dmsCtx->MSISDN) + 1 );
                    CELLULAR_HAL_DBG_PRINT("%s - MSISDN '%s' Retrived\n", __FUNCTION__,msisdn);
                    break;
                }
                else if( iLapsedSeconds >= CELLULAR_QMI_GETIDS_VIA_DML_MAX_WAITIME )  
                {
                    CELLULAR_HAL_DBG_PRINT("%s - Timeout during IDs Query so sending empty MSISDN\n",__FUNCTION__);
                    break;
                }

                sleep(1);
                iLapsedSeconds++;
            }

            //Free Resources
            if( NULL != pstCtxGetIDs )
            {
                g_slice_free(ContextGetIDs, pstCtxGetIDs);
                pstCtxGetIDs  = NULL;
            }
        }

        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't get msisdn\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}
static void
cellular_hal_qmi_get_capabilities_cb (QmiClientDms *client,
                        GAsyncResult *res,
			gpointer user_data )
{
    QmiMessageDmsGetCapabilitiesOutput *output;
    GArray *radio_interface_list;
    GError *error = NULL;
    GString *networks;
    char *interface_str;
    guint i;
    ContextDMSInfo *dmsCtx  = (ContextDMSInfo*)user_data;
    output = qmi_client_dms_get_capabilities_finish (client, res, &error);
    if (!output) {
	CELLULAR_HAL_DBG_PRINT("%s - error: operation failed %s\n", __FUNCTION__,error->message);
        g_error_free (error);
        return;
    }
    if (!qmi_message_dms_get_capabilities_output_get_result (output, &error)) {
	CELLULAR_HAL_DBG_PRINT("%s - error: couldn't get capabilities %s\n", __FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_dms_get_capabilities_output_unref (output);
        return;
    }
    qmi_message_dms_get_capabilities_output_get_info (output, NULL, NULL, NULL, NULL,
                                                      &radio_interface_list, NULL);

    networks = g_string_new ("");
    memset(dmsCtx->SupportedRAT, 0, sizeof(dmsCtx->SupportedRAT));
    for (i = 0; i < radio_interface_list->len; i++) {
        g_string_append (networks, 
			qmi_dms_radio_interface_get_string (g_array_index (radio_interface_list, QmiDmsRadioInterface, i)));
        switch (g_array_index (radio_interface_list,QmiDmsRadioInterface,i))
        {
          case QMI_DMS_RADIO_INTERFACE_CDMA20001X:
             interface_str = "CDMA20001X";
             break;
          case QMI_DMS_RADIO_INTERFACE_EVDO:
             interface_str = "EVDO";
             break;
          case QMI_DMS_RADIO_INTERFACE_GSM:
             interface_str = "GSM";
             break;
          case QMI_DMS_RADIO_INTERFACE_UMTS:
             interface_str = "UMTS";
             break;
          case QMI_DMS_RADIO_INTERFACE_LTE:
             interface_str = "LTE";
             break;
          case QMI_DMS_RADIO_INTERFACE_5GNR:
             interface_str = "5GNR";
             break;
	  default:
             interface_str = "UNKNOWN";
        }
        strncat(dmsCtx->SupportedRAT,interface_str,strlen(interface_str));
        if (i != radio_interface_list->len - 1) {
            g_string_append (networks, ", ");
            strncat(dmsCtx->SupportedRAT,", ",sizeof(", "));
	}
    }
    
    CELLULAR_HAL_DBG_PRINT("%s - On ready capabilities value is  %s  \n", __FUNCTION__,dmsCtx->SupportedRAT);
    g_string_free (networks, TRUE);
    qmi_message_dms_get_capabilities_output_unref (output);
}

int cellular_hal_qmi_get_supported_radio_technology( char *supported_rat)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {

        ContextGetIDs *pstCtxGetIDs = NULL;
        int iLapsedSeconds = 0;
        ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);
        if('\0' != dmsCtx->SupportedRAT[0] )
        {
           memcpy( supported_rat, dmsCtx->SupportedRAT, strlen(dmsCtx->SupportedRAT) + 1 );
           CELLULAR_HAL_DBG_PRINT("%s - SupportedRAT '%s' Retrived\n", __FUNCTION__, supported_rat);
           return RETURN_OK;
        }
	else
        {
            // supported RAT value not available in locally .. need to query QMI
	    qmi_client_dms_get_capabilities (QMI_CLIENT_DMS(dmsCtx->dmsClient),
                                         NULL,
                                         10,
                                         NULL,
                                         (GAsyncReadyCallback)cellular_hal_qmi_get_capabilities_cb,
                                         dmsCtx);
         }

         //Wait here till IDs query
         while ( 1 )
         {
	     if('\0' != dmsCtx->SupportedRAT[0] )
             {
                 memcpy( supported_rat, dmsCtx->SupportedRAT, strlen(dmsCtx->SupportedRAT) + 1 );
                 CELLULAR_HAL_DBG_PRINT("%s - Operation complete and value is %s  \n", __FUNCTION__,supported_rat);
                 break;
             }
             else if( iLapsedSeconds >= CELLULAR_QMI_GET_CAPS_MAX_WAITIME )
             {
                 CELLULAR_HAL_DBG_PRINT("%s - Timeout during IDs Query so sending empty supportedRAT\n",__FUNCTION__);
                 break;
             }
             sleep(1);
             iLapsedSeconds++;
         }
         return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI not ready so can't get %s \n", __FUNCTION__, supported_rat);
        return RETURN_ERROR;
    }
}

/* UIM */
static gchar* cellular_hal_qmi_decode_iccid (const gchar *bcd, gsize bcd_len)
{
    GString *str;
    gsize i;

    if (!bcd)
        return NULL;

    str = g_string_sized_new (bcd_len * 2 + 1);
    for (i = 0; i < bcd_len; i++) {
        str = g_string_append_c (str, bcd_chars[bcd[i] & 0xF]);
        str = g_string_append_c (str, bcd_chars[(bcd[i] >> 4) & 0xF]);
    }
    return g_string_free (str, FALSE);
}

static gchar* cellular_hal_qmi_decode_eid (const gchar *eid, gsize eid_len)
{
    GString *str;
    gsize i;

    if (!eid)
        return NULL;
    if (eid_len != CELLULAR_QMI_EID_LENGTH)
        return NULL;

    str = g_string_sized_new (eid_len * 2 + 1);
    for (i = 0; i < eid_len; i++) {
        str = g_string_append_c (str, bcd_chars[(eid[i] >> 4) & 0xF]);
        str = g_string_append_c (str, bcd_chars[eid[i] & 0xF]);
    }
    return g_string_free (str, FALSE);
}

static void cellular_hal_qmi_select_slot_and_notify (GArray *physical_slots, GArray *ext_information, GArray *slot_eids, gpointer user_data)
{
    ContextUIMInfo      *uimCtx = &(gpstQMIContext->uimCtx);
    guint               i;
    gboolean            bIsCardStatusActive = FALSE;
    cellular_device_slot_status_api_callback device_slot_status_cb = (cellular_device_slot_status_api_callback)user_data;

    if (ext_information && physical_slots->len != ext_information->len) {
        CELLULAR_HAL_DBG_PRINT ("Malformed extended information data");
        ext_information = NULL;
    }

    if (slot_eids && physical_slots->len != slot_eids->len) {
        CELLULAR_HAL_DBG_PRINT ("Malformed slot EID data");
        slot_eids = NULL;
    }

    uimCtx->total_no_of_slots = physical_slots->len;

    //Allocate memory for first time
    if( NULL == uimCtx->ctxSlotInfo )
    {
        uimCtx->ctxSlotInfo = ( ContextSlotInfo* ) malloc( sizeof(ContextSlotInfo) * uimCtx->total_no_of_slots );
        memset( uimCtx->ctxSlotInfo, 0, sizeof(ContextSlotInfo) * uimCtx->total_no_of_slots );
    }

    //Display Slot Status
    for (i = 0; i < physical_slots->len; i++) 
    {
        QmiPhysicalSlotStatusSlot *slot_status;
        QmiPhysicalSlotInformationSlot *slot_info = NULL;
        GArray *slot_eid = NULL;
        g_autofree gchar *iccid = NULL;
        g_autofree gchar *eid = NULL;
        ContextSlotInfo *pstSlotInfo = &(uimCtx->ctxSlotInfo[i]);

        slot_status = &g_array_index (physical_slots, QmiPhysicalSlotStatusSlot, i);

        CELLULAR_HAL_DBG_PRINT ("  Physical slot %u:\n", i + 1);
        CELLULAR_HAL_DBG_PRINT ("     Card status: %s\n",
                 qmi_uim_physical_card_state_get_string (slot_status->physical_card_status));
        CELLULAR_HAL_DBG_PRINT ("     Slot status: %s\n",
                 qmi_uim_slot_state_get_string (slot_status->physical_slot_status));

        pstSlotInfo->bSlotState = FALSE;
        if (slot_status->physical_slot_status == QMI_UIM_SLOT_STATE_ACTIVE)
        {
            CELLULAR_HAL_DBG_PRINT ("    Logical slot: %u\n", slot_status->logical_slot);
            pstSlotInfo->bSlotState = TRUE;
        }
            
        if (slot_status->physical_card_status != QMI_UIM_PHYSICAL_CARD_STATE_PRESENT)
        {
            pstSlotInfo->bCardPresentState = FALSE;
            pstSlotInfo->bCardEnable = FALSE;
            continue;
        }

        pstSlotInfo->bCardPresentState = TRUE;
        pstSlotInfo->bCardEnable = TRUE;

        memset( pstSlotInfo->iccid, 0, sizeof(pstSlotInfo->iccid) );

        if (slot_status->iccid->len)
        {
            iccid = cellular_hal_qmi_decode_iccid (slot_status->iccid->data, slot_status->iccid->len);
            CELLULAR_HAL_DBG_PRINT ("           ICCID: %s\n", VALIDATE_UNKNOWN (iccid));

            memcpy( pstSlotInfo->iccid, iccid, strlen(iccid) + 1 );
        }

        /* Extended information, if available */
        if (!ext_information)
            continue;

        slot_info = &g_array_index (ext_information, QmiPhysicalSlotInformationSlot, i);
        CELLULAR_HAL_DBG_PRINT ("        Protocol: %s\n",
                 qmi_uim_card_protocol_get_string (slot_info->card_protocol));
        CELLULAR_HAL_DBG_PRINT ("        Num apps: %u\n", slot_info->valid_applications);
        CELLULAR_HAL_DBG_PRINT ("        Is eUICC: %s\n", slot_info->is_euicc ? "yes" : "no");

        pstSlotInfo->beUICCAvailable = FALSE;
        if( slot_info->is_euicc )
        {
            pstSlotInfo->beUICCAvailable = TRUE;
        }

        /* EID info, if available and this is an eUICC */
        if (!slot_info->is_euicc || !slot_eids)
            continue;

        memset( pstSlotInfo->eid, 0, sizeof(pstSlotInfo->eid) );

        slot_eid = g_array_index (slot_eids, GArray *, i);
        if (slot_eid->len)
        {
            eid = cellular_hal_qmi_decode_eid (slot_eid->data, slot_eid->len);
            CELLULAR_HAL_DBG_PRINT ("             EID: %s\n", VALIDATE_UNKNOWN (eid));

            memcpy( pstSlotInfo->eid, eid, strlen(eid) + 1 );
        }
    }

    /*
    * Case 1:
    * Check any slot choosed or not. 
    * If no slot choosed then need to check active and valid slot notification to upper layer.
    * 
    * Case 2:
    * If already slot selected and Check any changes occured in currently selected slot. 
    * If yes then check alternate valid slot presents or not. If yes then switch to that slot and send notify.
    * If no then send slot not active notification to uppper layer.
    * 
    */
    for (i = 0; i < physical_slots->len; i++) 
    {
        QmiPhysicalSlotStatusSlot *slot_status;

        slot_status = &g_array_index (physical_slots, QmiPhysicalSlotStatusSlot, i);

        if( CELLULAR_SLOT_ID_UNKNOWN == uimCtx->selected_slot_number )
        {
            //Which means no slot selected earlier so choose valid slot
            if( ( slot_status->physical_slot_status == QMI_UIM_SLOT_STATE_ACTIVE ) && \
                ( slot_status->physical_card_status == QMI_UIM_PHYSICAL_CARD_STATE_PRESENT ) && \
                ( slot_status->iccid->len ) )
            {

                //Use this slot
                CELLULAR_HAL_DBG_PRINT ("%s - Slot %d is active and card present, iccid also valid\n",__FUNCTION__,i + 1);
                uimCtx->selected_slot_number = i + 1;

                //Send slot ready status if CB is not null
                if( NULL != device_slot_status_cb )
                {
                    CELLULAR_HAL_DBG_PRINT("%s - UIM slot choosed. Sending notification to consumer\n", __FUNCTION__);
                    device_slot_status_cb( "slot1", "logical_slot1", uimCtx->selected_slot_number, DEVICE_SLOT_STATUS_READY );
                    break;
                }
            }
        }
        else
        {
            //Which means already slot selected so checking any changes
            if( uimCtx->selected_slot_number == (i + 1) )
            {
                if( ( slot_status->physical_slot_status != QMI_UIM_SLOT_STATE_ACTIVE ) || \
                    ( slot_status->physical_card_status != QMI_UIM_PHYSICAL_CARD_STATE_PRESENT ) || \
                    ( slot_status->iccid->len == 0 ) )
                {
                    uimCtx->selected_slot_number = CELLULAR_SLOT_ID_UNKNOWN;

                    //Send slot not ready status if CB is not null
                    if( NULL != device_slot_status_cb )
                    {
                        CELLULAR_HAL_DBG_PRINT("%s - UIM slot not choosed. Sending notification to consumer\n", __FUNCTION__);
                        device_slot_status_cb( "invalid_phy_slot", "invalid_logical_slot", -1, DEVICE_SLOT_STATUS_NOT_READY );
                        break;
                    }
                }
            }
        }
    }
}

static void cellular_hal_qmi_slot_status_CB (QmiClientUim *uimClient, QmiIndicationUimSlotStatusOutput *output, gpointer user_data)
{
    GArray *physical_slots;
    GArray *ext_information = NULL;
    GArray *slot_eids = NULL;
    GError *error = NULL;
    ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);

    CELLULAR_HAL_DBG_PRINT ("%s - [%s] Received slot status indication:\n", __FUNCTION__, qmi_device_get_path_display (gpstQMIContext->qmiDevice));

    //Reinitialize ICCID and MSISDN buffer since we are not sure which slot may select in this case
    memset(dmsCtx->ICCID, 0, sizeof(dmsCtx->ICCID));
    memset(dmsCtx->MSISDN, 0, sizeof(dmsCtx->MSISDN));

    if (!qmi_indication_uim_slot_status_output_get_physical_slot_status ( output, &physical_slots, &error)) {
        CELLULAR_HAL_DBG_PRINT ("Unable to parse slots status: %s\n", error->message);
        g_error_free (error);
        return;
    }

    /* Both of these are recoverable, just print less information per slot */
    qmi_indication_uim_slot_status_output_get_physical_slot_information (output, &ext_information, NULL);
    qmi_indication_uim_slot_status_output_get_slot_eid_information (output, &slot_eids, NULL);

    cellular_hal_qmi_select_slot_and_notify (physical_slots, ext_information, slot_eids, user_data);
}

static void cellular_hal_qmi_register_physical_slot_status_events (QmiClientUim *uimClient, GAsyncResult *res, gpointer  user_data)
{
    QmiMessageUimRegisterEventsOutput *output;
    GError                            *error  = NULL;
    ContextUIMInfo                    *uimCtx = &(gpstQMIContext->uimCtx);

    CELLULAR_HAL_DBG_PRINT ("%s %d Entry\n", __FUNCTION__, __LINE__);

    output = qmi_client_uim_register_events_finish (uimClient, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT ("failed to register events: %s\n", error->message);
        g_error_free (error);
        return;
    }

    if (!qmi_message_uim_register_events_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT ("failed to register slot status change events: %s\n", error->message);
        qmi_message_uim_register_events_output_unref (output);
        g_error_free (error);
        return;
    }

    CELLULAR_HAL_DBG_PRINT ("%s Registered physical slot status change events...\n",__FUNCTION__);
    uimCtx->slot_status_indication_id =  g_signal_connect (uimClient,
                                                            "slot-status",
                                                            G_CALLBACK (cellular_hal_qmi_slot_status_CB),
                                                            user_data);

    CELLULAR_HAL_DBG_PRINT ("%s %d Exit slot indication id:%u\n", __FUNCTION__, __LINE__,uimCtx->slot_status_indication_id);
}

static void cellular_hal_qmi_get_slot_status (QmiClientUim *uimClient, GAsyncResult *result, gpointer  user_data)
{
    QmiMessageUimGetSlotStatusOutput *output;
    GArray *physical_slots;
    GArray *ext_information = NULL;
    GArray *slot_eids = NULL;
    GError *error = NULL;

    output = qmi_client_uim_get_slot_status_finish (uimClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT ("failed to get slot status: %s\n", error->message);
        g_error_free (error);
        return;
    }

    if (!qmi_message_uim_get_slot_status_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT ("failed to get slot status: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_slot_status_output_unref (output);
        return;
    }

    CELLULAR_HAL_DBG_PRINT ("[%s] Successfully got slots status\n", qmi_device_get_path_display (gpstQMIContext->qmiDevice));

    if (!qmi_message_uim_get_slot_status_output_get_physical_slot_status (
            output, &physical_slots, &error)) {
        CELLULAR_HAL_DBG_PRINT ("Unable to parse slots status response: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_slot_status_output_unref (output);
        return;
    }

    /* Both of these are recoverable, just print less information per slot */
    qmi_message_uim_get_slot_status_output_get_physical_slot_information (output, &ext_information, NULL);
    qmi_message_uim_get_slot_status_output_get_slot_eid_information (output, &slot_eids, NULL);

    CELLULAR_HAL_DBG_PRINT ("[%s] %u physical slots found:\n", qmi_device_get_path_display (gpstQMIContext->qmiDevice), physical_slots->len);

    cellular_hal_qmi_select_slot_and_notify (physical_slots, ext_information, slot_eids, user_data);

    qmi_message_uim_get_slot_status_output_unref (output);
}

static void cellular_hal_qmi_client_uim ( QmiDevice *device, GAsyncResult *res, gpointer  user_data ) 
{
    cellular_device_slot_status_api_callback device_slot_status_cb = (cellular_device_slot_status_api_callback)user_data;
    QmiMessageUimRegisterEventsInput *re_input;
    GError                           *error = NULL;
    QmiClient                        *client;
    ContextUIMInfo                   *uimCtx = &(gpstQMIContext->uimCtx);

    client = qmi_device_allocate_client_finish (device, res, &error);
    if (!client) {
        CELLULAR_HAL_DBG_PRINT("%s %d failed to create client for the '%s' service: %s\n",
                                __FUNCTION__,
                                __LINE__,
                                qmi_service_get_string (QMI_SERVICE_UIM),
                                error->message);
        goto NOTIFY;
    }

    //Store UIM context information
    uimCtx->uimClient = client;
    uimCtx->bIsValiduimClient = TRUE;

    //Check and Get Current slot status
    qmi_client_uim_get_slot_status( QMI_CLIENT_UIM (client),
                                    NULL,
                                    10,
                                    NULL,
                                    (GAsyncReadyCallback)cellular_hal_qmi_get_slot_status,
                                    user_data);

    //Register Slot Status Events
    CELLULAR_HAL_DBG_PRINT("%s UIM Client is ready so register slot status events\n", __FUNCTION__);

    re_input = qmi_message_uim_register_events_input_new ();
    qmi_message_uim_register_events_input_set_event_registration_mask ( re_input, 
                                                                        QMI_UIM_EVENT_REGISTRATION_FLAG_PHYSICAL_SLOT_STATUS, 
                                                                        NULL);

    qmi_client_uim_register_events (    QMI_CLIENT_UIM (client),
                                        re_input,
                                        10,
                                        NULL,
                                        (GAsyncReadyCallback) cellular_hal_qmi_register_physical_slot_status_events,
                                        user_data);

    qmi_message_uim_register_events_input_unref (re_input);
    return;

NOTIFY:
    //Send QMI not ready status if CB is not null
    if( NULL != device_slot_status_cb )
    {
        CELLULAR_HAL_DBG_PRINT("%s - UIM is not ready so slot can't be choose now and sending status via CB\n", __FUNCTION__);
        device_slot_status_cb( "unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY );
    }
}

/* cellular_hal_qmi_select_device_slot() */
int cellular_hal_qmi_select_device_slot(cellular_device_slot_status_api_callback device_slot_status_cb)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextUIMInfo   *uimCtx = &(gpstQMIContext->uimCtx);

        //Allocate uim client if uim not allocated case
        if( FALSE == uimCtx->bIsValiduimClient )
        {
            CELLULAR_HAL_DBG_PRINT("%s %d - QMI device already open. so choosing slots via UIM service\n",__FUNCTION__,__LINE__);

            qmi_device_allocate_client (gpstQMIContext->qmiDevice,
                                        QMI_SERVICE_UIM,
                                        QMI_CID_NONE,
                                        10,
                                        NULL,
                                        (GAsyncReadyCallback) cellular_hal_qmi_client_uim,
                                        device_slot_status_cb);
        }
        else
        {
            //Check whether slot already choosed or not
            if( CELLULAR_SLOT_ID_UNKNOWN == uimCtx->selected_slot_number )
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - QMI and UIM device already open. so choosing slots via UIM service\n",__FUNCTION__,__LINE__);
            }
            else
            {   
                //Send slot ready status if CB is not null
                if( NULL != device_slot_status_cb )
                {
                    CELLULAR_HAL_DBG_PRINT("%s - UIM slot already choosed. Sending notification to consumer\n", __FUNCTION__);
                    device_slot_status_cb( "slot1", "logical_slot1", uimCtx->selected_slot_number, DEVICE_SLOT_STATUS_READY );
                }
            }
        }
    }
    else
    {
        //TBD - Needs to register connection indication CB - QMI_DEVICE_SIGNAL_INDICATION

        //Send QMI not ready status if CB is not null
        if( NULL != device_slot_status_cb )
        {
            CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so slot can't be chosen now and sending status via CB\n", __FUNCTION__, gpstQMIContext->modem_device_name);
            device_slot_status_cb( "unknown", "unknown", -1, DEVICE_SLOT_STATUS_NOT_READY );
        }
    }

    return RETURN_OK;
}

static void  cellular_hal_qmi_power_off_sim (QmiClientUim *client,
                                                   GAsyncResult *res,
                                                   gpointer  user_data)
{
    QmiMessageUimPowerOffSimOutput *output;
    GError *error = NULL;
    unsigned int slot_no = (unsigned int)user_data;

    output = qmi_client_uim_power_off_sim_finish (client, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Failed to perform SIM powet off operation: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        return;
    }

    if (!qmi_message_uim_power_off_sim_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Unable to power off SIM: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_uim_power_off_sim_output_unref (output);
        return;
    }

    CELLULAR_HAL_DBG_PRINT("[%s] Successfully performed SIM power off for '%d' slot\n",__FUNCTION__,slot_no);

    qmi_message_uim_power_off_sim_output_unref (output);
}

static void  cellular_hal_qmi_power_on_sim (QmiClientUim *client,
                                            GAsyncResult *res,
                                            gpointer  user_data)
{
    QmiMessageUimPowerOnSimOutput *output;
    GError *error = NULL;
    unsigned int slot_no = (unsigned int)user_data;

    output = qmi_client_uim_power_on_sim_finish (client, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Failed to perform SIM on operation: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        return;
    }

    if (!qmi_message_uim_power_on_sim_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Unable to power on SIM: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_uim_power_on_sim_output_unref (output);
        return;
    }

    CELLULAR_HAL_DBG_PRINT("%s Successfully performed SIM power on for '%d' slot\n",__FUNCTION__,slot_no);

    qmi_message_uim_power_on_sim_output_unref (output);
}

int cellular_hal_qmi_sim_power_enable(unsigned int slot_id, unsigned char enable)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextUIMInfo   *uimCtx = &(gpstQMIContext->uimCtx);

        //Check if uim client is valid or not
        if( TRUE == uimCtx->bIsValiduimClient )
        {
            CELLULAR_HAL_DBG_PRINT("%s Requesting SIM power %s for '%d' slot\n",__FUNCTION__,(enable) ? "on" : "off",(slot_id + 1));

            if( enable )
            {
                QmiMessageUimPowerOnSimInput *input;
                GError                       *error = NULL;

                input = qmi_message_uim_power_on_sim_input_new ();

                if (!qmi_message_uim_power_on_sim_input_set_slot (input, (slot_id + 1), &error)) 
                {
                    g_printerr ("%s Unable to create SIM power on input: %s\n",__FUNCTION__, error->message);
                    g_error_free (error);
                    qmi_message_uim_power_on_sim_input_unref (input);
                    return RETURN_ERROR;
                }

                qmi_client_uim_power_on_sim (QMI_CLIENT_UIM (uimCtx->uimClient),
                                             input,
                                             10,
                                             NULL,
                                             (GAsyncReadyCallback)cellular_hal_qmi_power_on_sim,
                                             (void*)slot_id);

                qmi_message_uim_power_on_sim_input_unref (input);
            }
            else
            {
                QmiMessageUimPowerOffSimInput *input;
                GError                       *error = NULL;

                input = qmi_message_uim_power_off_sim_input_new ();

                if (!qmi_message_uim_power_off_sim_input_set_slot (input, (slot_id + 1), &error)) 
                {
                    g_printerr ("%s Unable to create SIM power off input: %s\n",__FUNCTION__, error->message);
                    g_error_free (error);
                    qmi_message_uim_power_off_sim_input_unref (input);
                    return RETURN_ERROR;
                }

                qmi_client_uim_power_off_sim (QMI_CLIENT_UIM (uimCtx->uimClient),
                                              input,
                                              10,
                                              NULL,
                                              (GAsyncReadyCallback)cellular_hal_qmi_power_off_sim,
                                              (void*)slot_id);

                qmi_message_uim_power_off_sim_input_unref (input);
            }

            //Update SIM Card Status
            {
                ContextSlotInfo   *pSlotInfo = &(uimCtx->ctxSlotInfo[slot_id]);
                pSlotInfo->bCardEnable = enable;
            }

            return RETURN_OK;
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s - UIM client is not ready so slot can't be enable/disable power now\n", __FUNCTION__);
            return RETURN_ERROR;
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so slot can't be enable/disable power now\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

int cellular_hal_qmi_get_total_no_of_uicc_slots(unsigned int *total_count)
{
    *total_count = 0;

    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextUIMInfo   *uimCtx = &(gpstQMIContext->uimCtx);

        *total_count = uimCtx->total_no_of_slots;

        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't be query UICC slot count\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

static int cellular_hal_qmi_get_mno_name(char *mnoname)
{
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNASInfo *nasCtx = &(gpstQMIContext->nasCtx);

        if( NULL != nasCtx->nasClient  && nasCtx->enNASRegistrationStatus == DEVICE_NAS_STATUS_REGISTERED)
        {
            if( '\0' != nasCtx->operator_name[0] )
            {
              strncpy( mnoname, nasCtx->operator_name, CELLULAR_QMI_OPERATOR_LENGTH);  
              CELLULAR_HAL_DBG_PRINT("%s - Operator/Mno Name '%s' Retrived\n", __FUNCTION__,mnoname);
            }
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s - NAS client is not ready so current operator name is empty \n", __FUNCTION__);
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI Device is not ready  ..can not populate any info now \n", __FUNCTION__);
        return RETURN_ERROR;
    }
    return RETURN_OK;
}

int cellular_hal_qmi_get_uicc_slot_info(unsigned int slot_id, CellularUICCSlotInfoStruct *pstSlotInfo)
{    
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextUIMInfo   *uimCtx = &(gpstQMIContext->uimCtx);

        if( uimCtx->total_no_of_slots > 0 )
        {
            int i = 0;

            memset( pstSlotInfo, 0, sizeof( CellularUICCSlotInfoStruct ) );

            for( i = 0; i < uimCtx->total_no_of_slots; i++ )
            {
                //Get the slot info for requested slot id
                if( i == slot_id )
                {
                    ContextSlotInfo *pstSlot = &(uimCtx->ctxSlotInfo[i]);

                    pstSlotInfo->SlotEnable    = pstSlot->bSlotState;
                    pstSlotInfo->IsCardPresent = pstSlot->bCardPresentState;
                    pstSlotInfo->CardEnable    = pstSlot->bCardEnable;

                    if( TRUE == pstSlotInfo->CardEnable )
                    {
                        pstSlotInfo->Status = CELLULAR_UICC_STATUS_VALID;
                    }
                    else
                    {
                        pstSlotInfo->Status = CELLULAR_UICC_STATUS_ERROR;
                    }
                
                    if (pstSlotInfo->CardEnable)   // card is present and powered on
                    {
                      if( '\0' != pstSlot->iccid[0] )
                      {
                        memcpy(pstSlotInfo->iccid, pstSlot->iccid, strlen(pstSlot->iccid) + 1 );
                      }
                      
                      if( '\0' != pstSlot->msisdn[0] )
                      {
                        memcpy(pstSlotInfo->msisdn, pstSlot->msisdn, strlen(pstSlot->msisdn) + 1 );
                      }
                      else
                      {
                        char msisdn[20] = {0};
                        
                        memset(msisdn, 0, sizeof(msisdn));
                        cellular_hal_qmi_get_msisdn_information(msisdn);

                        if( ( '\0' != msisdn[0] ) &&
                            ( strlen(msisdn) > 0 ) )
                        {
                            memcpy(pstSlot->msisdn, msisdn, strlen(msisdn) + 1 );
                            memcpy(pstSlotInfo->msisdn, pstSlot->msisdn, strlen(pstSlot->msisdn) + 1 );
                        }
                      }
                      //get operator name from NAS context only when slot is active
                      if(pstSlotInfo->SlotEnable)
                        cellular_hal_qmi_get_mno_name(pstSlotInfo->MnoName);
                    }
                    return RETURN_OK;
                }
            }

            CELLULAR_HAL_DBG_PRINT("%s - Invalid slot '%d' so can't be query UICC slot info\n", __FUNCTION__, slot_id);
            return RETURN_ERROR;
        }

    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't be query UICC slot info\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

int cellular_hal_qmi_get_active_card_status(CellularUICCStatus_t *card_status)
{   
     *card_status = CELLULAR_UICC_STATUS_EMPTY;

    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextUIMInfo    *uimCtx    = &(gpstQMIContext->uimCtx);

        if( CELLULAR_SLOT_ID_UNKNOWN != uimCtx->selected_slot_number )
        {
            ContextSlotInfo   *pSlotInfo = &(uimCtx->ctxSlotInfo[uimCtx->selected_slot_number-1]);
            
            if( TRUE == pSlotInfo->bCardEnable )
            {
                *card_status = CELLULAR_UICC_STATUS_VALID;
            }
            else
            {
                *card_status = CELLULAR_UICC_STATUS_EMPTY;
            }
        }

        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't be query UICC slot info\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

/* NAS */
static void  cellular_hal_qmi_serving_system_indication_cb ( QmiClientNas *nasClient,
                                                             QmiIndicationNasServingSystemOutput *output,
                                                             gpointer  user_data)
{
    //CELLULAR_HAL_DBG_PRINT("%s - Received serving system indications \n",__FUNCTION__);
    GError *error = NULL;
    cellular_device_registration_status_callback device_registration_status_cb = (cellular_device_registration_status_callback) user_data;
    QmiNasRegistrationState registration_state;
    QmiNasAttachState cs_attach_state;
    QmiNasAttachState ps_attach_state;
    QmiNasNetworkType selected_network;
    QmiNasRoamingIndicatorStatus roaming_state;
    GArray *radio_interfaces;
    CellularDeviceNASStatus_t           registration_status;
    CellularDeviceNASRoamingStatus_t    roaming_status;
    int send_notify = 0;
    CellularModemRegisteredServiceType_t registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;

    //Initialize with previous values ...update with latest info in any change
    //Previous values will be considered when there is no change in value or On error cases
    registration_status = (registration_state_previous == QMI_NAS_REGISTRATION_STATE_REGISTERED) ?
                                                     DEVICE_NAS_STATUS_REGISTERED : DEVICE_NAS_STATUS_NOT_REGISTERED;
    roaming_status = (roaming_state_previous  == QMI_NAS_ROAMING_INDICATOR_STATUS_ON) ?
                                           DEVICE_NAS_STATUS_ROAMING_ON : DEVICE_NAS_STATUS_ROAMING_OFF;

    if (!qmi_indication_nas_serving_system_output_get_serving_system (output,
                                &registration_state,
                                &cs_attach_state,
                                &ps_attach_state,
                                &selected_network,
                                &radio_interfaces,
                                &error))
    {
        CELLULAR_HAL_DBG_PRINT("%s - Failed to get serving system info:%s \n",__FUNCTION__,error->message);
        g_error_free (error);
    }
    else
    {
        if (registration_state_previous != registration_state ) 
        {
            registration_state_previous = registration_state;
            registration_status = (registration_state == QMI_NAS_REGISTRATION_STATE_REGISTERED) ?
                                                     DEVICE_NAS_STATUS_REGISTERED : DEVICE_NAS_STATUS_NOT_REGISTERED;
            CELLULAR_HAL_DBG_PRINT("Registration Status: '%s' CS Attach State: '%s' PS Attach State: '%s' Network: '%s' Radios: '%u'\n",
                                    qmi_nas_registration_state_get_string (registration_state),
                                    qmi_nas_attach_state_get_string (cs_attach_state),
                                    qmi_nas_attach_state_get_string (ps_attach_state),
                                    qmi_nas_network_type_get_string (selected_network),
                                    radio_interfaces->len);
            send_notify = 1 ;

            if( ( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) &&
                ( cs_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) )
            {
                registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_CS_PS;
            }
            else if( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED )
            {
                registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_PS;
            }
            else if( cs_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED )
            {
                registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_CS;
            }
            else
            {
                registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;
            }
        }
    }
    if (!qmi_indication_nas_serving_system_output_get_roaming_indicator(output, &roaming_state, &error))
    {
        CELLULAR_HAL_DBG_PRINT("%s - Failed to get Roaming status: %s \n",__FUNCTION__,error->message);
        g_error_free (error);
    }
    else
    {
        if (roaming_state_previous != roaming_state)
        {
           roaming_state_previous = roaming_state;
           roaming_status = (roaming_state == QMI_NAS_ROAMING_INDICATOR_STATUS_ON) ?
                                           DEVICE_NAS_STATUS_ROAMING_ON : DEVICE_NAS_STATUS_ROAMING_OFF;
           CELLULAR_HAL_DBG_PRINT(" %s, Roaming state: '%s'\n",__FUNCTION__, qmi_nas_roaming_indicator_status_get_string (roaming_state));
           send_notify = 1 ;
        }
    }

    if( send_notify && device_registration_status_cb) {
        device_registration_status_cb(registration_status, roaming_status, registered_service);
    }
    return;
}

static void cellular_hal_qmi_monitor_get_serving_system (QmiClientNas *nasClient, GAsyncResult *result, gpointer  user_data)
{
    QmiMessageNasGetServingSystemOutput *output;
    GError *error = NULL;
    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate   *pstQMIContext  = NULL;
    ContextModemRegistration  *pDeviceRegCtx = NULL;
    ContextNASInfo   *nasCtx = NULL;
    const gchar *description = NULL;

    pDeviceRegCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceRegCtx->vpPrivateData;
    nasCtx = &(pstQMIContext->nasCtx);

    output = qmi_client_nas_get_serving_system_finish (nasClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s failed to get serving system: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        pDeviceRegCtx->enNASRegistrationStatus = DEVICE_NAS_STATUS_NOT_REGISTERED;
        pDeviceRegCtx->enNASRoamingStatus = DEVICE_NAS_STATUS_ROAMING_OFF;
        goto LASTSTEP;
    }

    if (!qmi_message_nas_get_serving_system_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s failed to get serving system: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_nas_get_serving_system_output_unref (output);
        pDeviceRegCtx->enNASRegistrationStatus = DEVICE_NAS_STATUS_NOT_REGISTERED;
        pDeviceRegCtx->enNASRoamingStatus = DEVICE_NAS_STATUS_ROAMING_OFF;
        goto LASTSTEP;
    }

    {
        QmiNasRegistrationState registration_state;
        QmiNasAttachState cs_attach_state;
        QmiNasAttachState ps_attach_state;
        QmiNasNetworkType selected_network;
        GArray *radio_interfaces;
        guint i;
        guint8 bIsLTENetworkDetected = FALSE;
        retry_get_serving_system++;

        qmi_message_nas_get_serving_system_output_get_serving_system ( output,
                                                                       &registration_state,
                                                                       &cs_attach_state,
                                                                       &ps_attach_state,
                                                                       &selected_network,
                                                                       &radio_interfaces,
                                                                       NULL);

        CELLULAR_HAL_DBG_PRINT("Registration Status: '%s' CS Attach State: '%s' PS Attach State: '%s' Network: '%s' Radios: '%u'\n",
                                qmi_nas_registration_state_get_string (registration_state),
                                qmi_nas_attach_state_get_string (cs_attach_state),
                                qmi_nas_attach_state_get_string (ps_attach_state),
                                qmi_nas_network_type_get_string (selected_network),
                                radio_interfaces->len);

	    QmiNasRoamingIndicatorStatus roaming;

        if (!qmi_message_nas_get_serving_system_output_get_roaming_indicator(output, &roaming, NULL))
        {
            CELLULAR_HAL_DBG_PRINT("%s - Failed to get Roaming status \n",__FUNCTION__);
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("\tRoaming status: '%s'\n", qmi_nas_roaming_indicator_status_get_string (roaming));
            roaming_state_previous = roaming;

            if(roaming == QMI_NAS_ROAMING_INDICATOR_STATUS_ON)
                pDeviceRegCtx->enNASRoamingStatus = DEVICE_NAS_STATUS_ROAMING_ON;
            else
                pDeviceRegCtx->enNASRoamingStatus = DEVICE_NAS_STATUS_ROAMING_OFF;
        }

        for (i = 0; i < radio_interfaces->len; i++) 
        {
            QmiNasRadioInterface iface;

            iface = g_array_index (radio_interfaces, QmiNasRadioInterface, i);
            CELLULAR_HAL_DBG_PRINT("\t\t[%u]: '%s'\n", i, qmi_nas_radio_interface_get_string (iface));

            if( 0 == strcmp( "lte", qmi_nas_radio_interface_get_string (iface) ) )
            {
                bIsLTENetworkDetected = TRUE;
            }
        }
		
        memset(nasCtx->operator_name,0, sizeof(nasCtx->operator_name)); 
        qmi_message_nas_get_serving_system_output_get_current_plmn (output, NULL, NULL, &description, NULL);
		
        if( description != NULL && description[0] != '\0')
        {
            strncpy(nasCtx->operator_name, description, sizeof(nasCtx->operator_name));
            CELLULAR_HAL_DBG_PRINT("%s Operator/Mno Name retrieved: '%s'\n",__FUNCTION__,nasCtx->operator_name);
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s ERORR!! Couldn't get Operator/Mno name\n",__FUNCTION__);
        }

        nasCtx->enNASRegistrationStatus = DEVICE_NAS_STATUS_NOT_REGISTERED;

        //All good so notify otherwise retry after 5seconds and cs_attach_state is only for 2G. When LTE PS should be attached
        if( ( registration_state == QMI_NAS_REGISTRATION_STATE_REGISTERED  ) &&
            ( ( ( TRUE == bIsLTENetworkDetected ) && ( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) ) ||
              ( FALSE == bIsLTENetworkDetected ) ) )
        {
            if( FALSE == bIsLTENetworkDetected )
            {
                pDeviceRegCtx->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;
            }
            else
            {
                if( ( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) && 
                    ( cs_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) )
                {
                    pDeviceRegCtx->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_CS_PS;
                }
                else if( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED )
                {
                    pDeviceRegCtx->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_PS;
                }
                else if( cs_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED )
                {
                    pDeviceRegCtx->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_CS;
                }
                else
                {
                    pDeviceRegCtx->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;
                }
            }

            pDeviceRegCtx->enNASRegistrationStatus = DEVICE_NAS_STATUS_REGISTERED;
            nasCtx->enNASRegistrationStatus = DEVICE_NAS_STATUS_REGISTERED;
            pDeviceRegCtx->uiCurrentStep = NAS_MODEM_NOTIFY_REGISTRATION_STATUS;
	        registration_state_previous = QMI_NAS_REGISTRATION_STATE_REGISTERED;
        }
        else
        {
            if( NAS_MODEM_REGISTRATION_MONITOR_VIA_SM == pDeviceRegCtx->enQuerySource )
            {
                CELLULAR_HAL_DBG_PRINT("%s - Modem Registration not completed so trying to sync again in 5seconds\n",__FUNCTION__);
		if (retry_get_serving_system >= 3) // This is to avoid retry in QMI layer.Indication comes instead of retry op
		{
                    pDeviceRegCtx->enNASRegistrationStatus = DEVICE_NAS_STATUS_REGISTERING;
                    retry_get_serving_system = 0;
                    goto LASTSTEP;
                }

                qmi_message_nas_get_serving_system_output_unref (output);

                sleep(5); //Retry after 5seconds

                pDeviceRegCtx->uiCurrentStep = NAS_MODEM_GET_REGISTRATION_STATUS;
                cellular_hal_qmi_monitor_device_registration_step(task);
                return;
            }
        }
    }

LASTSTEP:
    qmi_message_nas_get_serving_system_output_unref (output);
    pDeviceRegCtx->uiCurrentStep = NAS_MODEM_NOTIFY_REGISTRATION_STATUS;
    cellular_hal_qmi_monitor_device_registration_step(task);
}

static void cellular_hal_qmi_monitor_device_registration_step( GTask *task )
{
    QMIContextStructPrivate   *pstQMIContext  = NULL;
    ContextModemRegistration  *pDeviceRegCtx = NULL;

    pDeviceRegCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pDeviceRegCtx->vpPrivateData;
    
    switch ( pDeviceRegCtx->uiCurrentStep ) 
    {
        case NAS_MODEM_REGISTRATION_STATUS_BEGIN:
        {
            pDeviceRegCtx->uiCurrentStep++;
            /* fall through */
        }
        case NAS_MODEM_GET_REGISTRATION_STATUS:
        {
            ContextNASInfo   *nasCtx = &(pstQMIContext->nasCtx);
            if ( nasCtx->serving_system_id == 0)
            {
               CELLULAR_HAL_DBG_PRINT("%s - Registering for serving system info events  \n",__FUNCTION__);
               nasCtx->serving_system_id  =   g_signal_connect (QMI_CLIENT_NAS(nasCtx->nasClient),"serving-system",
                                                             G_CALLBACK (cellular_hal_qmi_serving_system_indication_cb),
                                                             pDeviceRegCtx->device_registration_status_cb);
            }
            qmi_client_nas_get_serving_system ( QMI_CLIENT_NAS(nasCtx->nasClient),
                                                NULL,
                                                10,
                                                NULL,
                                                (GAsyncReadyCallback)cellular_hal_qmi_monitor_get_serving_system,
                                                task);
            return;
        }
        case NAS_MODEM_NOTIFY_REGISTRATION_STATUS:
        {
            //Send NAS registration status if CB is not null
            if( NULL != pDeviceRegCtx->device_registration_status_cb )
            {
                CELLULAR_HAL_DBG_PRINT("%s - Modem Register Status is '%d' '%d' and sending status via CB\n",__FUNCTION__,pDeviceRegCtx->enNASRegistrationStatus, pDeviceRegCtx->registered_service);
                pDeviceRegCtx->device_registration_status_cb( pDeviceRegCtx->enNASRegistrationStatus, pDeviceRegCtx->enNASRoamingStatus, pDeviceRegCtx->registered_service );
            }
        }
        case NAS_MODEM_REGISTRATION_STATUS_END:
        default:
        {
            //Assign NULL task pointer after free
            if( NULL != pstQMIContext )
            {
                if( NAS_MODEM_REGISTRATION_MONITOR_VIA_SM == pDeviceRegCtx->enQuerySource )
                {
                    ContextNASInfo   *nasCtx = &(pstQMIContext->nasCtx);

                    nasCtx->bIsModemRegTaskStillRunning = FALSE;

                    if( NULL != nasCtx->cancellableForModemRegTask )
                    {
                        g_object_unref (nasCtx->cancellableForModemRegTask);
                        nasCtx->cancellableForModemRegTask = NULL;
                    }
                }
                else if( NAS_MODEM_REGISTRATION_GET_VIA_DML == pDeviceRegCtx->enQuerySource )
                {
                    pDeviceRegCtx->bIsModemRegTaskDMLGetCompleted = TRUE;
                }
            }

            g_object_unref (task);
        }
    }
}

static void cellular_hal_qmi_monitor_device_registration_context_free( ContextModemRegistration *pDeviceRegCtx )
{
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources\n",__FUNCTION__);

    if( ( NULL != pDeviceRegCtx ) && ( NAS_MODEM_REGISTRATION_MONITOR_VIA_SM == pDeviceRegCtx->enQuerySource ) )
    {
        g_slice_free(ContextModemRegistration, pDeviceRegCtx);
        pDeviceRegCtx  = NULL;
    }
}

int cellular_hal_qmi_monitor_device_registration(cellular_device_registration_status_callback device_registration_status_cb)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);

        if( NULL != nasCtx->nasClient )
        {
            ContextModemRegistration  *pDeviceRegCtx = NULL;
            GTask                     *task;

            pDeviceRegCtx                  = (ContextModemRegistration*)g_slice_new0 (ContextModemRegistration);
            pDeviceRegCtx->uiCurrentStep   = NAS_MODEM_REGISTRATION_STATUS_BEGIN;
            pDeviceRegCtx->vpPrivateData   = (void*) gpstQMIContext;
            pDeviceRegCtx->device_registration_status_cb = device_registration_status_cb;
            pDeviceRegCtx->enQuerySource   = NAS_MODEM_REGISTRATION_MONITOR_VIA_SM;
            
            nasCtx->bIsModemRegTaskStillRunning = TRUE;
            nasCtx->cancellableForModemRegTask = g_cancellable_new ();

            task = g_task_new (NULL, nasCtx->cancellableForModemRegTask, NULL, NULL);
            g_task_set_task_data (task, (gpointer)pDeviceRegCtx, (GDestroyNotify)cellular_hal_qmi_monitor_device_registration_context_free);

            CELLULAR_HAL_DBG_PRINT("%s - Monitor Modem Device Task:0x%p\n",__FUNCTION__,task);
            cellular_hal_qmi_monitor_device_registration_step(task);
            return RETURN_OK;
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s - NAS client is not ready so slot can't be monitor registration status\n", __FUNCTION__);
            return RETURN_ERROR;
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so slot can't be monitor registration status\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

static void cellular_hal_qmi_network_scan_ready (QmiClientNas *client,
                                                 GAsyncResult *res,
                                                 gpointer  user_data)
{
    QmiMessageNasNetworkScanOutput *output;
    QmiNasNetworkScanResult network_scan_result = QMI_NAS_NETWORK_SCAN_RESULT_ABORT;
    GError *error = NULL;
    GArray *array;
    ContextNetworkScan        *pNetworkScanCtx = (ContextNetworkScan*)user_data;
    QMIContextStructPrivate   *pstQMIContext  = NULL;
    guint TotalEntries = 0;
    ContextNASInfo   *nasCtx = NULL;

    pstQMIContext   = (QMIContextStructPrivate*)pNetworkScanCtx->vpPrivateData;
    nasCtx          = &(pstQMIContext->nasCtx);

    output = qmi_client_nas_network_scan_finish (client, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s error: operation failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        return;
    }

    if (!qmi_message_nas_network_scan_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s error: couldn't scan networks: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_nas_network_scan_output_unref (output);
        return;
    }

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_network_information (output, &array, NULL)) 
    {
        guint i;

        pthread_mutex_lock(&(nasCtx->idwMutexForSCANData));

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputNetworkInformationElement *element;
            g_autofree gchar *status_str = NULL;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputNetworkInformationElement, i);
            status_str = qmi_nas_network_status_build_string_from_mask (element->network_status);

            nasCtx->astNetworkInfo[TotalEntries].mcc = element->mcc;
            nasCtx->astNetworkInfo[TotalEntries].mnc = element->mnc;
            snprintf(nasCtx->astNetworkInfo[TotalEntries].name, sizeof(nasCtx->astNetworkInfo[TotalEntries].name), "%s", element->description);
            
            nasCtx->astNetworkInfo[TotalEntries].network_allowed_flag = FALSE; 
            if( ( NULL != status_str ) &&
                ( NULL != strstr(status_str, "not-forbidden" )) )
            {
                nasCtx->astNetworkInfo[TotalEntries].network_allowed_flag = TRUE; 
            }

            ++TotalEntries;
        }

        nasCtx->iTotalNoofNetworkInfo = TotalEntries;

        pthread_mutex_unlock(&(nasCtx->idwMutexForSCANData));
    }

    if (qmi_message_nas_network_scan_output_get_network_scan_result (output,
                                                                     &network_scan_result,
                                                                     NULL)) {
        //NOP
    }

    qmi_message_nas_network_scan_output_unref (output);
}

static void* cellular_hal_qmi_network_scan_start_thread( void *arg )
{
    //detach thread from caller stack
    pthread_detach(pthread_self());

    CELLULAR_HAL_DBG_PRINT("%s - Entering into Network SCAN thread\n",__FUNCTION__);

    QMIContextStructPrivate   *pstQMIContext  = (QMIContextStructPrivate*)arg;
    ContextNetworkScan  *pNetworkScanCtx = NULL;
    ContextNASInfo  *nasCtx = &(pstQMIContext->nasCtx);

    nasCtx->bIsNetworkScanTaskStillRunning = TRUE;
    nasCtx->bIsNeed2StopNetworkInfoCollection = FALSE;

    pNetworkScanCtx                  = (ContextNetworkScan*)g_slice_new0 (ContextNetworkScan);
    pNetworkScanCtx->vpPrivateData   = (void*) pstQMIContext;

    while( ( FALSE == pstQMIContext->IsDeviceRemovedSignalReceived ) &&
           ( FALSE == nasCtx->bIsNeed2StopNetworkInfoCollection ) )
    {   
        struct timespec _ts = { 0 };
        struct timespec _now = { 0 };
        int n;

        //Sleep for certain time for collection
        pthread_mutex_lock(&(nasCtx->idwMutexForSCAN));
        clock_gettime(CLOCK_REALTIME, &_now);
        _ts.tv_sec = _now.tv_sec + CELLULAR_QMI_NETWORKSCAN_COLLECTION_PERIODIC_INTERVAL;
        pthread_cond_timedwait(&(nasCtx->idwCondForSCAN), &(nasCtx->idwMutexForSCAN), &_ts);
        pthread_mutex_unlock(&(nasCtx->idwMutexForSCAN));

        if( ( TRUE == pstQMIContext->IsDeviceRemovedSignalReceived ) ||
            ( TRUE == nasCtx->bIsNeed2StopNetworkInfoCollection ) )
        {

            break;
        }
            
        if( nasCtx->enNASRegistrationStatus == DEVICE_NAS_STATUS_REGISTERED )
        {

            qmi_client_nas_network_scan (QMI_CLIENT_NAS(nasCtx->nasClient),
                                        NULL,
                                        60, /* this operation takes a lot of time! */
                                        NULL,
                                        (GAsyncReadyCallback)cellular_hal_qmi_network_scan_ready,
                                        pNetworkScanCtx);
        }
    }

    if( NULL != pNetworkScanCtx )
    {
        g_slice_free(ContextNetworkScan, pNetworkScanCtx);
        pNetworkScanCtx  = NULL;
    }

    CELLULAR_HAL_DBG_PRINT("%s - Exiting from SCAN thread\n",__FUNCTION__);
    
    //Cleanup current thread when exit
    pthread_exit(NULL);
    
    return RETURN_OK;
}

static int cellular_hal_qmi_network_scan_data_collection_task( void )
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);

        if( NULL != nasCtx->nasClient )
        {
            if( FALSE == nasCtx->bIsNetworkScanTaskStillRunning )
            {
                //Initiate the thread for cellular network scan 
                pthread_create( &gloopThread, NULL, &cellular_hal_qmi_network_scan_start_thread, (void*)(gpstQMIContext));
                return RETURN_OK;
            }
            else
            {
                CELLULAR_HAL_DBG_PRINT("%s - Network Scan is already running so ignore another instance\n", __FUNCTION__);
                return RETURN_ERROR; 
            }
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s - NAS client is not ready so can't be collect network scan information\n", __FUNCTION__);
            return RETURN_ERROR;
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't be collect network scan information\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

int cellular_hal_qmi_get_available_networks_information(CellularNetworkScanResultInfoStruct **network_info, unsigned int *total_network_count)
{
    *total_network_count = 0;

    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);

        if( NULL != nasCtx->nasClient )
        {
            pthread_mutex_lock(&(nasCtx->idwMutexForSCANData));
            *total_network_count = nasCtx->iTotalNoofNetworkInfo;
            *network_info = NULL;
            if( 0 < nasCtx->iTotalNoofNetworkInfo )
            {
                int i;
                CellularNetworkScanResultInfoStruct *tmp_network_info = NULL;

                tmp_network_info = malloc( sizeof( CellularNetworkScanResultInfoStruct ) * nasCtx->iTotalNoofNetworkInfo );
                memset( tmp_network_info, 0, sizeof( CellularNetworkScanResultInfoStruct ) * nasCtx->iTotalNoofNetworkInfo );
 		
                for( i = 0; i < nasCtx->iTotalNoofNetworkInfo; i++ )
                {
                    tmp_network_info[i].MCC = nasCtx->astNetworkInfo[i].mcc;
                    tmp_network_info[i].MNC = nasCtx->astNetworkInfo[i].mnc;
                    snprintf(tmp_network_info[i].network_name, sizeof(tmp_network_info[i].network_name), "%s", nasCtx->astNetworkInfo[i].name);
                    tmp_network_info[i].network_allowed_flag = nasCtx->astNetworkInfo[i].network_allowed_flag;
                }
		
                *network_info = tmp_network_info;
           }
           pthread_mutex_unlock(&(nasCtx->idwMutexForSCANData));
           return RETURN_OK;
        }
        else
        {
            return RETURN_ERROR;
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't be collect network scan information\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

static void cellular_hal_qmi_get_current_plmn_network_info_ready (QmiClientNas *client, GAsyncResult *res, gpointer  user_data)
{
    QmiMessageNasGetServingSystemOutput *output;
    GError *error = NULL;
    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate   *pstQMIContext  = NULL;
    ContextGetNetworkInfo     *pNetworkGetCtx = NULL;
    CellularCurrentPlmnInfoStruct *pstPlmnInfo = NULL;
    ContextNASInfo   *nasCtx = NULL;

    pNetworkGetCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pNetworkGetCtx->vpPrivateData;
    nasCtx          = &(pstQMIContext->nasCtx);
    pstPlmnInfo     = &(nasCtx->stPlmnInfo);

    output = qmi_client_nas_get_serving_system_finish (client, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s error: operation failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        goto NEXTSTEP;
    }

    if (!qmi_message_nas_get_serving_system_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s error: couldn't get serving system: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_nas_get_serving_system_output_unref (output);
        goto NEXTSTEP;
    }

    {
        QmiNasRegistrationState registration_state;
        QmiNasAttachState cs_attach_state;
        QmiNasAttachState ps_attach_state;
        QmiNasNetworkType selected_network;
        GArray *radio_interfaces;
        guint i;
        guint8 bIsLTENetworkDetected = FALSE;

        qmi_message_nas_get_serving_system_output_get_serving_system ( output,
                                                                       &registration_state,
                                                                       &cs_attach_state,
                                                                       &ps_attach_state,
                                                                       &selected_network,
                                                                       &radio_interfaces,
                                                                       NULL);

        for (i = 0; i < radio_interfaces->len; i++) 
        {
            QmiNasRadioInterface iface;

            iface = g_array_index (radio_interfaces, QmiNasRadioInterface, i);

            if( 0 == strcmp( "lte", qmi_nas_radio_interface_get_string (iface) ) )
            {
                bIsLTENetworkDetected = TRUE;
            }
        }

        if( registration_state == QMI_NAS_REGISTRATION_STATE_REGISTERED )
        {
            pstPlmnInfo->registration_status = DEVICE_REGISTERED;
        }
        else
        {
            pstPlmnInfo->registration_status = DEVICE_NOT_REGISTERED;
        }

        pstPlmnInfo->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;

        //All good so notify otherwise retry after 5seconds and cs_attach_state is only for 2G. When LTE PS should be attached
        if( ( registration_state == QMI_NAS_REGISTRATION_STATE_REGISTERED  ) &&
            ( ( ( TRUE == bIsLTENetworkDetected ) && ( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) ) ||
              ( FALSE == bIsLTENetworkDetected ) ) )
        {
            if( FALSE == bIsLTENetworkDetected )
            {
                pstPlmnInfo->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;
            }
            else
            {
                if( ( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) && 
                    ( cs_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED ) )
                {
                    pstPlmnInfo->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_CS_PS;
                }
                else if( ps_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED )
                {
                    pstPlmnInfo->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_PS;
                }
                else if( cs_attach_state == QMI_NAS_ATTACH_STATE_ATTACHED )
                {
                    pstPlmnInfo->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_CS;
                }
                else
                {
                    pstPlmnInfo->registered_service = CELLULAR_MODEM_REGISTERED_SERVICE_NONE;
                }
            }
        }
    }

    {
        QmiNasRoamingIndicatorStatus roaming;

        pstPlmnInfo->roaming_enabled = FALSE;

        if (qmi_message_nas_get_serving_system_output_get_roaming_indicator (
                output,
                &roaming,
                NULL)) 
        {

            if( roaming == QMI_NAS_ROAMING_INDICATOR_STATUS_ON )
            {
                pstPlmnInfo->roaming_enabled = TRUE;
            }
        }
    }

    {   
        guint16 current_plmn_mcc;
        guint16 current_plmn_mnc;
        const gchar *current_plmn_description;
        
        if (qmi_message_nas_get_serving_system_output_get_current_plmn (
                output,
                &current_plmn_mcc,
                &current_plmn_mnc,
                &current_plmn_description,
                NULL)) 
        {
            pstPlmnInfo->MCC = current_plmn_mcc;
            pstPlmnInfo->MNC = current_plmn_mnc;
            snprintf(pstPlmnInfo->plmn_name, sizeof(pstPlmnInfo->plmn_name), current_plmn_description);
        }
    }

    {   
        guint16 lac;
        
        if (qmi_message_nas_get_serving_system_output_get_lac_3gpp (
                output,
                &lac,
                NULL)) 
        {
            pstPlmnInfo->area_code = lac;
        }
    }

    {
        guint32 cid;

        if (qmi_message_nas_get_serving_system_output_get_cid_3gpp (
                output,
                &cid,
                NULL)) 
        {
            pstPlmnInfo->cell_id = cid;
        }
    }

NEXTSTEP:
    qmi_message_nas_get_serving_system_output_unref (output);

    pNetworkGetCtx->uiCurrentStep++;
    cellular_hal_qmi_get_current_plmn_information_step(task);
}

static void cellular_hal_qmi_get_current_plmn_information_step( GTask *task )
{
    QMIContextStructPrivate   *pstQMIContext  = NULL;
    ContextGetNetworkInfo     *pNetworkGetCtx = NULL;

    pNetworkGetCtx  = g_task_get_task_data (task);
    pstQMIContext   = (QMIContextStructPrivate*)pNetworkGetCtx->vpPrivateData;
    
    switch ( pNetworkGetCtx->uiCurrentStep ) 
    {
        case NAS_GET_NETWORK_INFO_BEGIN:
        {
            pNetworkGetCtx->uiCurrentStep++;
            /* fall through */
        }
        case NAS_GET_NETWORK_INFO_COLLECT_SERVING_SYSTEM:
        {
            ContextNASInfo   *nasCtx = &(pstQMIContext->nasCtx);

            qmi_client_nas_get_serving_system ( QMI_CLIENT_NAS(nasCtx->nasClient),
                                                NULL,
                                                10,
                                                NULL,
                                                (GAsyncReadyCallback)cellular_hal_qmi_get_current_plmn_network_info_ready,
                                                task);
            return;
        }
        case NAS_GET_NETWORK_INFO_COLLECT_SYSTEM_INFO:
        {
        }
        case NAS_GET_NETWORK_INFO_END:
        default:
        {
            ContextNASInfo   *nasCtx = &(pstQMIContext->nasCtx);
            unsigned long lCurrentUptime = 0;

            nasCtx->bIsGetNetworkInfoCollectionDone = TRUE;
            cellular_hal_util_GetUptime( &lCurrentUptime );
            nasCtx->LastCollectedTimeForPlmnInfo = lCurrentUptime;

            g_object_unref (task);
        }
    }
}

static void cellular_hal_qmi_get_current_plmn_information_context_free( ContextGetNetworkInfo *pNetworkGetCtx )
{
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources\n",__FUNCTION__);

    if( NULL != pNetworkGetCtx )
    {
        g_slice_free(ContextGetNetworkInfo, pNetworkGetCtx);
        pNetworkGetCtx  = NULL;
    }
}

int cellular_hal_qmi_get_current_plmn_information(CellularCurrentPlmnInfoStruct *plmn_info)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);

        if( NULL != nasCtx->nasClient )
        {
            unsigned long lCurrentUptime = 0;

            cellular_hal_util_GetUptime( &lCurrentUptime );

            if ( ( nasCtx->LastCollectedTimeForPlmnInfo > 0 ) &&
                 ( lCurrentUptime - nasCtx->LastCollectedTimeForPlmnInfo ) <= CELLULAR_QMI_GETNETWORK_VIA_DML_MAX_TTL )
            {
                memcpy( plmn_info, &(nasCtx->stPlmnInfo), sizeof(CellularCurrentPlmnInfoStruct) );
                CELLULAR_HAL_DBG_PRINT("%s - (C) Current PLMN information retrived\n", __FUNCTION__);

                return RETURN_OK;
            }
            else
            {
                ContextGetNetworkInfo  *pNetworkGetCtx = NULL;
                GTask                  *task;
                int iLapsedSeconds = 0;

                pNetworkGetCtx                  = (ContextGetNetworkInfo*)g_slice_new0 (ContextGetNetworkInfo);
                pNetworkGetCtx->uiCurrentStep   = NAS_GET_NETWORK_INFO_BEGIN;
                pNetworkGetCtx->vpPrivateData   = (void*) gpstQMIContext;
                nasCtx->bIsGetNetworkInfoCollectionDone = FALSE;
                
                task = g_task_new (NULL, NULL, NULL, NULL);
                g_task_set_task_data (task, (gpointer)pNetworkGetCtx, (GDestroyNotify)cellular_hal_qmi_get_current_plmn_information_context_free);

                CELLULAR_HAL_DBG_PRINT("%s - Get current plmn information Task:0x%p\n",__FUNCTION__,task);
                cellular_hal_qmi_get_current_plmn_information_step(task);

                //Wait here query
                while ( 1 )
                {
                    if( TRUE == nasCtx->bIsGetNetworkInfoCollectionDone )
                    {
                        memcpy( plmn_info, &(nasCtx->stPlmnInfo), sizeof(CellularCurrentPlmnInfoStruct) );
                        CELLULAR_HAL_DBG_PRINT("%s - (A) Current PLMN information retrived\n", __FUNCTION__);
                        break;
                    }
                    else if( iLapsedSeconds >= CELLULAR_QMI_GETNETWORK_VIA_DML_MAX_WAITIME )  
                    {
                        CELLULAR_HAL_DBG_PRINT("%s - Timeout during NAS network Query\n",__FUNCTION__);
                        break;
                    }

                    sleep(1);
                    iLapsedSeconds++;
                }
            }

            return RETURN_OK;
        }
        else
        {
            CELLULAR_HAL_DBG_PRINT("%s - NAS client is not ready so an't be query network information\n", __FUNCTION__);
            return RETURN_ERROR;
        }
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so slot can't be query current plmn information\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

static void cellular_hal_set_qmi_modem_network_operation (QmiClientNas *nasClient, GAsyncResult *result, gpointer  user_data)
{
    QmiMessageNasAttachDetachOutput *output;
    GError *error = NULL;
    NASAttachDetachOperation_t network_operation = (NASAttachDetachOperation_t) user_data;

    output = qmi_client_nas_attach_detach_finish (nasClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s attach-detach operation failed %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        return;
    }

    if (!qmi_message_nas_attach_detach_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s failed attach/detach operation: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_nas_attach_detach_output_unref (output);
        return;
    }

    qmi_message_nas_attach_detach_output_unref (output);

    CELLULAR_HAL_DBG_PRINT("%s - Modem %s request success\n", __FUNCTION__,(network_operation == NAS_NETWORK_ATTACH) ? "attach":"detach");
}

int cellular_hal_qmi_set_modem_network_operation( NASAttachDetachOperation_t network_operation)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        QmiMessageNasAttachDetachInput *input = NULL;
        ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);

        CELLULAR_HAL_DBG_PRINT("%s - Modem %s requested\n", __FUNCTION__,(network_operation == NAS_NETWORK_ATTACH) ? "attach":"detach");

        input = qmi_message_nas_attach_detach_input_new();
        qmi_message_nas_attach_detach_input_set_action( input, 
                                                        (network_operation == NAS_NETWORK_ATTACH) ? QMI_NAS_PS_ATTACH_ACTION_ATTACH : QMI_NAS_PS_ATTACH_ACTION_DETACH,
                                                        NULL );
        qmi_client_nas_attach_detach ( QMI_CLIENT_NAS(nasCtx->nasClient),
                                       input,
                                       10,
                                       NULL,
                                       cellular_hal_set_qmi_modem_network_operation,
                                       (void*)network_operation);
        qmi_message_nas_attach_detach_input_unref (input);
        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't configure modem attach/detach\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}

static void cellular_hal_qmi_get_technology_preference_cb (QmiClientNas *client,
                                                              GAsyncResult *res,
							      gpointer user_data)
{
    QmiMessageNasGetTechnologyPreferenceOutput *output;
    GError *error = NULL;
    QmiNasRadioTechnologyPreference preference,mask;
    QmiNasPreferenceDuration duration;
    g_autofree gchar *preference_string = NULL;
    ContextNASInfo *nasCtx = (ContextNASInfo*)user_data;
    output = qmi_client_nas_get_technology_preference_finish (client, res, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s - ERROR !! Operation failed :%s  \n", __FUNCTION__,error->message);
        g_error_free (error);
        return;
    }
    if (!qmi_message_nas_get_technology_preference_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s - ERROR !! could not get technology preference  :%s  \n", __FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_nas_get_technology_preference_output_unref (output);
        return;
    }
    qmi_message_nas_get_technology_preference_output_get_active (
        output,
        &preference,
        &duration,
        NULL);
    memset(nasCtx->preferredRAT,0,sizeof(nasCtx->preferredRAT));
    if (preference == QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_AUTO)
    {
       strncpy(nasCtx->preferredRAT,"AUTO,",sizeof(nasCtx->preferredRAT));
    }
    else {
            if ( preference & QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_3GPP2)
            {
                if (preference & QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_CDMA_OR_WCDMA)
                     strcat(nasCtx->preferredRAT,"CDMA20001X,");
                if (preference & QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_HDR)
                     strcat(nasCtx->preferredRAT,"EVDO,");
            }
	    if( preference & QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_3GPP)
            {
                if (preference & QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_AMPS_OR_GSM)
                     strcat(nasCtx->preferredRAT,"GSM,");
                else{
                     //strcat(nasCtx->preferredRAT,"UMTS,");
		     //Note: Display this tech preference only modem firmware supports UMTS alone 
                }

	    }
            if (preference & QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_LTE )
                strcat(nasCtx->preferredRAT,"LTE,");
    }
    nasCtx->preferredRAT[strlen(nasCtx->preferredRAT)-1] = '\0'; // just to remove last comma.

    CELLULAR_HAL_DBG_PRINT("%s - On ready preferred RAT value is  %s  \n", __FUNCTION__,nasCtx->preferredRAT);
    qmi_message_nas_get_technology_preference_output_unref (output);
}

QmiNasRadioTechnologyPreference
supported_mode_to_qmi_radio_technology_preference (char* pref_mode)
{
    ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);
    QmiNasRadioTechnologyPreference pref = 0;

    // Note: make sure pref_mode  must be part of supported tech
    if (strstr(pref_mode,"CDMA20001X") && strstr(dmsCtx->SupportedRAT,"CDMA20001X"))
    {
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_3GPP2;
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_CDMA_OR_WCDMA;
    }
    if (strstr (pref_mode,"EVDO") && strstr(dmsCtx->SupportedRAT,"EVDO"))
    {
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_3GPP2;
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_HDR;
    }
    if (strstr (pref_mode,"GSM") && strstr(dmsCtx->SupportedRAT,"GSM"))
    {
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_3GPP;
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_AMPS_OR_GSM;
    }
    if (strstr (pref_mode,"UMTS") && strstr(dmsCtx->SupportedRAT,"UMTS"))
    {
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_3GPP;
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_CDMA_OR_WCDMA;
    }
    if (strstr (pref_mode,"LTE") && strstr(dmsCtx->SupportedRAT,"LTE"))
    {
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_3GPP;    //FIXME:Adding 3G flags as LTE option alone is not operational  
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_CDMA_OR_WCDMA; //Adding 3G flags  
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_LTE;
    }
    if (strstr (pref_mode,"5GNR") && strstr(dmsCtx->SupportedRAT,"5GNR"))
    {
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_AUTO;  //5G not defined in tech preference..add auto for now
    }
    if (strstr (pref_mode,"AUTO")) 
    {
        pref |= QMI_NAS_RADIO_TECHNOLOGY_PREFERENCE_AUTO;
    }
    CELLULAR_HAL_DBG_PRINT("%s - RAT preference mapped value %d... \n", __FUNCTION__,pref);
    return pref;
}

int cellular_hal_qmi_get_preferred_radio_technology( char *preferred_rat)
{
    ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) )&& \
        ( NULL != nasCtx->nasClient ) )
    {
        if( '\0' != nasCtx->preferredRAT[0] )
        {
            strncpy( preferred_rat, nasCtx->preferredRAT, strlen(nasCtx->preferredRAT) + 1 );
            CELLULAR_HAL_DBG_PRINT("%s -  Preferred RAT '%s' Retrived via DML\n", __FUNCTION__,preferred_rat);
            return RETURN_OK;
        }
 
        CELLULAR_HAL_DBG_PRINT("%s - Invoking QMI call to get preferred RAT \n", __FUNCTION__);
        int iLapsedSeconds = 0;
        qmi_client_nas_get_technology_preference (QMI_CLIENT_NAS(nasCtx->nasClient),
                                                  NULL,
                                                  10,
                                                  NULL,
                                                  (GAsyncReadyCallback)cellular_hal_qmi_get_technology_preference_cb,
                                                  nasCtx);
        //Wait here till IDs query
        while ( 1 )
        {
            if('\0' != nasCtx->preferredRAT[0])
            {
                strncpy(preferred_rat, nasCtx->preferredRAT, strlen(nasCtx->preferredRAT));
                CELLULAR_HAL_DBG_PRINT("%s - preferredRAT'%s' from QMI DML\n", __FUNCTION__,preferred_rat);
                break;
            }
            else if( iLapsedSeconds >= CELLULAR_QMI_GET_CAPS_MAX_WAITIME )
            {
                CELLULAR_HAL_DBG_PRINT("%s - Timeout during get Prefferer RAT \n",__FUNCTION__);
                break;
            }
            sleep(1);
            iLapsedSeconds++;
        }
        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI not ready so can't quey for preferred tech \n", __FUNCTION__);
        return RETURN_ERROR;
    }
}

static void
set_current_capabilities_set_technology_preference_cb (QmiClientNas *client,
                                                          GAsyncResult *res,
                                                          gpointer  user_data)
{
    QmiMessageNasSetTechnologyPreferenceOutput *output = NULL;
    GError                                     *error = NULL;
    char *preferred_rat  = (char*) user_data;
    ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);
    output = qmi_client_nas_get_technology_preference_finish (client, res, &error);

    output = qmi_client_nas_set_technology_preference_finish (client, res, &error);
    if (!output || !qmi_message_nas_set_technology_preference_output_get_result (output, &error)) {
        g_clear_error (&error);
	// In case of QMI_PROTOCOL_ERROR_NO_EFFECT ---no need of reset
        CELLULAR_HAL_DBG_PRINT("%s - No operation ..or Error  \n", __FUNCTION__);
    }
    else { //sucess... Issue the reset
        CELLULAR_HAL_DBG_PRINT("%s - Issuing the RESET  \n",__FUNCTION__);
        cellular_hal_qmi_set_modem_operating_configuration(CELLULAR_MODEM_SET_OFFLINE);
        cellular_hal_qmi_set_modem_operating_configuration(CELLULAR_MODEM_SET_ONLINE);
    }
    if(output)
        qmi_message_nas_set_technology_preference_output_unref (output);
    // update local preferred RAT value..dont update on error (No error handling as of now)
    memset(nasCtx->preferredRAT,0,sizeof(nasCtx->preferredRAT));
    strncpy(nasCtx->preferredRAT, preferred_rat, strlen(preferred_rat) + 1 );
    if(preferred_rat)
        free(preferred_rat);
}

int cellular_hal_qmi_set_preferred_radio_technology (char *preferred_rat)
{
    QmiMessageNasSetTechnologyPreferenceInput *input;
    QmiNasRadioTechnologyPreference            pref;
    ContextNASInfo   *nasCtx = &(gpstQMIContext->nasCtx);
    char *preferred_rat_local = NULL;
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) )&& \
        ( NULL != nasCtx->nasClient ) )
     {
         ContextDMSInfo  *dmsCtx = &(gpstQMIContext->dmsCtx);
         if('\0' == dmsCtx->SupportedRAT[0] )  // Make sure we have supported RAT ready
         {
            char supported_rat[128];
            cellular_hal_qmi_get_supported_radio_technology(supported_rat);
         }

         CELLULAR_HAL_DBG_PRINT("%s - Invoking QMI call to set preferred RAT \n", __FUNCTION__);
         ContextNASQuery *pstCtxNASQuery = NULL;
         int iLapsedSeconds = 0;
         preferred_rat_local = strdup(preferred_rat);

         // prepare RAT pref bit mapping
         pref = supported_mode_to_qmi_radio_technology_preference(preferred_rat);

         input = qmi_message_nas_set_technology_preference_input_new ();
         qmi_message_nas_set_technology_preference_input_set_current (input, pref, QMI_NAS_PREFERENCE_DURATION_PERMANENT, NULL);

         qmi_client_nas_set_technology_preference (nasCtx->nasClient, input, 5, NULL,
                                                   (GAsyncReadyCallback)set_current_capabilities_set_technology_preference_cb,
                                                   preferred_rat_local);
         qmi_message_nas_set_technology_preference_input_unref (input);
         return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI not ready so can't set  preferred tech \n", __FUNCTION__);
        return RETURN_ERROR;
    }
}

/* WDS */
static void cellular_hal_qmi_wds_create_profile (QmiClientWds *wdsClient, GAsyncResult *result, gpointer  user_data)
{
    QmiMessageWdsCreateProfileOutput *output;
    GError *error = NULL;
    QmiWdsProfileType profile_type;
    guint8 profile_index;
    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate    *pstQMIContext     = NULL;
    ContextProfileOperation    *pProfileCrCtx     = NULL;
    ContextWDSInfo             *wdsCtx            = NULL;
    CellularProfileStruct      *pstInputProfile   = NULL;
    cellular_device_profile_status_api_callback device_profile_status_cb = NULL;

    pProfileCrCtx       = g_task_get_task_data (task);
    pstQMIContext       = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx              = &(pstQMIContext->wdsCtx);
    device_profile_status_cb = pProfileCrCtx->device_profile_status_cb;
    pstInputProfile     = &(pProfileCrCtx->stProfileInput);

    output = qmi_client_wds_create_profile_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT ("failed to create profile: %s\n", error->message);
        g_error_free (error);
        goto NOTIFY;
    }

    if (!qmi_message_wds_create_profile_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s Failed to create profile: %s\n",
                                        __FUNCTION__,
                                        error->message);

        g_error_free (error);
        qmi_message_wds_create_profile_output_unref (output);
        goto NOTIFY;
    }

    if (qmi_message_wds_create_profile_output_get_profile_identifier (output,
                                                                      &profile_type,
                                                                      &profile_index,
                                                                      NULL)) 
    {
        CELLULAR_HAL_DBG_PRINT("%s - New Profile Created - Name:%s Type:%s Index:%d\n",
                                    __FUNCTION__,
                                    pstInputProfile->ProfileName,
                                    qmi_wds_profile_type_get_string(profile_type),
                                    profile_index);

        qmi_message_wds_create_profile_output_unref (output);

        pstInputProfile->ProfileID   = profile_index;
        pstInputProfile->ProfileType = profile_type;

        pProfileCrCtx->uiCurrentStep++;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }
    else
    {
        qmi_message_wds_create_profile_output_unref (output);
    }

NOTIFY:
    //Send QMI not ready status if CB is not null
    if( NULL != device_profile_status_cb )
    {
        CELLULAR_HAL_DBG_PRINT("%s - WDS is not-ready/error so profile can't be chosen now and sending status via CB\n", __FUNCTION__);
        device_profile_status_cb( "unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_NOT_READY );
    }

    pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
    cellular_hal_qmi_profile_operation_step(task);
}

static void cellular_hal_qmi_wds_profile_create (ContextWDSInfo  *wdsCtx, CellularProfileStruct *pstProfileInput, gpointer  user_data)
{
    QmiMessageWdsCreateProfileInput *input = NULL;
    GError *error = NULL;

    /* Create profile bundle */
    input = qmi_message_wds_create_profile_input_new ();

    /* Profile type */
    qmi_message_wds_create_profile_input_set_profile_type (input, QMI_WDS_PROFILE_TYPE_3GPP, NULL);

    if ( CELLULAR_PDP_CONTEXT_UNKNOWN != pstProfileInput->PDPContextNumber )
    qmi_message_wds_create_profile_input_set_pdp_context_number (input, pstProfileInput->PDPContextNumber, NULL);

    if ( CELLULAR_PDP_CONTEXT_UNKNOWN != pstProfileInput->PDPType )
    qmi_message_wds_create_profile_input_set_pdp_type (input, pstProfileInput->PDPType, NULL);

#if 0
    if (props.apn_type_set)
        qmi_message_wds_create_profile_input_set_apn_type_mask (input, props.apn_type, NULL);
#endif

    if ( ( '\0' != pstProfileInput->ProfileName[0] ) && ( strlen(pstProfileInput->ProfileName) > 0 ) )
    qmi_message_wds_create_profile_input_set_profile_name (input, pstProfileInput->ProfileName, NULL);

    if ( ( '\0' != pstProfileInput->APN[0] ) && ( strlen(pstProfileInput->APN) > 0 ) )
    qmi_message_wds_create_profile_input_set_apn_name (input, pstProfileInput->APN, NULL);

    qmi_message_wds_create_profile_input_set_authentication (input, pstProfileInput->PDPAuthentication, NULL);

    if ( ( '\0' != pstProfileInput->Username[0] ) && ( strlen(pstProfileInput->Username) > 0 ) )
    qmi_message_wds_create_profile_input_set_username (input, pstProfileInput->Username, NULL);

    if ( ( '\0' != pstProfileInput->Password[0] ) && ( strlen(pstProfileInput->Password) > 0 ) )
    qmi_message_wds_create_profile_input_set_password (input, pstProfileInput->Password, NULL);

    qmi_message_wds_create_profile_input_set_roaming_disallowed_flag (input, pstProfileInput->bIsNoRoaming, NULL);
    qmi_message_wds_create_profile_input_set_apn_disabled_flag (input, pstProfileInput->bIsAPNDisabled, NULL);
    
    CELLULAR_HAL_DBG_PRINT ("%s Creating new profile, name:%s apn:%s user:%s pwd:%s roaming:%d APNDisabled:%d\n",
                                                            __FUNCTION__,
                                                            pstProfileInput->ProfileName,
                                                            pstProfileInput->APN,
                                                            pstProfileInput->Username,
                                                            pstProfileInput->Password,
                                                            pstProfileInput->bIsNoRoaming,
                                                            pstProfileInput->bIsAPNDisabled);

    qmi_client_wds_create_profile (QMI_CLIENT_WDS (wdsCtx->wdsClient),
                                    input,
                                    10,
                                    NULL,
                                    (GAsyncReadyCallback)cellular_hal_qmi_wds_create_profile,
                                    user_data);

    qmi_message_wds_create_profile_input_unref (input);
}

static void cellular_hal_qmi_wds_delete_profile ( QmiClientWds *wdsClient,
                                                        GAsyncResult *result,
                                                        gpointer  user_data )
{   
    QmiMessageWdsDeleteProfileOutput *output;
    GError *error = NULL;
    GTask *task = (GTask*)user_data;
    ContextProfileOperation    *pProfileCrCtx     = NULL;

    pProfileCrCtx  = g_task_get_task_data (task);

    output = qmi_client_wds_delete_profile_finish (wdsClient, result, &error);
    if (!output) 
    { 
        CELLULAR_HAL_DBG_PRINT ("%s Delete operation failed: %s\n", __FUNCTION__, error->message);
        g_error_free (error);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }

    if (!qmi_message_wds_delete_profile_output_get_result (output, &error)) 
    {        
        CELLULAR_HAL_DBG_PRINT ("%s Unable to delete profile: %s\n",
                                                            __FUNCTION__,
                                                            error->message);

        g_error_free (error);
        qmi_message_wds_delete_profile_output_unref (output);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }

    qmi_message_wds_delete_profile_output_unref (output);

    pProfileCrCtx->uiCurrentStep++;
    cellular_hal_qmi_profile_operation_step(task);
}

static void cellular_hal_qmi_wds_profile_delete (ContextWDSInfo  *wdsCtx, CellularProfileStruct *pstProfileInput, gpointer  user_data)
{
    QmiMessageWdsDeleteProfileInput *input = NULL;

    input = qmi_message_wds_delete_profile_input_new ();
    qmi_message_wds_delete_profile_input_set_profile_identifier (input, QMI_WDS_PROFILE_TYPE_3GPP, (guint8)pstProfileInput->ProfileID, NULL);

    CELLULAR_HAL_DBG_PRINT ("%s Deleting '%d' profile, name:%s apn:%s user:%s pwd:%s roaming:%d APNDisabled:%d\n",
                                                            __FUNCTION__,
                                                            pstProfileInput->ProfileID,
                                                            pstProfileInput->ProfileName,
                                                            pstProfileInput->APN,
                                                            pstProfileInput->Username,
                                                            pstProfileInput->Password,
                                                            pstProfileInput->bIsNoRoaming,
                                                            pstProfileInput->bIsAPNDisabled);

    qmi_client_wds_delete_profile ( QMI_CLIENT_WDS (wdsCtx->wdsClient),
                                    input,
                                    10,
                                    NULL,
                                    (GAsyncReadyCallback)cellular_hal_qmi_wds_delete_profile,
                                    user_data);
    qmi_message_wds_delete_profile_input_unref (input);
}

static void  cellular_hal_qmi_client_wds_set_lte_attach_pdn_list (QmiClientWds *wdsClient,
                                                                        GAsyncResult *result,
                                                                        gpointer  user_data)
{
    g_autoptr(QmiMessageWdsSetLteAttachPdnListOutput) output = NULL;
    g_autoptr(GError)                                 error = NULL;
    GTask                                            *task  = (GTask *)user_data;
    QMIContextStructPrivate                          *pstQMIContext   = NULL;
    ContextProfileOperation                          *pProfileCrCtx   = NULL;
    ContextWDSInfo                                   *wdsCtx          = NULL;
    CellularProfileStruct                            *pstInputProfile = NULL;

    pProfileCrCtx     = g_task_get_task_data (task);
    pstQMIContext     = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx            = &(gpstQMIContext->wdsCtx);
    pstInputProfile   = &(pProfileCrCtx->stProfileInput);

    output = qmi_client_wds_set_lte_attach_pdn_list_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("lte pdn attach operation failed: %s\n", error->message);
        goto NEXT;
    }

    if (!qmi_message_wds_set_lte_attach_pdn_list_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT ("Unable to set attach PDN list: %s\n", error->message);
        goto NEXT;
    }

    CELLULAR_HAL_DBG_PRINT ("%s PDN list updated with this '%d' profile successfully\n",__FUNCTION__,pstInputProfile->ProfileID);

NEXT:
    pProfileCrCtx->uiCurrentStep++;
    cellular_hal_qmi_profile_operation_step(task);
}

static void cellular_hal_qmi_client_wds_get_lte_attach_pdn_list (QmiClientWds *wdsClient,
                                                                       GAsyncResult *result,
                                                                       gpointer  user_data)
{
    g_autoptr(QmiMessageWdsGetLteAttachPdnListOutput) output = NULL;
    g_autoptr(GError)                                 error = NULL;
    GArray                                            *current_list = NULL;
    GArray                                            *pending_list = NULL;
    guint                                             i;
    GTask                                            *task  = (GTask *)user_data;
    QMIContextStructPrivate                          *pstQMIContext   = NULL;
    ContextWDSInfo                                   *wdsCtx          = NULL;
    ContextProfileOperation                          *pProfileCrCtx   = NULL;
    CellularProfileStruct                            *pstInputProfile = NULL;
    guint8                                            bIsCurrentListHasThatProfile = FALSE,
                                                      bIsPendingListHasThatProfile = FALSE;

    pProfileCrCtx     = g_task_get_task_data (task);
    pstQMIContext     = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx            = &(gpstQMIContext->wdsCtx);
    pstInputProfile   = &(pProfileCrCtx->stProfileInput);

    output = qmi_client_wds_get_lte_attach_pdn_list_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT ("%s Get lte pdn attach operation failed: %s\n", __FUNCTION__,error->message);
        goto NEXT;
    }

    if (!qmi_message_wds_get_lte_attach_pdn_list_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT ("%s Unable to get the list of LTE attach PDN: %s\n", __FUNCTION__,error->message);
        goto NEXT;
    }

    CELLULAR_HAL_DBG_PRINT ("%s Attach PDN list retrieved:\n",__FUNCTION__);

    qmi_message_wds_get_lte_attach_pdn_list_output_get_current_list (output, &current_list, NULL);
    if (!current_list || !current_list->len) 
    {
        CELLULAR_HAL_DBG_PRINT ("\tCurrent list: n/a\n");
    } 
    else 
    {
        CELLULAR_HAL_DBG_PRINT ("\tCurrent list: '");
        for (i = 0; i < current_list->len; i++)
        {
            guint16 ui16PDNProfile = g_array_index (current_list, guint16, i);

            CELLULAR_HAL_DBG_PRINT ("%s%u", i > 0 ? ", " : "", ui16PDNProfile);
            if( ui16PDNProfile == pstInputProfile->ProfileID )
            {
                bIsCurrentListHasThatProfile = TRUE;
            }
        }
        CELLULAR_HAL_DBG_PRINT ("'\n");
    }

    qmi_message_wds_get_lte_attach_pdn_list_output_get_pending_list (output, &pending_list, NULL);
    if (!pending_list || !pending_list->len) 
    {
        CELLULAR_HAL_DBG_PRINT ("\tPending list: n/a\n");
    } 
    else 
    {
        CELLULAR_HAL_DBG_PRINT ("\tPending list: '");
        for (i = 0; i < pending_list->len; i++)
        {
            guint16 ui16PDNProfile = g_array_index (pending_list, guint16, i);

            CELLULAR_HAL_DBG_PRINT ("%s%u", i > 0 ? ", " : "", ui16PDNProfile);

            if( ui16PDNProfile == pstInputProfile->ProfileID )
            {
                bIsPendingListHasThatProfile = TRUE;
            }
        }
        CELLULAR_HAL_DBG_PRINT ("'\n");
    }

    if( ( TRUE == bIsCurrentListHasThatProfile ) && ( FALSE == bIsPendingListHasThatProfile ) )
    {
        CELLULAR_HAL_DBG_PRINT("%s - PDN List already attached this '%d' profile\n",__FUNCTION__,pstInputProfile->ProfileID);
        goto NEXT;
    }
    else if ( TRUE == bIsPendingListHasThatProfile )
    {
        CELLULAR_HAL_DBG_PRINT("%s - PDN List already is pending in attachment list for this '%d' profile\n",__FUNCTION__,pstInputProfile->ProfileID);
        goto NEXT;
    }
    else
    {
        g_autoptr(QmiMessageWdsSetLteAttachPdnListInput) input = NULL;
        GArray                                           *current_pdn_list = NULL;
        guint16  profile_index;

        CELLULAR_HAL_DBG_PRINT("%s - PDN List doesn't have this '%d' profile. So attaching it\n",__FUNCTION__,pstInputProfile->ProfileID);

        //Multiple PDN not able to attach as part of Telit FW. Needs to discuss with telit on this.
        //g_array_append_val(current_list, pstInputProfile->ProfileID);
        
        profile_index = pstInputProfile->ProfileID;
        current_pdn_list = g_array_new (FALSE, FALSE, sizeof(guint16));
        g_array_append_val (current_pdn_list, profile_index);

        input = qmi_message_wds_set_lte_attach_pdn_list_input_new ();
        if (!qmi_message_wds_set_lte_attach_pdn_list_input_set_list (input, current_pdn_list, &error)) {
            CELLULAR_HAL_DBG_PRINT("%s Unabler to set attach PDN list: '%s'\n",__FUNCTION__,error->message);
            goto NEXT;
        }

        if (!qmi_message_wds_set_lte_attach_pdn_list_input_set_action (input, QMI_WDS_ATTACH_PDN_LIST_ACTION_DETACH_OR_PDN_DISCONNECT, &error)) {
            CELLULAR_HAL_DBG_PRINT("%s Unable to set attach PDN list action: '%s'\n",__FUNCTION__,error->message);
            goto NEXT;
        }

        qmi_client_wds_set_lte_attach_pdn_list (QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                input,
                                                10,
                                                NULL,
                                                (GAsyncReadyCallback) cellular_hal_qmi_client_wds_set_lte_attach_pdn_list,
                                                task);
        g_array_unref(current_pdn_list);
        return;
    }

NEXT:
    pProfileCrCtx->uiCurrentStep++;
    cellular_hal_qmi_profile_operation_step(task);
}

static void cellular_hal_qmi_client_wds_set_default_profile_number (QmiClientWds *wdsClient,
                                                                          GAsyncResult *result,
                                                                          gpointer  user_data)
{   
    g_autoptr(QmiMessageWdsSetDefaultProfileNumberOutput) output = NULL;
    g_autoptr(GError)                                     error = NULL;
    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate    *pstQMIContext     = NULL;
    ContextProfileOperation    *pProfileCrCtx     = NULL;
    ContextWDSInfo             *wdsCtx            = NULL;
    CellularProfileStruct      *pstInputProfile   = NULL;

    pProfileCrCtx       = g_task_get_task_data (task);
    pstQMIContext       = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx              = &(pstQMIContext->wdsCtx);
    pstInputProfile     = &(pProfileCrCtx->stProfileInput);

    output = qmi_client_wds_set_default_profile_number_finish (wdsClient, result, &error);
    if (!output) 
    { 
        CELLULAR_HAL_DBG_PRINT ("%s Default profile num set operation failed: %s\n", __FUNCTION__, error->message);
        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }
    
    if (!qmi_message_wds_set_default_profile_number_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s Unable to set default profile number: %s\n",
                                    __FUNCTION__,
                                    error->message);
                                        
        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }
    
    CELLULAR_HAL_DBG_PRINT ("%s - Default profile number '%d' updated\n",__FUNCTION__,pstInputProfile->ProfileID);

    //Copy Input Profile information Default Profile
    memset(&(wdsCtx->stDefaultProfileFromModem), 0, sizeof(CellularProfileStruct));
    memcpy(&(wdsCtx->stDefaultProfileFromModem), pstInputProfile, sizeof(CellularProfileStruct));

    pProfileCrCtx->uiCurrentStep++;
    cellular_hal_qmi_profile_operation_step(task);
}

static void  cellular_hal_qmi_client_wds_get_profile_settings ( QmiClientWds *wdsClient,
                                                                      GAsyncResult *result,
                                                                      gpointer  user_data)
{
    QmiMessageWdsGetProfileSettingsOutput *output;
    GError *error = NULL;
    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate    *pstQMIContext     = NULL;
    ContextProfileOperation    *pProfileCrCtx     = NULL;
    ContextWDSInfo             *wdsCtx            = NULL;
    CellularProfileStruct      *pstDefProfFromModem = NULL;

    pProfileCrCtx       = g_task_get_task_data (task);
    pstQMIContext       = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx              = &(pstQMIContext->wdsCtx);
    pstDefProfFromModem = &(wdsCtx->stDefaultProfileFromModem);

    output = qmi_client_wds_get_profile_settings_finish (wdsClient, result, &error);
    if (!output) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s Get profile settings operation failed: %s\n", __FUNCTION__,error->message);
        g_error_free (error);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
    } 
    else if (!qmi_message_wds_get_profile_settings_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s Unable to get profile settings: %s\n",
                                        __FUNCTION__,
                                        error->message);

        g_error_free (error);
        qmi_message_wds_get_profile_settings_output_unref (output);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
    }
    else 
    {
        const gchar *str;
        guint8 context_number;
        QmiWdsPdpType pdp_type;
        QmiWdsAuthentication auth;
        QmiWdsApnTypeMask apn_type;
        gboolean flag;

        if (qmi_message_wds_get_profile_settings_output_get_profile_name (output, &str, NULL))
        {
            snprintf(pstDefProfFromModem->ProfileName, sizeof(pstDefProfFromModem->ProfileName), "%s",str);
            CELLULAR_HAL_DBG_PRINT ("\t\tProfile Name: '%s'\n", str);
        }

        if (qmi_message_wds_get_profile_settings_output_get_apn_name (output, &str, NULL))
        {
            snprintf(pstDefProfFromModem->APN, sizeof(pstDefProfFromModem->APN), "%s",str);
            CELLULAR_HAL_DBG_PRINT ("\t\tAPN Name: '%s'\n", str);
        }
            
        if (qmi_message_wds_get_profile_settings_output_get_apn_type_mask (output, &apn_type, NULL)) 
        {
            g_autofree gchar *aux = NULL;

            aux = qmi_wds_apn_type_mask_build_string_from_mask (apn_type);
            CELLULAR_HAL_DBG_PRINT ("\t\tAPN type: '%s'\n", CELLULAR_QMI_VALIDATE_MASK_NONE (aux));
        }

        if (qmi_message_wds_get_profile_settings_output_get_pdp_type (output, &pdp_type, NULL))
        {
            if( QMI_WDS_PDP_TYPE_IPV4 == pdp_type )
            {
                pstDefProfFromModem->PDPType = CELLULAR_PDP_TYPE_IPV4;
            }
            else if( QMI_WDS_PDP_TYPE_PPP == pdp_type )
            {
                pstDefProfFromModem->PDPType = CELLULAR_PDP_TYPE_PPP;
            }
            else if( QMI_WDS_PDP_TYPE_IPV6 == pdp_type )
            {
                pstDefProfFromModem->PDPType = CELLULAR_PDP_TYPE_IPV6;
            }
            else if( QMI_WDS_PDP_TYPE_IPV4_OR_IPV6 == pdp_type )
            {
                pstDefProfFromModem->PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
            }
            else
            {
                //Fallback
                pstDefProfFromModem->PDPType = CELLULAR_PDP_TYPE_IPV4;
            }

            CELLULAR_HAL_DBG_PRINT ("\t\tPDP type: '%s'\n", qmi_wds_pdp_type_get_string (pdp_type));            
        }
            
        if (qmi_message_wds_get_profile_settings_output_get_pdp_context_number (output, &context_number, NULL))
        {
            pstDefProfFromModem->PDPContextNumber = context_number;
            CELLULAR_HAL_DBG_PRINT ("\t\tPDP ContextNumber: '%d'\n", context_number);
        }
            
        if (qmi_message_wds_get_profile_settings_output_get_username (output, &str, NULL))
        {
            snprintf(pstDefProfFromModem->Username, sizeof(pstDefProfFromModem->Username), "%s",str);
            CELLULAR_HAL_DBG_PRINT ("\t\tUsername: '%s'\n", str);
        }
            
        if (qmi_message_wds_get_profile_settings_output_get_password (output, &str, NULL))
        {
            snprintf(pstDefProfFromModem->Password, sizeof(pstDefProfFromModem->Password), "%s",str);
            CELLULAR_HAL_DBG_PRINT ("\t\tPassword: '%s'\n", str);
        }
            
        if (qmi_message_wds_get_profile_settings_output_get_authentication (output, &auth, NULL)) 
        {
            g_autofree gchar *aux = NULL;

            if( QMI_WDS_AUTHENTICATION_NONE == auth )
            {
                pstDefProfFromModem->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_NONE;
            }
            else  if( QMI_WDS_AUTHENTICATION_PAP == auth )
            {
                pstDefProfFromModem->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_PAP;
            }
            else  if( QMI_WDS_AUTHENTICATION_CHAP == auth )
            {
                pstDefProfFromModem->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_CHAP;
            }
            else
            {
                //Fallback
                pstDefProfFromModem->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_NONE;
            }

            aux = qmi_wds_authentication_build_string_from_mask (auth);
            CELLULAR_HAL_DBG_PRINT ("\t\tAuthentication: '%s'\n", CELLULAR_QMI_VALIDATE_MASK_NONE (aux));
        }
        if (qmi_message_wds_get_profile_settings_output_get_roaming_disallowed_flag (output, &flag, NULL))
        {
            pstDefProfFromModem->bIsNoRoaming = flag;
            CELLULAR_HAL_DBG_PRINT ("\t\tNo Roaming: '%s'\n", flag ? "yes" : "no");
        }
            
        if (qmi_message_wds_get_profile_settings_output_get_apn_disabled_flag (output, &flag, NULL))
        {
            pstDefProfFromModem->bIsAPNDisabled = flag;
            CELLULAR_HAL_DBG_PRINT ("\t\tAPN disabled: '%s'\n", flag ? "yes" : "no");
        }
            
        qmi_message_wds_get_profile_settings_output_unref (output);

        wdsCtx->bIsDefaultProfileFromModemCached = TRUE;
        
        pProfileCrCtx->uiCurrentStep++;
        cellular_hal_qmi_profile_operation_step(task);
    }
}

static void  cellular_hal_qmi_client_wds_default_profile_num (QmiClientWds *wdsClient,
                                                                    GAsyncResult *result,
                                                                    gpointer  user_data)
{
    g_autoptr(QmiMessageWdsGetDefaultProfileNumberOutput) output = NULL;
    g_autoptr(GError)                                     error = NULL;
    guint8                                                profile_num;
    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate    *pstQMIContext     = NULL;
    ContextProfileOperation    *pProfileCrCtx     = NULL;
    ContextWDSInfo             *wdsCtx            = NULL;
    CellularProfileStruct      *pstDefProfFromModem = NULL;

    pProfileCrCtx       = g_task_get_task_data (task);
    pstQMIContext       = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx              = &(pstQMIContext->wdsCtx);
    pstDefProfFromModem = &(wdsCtx->stDefaultProfileFromModem);
    memset(pstDefProfFromModem, 0, sizeof(CellularProfileStruct));

    wdsCtx->bIsDefaultProfileFromModemCached = FALSE;

    output = qmi_client_wds_get_default_profile_number_finish (wdsClient, result, &error);
    if (!output) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s Get default profile number operation failed: %s\n", __FUNCTION__,error->message);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }

    if (!qmi_message_wds_get_default_profile_number_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s Unable to get default profile number: %s\n",
                                    __FUNCTION__,
                                    error->message);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }

    if (qmi_message_wds_get_default_profile_number_output_get_index (output, &profile_num, NULL))
    {
        QmiMessageWdsGetProfileSettingsInput *input;

        pstDefProfFromModem->ProfileID = profile_num;
        CELLULAR_HAL_DBG_PRINT ("%s - Default profile number is '%d'\n", __FUNCTION__, pstDefProfFromModem->ProfileID);
        
        input = qmi_message_wds_get_profile_settings_input_new ();
        qmi_message_wds_get_profile_settings_input_set_profile_id ( input,
                                                                    QMI_WDS_PROFILE_TYPE_3GPP,
                                                                    pstDefProfFromModem->ProfileID,
                                                                    NULL);
        qmi_client_wds_get_profile_settings (wdsClient,
                                            input,
                                            3,
                                            NULL,
                                            (GAsyncReadyCallback)cellular_hal_qmi_client_wds_get_profile_settings,
                                            task); 
        qmi_message_wds_get_profile_settings_input_unref (input);
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT ("%s - Unable to read default profile number\n",__FUNCTION__);
        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
    }
}

static void cellular_hal_qmi_modify_profile (QmiClientWds *wdsClient,
                                                   GAsyncResult *result,
                                                   gpointer  user_data)
{   
    QmiMessageWdsModifyProfileOutput *output;
    GError *error = NULL;
    GTask        *task  = (GTask *)user_data;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextProfileOperation    *pProfileCrCtx   = NULL;
    ContextWDSInfo             *wdsCtx          = NULL;
    CellularProfileStruct      *pstProfileInput = NULL;

    pProfileCrCtx     = g_task_get_task_data (task);
    pstQMIContext     = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx            = &(pstQMIContext->wdsCtx);
    pstProfileInput   = &(pProfileCrCtx->stProfileInput);

    output = qmi_client_wds_modify_profile_finish (wdsClient, result, &error);
    if (!output) 
    { 
        CELLULAR_HAL_DBG_PRINT("%s Modify profile operation failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }
    
    if (!qmi_message_wds_modify_profile_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT("Unable to modify profile: %s\n",
                                                    __FUNCTION__,
                                                    error->message);
        g_error_free (error);
        qmi_message_wds_modify_profile_output_unref (output);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }

    qmi_message_wds_modify_profile_output_unref(output);
    CELLULAR_HAL_DBG_PRINT("%s - Profile(%d) successfully modified\n",__FUNCTION__,pstProfileInput->ProfileID);

    pProfileCrCtx->uiCurrentStep++;
    cellular_hal_qmi_profile_operation_step(task);
}

static void cellular_hal_qmi_client_wds (QmiDevice *device, GAsyncResult *result, gpointer  user_data)
{
    GError       *error = NULL;
    GTask        *task  = (GTask *)user_data;
    QmiClient    *wdsclient;
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextProfileOperation    *pProfileCrCtx = NULL;
    ContextWDSInfo             *wdsCtx          = NULL;
    cellular_device_profile_status_api_callback device_profile_status_cb;

    pProfileCrCtx     = g_task_get_task_data (task);
    pstQMIContext     = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx            = &(pstQMIContext->wdsCtx);

    wdsclient = qmi_device_allocate_client_finish (device, result, &error);
    if (!wdsclient) {
        CELLULAR_HAL_DBG_PRINT ("%s Failed to create client for the '%s' service: %s\n",
                                __FUNCTION__,
                                qmi_service_get_string (QMI_SERVICE_WDS),
                                error->message);

        pProfileCrCtx->uiCurrentStep = pProfileCrCtx->uiLastStep;
        cellular_hal_qmi_profile_operation_step(task);
        return;
    }

    //Store WDS context 
    wdsCtx->wdsClient        = wdsclient;
    wdsCtx->bIsValidwdsClient = TRUE;

    pProfileCrCtx->uiCurrentStep++;
    cellular_hal_qmi_profile_operation_step(task);
}

static void cellular_hal_qmi_profile_operation_step( GTask *task )
{
    QMIContextStructPrivate    *pstQMIContext   = NULL;
    ContextProfileOperation    *pProfileCrCtx = NULL;
    ContextWDSInfo             *wdsCtx          = NULL;

    pProfileCrCtx     = g_task_get_task_data (task);
    pstQMIContext     = (QMIContextStructPrivate*)pProfileCrCtx->vpPrivateData;
    wdsCtx            = &(pstQMIContext->wdsCtx);

    switch ( pProfileCrCtx->uiCurrentStep ) 
    {
        case WDS_PROFILE_CREATE_BEGIN:
        case WDS_PROFILE_DELETE_BEGIN:
        case WDS_PROFILE_MODIFY_BEGIN:
        case WDS_PROFILE_GETLIST_BEGIN:
        {
            pProfileCrCtx->uiCurrentStep++;
            /* fall through */
        }
        case WDS_PROFILE_CREATE_ALLOCATE_WDS_CLIENT:
        case WDS_PROFILE_DELETE_ALLOCATE_WDS_CLIENT:
        case WDS_PROFILE_MODIFY_ALLOCATE_WDS_CLIENT:
        case WDS_PROFILE_GETLIST_ALLOCATE_WDS_CLIENT:
        {   
            //Check whether WDS client is valid or not. If not then needs to create new client for profile
            if ( FALSE == wdsCtx->bIsValidwdsClient )
            {
                CELLULAR_HAL_DBG_PRINT("%s %d - Allocating allocating WDS client\n",__FUNCTION__,__LINE__);

                qmi_device_allocate_client (pstQMIContext->qmiDevice,
                                            QMI_SERVICE_WDS,
                                            QMI_CID_NONE,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback) cellular_hal_qmi_client_wds,
                                            task);
                return;
            }

            pProfileCrCtx->uiCurrentStep++;
            /* fall through */
        }  
        case WDS_PROFILE_CREATE_READ_DEFAULT_SETTINGS:
        case WDS_PROFILE_GETLIST_READ_DEFAULT_SETTINGS:
        {
            if ( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest ) ||
                 ( WDS_PROFILE_OPERATION_LIST == pProfileCrCtx->enProfileOperationRequest ) )
            {
                if ( FALSE == wdsCtx->bIsDefaultProfileFromModemCached )
                {
                    g_autoptr(QmiMessageWdsGetDefaultProfileNumberInput)  input = NULL;
                    const gchar                                          *str;
                    
                    input = qmi_message_wds_get_default_profile_number_input_new ();

                    /* Always use profile family 'tethered' */
                    qmi_message_wds_get_default_profile_number_input_set_profile_type ( input,
                                                                                        QMI_WDS_PROFILE_TYPE_3GPP,
                                                                                        QMI_WDS_PROFILE_FAMILY_TETHERED,
                                                                                        NULL);
                    
                    CELLULAR_HAL_DBG_PRINT ("%s Querying Default Profile Number\n",__FUNCTION__);
                    qmi_client_wds_get_default_profile_number ( QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                                input,
                                                                10,
                                                                NULL,
                                                                (GAsyncReadyCallback)cellular_hal_qmi_client_wds_default_profile_num,
                                                                task);

                    return;
                }

                pProfileCrCtx->uiCurrentStep++;
                //fall through
            }
        }
        case WDS_PROFILE_DELETE:
        {
            if ( WDS_PROFILE_OPERATION_DELETE == pProfileCrCtx->enProfileOperationRequest )
            {
                CellularProfileStruct    *pstInputProfile   = &(pProfileCrCtx->stProfileInput);

                cellular_hal_qmi_wds_profile_delete( wdsCtx, 
                                                     pstInputProfile, 
                                                     task );
                return;
            }
        }
        case WDS_PROFILE_CREATE_READ_PROFILE_LIST:
        case WDS_PROFILE_GETLIST:
        case WDS_PROFILE_DELETE_RESYNC_PROFILE_LIST:
        {
            if ( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest ) ||
                 ( WDS_PROFILE_OPERATION_DELETE == pProfileCrCtx->enProfileOperationRequest ) ||
                 ( WDS_PROFILE_OPERATION_LIST == pProfileCrCtx->enProfileOperationRequest ) )
            {
                ContextWDSInfo  *wdsCtx = &(pstQMIContext->wdsCtx);

                //Verify whether profile list available or not and force sync after delete
                if( ( NULL == wdsCtx->pstProfileList ) ||
                    ( WDS_PROFILE_OPERATION_LIST == pProfileCrCtx->enProfileOperationRequest ) ||
                    ( WDS_PROFILE_OPERATION_DELETE == pProfileCrCtx->enProfileOperationRequest ) )
                {
                    QmiMessageWdsGetProfileListInput *input;
                    ContextProfileList  *iter = NULL;

                    iter = (ContextProfileList*)malloc(sizeof(ContextProfileList));
                    iter->wdsCtx = wdsCtx;
                    wdsCtx->bIsProfileListCollectionDone = FALSE;

                    if( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest )
                    {
                        iter->enQuerySource = WDS_PROFILE_GETLIST_VIA_PROFILE_CREATE; 
                    }
                    else if( WDS_PROFILE_OPERATION_DELETE == pProfileCrCtx->enProfileOperationRequest )
                    {
                        iter->enQuerySource = WDS_PROFILE_GETLIST_VIA_PROFILE_DELETE; 
                    }
                    else if( WDS_PROFILE_OPERATION_LIST == pProfileCrCtx->enProfileOperationRequest )
                    {
                        iter->enQuerySource = WDS_PROFILE_GETLIST_VIA_DML; 
                    }

                    iter->vpPrivateData = task;

                    CELLULAR_HAL_DBG_PRINT("%s - Querying profile list from profile operation, mode:%d\n",__FUNCTION__,pProfileCrCtx->enProfileOperationRequest);

                    input = qmi_message_wds_get_profile_list_input_new();
                    qmi_message_wds_get_profile_list_input_set_profile_type (input, QMI_WDS_PROFILE_TYPE_3GPP, NULL);
                    qmi_client_wds_get_profile_list (QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                    input,
                                                    10,
                                                    NULL,
                                                    (GAsyncReadyCallback)cellular_hal_qmi_get_profile_list_cb,
                                                    iter);
                    qmi_message_wds_get_profile_list_input_unref (input);
                    return;
                }

                //fall through
                pProfileCrCtx->uiCurrentStep++;
            }
        }
        case WDS_PROFILE_CREATE_VERIFY_PROFILE_PRESENT:
        case WDS_PROFILE_MODIFY_VERIFY_PROFILE_PRESENT:
        {
            if ( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest ) ||
                 ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest ) )
            {
                if( NULL != wdsCtx->pstProfileList )
                {
                    CellularProfileStruct    *pstProfileList    = wdsCtx->pstProfileList;
                    CellularProfileStruct    *pstInputProfile   = &(pProfileCrCtx->stProfileInput);

                    //Verify whether proposing profile is already existing or not
                    pProfileCrCtx->bIsProfileAlreadyPresent = FALSE;
                    if ( NULL != pstInputProfile )
                    {
                        int i;

                        for( i = 0; i < wdsCtx->ui8ProfileCount; i++ )
                        {
                            if ( ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest ) &&
                                 ( pstInputProfile->ProfileID == pstProfileList[i].ProfileID ) )
                            {
                                //we need to ignore verifying for same profile during profile modify request
                                continue;
                            }

                            if ( ( 0 == strcasecmp(pstProfileList[i].APN, pstInputProfile->APN) ) && \
                                ( 0 == strcmp(pstProfileList[i].Username, pstInputProfile->Username) )  && \
                                ( 0 == strcmp(pstProfileList[i].Password, pstInputProfile->Password) ) && \
                                ( pstProfileList[i].PDPType == pstInputProfile->PDPType ) && \
                                ( pstProfileList[i].PDPAuthentication == pstInputProfile->PDPAuthentication ) )
                            {
                                pProfileCrCtx->bIsProfileAlreadyPresent = TRUE;
                                CELLULAR_HAL_DBG_PRINT("%s - Matched with existing profile(%d). so ignore profile creation/modification\n",__FUNCTION__,pstProfileList[i].ProfileID);

                                //Copy Default profile from Modem to Current Profile
                                if( TRUE == pstInputProfile->bIsThisDefaultProfile )
                                {
                                    pstProfileList[i].bIsThisDefaultProfile = TRUE;
                                }

                                memset(pstInputProfile, 0, sizeof(CellularProfileStruct));
                                memcpy(pstInputProfile, &(pstProfileList[i]), sizeof(CellularProfileStruct));
                                break;
                            }
                        }
                    }
                }

                //fall through
                pProfileCrCtx->uiCurrentStep++;
            }
        }
        case WDS_PROFILE_CREATE:
        {
            if ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest )
            {
                CellularProfileStruct    *pstInputProfile   = &(pProfileCrCtx->stProfileInput);

                //If it is new one then needs to create
                if( FALSE == pProfileCrCtx->bIsProfileAlreadyPresent )
                {
                    cellular_hal_qmi_wds_profile_create( wdsCtx, 
                                                         pstInputProfile, 
                                                         task );
                    return;
                }

                pProfileCrCtx->uiCurrentStep++;
                //fall through
            }
        }
        case WDS_PROFILE_MODIFY:
        {
            if ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest )
            {
                //If it is not existing one then no needs to modify
                if( FALSE == pProfileCrCtx->bIsProfileAlreadyPresent )
                {
                    QmiMessageWdsModifyProfileInput *input = NULL;
                    CellularProfileStruct    *pstInputProfile   = &(pProfileCrCtx->stProfileInput);

                    input = qmi_message_wds_modify_profile_input_new ();

                    if (0 < pstInputProfile->PDPContextNumber)
                    qmi_message_wds_modify_profile_input_set_pdp_context_number (input, pstInputProfile->PDPContextNumber, NULL);
                    qmi_message_wds_modify_profile_input_set_profile_identifier (input, pstInputProfile->ProfileType, pstInputProfile->ProfileID, NULL);
                    qmi_message_wds_modify_profile_input_set_pdp_type (input, pstInputProfile->PDPType, NULL);
                    //qmi_message_wds_modify_profile_input_set_apn_type_mask (input, props.apn_type, NULL);
                    qmi_message_wds_modify_profile_input_set_profile_name (input, pstInputProfile->ProfileName, NULL);
                    qmi_message_wds_modify_profile_input_set_apn_name (input, pstInputProfile->APN, NULL);
                    qmi_message_wds_modify_profile_input_set_authentication (input, pstInputProfile->PDPAuthentication, NULL);
                    qmi_message_wds_modify_profile_input_set_username (input, pstInputProfile->Username, NULL);
                    qmi_message_wds_modify_profile_input_set_password (input, pstInputProfile->Password, NULL);
                    qmi_message_wds_modify_profile_input_set_roaming_disallowed_flag (input, pstInputProfile->bIsNoRoaming, NULL);
                    qmi_message_wds_modify_profile_input_set_apn_disabled_flag (input, pstInputProfile->bIsAPNDisabled, NULL);

                    qmi_client_wds_modify_profile ( QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                    input,
                                                    10,
                                                    NULL,
                                                    (GAsyncReadyCallback)cellular_hal_qmi_modify_profile,
                                                    task);
                    qmi_message_wds_modify_profile_input_unref (input);
                    return; 
                }
            }
        }
        case WDS_PROFILE_CREATE_SET_DEFAULT_PROFILE:
        case WDS_PROFILE_MODIFY_SET_DEFAULT_PROFILE:
        {
            if ( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest ) ||
                 ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest ) )
            {
                CellularProfileStruct    *pstInputProfile   = &(pProfileCrCtx->stProfileInput);

                if ( TRUE == pstInputProfile->bIsThisDefaultProfile )
                {
                    if( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest )  ||
                        ( ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest ) &&
                          ( FALSE == pProfileCrCtx->bIsProfileAlreadyPresent ) ) )
                    {
                        g_autoptr(QmiMessageWdsSetDefaultProfileNumberInput) input = NULL;
                        g_autoptr(GError)                                    error = NULL;

                        CELLULAR_HAL_DBG_PRINT("%s - Setting default profile number as '%d'\n",__FUNCTION__,pstInputProfile->ProfileID);
                    
                        input = qmi_message_wds_set_default_profile_number_input_new ();
                        qmi_message_wds_set_default_profile_number_input_set_profile_identifier ( input,
                                                                                                pstInputProfile->ProfileType,
                                                                                                QMI_WDS_PROFILE_FAMILY_TETHERED,
                                                                                                pstInputProfile->ProfileID,
                                                                                                &error);

                        qmi_client_wds_set_default_profile_number ( QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                                    input,
                                                                    10,
                                                                    NULL,
                                                                    (GAsyncReadyCallback)cellular_hal_qmi_client_wds_set_default_profile_number,
                                                                    task);
                        return;
                    }
                }
            }
        }
        case WDS_PROFILE_CREATE_VERIFY_AND_SET_LTE_PDN_ATTACH:
        {
#if 0 //Don't perform attach opeartion..Modem takes care of it.
            if ( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest ) ||
                 ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest ) )
            {
                CellularProfileStruct    *pstInputProfile   = &(pProfileCrCtx->stProfileInput);

                if ( TRUE == pstInputProfile->bIsThisDefaultProfile )
                {
                    if( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest )  ||
                        ( ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest ) &&
                          ( FALSE == pProfileCrCtx->bIsProfileAlreadyPresent ) ) )
                    {
                        qmi_client_wds_get_lte_attach_pdn_list ( QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                                NULL,
                                                                10,
                                                                NULL,
                                                                (GAsyncReadyCallback)cellular_hal_qmi_client_wds_get_lte_attach_pdn_list,
                                                                task);
                        return;
                    }
                }
            }
#endif
        }
        case WDS_PROFILE_CREATE_NOTIFY:
        case WDS_PROFILE_DELETE_NOTIFY:
        case WDS_PROFILE_MODIFY_NOTIFY:
        {
            if ( ( WDS_PROFILE_OPERATION_CREATE == pProfileCrCtx->enProfileOperationRequest ) || 
                 ( WDS_PROFILE_OPERATION_DELETE == pProfileCrCtx->enProfileOperationRequest ) ||
                 ( WDS_PROFILE_OPERATION_MODIFY == pProfileCrCtx->enProfileOperationRequest ) )
            {
                CellularProfileStruct    *pstInputProfile   = &(pProfileCrCtx->stProfileInput);

                //Send Profile ready status if CB is not null
                if( NULL != pProfileCrCtx->device_profile_status_cb )
                {
                    if( WDS_PROFILE_OPERATION_DELETE == pProfileCrCtx->enProfileOperationRequest )
                    {
                        CELLULAR_HAL_DBG_PRINT("%s - Profile(%d, %s) %s now deleted and sending status via CB\n", __FUNCTION__,pstInputProfile->ProfileID,pstInputProfile->ProfileName);
                        pProfileCrCtx->device_profile_status_cb( pstInputProfile->ProfileName, pstInputProfile->PDPType, DEVICE_PROFILE_STATUS_DELETED );
                    }
                    else
                    {
                        CELLULAR_HAL_DBG_PRINT("%s - Profile(%d, %s) %s now and sending status via CB\n", __FUNCTION__,pstInputProfile->ProfileID,pstInputProfile->ProfileName,((pProfileCrCtx->enProfileOperationRequest) ?  "created" : "modified"));
                        pProfileCrCtx->device_profile_status_cb( pstInputProfile->ProfileName, pstInputProfile->PDPType, DEVICE_PROFILE_STATUS_READY );
                    }
                }

                //fall through
                pProfileCrCtx->uiCurrentStep++;
            }
        }
        case WDS_PROFILE_CREATE_END:
        case WDS_PROFILE_DELETE_END:
        case WDS_PROFILE_MODIFY_END:
        case WDS_PROFILE_GETLIST_END:
        default:
        {
            pProfileCrCtx->uiCurrentStep++;
            g_object_unref (task);
        }
    }
}

static void cellular_hal_qmi_profile_operation_context_free( ContextProfileOperation *pProfileCrCtx )
{
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources for profile operation mode:%d\n",__FUNCTION__,pProfileCrCtx->enProfileOperationRequest);

    if( NULL != pProfileCrCtx)
    {
        g_slice_free(ContextProfileOperation, pProfileCrCtx);
        pProfileCrCtx  = NULL;
    }
}

int cellular_hal_qmi_profile_operation(QMIHALProfileOperationInputStruct *pstProfileOperationInput)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextProfileOperation    *pstProfileCrCtx = NULL;
        ContextWDSInfo             *wdsCtx = &(gpstQMIContext->wdsCtx);
        GTask                      *task;
        QMIHALProfileOperationUnion     *punProfileOperation = &(pstProfileOperationInput->unProfileOperation);

        pstProfileCrCtx = g_slice_new0 (ContextProfileOperation);

        if( WDS_PROFILE_OPERATION_CREATE == pstProfileOperationInput->enProfileOperationInput )
        {
            QMIHALProfileCreateDeleteModifyStruct *pstProfileCreateDeleteModify = &(punProfileOperation->stProfileCreateDeleteModify);

            pstProfileCrCtx->uiCurrentStep = WDS_PROFILE_CREATE_BEGIN;
            pstProfileCrCtx->uiLastStep    = WDS_PROFILE_CREATE_END;

            if(NULL != pstProfileCreateDeleteModify->pstProfileInput)
            {
                memcpy(&(pstProfileCrCtx->stProfileInput), pstProfileCreateDeleteModify->pstProfileInput, sizeof(CellularProfileStruct));
            }
            else
            {
                memset(&(pstProfileCrCtx->stProfileInput), 0, sizeof(CellularProfileStruct));
                memcpy(&(pstProfileCrCtx->stProfileInput), &(wdsCtx->stDefaultProfileFromDB), sizeof(CellularProfileStruct));
            }

            pstProfileCrCtx->device_profile_status_cb  = pstProfileCreateDeleteModify->device_profile_status_cb;
        }
        else if( WDS_PROFILE_OPERATION_DELETE == pstProfileOperationInput->enProfileOperationInput )
        {
            QMIHALProfileCreateDeleteModifyStruct *pstProfileCreateDeleteModify = &(punProfileOperation->stProfileCreateDeleteModify);

            pstProfileCrCtx->uiCurrentStep = WDS_PROFILE_DELETE_BEGIN;
            pstProfileCrCtx->uiLastStep    = WDS_PROFILE_DELETE_END;
            memcpy(&(pstProfileCrCtx->stProfileInput), pstProfileCreateDeleteModify->pstProfileInput, sizeof(CellularProfileStruct));

            pstProfileCrCtx->device_profile_status_cb  = pstProfileCreateDeleteModify->device_profile_status_cb;
        }
        else if( WDS_PROFILE_OPERATION_MODIFY == pstProfileOperationInput->enProfileOperationInput )
        {
            QMIHALProfileCreateDeleteModifyStruct *pstProfileCreateDeleteModify = &(punProfileOperation->stProfileCreateDeleteModify);

            //If Modify input profile is NULL then no need to proceed.
            if( NULL == pstProfileCreateDeleteModify->pstProfileInput )
            {
                if( NULL != pstProfileCrCtx )
                {
                    g_slice_free(ContextProfileOperation, pstProfileCrCtx);
                    pstProfileCrCtx  = NULL;
                }

                CELLULAR_HAL_DBG_PRINT("%s - Profile Operation Error, Operation:%d\n", __FUNCTION__, pstProfileOperationInput->enProfileOperationInput);
                return RETURN_ERROR;
            }

            pstProfileCrCtx->uiCurrentStep = WDS_PROFILE_MODIFY_BEGIN;
            pstProfileCrCtx->uiLastStep    = WDS_PROFILE_MODIFY_END;
            memcpy(&(pstProfileCrCtx->stProfileInput), pstProfileCreateDeleteModify->pstProfileInput, sizeof(CellularProfileStruct));
            pstProfileCrCtx->device_profile_status_cb  = pstProfileCreateDeleteModify->device_profile_status_cb;
        }
        else if( WDS_PROFILE_OPERATION_LIST == pstProfileOperationInput->enProfileOperationInput )
        {
            QMIHALProfileGetListStruct *pstProfileGetList = &(punProfileOperation->stProfileGetList);

            pstProfileCrCtx->uiCurrentStep = WDS_PROFILE_GETLIST_BEGIN;
            pstProfileCrCtx->uiLastStep    = WDS_PROFILE_GETLIST_END;

            wdsCtx->bIsProfileListCollectionDone = FALSE;
        }
        else
        {
            if( NULL != pstProfileCrCtx )
            {
                g_slice_free(ContextProfileOperation, pstProfileCrCtx);
                pstProfileCrCtx  = NULL;
            }

            CELLULAR_HAL_DBG_PRINT("%s - Profile Operation Error, Operation:%d\n", __FUNCTION__, pstProfileOperationInput->enProfileOperationInput);
            return RETURN_ERROR;
        }

        pstProfileCrCtx->vpPrivateData    = (void*) gpstQMIContext;
        pstProfileCrCtx->enProfileOperationRequest =  pstProfileOperationInput->enProfileOperationInput;
 
        task = g_task_new (NULL, NULL, NULL, NULL);
        g_task_set_task_data (task, (gpointer)pstProfileCrCtx, (GDestroyNotify)cellular_hal_qmi_profile_operation_context_free);

        CELLULAR_HAL_DBG_PRINT("%s - Profile Operation(%d) is starting Task:0x%p\n",__FUNCTION__, pstProfileCrCtx->enProfileOperationRequest, task);
        cellular_hal_qmi_profile_operation_step(task);

        //ProfileList Get
        if( WDS_PROFILE_OPERATION_LIST == pstProfileOperationInput->enProfileOperationInput )
        {
            QMIHALProfileGetListStruct *pstProfileGetList = &(punProfileOperation->stProfileGetList);
            int                         iLapsedSeconds    = 0;

            //Wait here till profile list 
            while ( 1 )
            {
                if( TRUE == wdsCtx->bIsProfileListCollectionDone )
                {
                    wdsCtx->bIsProfileListCollectionDone = FALSE;

                    CELLULAR_HAL_DBG_PRINT("%s Profile List Collection Done. Total Profile Count is %d\n",__FUNCTION__,wdsCtx->ui8ProfileCount);

                    *(pstProfileGetList->profile_count)  = wdsCtx->ui8ProfileCount;

                    if( 0 < wdsCtx->ui8ProfileCount )
                    {
                        CellularProfileStruct *pstTmpProfileOutput = NULL;

                        pstTmpProfileOutput  = ( CellularProfileStruct* ) malloc( sizeof(CellularProfileStruct) * wdsCtx->ui8ProfileCount );
                        memcpy(pstTmpProfileOutput, wdsCtx->pstProfileList, sizeof(CellularProfileStruct) * wdsCtx->ui8ProfileCount );
                        *(pstProfileGetList->ppstProfileOutput) = pstTmpProfileOutput;
                    }
                    else
                    {
                        *(pstProfileGetList->ppstProfileOutput) = NULL;

                        //Free existing resource
                        if( NULL != wdsCtx->pstProfileList )
                        {
                            free(wdsCtx->pstProfileList);
                            wdsCtx->pstProfileList = NULL;
                        }
                    }

                    break;
                }
                else if( iLapsedSeconds >= CELLULAR_QMI_PROFILELIST_VIA_DML_MAX_WAITIME )  
                {
                    *(pstProfileGetList->profile_count) = 0;
                    *(pstProfileGetList->ppstProfileOutput) = NULL;

                    CELLULAR_HAL_DBG_PRINT("%s - Timeout during ProfileList Query so sending empty list\n", __FUNCTION__);
                    break;
                }

                sleep(1);
                iLapsedSeconds++;
            }
        }
    }
    else
    {
        if( ( WDS_PROFILE_OPERATION_CREATE == pstProfileOperationInput->enProfileOperationInput ) ||
            ( WDS_PROFILE_OPERATION_DELETE == pstProfileOperationInput->enProfileOperationInput ) ||
            ( WDS_PROFILE_OPERATION_MODIFY == pstProfileOperationInput->enProfileOperationInput ) )
        {
            QMIHALProfileOperationUnion     *punProfileOperation = &(pstProfileOperationInput->unProfileOperation);
            QMIHALProfileCreateDeleteModifyStruct *pstProfileCreateDeleteModify = &(punProfileOperation->stProfileCreateDeleteModify);

            //Send Profile not ready status if CB is not null
            if( NULL != pstProfileCreateDeleteModify->device_profile_status_cb )
            {
                CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so profile can't be chosen now and sending status via CB\n", __FUNCTION__, gpstQMIContext->modem_device_name);
                pstProfileCreateDeleteModify->device_profile_status_cb( "unknown", CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_PROFILE_STATUS_NOT_READY );
            }
        }
    }

    return RETURN_OK;
}

static void cellular_hal_qmi_profile_list_last_step( ContextProfileList *iter )
{
    if ( WDS_PROFILE_GETLIST_VIA_DEVICE_OPEN == iter->enQuerySource )
    {
        GTask        *task  = (GTask *)iter->vpPrivateData;
        QMIContextStructPrivate    *pstQMIContext   = NULL;
        ContextDeviceOpen          *pDeviceOpenCtx  = NULL;

        pDeviceOpenCtx  = g_task_get_task_data (task);

        if( NULL != iter )
        {
            free(iter);
            iter = NULL;
        }

        pDeviceOpenCtx->uiCurrentStep++;
        cellular_hal_qmi_device_open_step(task);
    }
    else if ( ( WDS_PROFILE_GETLIST_VIA_PROFILE_CREATE == iter->enQuerySource ) ||
              ( WDS_PROFILE_GETLIST_VIA_PROFILE_DELETE == iter->enQuerySource ) ||
              ( WDS_PROFILE_GETLIST_VIA_DML == iter->enQuerySource ) )
    {
        GTask        *task  = (GTask *)iter->vpPrivateData;
        QMIContextStructPrivate    *pstQMIContext   = NULL;
        ContextProfileOperation    *pProfileCrCtx   = NULL;

        pProfileCrCtx  = g_task_get_task_data (task);

        if( NULL != iter )
        {
            free(iter);
            iter = NULL;
        }

        pProfileCrCtx->uiCurrentStep++;
        cellular_hal_qmi_profile_operation_step(task);
    }
    else
    {
        //Fallback
    }   
}

static void cellular_hal_qmi_get_profile_settings (QmiClientWds *wdsClient,
                                                         GAsyncResult *result,
                                                         gpointer  user_data )
{
    QmiMessageWdsGetProfileSettingsOutput *output;
    GError *error = NULL;
    ContextProfileList *iter = (ContextProfileList*)user_data;
    ContextWDSInfo    *wdsCtx = iter->wdsCtx;

    output = qmi_client_wds_get_profile_settings_finish (wdsClient, result, &error);
    if (!output) 
    {
        CELLULAR_HAL_DBG_PRINT("%s Get profile settings operation failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);

        wdsCtx->ui8ProfileCount = 0;
        wdsCtx->bIsProfileListCollectionDone = TRUE;

        cellular_hal_qmi_profile_list_last_step( iter );
        return;
    } 
    else if (!qmi_message_wds_get_profile_settings_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT ("%s Unable to get profile settings: %s\n",
                                            __FUNCTION__,
                                            error->message);
        g_error_free (error);
        qmi_message_wds_get_profile_settings_output_unref (output);

        wdsCtx->ui8ProfileCount = 0;
        wdsCtx->bIsProfileListCollectionDone = TRUE;

        cellular_hal_qmi_profile_list_last_step( iter );
        return;
    }
    else 
    {
        const gchar *str;
        guint8 context_number;
        QmiWdsPdpType pdp_type;
        QmiWdsAuthentication auth;
        QmiWdsApnTypeMask apn_type;
        gboolean flag;
        CellularProfileStruct *pstProfile = &(wdsCtx->pstProfileList[iter->iteration]);
        
        if (qmi_message_wds_get_profile_settings_output_get_apn_name (output, &str, NULL))
            snprintf(pstProfile->APN, sizeof(pstProfile->APN), str);
#if 0
        if (qmi_message_wds_get_profile_settings_output_get_apn_type_mask (output, &apn_type, NULL)) {
            g_autofree gchar *aux = NULL;

            aux = qmi_wds_apn_type_mask_build_string_from_mask (apn_type);
            g_print ("\t\tAPN type: '%s'\n", VALIDATE_MASK_NONE (aux));
        }
#endif
        if (qmi_message_wds_get_profile_settings_output_get_pdp_type (output, &pdp_type, NULL))
        {
            if( QMI_WDS_PDP_TYPE_IPV4 == pdp_type )
            {
                pstProfile->PDPType = CELLULAR_PDP_TYPE_IPV4;
            }
            else if( QMI_WDS_PDP_TYPE_PPP == pdp_type )
            {
                pstProfile->PDPType = CELLULAR_PDP_TYPE_PPP;
            }
            else if( QMI_WDS_PDP_TYPE_IPV6 == pdp_type )
            {
                pstProfile->PDPType = CELLULAR_PDP_TYPE_IPV6;
            }
            else if( QMI_WDS_PDP_TYPE_IPV4_OR_IPV6 == pdp_type )
            {
                pstProfile->PDPType = CELLULAR_PDP_TYPE_IPV4_OR_IPV6;
            }
            else
            {
                //Fallback
                pstProfile->PDPType = CELLULAR_PDP_TYPE_IPV4;
            }
        }

        if (qmi_message_wds_get_profile_settings_output_get_pdp_context_number (output, &context_number, NULL))
            pstProfile->PDPContextNumber = context_number;

        if (qmi_message_wds_get_profile_settings_output_get_username (output, &str, NULL))
            snprintf(pstProfile->Username, sizeof(pstProfile->Username), str);

        if (qmi_message_wds_get_profile_settings_output_get_password (output, &str, NULL))
            snprintf(pstProfile->Password, sizeof(pstProfile->Password), str);

        if (qmi_message_wds_get_profile_settings_output_get_authentication (output, &auth, NULL)) 
        {
            if( QMI_WDS_AUTHENTICATION_NONE == auth )
            {
                pstProfile->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_NONE;
            }
            else  if( QMI_WDS_AUTHENTICATION_PAP == auth )
            {
                pstProfile->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_PAP;
            }
            else  if( QMI_WDS_AUTHENTICATION_CHAP == auth )
            {
                pstProfile->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_CHAP;
            }
            else
            {
                //Fallback
                pstProfile->PDPAuthentication = CELLULAR_PDP_AUTHENTICATION_NONE;
            }
        }

        if (qmi_message_wds_get_profile_settings_output_get_roaming_disallowed_flag (output, &flag, NULL))
            pstProfile->bIsNoRoaming = (flag) ? TRUE : FALSE;

        if (qmi_message_wds_get_profile_settings_output_get_apn_disabled_flag (output, &flag, NULL))
            pstProfile->bIsAPNDisabled = (flag) ? TRUE : FALSE;

        CELLULAR_HAL_DBG_PRINT("%s - Profile Name[%s] Idx[%d] APN[%s] User[%s] Password[%s] Auth[%d] Roaming[%d] APN Disabled[%d] Default[%d]\n",
                                                __FUNCTION__,
                                                pstProfile->ProfileName,
                                                pstProfile->ProfileID,
                                                pstProfile->APN,
                                                pstProfile->Username,
                                                pstProfile->Password,
                                                pstProfile->PDPAuthentication,
                                                pstProfile->bIsNoRoaming,
                                                pstProfile->bIsAPNDisabled,
                                                pstProfile->bIsThisDefaultProfile);

        qmi_message_wds_get_profile_settings_output_unref (output);
    }

    /* Keep on */
    iter->iteration++;
    cellular_hal_qmi_get_next_profile_settings (iter);
}

static void cellular_hal_qmi_get_next_profile_settings (ContextProfileList *inner_ctx)
{
    QmiMessageWdsGetProfileListOutputProfileListProfile *profile;
    QmiMessageWdsGetProfileSettingsInput *input;
    ContextWDSInfo    *wdsCtx = inner_ctx->wdsCtx;
    CellularProfileStruct *pstProfile = NULL;
    CellularProfileStruct *pstDefaultProfileFromModem = NULL;

    if (inner_ctx->iteration >= inner_ctx->list->len) 
    {
        /* All done */
        g_array_unref (inner_ctx->list);
        wdsCtx->bIsProfileListCollectionDone = TRUE;

        cellular_hal_qmi_profile_list_last_step( inner_ctx );
        return;
    }

    //Choose proper profile object
    pstProfile = &(wdsCtx->pstProfileList[inner_ctx->iteration]);
    pstDefaultProfileFromModem = &(wdsCtx->stDefaultProfileFromModem);

    profile = &g_array_index (inner_ctx->list, QmiMessageWdsGetProfileListOutputProfileListProfile, inner_ctx->iteration);
    
    pstProfile->ProfileID = profile->profile_index;
    if( pstDefaultProfileFromModem->ProfileID == pstProfile->ProfileID )
    {
        pstProfile->bIsThisDefaultProfile = TRUE;
    }
    else
    {
        pstProfile->bIsThisDefaultProfile = FALSE;
    }
    snprintf(pstProfile->ProfileName, sizeof(pstProfile->ProfileName), profile->profile_name);
    pstProfile->ProfileType = CELLULAR_PROFILE_TYPE_3GPP;

    input = qmi_message_wds_get_profile_settings_input_new ();
    qmi_message_wds_get_profile_settings_input_set_profile_id ( input,
                                                                profile->profile_type,
                                                                profile->profile_index,
                                                                NULL);
    qmi_client_wds_get_profile_settings (QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                         input,
                                         3,
                                         NULL,
                                         (GAsyncReadyCallback)cellular_hal_qmi_get_profile_settings,
                                         inner_ctx);
    qmi_message_wds_get_profile_settings_input_unref (input);
}

static void cellular_hal_qmi_get_profile_list_cb (QmiClientWds *wdsClient,
                                                     GAsyncResult *result,
                                                     gpointer  user_data)
{
    GError *error = NULL;
    QmiMessageWdsGetProfileListOutput *output;
    ContextProfileList *iter = (ContextProfileList*)user_data;
    GArray *profile_list = NULL;
    ContextWDSInfo    *wdsCtx = iter->wdsCtx;

    output = qmi_client_wds_get_profile_list_finish (wdsClient, result, &error);
    if (!output) 
    {
        CELLULAR_HAL_DBG_PRINT("%s Get profile list operation failed: %s\n",
                                        __FUNCTION__,
                                        error->message);
        g_error_free (error);
        goto LASTSTEP;
    }

    if (!qmi_message_wds_get_profile_list_output_get_result (output, &error)) 
    {
        CELLULAR_HAL_DBG_PRINT("%s Unable to get profile list: %s\n",
                                            __FUNCTION__,
                                            error->message);

        g_error_free (error);
        qmi_message_wds_get_profile_list_output_unref (output);
        goto LASTSTEP;
    }

    qmi_message_wds_get_profile_list_output_get_profile_list (output, &profile_list, NULL);

    if (!profile_list || !profile_list->len) 
    {
        qmi_message_wds_get_profile_list_output_unref (output);
        goto LASTSTEP;
    }

    wdsCtx->ui8ProfileCount = profile_list->len;

    //Free existing resource
    if( NULL != wdsCtx->pstProfileList )
    {
        free(wdsCtx->pstProfileList);
        wdsCtx->pstProfileList = NULL;
    }

    //Allocate new memory for current profile list
    wdsCtx->pstProfileList = (CellularProfileStruct*)malloc(sizeof(CellularProfileStruct) * wdsCtx->ui8ProfileCount);
    if( NULL != wdsCtx->pstProfileList )
    {
        iter->list = g_array_ref (profile_list);
        iter->iteration = 0;
        cellular_hal_qmi_get_next_profile_settings (iter);
    }
    else
    {
        wdsCtx->ui8ProfileCount = 0;
        wdsCtx->bIsProfileListCollectionDone = TRUE;
    }

    return;

LASTSTEP:
    wdsCtx->ui8ProfileCount = 0;
    wdsCtx->bIsProfileListCollectionDone = TRUE;

    cellular_hal_qmi_profile_list_last_step( iter );
}

/* WDS Network Start/Stop*/
static void  cellular_hal_qmi_packet_service_status_indication_cb ( QmiClientWds *wdsClient,
                                                                    QmiIndicationWdsPacketServiceStatusOutput *output,
                                                                    gpointer  user_data)
{
    QmiWdsConnectionStatus connection_status;
    cellular_network_packet_service_status_api_callback packet_service_status_cb = (cellular_network_packet_service_status_api_callback)user_data;
    ContextWDSInfo         *wdsCtx = &(gpstQMIContext->wdsCtx);

    CELLULAR_HAL_DBG_PRINT("%s - Received Packet Service Status\n",__FUNCTION__);

    if (!qmi_indication_wds_packet_service_status_output_get_connection_status ( output,
                                                                                 &connection_status,
                                                                                 NULL,
                                                                                 NULL))
    {
        return;
    }
        
    /**
     * QmiWdsConnectionStatus:
     * @QMI_WDS_CONNECTION_STATUS_UNKNOWN: Unknown status.
     * @QMI_WDS_CONNECTION_STATUS_DISCONNECTED: Network is disconnected
     * @QMI_WDS_CONNECTION_STATUS_CONNECTED: Network is connected.
     * @QMI_WDS_CONNECTION_STATUS_SUSPENDED: Network connection is suspended.
     * @QMI_WDS_CONNECTION_STATUS_AUTHENTICATING: Network authentication is ongoing.
     * 
     * Status of the network connection. 
     *
     */

    CELLULAR_HAL_DBG_PRINT("%s - Device:%s IPv%d Service Status:%s\n",__FUNCTION__,qmi_device_get_path_display (gpstQMIContext->qmiDevice),(( wdsClient == QMI_CLIENT_WDS(wdsCtx->client_ipv4) ) ? 4 : 6 ),qmi_wds_connection_status_get_string(connection_status));

    if ( NULL != packet_service_status_cb )
    {
        packet_service_status_cb( qmi_device_get_path_display (gpstQMIContext->qmiDevice), 
                                  ( wdsClient == QMI_CLIENT_WDS(wdsCtx->client_ipv4) )  ?  CELLULAR_NETWORK_IP_FAMILY_IPV4 : CELLULAR_NETWORK_IP_FAMILY_IPV6,
                                  ( QMI_WDS_CONNECTION_STATUS_CONNECTED == connection_status) ? DEVICE_NETWORK_STATUS_CONNECTED : DEVICE_NETWORK_STATUS_DISCONNECTED );
    }
}

static void cellular_hal_qmi_event_report_indication_cb ( QmiClientWds *wdsClient,
                                                          QmiIndicationWdsEventReportOutput *output )
{
    //CELLULAR_HAL_DBG_PRINT("%s %d - got QMI WDS event report\n",__FUNCTION__,__LINE__);
}

static guint cellular_hal_qmi_connect_enable_event_report ( QmiClientWds *wdsClient,
                                                            GAsyncResult *result,
                                                            GError **error)
{
    QmiMessageWdsSetEventReportOutput *output;

    /* Don't care about the result */
    output = qmi_client_wds_set_event_report_finish (wdsClient, result, error);
    if (!output || !qmi_message_wds_set_event_report_output_get_result (output, error)) {
        if (output)
            qmi_message_wds_set_event_report_output_unref (output);
        return 0;
    }
    qmi_message_wds_set_event_report_output_unref (output);

    return g_signal_connect (wdsClient,
                             "event-report",
                             G_CALLBACK (cellular_hal_qmi_event_report_indication_cb),
                             NULL);
}

static void  cellular_hal_qmi_get_current_settings_cb (QmiClientWds *wdsClient,
                                                          GAsyncResult *result,
                                                          gpointer  user_data)
{
    GError *error = NULL;
    GTask                   *task        = (GTask*)user_data;
    QmiMessageWdsGetCurrentSettingsOutput *output;
    QmiWdsIpFamily ip_family = QMI_WDS_IP_FAMILY_UNSPECIFIED;
    guint32 mtu = 0;
    GArray *array;
    guint32 addr = 0;
    struct in_addr in_addr_val;
    struct in6_addr in6_addr_val;
    gchar buf4[INET_ADDRSTRLEN];
    gchar buf6[INET6_ADDRSTRLEN];
    guint8 prefix = 0;
    guint i;
    ContextWDSInfo          *wdsCtx   = NULL; 
    CellularIPStruct         stIPInfo = {0};
    CellularNetworkCBStruct *pstCBStruct = NULL;
    QMIContextStructPrivate *pstQMIContext = NULL;
    ContextNWStart          *pNWStartCtx   = NULL;
    cellular_device_network_ip_ready_api_callback device_network_ip_ready_cb = NULL;

    pNWStartCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStartCtx->vpPrivateData;

    wdsCtx         = &(pstQMIContext->wdsCtx);
    pstCBStruct    = &(pNWStartCtx->stNetworkCB);
    device_network_ip_ready_cb = pstCBStruct->device_network_ip_ready_cb;

    output = qmi_client_wds_get_current_settings_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT ("Get Current Network Settings Operation failed: %s\n", error->message);
        g_error_free (error);

        //End Task
        pNWStartCtx->uiCurrentStep = WDS_NW_START_END;
        cellular_hal_qmi_start_network_connection_step(task);
        return;
    }

    if (!qmi_message_wds_get_current_settings_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT ("Unable to get current network settings: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_get_current_settings_output_unref (output);

        //End Task
        pNWStartCtx->uiCurrentStep = WDS_NW_START_END;
        cellular_hal_qmi_start_network_connection_step(task);
        return;
    }

    CELLULAR_HAL_DBG_PRINT ("[%s] Current settings retrieved:\n",
                                        qmi_device_get_path_display (pstQMIContext->qmiDevice));

    if (qmi_message_wds_get_current_settings_output_get_ip_family (output, &ip_family, NULL))
        CELLULAR_HAL_DBG_PRINT ("           IP Family: %s\n",
                                        ((ip_family == QMI_WDS_IP_FAMILY_IPV4) ? "IPv4" :
                                        ((ip_family == QMI_WDS_IP_FAMILY_IPV6) ? "IPv6" :
                                        "unknown")));

    stIPInfo.IPType = CELLULAR_NETWORK_IP_FAMILY_UNKNOWN;
    switch( ip_family )
    {
        case QMI_WDS_IP_FAMILY_IPV4:
            stIPInfo.IPType = CELLULAR_NETWORK_IP_FAMILY_IPV4;
        break;
        case QMI_WDS_IP_FAMILY_IPV6:
            stIPInfo.IPType = CELLULAR_NETWORK_IP_FAMILY_IPV6;
        break;
        default:
            stIPInfo.IPType = CELLULAR_NETWORK_IP_FAMILY_UNKNOWN;
    }

    /* IPv4... */
    if (qmi_message_wds_get_current_settings_output_get_ipv4_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        CELLULAR_HAL_DBG_PRINT ("        IPv4 address: %s\n", buf4);
        snprintf(stIPInfo.IPAddress, sizeof(stIPInfo.IPAddress), "%s", buf4);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv4_gateway_subnet_mask (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        CELLULAR_HAL_DBG_PRINT ("    IPv4 subnet mask: %s\n", buf4);
        snprintf(stIPInfo.SubnetMask, sizeof(stIPInfo.SubnetMask), "%s", buf4);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv4_gateway_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        CELLULAR_HAL_DBG_PRINT ("IPv4 gateway address: %s\n", buf4);
        snprintf(stIPInfo.DefaultGateWay, sizeof(stIPInfo.DefaultGateWay), "%s", buf4);
    }

    if (qmi_message_wds_get_current_settings_output_get_primary_ipv4_dns_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        CELLULAR_HAL_DBG_PRINT ("    IPv4 primary DNS: %s\n", buf4);
        snprintf(stIPInfo.DNSServer1, sizeof(stIPInfo.DNSServer1), "%s", buf4);
    }
    if (qmi_message_wds_get_current_settings_output_get_secondary_ipv4_dns_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        CELLULAR_HAL_DBG_PRINT ("  IPv4 secondary DNS: %s\n", buf4);
        snprintf(stIPInfo.DNSServer2, sizeof(stIPInfo.DNSServer2), "%s", buf4);
    }

    /* IPv6... */
    if (qmi_message_wds_get_current_settings_output_get_ipv6_address (output, &array, &prefix, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        CELLULAR_HAL_DBG_PRINT ("        IPv6 address: %s/%d\n", buf6, prefix);
        snprintf(stIPInfo.IPAddress, sizeof(stIPInfo.IPAddress), "%s/%d", buf6,prefix);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv6_gateway_address (output, &array, &prefix, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        CELLULAR_HAL_DBG_PRINT ("IPv6 gateway address: %s/%d\n", buf6, prefix);
        snprintf(stIPInfo.DefaultGateWay, sizeof(stIPInfo.DefaultGateWay), "%s/%d", buf6,prefix);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv6_primary_dns_address (output, &array, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        CELLULAR_HAL_DBG_PRINT ("    IPv6 primary DNS: %s\n", buf6);
        snprintf(stIPInfo.DNSServer1, sizeof(stIPInfo.DNSServer1), "%s", buf6);
    }
    if (qmi_message_wds_get_current_settings_output_get_ipv6_secondary_dns_address (output, &array, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        CELLULAR_HAL_DBG_PRINT ("  IPv6 secondary DNS: %s\n", buf6);
        snprintf(stIPInfo.DNSServer2, sizeof(stIPInfo.DNSServer2), "%s", buf6);
    }

    /* Other... */
    if (qmi_message_wds_get_current_settings_output_get_mtu (output, &mtu, NULL))
        CELLULAR_HAL_DBG_PRINT ("                 MTU: %u\n", mtu);
    stIPInfo.MTUSize = mtu;

    if (qmi_message_wds_get_current_settings_output_get_domain_name_list (output, &array, &error)) {
        GString *s = NULL;

        if (array) {
            for (i = 0; i < array->len; i++) {
                if (!s)
                    s = g_string_new ("");
                else
                    g_string_append (s, ", ");
                g_string_append (s, g_array_index (array, const gchar *, i));
            }
        }
        if (s) {
            CELLULAR_HAL_DBG_PRINT ("             Domains: %s\n", s->str);
            snprintf(stIPInfo.Domains, sizeof(stIPInfo.Domains), "%s", s->str);
            g_string_free (s, TRUE);
        } else
            CELLULAR_HAL_DBG_PRINT ("             Domains: none\n");
    }

    qmi_message_wds_get_current_settings_output_unref (output);

    //Wan Interface Name
    snprintf(stIPInfo.WANIFName, sizeof(stIPInfo.WANIFName), "%s", pstQMIContext->wwan_iface);

    //Send Network IP ready status if CB is not null
    if( NULL != device_network_ip_ready_cb )
    {
        CELLULAR_HAL_DBG_PRINT("%s - Network is ready and sending status via CB, Device:%s\n", __FUNCTION__, pstQMIContext->modem_device_name);
        device_network_ip_ready_cb( &stIPInfo, DEVICE_NETWORK_IP_READY );
    }

    pNWStartCtx->uiCurrentStep++;
    cellular_hal_qmi_start_network_connection_step(task);
}

static void  cellular_hal_qmi_get_current_settings ( QmiClientWds *wdsClient, gpointer  user_data )
{
    QmiMessageWdsGetCurrentSettingsInput *input;
    QmiWdsGetCurrentSettingsRequestedSettings req_settings;

    req_settings =  QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DNS_ADDRESS |
                    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GRANTED_QOS |
                    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_ADDRESS |
                    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GATEWAY_INFO |
                    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_MTU |
                    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DOMAIN_NAME_LIST |
                    QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_FAMILY;

    input = qmi_message_wds_get_current_settings_input_new ();
    qmi_message_wds_get_current_settings_input_set_requested_settings (input, req_settings, NULL);
    qmi_client_wds_get_current_settings (wdsClient,
                                         input,
                                         10,
                                         NULL,
                                         (GAsyncReadyCallback)cellular_hal_qmi_get_current_settings_cb,
                                         user_data);
    qmi_message_wds_get_current_settings_input_unref (input);
}

static void
cellular_hal_qmi_get_data_bearer_technology_cb (QmiClientWds *client,
                                                   GAsyncResult *res,
						   gpointer  user_data)
{
    GError *error = NULL;
    GTask                   *task        = (GTask*)user_data;
    QmiMessageWdsGetDataBearerTechnologyOutput *output;
    QmiWdsDataBearerTechnology current;
    ContextNWStart          *pNWStartCtx   = NULL;
    ContextWDSInfo          *wdsCtx   = NULL;
    QMIContextStructPrivate *pstQMIContext = NULL;
    pNWStartCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStartCtx->vpPrivateData;
    
    wdsCtx         = &(pstQMIContext->wdsCtx);
    output = qmi_client_wds_get_data_bearer_technology_finish (client, res, &error);
    if (!output) {
	CELLULAR_HAL_DBG_PRINT("%s - error: operation failed  %s\n", __FUNCTION__,error->message);
        g_error_free (error);
	//End Task
        pNWStartCtx->uiCurrentStep = WDS_NW_START_END;
        cellular_hal_qmi_start_network_connection_step(task);
        return;
    }
    if (!qmi_message_wds_get_data_bearer_technology_output_get_result (output, &error)) {
	CELLULAR_HAL_DBG_PRINT("%s - error: couldn't get data bearer tech %s\n", __FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_wds_get_data_bearer_technology_output_unref (output);
	//End Task
        pNWStartCtx->uiCurrentStep = WDS_NW_START_END;
        cellular_hal_qmi_start_network_connection_step(task);
        return;
    }
    qmi_message_wds_get_data_bearer_technology_output_get_current (
        output,
        &current,
        NULL);
    switch (current){
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_GSM:
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_EDGE:
           strncpy(wdsCtx->currentRAT,"GSM",sizeof(wdsCtx->currentRAT));
           break;
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_CDMA20001X:
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_EHRPD:
           strncpy(wdsCtx->currentRAT,"CDMA20001X",sizeof(wdsCtx->currentRAT));
           break;
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO:
	case QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO_REVA:
           strncpy(wdsCtx->currentRAT,"EVDO",sizeof(wdsCtx->currentRAT));
           break;
	case QMI_WDS_DATA_BEARER_TECHNOLOGY_UMTS:
	case QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA:
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA_HSUPDA:
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS:
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS_HSUPA:
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS:
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS_HSUPA:
           strncpy(wdsCtx->currentRAT,"UMTS",sizeof(wdsCtx->currentRAT));
           break;
        case QMI_WDS_DATA_BEARER_TECHNOLOGY_LTE: 
           strncpy(wdsCtx->currentRAT,"LTE",sizeof(wdsCtx->currentRAT));
           break;
	case QMI_WDS_DATA_BEARER_TECHNOLOGY_UNKNOWN:   
	default:
           strncpy(wdsCtx->currentRAT,"UNKNOWN",sizeof(wdsCtx->currentRAT));
    }
    CELLULAR_HAL_DBG_PRINT ("%s Data bearer technology (current): '%s'\n",__FUNCTION__,wdsCtx->currentRAT);
    qmi_message_wds_get_data_bearer_technology_output_unref (output);
    pNWStartCtx->uiCurrentStep++;
    cellular_hal_qmi_start_network_connection_step(task);
}


static void cellular_hal_qmi_start_network_cb ( QmiClientWds *wdsClient,
                                                   GAsyncResult *result,
                                                   gpointer  user_data)
{   
    GError *error = NULL;
    GTask                   *task        = (GTask*)user_data;
    QMIContextStructPrivate *pstQMIContext = NULL;
    ContextWDSInfo          *wdsCtx      = NULL;  
    CellularNetworkCBStruct *pstCBStruct = NULL;
    ContextNWStart          *pNWStartCtx   = NULL;
    QmiMessageWdsStartNetworkOutput *output;
    cellular_device_network_ip_ready_api_callback device_network_ip_ready_cb = NULL;
    CellularIPStruct         stIPInfo = {0};
    int op_shutdown = 0;
    pNWStartCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStartCtx->vpPrivateData;

    wdsCtx         = &(pstQMIContext->wdsCtx);
    pstCBStruct    = &(pNWStartCtx->stNetworkCB);
    device_network_ip_ready_cb = pstCBStruct->device_network_ip_ready_cb;

    output = qmi_client_wds_start_network_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT ("Start Network Operation failed: %s\n",
                                        error->message);
        g_error_free (error);
        op_shutdown = 1;
    }

    else if (!qmi_message_wds_start_network_output_get_result (output, &error))
    {
        CELLULAR_HAL_DBG_PRINT ("Unable to start network: %s\n", error->message);

        g_error_free (error);
        qmi_message_wds_start_network_output_unref (output);
        op_shutdown = 1;
    }
    if (op_shutdown)
    {
        //Send Network IP ready or not ready status if CB is not null
        if( NULL != device_network_ip_ready_cb )
        {
            if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type )
            {
                wdsCtx->packet_data_handle_ipv4 = CELLULAR_PACKET_DATA_INVALID_HANDLE;
                stIPInfo.IPType = CELLULAR_NETWORK_IP_FAMILY_IPV4;
            }
            else
            {
                wdsCtx->packet_data_handle_ipv6 = CELLULAR_PACKET_DATA_INVALID_HANDLE;
                stIPInfo.IPType = CELLULAR_NETWORK_IP_FAMILY_IPV6;
            }
            CELLULAR_HAL_DBG_PRINT("%s - Network is not ready and sending status via CB, Device:%s\n", __FUNCTION__, pstQMIContext->modem_device_name);
            device_network_ip_ready_cb( &stIPInfo, DEVICE_NETWORK_IP_NOT_READY );
        }
        pNWStartCtx->uiCurrentStep = WDS_NW_START_END;
        cellular_hal_qmi_start_network_connection_step(task);
    }
    else
    {
        if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type )
        {
            qmi_message_wds_start_network_output_get_packet_data_handle (output, &wdsCtx->packet_data_handle_ipv4, NULL);
            CELLULAR_HAL_DBG_PRINT("%s - Network Started v4, Packet Data Handle:0x%X\n", __FUNCTION__, wdsCtx->packet_data_handle_ipv4);
        }
        else
        {
            qmi_message_wds_start_network_output_get_packet_data_handle (output, &wdsCtx->packet_data_handle_ipv6, NULL);
            CELLULAR_HAL_DBG_PRINT("%s - Network Started v6, Packet Data Handle:0x%X\n", __FUNCTION__, wdsCtx->packet_data_handle_ipv6);
        }
        
        if (output)
        qmi_message_wds_start_network_output_unref (output);

        pNWStartCtx->uiCurrentStep++;
        cellular_hal_qmi_start_network_connection_step(task);
    }
}

static void  cellular_hal_qmi_connect_enable_indications ( QmiClientWds *wdsClient,
                                                                 GAsyncResult *result,
                                                                 gpointer  user_data)
{
    GTask                   *task        = (GTask*)user_data;
    QMIContextStructPrivate *pstQMIContext = NULL;
    ContextWDSInfo          *wdsCtx      = NULL;  
    ContextNWStart          *pNWStartCtx   = NULL;
    CellularNetworkCBStruct *pstCBStruct = NULL;

    pNWStartCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStartCtx->vpPrivateData;

    wdsCtx         = &(pstQMIContext->wdsCtx);
    pstCBStruct    = &(pNWStartCtx->stNetworkCB);

    if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type )
    {
        wdsCtx->event_report_ipv4_id =  cellular_hal_qmi_connect_enable_event_report (wdsClient, result, &wdsCtx->error_ipv4);

        wdsCtx->packet_service_status_ipv4_id =   g_signal_connect (wdsClient,
                                                                    "packet-service-status",
                                                                    G_CALLBACK (cellular_hal_qmi_packet_service_status_indication_cb),
                                                                    pstCBStruct->packet_service_status_cb);
    
        CELLULAR_HAL_DBG_PRINT("%s - Event Register Success Mode:%d ReportID:%d PacketServiceID:%d\n",__FUNCTION__,pNWStartCtx->ip_request_type,wdsCtx->event_report_ipv4_id,wdsCtx->packet_service_status_ipv4_id);
    }
    else 
    {
        wdsCtx->event_report_ipv6_id =  cellular_hal_qmi_connect_enable_event_report (wdsClient, result, &wdsCtx->error_ipv6);

        wdsCtx->packet_service_status_ipv6_id =   g_signal_connect (wdsClient,
                                                                    "packet-service-status",
                                                                    G_CALLBACK (cellular_hal_qmi_packet_service_status_indication_cb),
                                                                    pstCBStruct->packet_service_status_cb);

        CELLULAR_HAL_DBG_PRINT("%s - Event Register Success Mode:%d ReportID:%d PacketServiceID:%d\n",__FUNCTION__,pNWStartCtx->ip_request_type,wdsCtx->event_report_ipv6_id,wdsCtx->packet_service_status_ipv6_id);
    }

    pNWStartCtx->uiCurrentStep++;
    cellular_hal_qmi_start_network_connection_step(task);
}

static QmiMessageWdsSetEventReportInput *cellular_hal_qmi_event_report_input_new (gboolean enable)
{
    QmiMessageWdsSetEventReportInput *input;

    input = qmi_message_wds_set_event_report_input_new ();
    qmi_message_wds_set_event_report_input_set_extended_data_bearer_technology (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_limited_data_system_status (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_uplink_flow_control (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_data_systems (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_evdo_pm_change (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_preferred_data_system (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_data_call_status (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_current_data_bearer_technology (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_mip_status (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_dormancy_status (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_data_bearer_technology (input, enable, NULL);
    qmi_message_wds_set_event_report_input_set_channel_rate (input, enable, NULL);

    return input;
}

static void cellular_hal_qmi_setup_event_report_unsolicited_events ( QmiClientWds *wdsClient,
                                                                     GAsyncReadyCallback callback,
                                                                     gpointer user_data )
{
    QmiMessageWdsSetEventReportInput *input;

    input = cellular_hal_qmi_event_report_input_new(TRUE);
    qmi_client_wds_set_event_report (wdsClient,
                                     input,
                                     5,
                                     NULL,
                                     callback,
                                     user_data);

    qmi_message_wds_set_event_report_input_unref (input);
}

static void cellular_hal_qmi_set_ip_family (QmiDevice *device, GAsyncResult *result, gpointer  user_data)
{
    GError *error = NULL;
    QmiMessageWdsSetIpFamilyOutput *output;
    GTask                          *task   = (GTask*)user_data;
    QMIContextStructPrivate        *pstQMIContext = NULL;
    ContextNWStart                 *pNWStartCtx   = NULL;
    ContextWDSInfo                 *wdsCtx = NULL;       

    pNWStartCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStartCtx->vpPrivateData;

    wdsCtx = &(pstQMIContext->wdsCtx);

    output = qmi_client_wds_set_ip_family_finish ( QMI_CLIENT_WDS(( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) ? wdsCtx->client_ipv4 : wdsCtx->client_ipv6), result, &error);
    if (output) {
        qmi_message_wds_set_ip_family_output_get_result (output, &error);
        qmi_message_wds_set_ip_family_output_unref (output);
    }

    if (error) {
        CELLULAR_HAL_DBG_PRINT("%d - Unable to set IP family preference: %s\n", __FUNCTION__, error->message);
        g_error_free (error);

        //Go to last step
        pNWStartCtx->uiCurrentStep = WDS_NW_START_END;
        cellular_hal_qmi_start_network_connection_step(task);
        return;
    }

    pNWStartCtx->uiCurrentStep++;
    cellular_hal_qmi_start_network_connection_step(task);
}

static void cellular_hal_qmi_client_wds_start_network (QmiDevice *device, GAsyncResult *result, gpointer  user_data)
{
    QMIContextStructPrivate    *pstQMIContext = NULL;
    ContextNWStart             *pNWStartCtx   = NULL;
    ContextWDSInfo             *wdsCtx        = NULL; 
    QmiClient                  *wdsclient;
    GError                     *error = NULL;
    GTask                      *task = (GTask*)user_data;

    CELLULAR_HAL_DBG_PRINT("%s %d tid:%d - WDS client allocate ready\n",__FUNCTION__,__LINE__,gettid());

    pNWStartCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStartCtx->vpPrivateData;

    wdsCtx = &(pstQMIContext->wdsCtx);

    wdsclient = qmi_device_allocate_client_finish (device, result, &error);
    if (!wdsclient) {
        CELLULAR_HAL_DBG_PRINT ("%s Unable to create client for the '%s' service: %s\n",
                                __FUNCTION__,
                                qmi_service_get_string (QMI_SERVICE_WDS),
                                error->message);

        pNWStartCtx->uiCurrentStep = WDS_NW_START_END;
        cellular_hal_qmi_start_network_connection_step(task);
        return;
    }

    //Store WDS to respective context 
    if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) 
    {
        wdsCtx->client_ipv4  = wdsclient;
    }
    else if( CELLULAR_NETWORK_IP_FAMILY_IPV6 == pNWStartCtx->ip_request_type ) 
    {
        wdsCtx->client_ipv6  = wdsclient;
    }

    pNWStartCtx->uiCurrentStep++;
    cellular_hal_qmi_start_network_connection_step(task);
}

static void cellular_hal_qmi_start_network_connection_step( GTask *task )
{
    QMIContextStructPrivate    *pstQMIContext = NULL;
    ContextNWStart             *pNWStartCtx   = NULL;

    pNWStartCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStartCtx->vpPrivateData;
    
    //CELLULAR_HAL_DBG_PRINT("%s %d - Inside State Start Network Mode:%d Task:0x%p Step:%d QMI:%d\n",__FUNCTION__,__LINE__,pNWStartCtx->ip_request_type,task, pNWStartCtx->uiCurrentStep, qmi_device_is_open( pstQMIContext->qmiDevice ));

    switch ( pNWStartCtx->uiCurrentStep ) 
    {
        case WDS_NW_START_BEGIN:
        {
            pNWStartCtx->uiCurrentStep++;
            /* fall through */
        }
        case WDS_NW_START_ALLOCATE_WDS_CLIENT:
        {
            CELLULAR_HAL_DBG_PRINT("%s %d - Allocate WDS Client Network Mode:%d Task:0x%p\n",__FUNCTION__,__LINE__,pNWStartCtx->ip_request_type,task);

            if( ( ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) &&
                  ( NULL == pstQMIContext->wdsCtx.client_ipv4 ) ) ||
                  ( ( CELLULAR_NETWORK_IP_FAMILY_IPV6 == pNWStartCtx->ip_request_type ) &&
                  ( NULL == pstQMIContext->wdsCtx.client_ipv4 ) ) )
            {
                CELLULAR_HAL_DBG_PRINT("%s - Allocating WDS client for type:%d\n",__FUNCTION__,pNWStartCtx->ip_request_type);
                qmi_device_allocate_client (pstQMIContext->qmiDevice,
                                            QMI_SERVICE_WDS,
                                            QMI_CID_NONE,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback) cellular_hal_qmi_client_wds_start_network,
                                            task);
                return;
            }

            /* fall through */
            pNWStartCtx->uiCurrentStep++;
        }
        case WDS_NW_START_CONFIGURE_IP_FAMILY:
        {
            QmiMessageWdsSetIpFamilyInput *input;
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) ? TRUE : FALSE;
            
            CELLULAR_HAL_DBG_PRINT("%s - Configuring IPv%d Family\n",__FUNCTION__,((bIsIPv4)? 4 : 6));
            input = qmi_message_wds_set_ip_family_input_new ();
            qmi_message_wds_set_ip_family_input_set_preference (input, ( TRUE == bIsIPv4 ) ? QMI_WDS_IP_FAMILY_IPV4 : QMI_WDS_IP_FAMILY_IPV6, NULL);
            qmi_client_wds_set_ip_family (  QMI_CLIENT_WDS(( TRUE == bIsIPv4 ) ? pstQMIContext->wdsCtx.client_ipv4 : pstQMIContext->wdsCtx.client_ipv6),
                                            input,
                                            10,
                                            NULL,
                                            (GAsyncReadyCallback)cellular_hal_qmi_set_ip_family,
                                            task);
            qmi_message_wds_set_ip_family_input_unref (input);
            return;
        }
        case WDS_NW_START_REGISTER_EVENTS:
        {
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) ? TRUE : FALSE;

            CELLULAR_HAL_DBG_PRINT("%s - Registering Events for IPv%d\n",__FUNCTION__,((bIsIPv4)? 4 : 6));
            cellular_hal_qmi_setup_event_report_unsolicited_events ( QMI_CLIENT_WDS(( TRUE == bIsIPv4 ) ? pstQMIContext->wdsCtx.client_ipv4 : pstQMIContext->wdsCtx.client_ipv6),
                                                                    (GAsyncReadyCallback) cellular_hal_qmi_connect_enable_indications,
                                                                    task );
            return;
        }
        case WDS_NW_START_START_NETWORK:
        {
            ContextWDSInfo          *wdsCtx = &(pstQMIContext->wdsCtx);  
            CellularProfileStruct   *pstCurrentProfile = &(pNWStartCtx->stProfileInput);       
            QmiMessageWdsStartNetworkInput *input;
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) ? TRUE : FALSE;

            CELLULAR_HAL_DBG_PRINT("%s %d - ProfileID:%d Starting IPv%d Connection...\n",__FUNCTION__,__LINE__,pstCurrentProfile->ProfileID,(TRUE == bIsIPv4) ? 4 : 6);
            input = qmi_message_wds_start_network_input_new ();

            /* Start Network Can be done by intimating profile ID */
            qmi_message_wds_start_network_input_set_profile_index_3gpp (input, (guint8)pstCurrentProfile->ProfileID, NULL);
            qmi_message_wds_start_network_input_set_ip_family_preference ( input,
                                                                            ( TRUE == bIsIPv4 ) ? (QMI_WDS_IP_FAMILY_IPV4) : (QMI_WDS_IP_FAMILY_IPV6),
                                                                            NULL);
            qmi_client_wds_start_network (  QMI_CLIENT_WDS(( TRUE == bIsIPv4 ) ? pstQMIContext->wdsCtx.client_ipv4 : pstQMIContext->wdsCtx.client_ipv6),
                                            input,
                                            60,
                                            NULL,
                                            (GAsyncReadyCallback)cellular_hal_qmi_start_network_cb,
                                            task);
            qmi_message_wds_start_network_input_unref (input);

            return;
        }
        case WDS_NW_START_GET_LEASE_SETTINGS:
        {
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) ? TRUE : FALSE;

            CELLULAR_HAL_DBG_PRINT("%s - Getting lease settings for IPv%d\n",__FUNCTION__,((bIsIPv4)? 4 : 6));

            //Get Current IP Settings
            cellular_hal_qmi_get_current_settings( QMI_CLIENT_WDS(( TRUE == bIsIPv4 ) ? pstQMIContext->wdsCtx.client_ipv4 : pstQMIContext->wdsCtx.client_ipv6), task );
            return;
        }
	case WDS_NW_START_GET_DATA_RAT:
	{
             unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) ? TRUE : FALSE;
             qmi_client_wds_get_data_bearer_technology (QMI_CLIENT_WDS(( TRUE == bIsIPv4 ) ?
                                                        pstQMIContext->wdsCtx.client_ipv4 : pstQMIContext->wdsCtx.client_ipv6),
                                                        NULL,
                                                        10,
                                                        NULL,
                                                        (GAsyncReadyCallback)cellular_hal_qmi_get_data_bearer_technology_cb,
                                                        task);
              return;
	}
        case WDS_NW_START_END:
        default:
        {
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStartCtx->ip_request_type ) ? TRUE : FALSE;

            //Reset the flag
            if( TRUE == bIsIPv4 )
            {
                pstQMIContext->wdsCtx.bIsStartNetworkIPv4InProgress = FALSE;
            }
            else
            {
                pstQMIContext->wdsCtx.bIsStartNetworkIPv6InProgress = FALSE;
            }

            pNWStartCtx->uiCurrentStep++;
            g_object_unref (task);
        }
    }
}

static void cellular_hal_qmi_start_network_context_free( ContextNWStart *pNWStartCtx )
{
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources for Mode:%d\n",__FUNCTION__,pNWStartCtx->ip_request_type);

    if( NULL != pNWStartCtx)
    {
        g_slice_free(ContextNWStart, pNWStartCtx);
        pNWStartCtx  = NULL;
    }
}

int cellular_hal_qmi_start_network(CellularNetworkIPType_t ip_request_type, CellularProfileStruct *pstProfileInput, CellularNetworkCBStruct *pstCBStruct)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextWDSInfo *wdsCtx = &(gpstQMIContext->wdsCtx); 
        ContextNWStart *pNWStartCtx = NULL;
        GTask          *task;

        pNWStartCtx         = g_slice_new0 (ContextNWStart);
        pNWStartCtx->uiCurrentStep   = WDS_NW_START_BEGIN;
        pNWStartCtx->vpPrivateData   = (void*) gpstQMIContext;
        pNWStartCtx->ip_request_type = ip_request_type;
        if(NULL != pstProfileInput)
        {
            memcpy(&(pNWStartCtx->stProfileInput), pstProfileInput, sizeof(CellularProfileStruct));
        }
        else
        {
            memcpy(&(pNWStartCtx->stProfileInput), &(wdsCtx->stDefaultProfileFromModem), sizeof(CellularProfileStruct));
        }

        if(NULL != pstCBStruct)
        memcpy(&(pNWStartCtx->stNetworkCB), pstCBStruct, sizeof(CellularNetworkCBStruct));

        //Mark Start NW inprogress
        if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == ip_request_type )
        {
            gpstQMIContext->wdsCtx.bIsStartNetworkIPv4InProgress = TRUE;
        }
        
        if( CELLULAR_NETWORK_IP_FAMILY_IPV6 == ip_request_type )
        {
            gpstQMIContext->wdsCtx.bIsStartNetworkIPv6InProgress = TRUE;
        }

        task = g_task_new (NULL, NULL, NULL, NULL);
        g_task_set_task_data (task, (gpointer)pNWStartCtx, (GDestroyNotify)cellular_hal_qmi_start_network_context_free);

        CELLULAR_HAL_DBG_PRINT("%s - Starting Network Mode:%d Task:0x%p\n",__FUNCTION__,ip_request_type,task);
        cellular_hal_qmi_start_network_connection_step(task);
    }
    else
    {
        //Send Profile not ready status if CB is not null
        if( NULL != pstCBStruct )
        {
            CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so network can't be started now and sending status via CB\n", __FUNCTION__, gpstQMIContext->modem_device_name);
            if( NULL != pstCBStruct->device_network_ip_ready_cb )
            {
                pstCBStruct->device_network_ip_ready_cb( NULL, DEVICE_NETWORK_IP_NOT_READY );
            }

            if( NULL != pstCBStruct->packet_service_status_cb )
            {
                pstCBStruct->packet_service_status_cb( NULL, CELLULAR_NETWORK_IP_FAMILY_UNKNOWN, DEVICE_NETWORK_STATUS_DISCONNECTED );
            }
        }
    }

    return RETURN_OK;
}

static void cellular_hal_qmi_stop_network_cb (QmiClientWds *wdsClient,
                                                 GAsyncResult *result,
                                                 gpointer  user_data)
{
    GError *error = NULL;
    GTask *task = (GTask*)user_data;
    QmiMessageWdsStopNetworkOutput *output;
    QMIContextStructPrivate    *pstQMIContext = NULL;
    ContextNWStop              *pNWStopCtx    = NULL;
    ContextWDSInfo             *wdsCtx        = NULL;
    unsigned char bIsIPv4;

    pNWStopCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStopCtx->vpPrivateData;
    wdsCtx = &(pstQMIContext->wdsCtx);

    bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStopCtx->ip_request_type ) ? TRUE : FALSE;

    //Reinitialize invalid packet handle
    if( TRUE == bIsIPv4 )
    {
        wdsCtx->packet_data_handle_ipv4 = CELLULAR_PACKET_DATA_INVALID_HANDLE;
    }
    else
    {
        wdsCtx->packet_data_handle_ipv6 = CELLULAR_PACKET_DATA_INVALID_HANDLE;
    }

    output = qmi_client_wds_stop_network_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT ("%s Stop network operation failed: %s\n",
                                        __FUNCTION__,
                                        error->message);
        g_error_free (error);

        pNWStopCtx->uiCurrentStep++;
        cellular_hal_qmi_stop_network_connection_step(task);
        return;
    }

    if (!qmi_message_wds_stop_network_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT ("%s Unable to stop network: %s\n", __FUNCTION__, error->message);
        g_error_free (error);
        qmi_message_wds_stop_network_output_unref (output);

        pNWStopCtx->uiCurrentStep++;
        cellular_hal_qmi_stop_network_connection_step(task);
        return;
    }

    CELLULAR_HAL_DBG_PRINT ("%s [%s] Network stopped for IPv%d\n",
                                __FUNCTION__,
                                qmi_device_get_path_display (pstQMIContext->qmiDevice),
                                ( TRUE == bIsIPv4 ) ? 4 : 6);
                                
    qmi_message_wds_stop_network_output_unref (output);

    pNWStopCtx->uiCurrentStep++;
    cellular_hal_qmi_stop_network_connection_step(task);
}

static void cellular_hal_qmi_stop_network_connection_step( GTask *task )
{
    QMIContextStructPrivate    *pstQMIContext = NULL;
    ContextNWStop              *pNWStopCtx   = NULL;

    pNWStopCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWStopCtx->vpPrivateData;
    
    //CELLULAR_HAL_DBG_PRINT("%s %d - Inside State Stop Network Mode:%d Task:0x%p Step:%d QMI:%d\n",__FUNCTION__,__LINE__,pNWStartCtx->ip_request_type,task, pNWStartCtx->uiCurrentStep, qmi_device_is_open( pstQMIContext->qmiDevice ));

    switch ( pNWStopCtx->uiCurrentStep ) 
    {
        case WDS_NW_STOP_BEGIN:
        {
            pNWStopCtx->uiCurrentStep++;
            /* fall through */
        }
        case WDS_NW_STOP_DEREGISTER_EVENTS:
        {
            ContextWDSInfo          *wdsCtx = &(pstQMIContext->wdsCtx); 
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStopCtx->ip_request_type ) ? TRUE : FALSE;

            CELLULAR_HAL_DBG_PRINT("%s - Deregistering Events for IPv%d\n",__FUNCTION__,((bIsIPv4)? 4 : 6));

            //Cleanup events signal before stopping network
            if( ( TRUE == bIsIPv4 ) && ( CELLULAR_QMI_INVALID_EVENT_INDICATION != wdsCtx->event_report_ipv4_id ) )
            {
                g_signal_handler_disconnect (wdsCtx->client_ipv4, wdsCtx->event_report_ipv4_id);
                wdsCtx->event_report_ipv4_id = CELLULAR_QMI_INVALID_EVENT_INDICATION;
            }
            else
            {
                if( CELLULAR_QMI_INVALID_EVENT_INDICATION != wdsCtx->event_report_ipv6_id )
                {
                    g_signal_handler_disconnect (wdsCtx->client_ipv6, wdsCtx->event_report_ipv6_id);
                    wdsCtx->event_report_ipv6_id = CELLULAR_QMI_INVALID_EVENT_INDICATION;
                }
            }

            if( ( TRUE == bIsIPv4 ) && ( CELLULAR_QMI_INVALID_EVENT_INDICATION != wdsCtx->packet_service_status_ipv4_id ) )
            {
                QmiMessageWdsSetEventReportInput *input;

                g_signal_handler_disconnect (wdsCtx->client_ipv4, wdsCtx->packet_service_status_ipv4_id);
                wdsCtx->packet_service_status_ipv4_id = CELLULAR_QMI_INVALID_EVENT_INDICATION;

                input = cellular_hal_qmi_event_report_input_new(FALSE);
                qmi_client_wds_set_event_report (QMI_CLIENT_WDS(wdsCtx->client_ipv4),
                                                input,
                                                5,
                                                NULL,
                                                NULL,
                                                NULL);
                qmi_message_wds_set_event_report_input_unref (input);
            }
            else
            {
                if ( CELLULAR_QMI_INVALID_EVENT_INDICATION != wdsCtx->packet_service_status_ipv6_id )
                {
                    QmiMessageWdsSetEventReportInput *input;

                    g_signal_handler_disconnect (wdsCtx->client_ipv6, wdsCtx->packet_service_status_ipv6_id);
                    wdsCtx->packet_service_status_ipv6_id = CELLULAR_QMI_INVALID_EVENT_INDICATION;

                    input = cellular_hal_qmi_event_report_input_new(FALSE);
                    qmi_client_wds_set_event_report (QMI_CLIENT_WDS(wdsCtx->client_ipv6),
                                                    input,
                                                    5,
                                                    NULL,
                                                    NULL,
                                                    NULL);
                    qmi_message_wds_set_event_report_input_unref (input);
                }
            }
            
            pNWStopCtx->uiCurrentStep++;
            /* fall through */
        }
        case WDS_NW_STOP_STOP_NETWORK:
        {
            ContextWDSInfo          *wdsCtx = &(pstQMIContext->wdsCtx);  
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStopCtx->ip_request_type ) ? TRUE : FALSE;

            //Stop Network
            if( ( TRUE == bIsIPv4 ) && (wdsCtx->packet_data_handle_ipv4 != CELLULAR_PACKET_DATA_INVALID_HANDLE ) )
            {
                QmiMessageWdsStopNetworkInput *stopinput; 

                stopinput = qmi_message_wds_stop_network_input_new ();
                qmi_message_wds_stop_network_input_set_packet_data_handle (stopinput, wdsCtx->packet_data_handle_ipv4, NULL);
                
                //Roaming disable or not disable TBD
                //qmi_message_wds_stop_network_input_set_disable_autoconnect (input, TRUE, NULL);
            
                CELLULAR_HAL_DBG_PRINT ("%s - Calling Stop Network for IPv4... Task:0x%p\n",__FUNCTION__,task);
                qmi_client_wds_stop_network (QMI_CLIENT_WDS(wdsCtx->client_ipv4), 
                                            stopinput,
                                            120,
                                            NULL,
                                            (GAsyncReadyCallback)cellular_hal_qmi_stop_network_cb,
                                            task);
                qmi_message_wds_stop_network_input_unref (stopinput);

                return;
            }
            else
            {
                if (wdsCtx->packet_data_handle_ipv6 != CELLULAR_PACKET_DATA_INVALID_HANDLE )
                {
                    QmiMessageWdsStopNetworkInput *stopinput; 

                    stopinput = qmi_message_wds_stop_network_input_new ();
                    qmi_message_wds_stop_network_input_set_packet_data_handle (stopinput, wdsCtx->packet_data_handle_ipv6, NULL);
                    
                    //Roaming disable or not disable TBD
                    //qmi_message_wds_stop_network_input_set_disable_autoconnect (input, TRUE, NULL);
                
                    CELLULAR_HAL_DBG_PRINT ("%s - Calling Stop Network for IPv6... Task:0x%p\n",__FUNCTION__,task);
                    qmi_client_wds_stop_network (QMI_CLIENT_WDS(wdsCtx->client_ipv6), 
                                                stopinput,
                                                120,
                                                NULL,
                                                (GAsyncReadyCallback)cellular_hal_qmi_stop_network_cb,
                                                task);
                    qmi_message_wds_stop_network_input_unref (stopinput);

                    return;
                }
            }

            pNWStopCtx->uiCurrentStep++;
            //fall through
        }
        case WDS_NW_STOP_RELEASE_WDS_CLIENT:
        {
            ContextWDSInfo          *wdsCtx = &(pstQMIContext->wdsCtx);  
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStopCtx->ip_request_type ) ? TRUE : FALSE;

            if( TRUE == bIsIPv4 )
            {
                if( NULL != wdsCtx->client_ipv4 )
                {
                    qmi_device_release_client ( pstQMIContext->qmiDevice,
                                                wdsCtx->client_ipv4,
                                                QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                                3, NULL, NULL, NULL);

                    g_clear_object (&(wdsCtx->client_ipv4));
                    wdsCtx->client_ipv4 = NULL;
                }
            }
            else
            {
                if( NULL != wdsCtx->client_ipv6 )
                {
                    qmi_device_release_client ( pstQMIContext->qmiDevice,
                                                wdsCtx->client_ipv6,
                                                QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                                3, NULL, NULL, NULL);

                    g_clear_object (&(wdsCtx->client_ipv6));
                    wdsCtx->client_ipv6 = NULL;
                }
            }
	    if ((wdsCtx->client_ipv6 == NULL) && (wdsCtx->client_ipv4 == NULL))
                memset(wdsCtx->currentRAT, 0, sizeof(wdsCtx->currentRAT));
            pNWStopCtx->uiCurrentStep++;
        }
        case WDS_NW_STOP_END:
        default:
        {
            unsigned char bIsIPv4 = ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == pNWStopCtx->ip_request_type ) ? TRUE : FALSE;

            //Reset the flag
            if( TRUE == bIsIPv4 )
            {
                pstQMIContext->wdsCtx.bIsStopNetworkIPv4InProgress = FALSE;
            }
            else
            {
                pstQMIContext->wdsCtx.bIsStopNetworkIPv6InProgress = FALSE;
            }

            pNWStopCtx->uiCurrentStep++;
            g_object_unref (task);
        }
    }
}

static void cellular_hal_qmi_stop_network_context_free( ContextNWStop *pNWStopCtx )
{
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources for Mode:%d\n",__FUNCTION__,pNWStopCtx->ip_request_type);

    if( NULL != pNWStopCtx)
    {
        g_slice_free(ContextNWStop, pNWStopCtx);
        pNWStopCtx  = NULL;
    }
}

int cellular_hal_qmi_stop_network(CellularNetworkIPType_t ip_request_type)
{
        //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextNWStop  *pNWStopCtx = NULL;
        GTask          *task;

        if( ( CELLULAR_NETWORK_IP_FAMILY_IPV4 == ip_request_type ) && ( TRUE == gpstQMIContext->wdsCtx.bIsStopNetworkIPv4InProgress ) )
        {
            CELLULAR_HAL_DBG_PRINT("%s - IPv4 Network Stop is in progress so ignore stop\n",__FUNCTION__);
            return RETURN_OK;
        }

        if( ( CELLULAR_NETWORK_IP_FAMILY_IPV6 == ip_request_type ) && ( TRUE == gpstQMIContext->wdsCtx.bIsStopNetworkIPv6InProgress ) )
        {
            CELLULAR_HAL_DBG_PRINT("%s - IPv6 Network Stop is in progress so ignore stop\n",__FUNCTION__);
            return RETURN_OK;
        }

        pNWStopCtx         = (ContextNWStop*)g_slice_new0 (ContextNWStop);
        pNWStopCtx->uiCurrentStep   = WDS_NW_STOP_BEGIN;
        pNWStopCtx->vpPrivateData   = (void*) gpstQMIContext;
        pNWStopCtx->ip_request_type = ip_request_type;

        //Mark Start NW inprogress
        if( CELLULAR_NETWORK_IP_FAMILY_IPV4 == ip_request_type )
        {
            gpstQMIContext->wdsCtx.bIsStopNetworkIPv4InProgress = TRUE;
        }
        
        if( CELLULAR_NETWORK_IP_FAMILY_IPV6 == ip_request_type )
        {
            gpstQMIContext->wdsCtx.bIsStopNetworkIPv6InProgress = TRUE;
        }

        task = g_task_new (NULL, NULL, NULL, NULL);
        g_task_set_task_data (task, (gpointer)pNWStopCtx, (GDestroyNotify)cellular_hal_qmi_stop_network_context_free);

        CELLULAR_HAL_DBG_PRINT("%s - Stopping Network Mode:%d Task:0x%p\n",__FUNCTION__,ip_request_type,task);
        cellular_hal_qmi_stop_network_connection_step(task);
    }
    else
    {
        //Network Can't be stopped due to QMI device not ready
        CELLULAR_HAL_DBG_PRINT("%s - QMI is not ready so cant be stop network\n",__FUNCTION__);
    }

    return RETURN_OK;
}

static void
cellular_hal_qmi_get_channel_rates (QmiClientWds *wdsClient,
                                          GAsyncResult *result,
                                          gpointer  user_data)
{
    QmiMessageWdsGetChannelRatesOutput *output;
    guint32 txrate = 0, rxrate = 0, maxtxrate = 0, maxrxrate = 0;
    GError *error = NULL;

    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate *pstQMIContext  = NULL;
    ContextNWPacketStats    *pNWPktStatsCtx = NULL;
    ContextWDSInfo          *wdsCtx         = NULL;
    CellularPacketStatsStruct   *pstPacketStats = NULL;

    pNWPktStatsCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWPktStatsCtx->vpPrivateData;
    wdsCtx = &(pstQMIContext->wdsCtx);
    pstPacketStats = &(wdsCtx->stPacketStats);

    output = qmi_client_wds_get_channel_rates_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Get Channel rates operation failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        goto NEXTSTEP;
    }

    if (!qmi_message_wds_get_channel_rates_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Unable to get channel rates: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_wds_get_channel_rates_output_unref (output);
        goto NEXTSTEP;
    }

    qmi_message_wds_get_channel_rates_output_get_channel_rates (output,
                                                                &txrate,
                                                                &rxrate,
                                                                &maxtxrate,
                                                                &maxrxrate,
                                                                NULL);

    pstPacketStats->UpStreamMaxBitRate   = (maxtxrate / 1000); //Kbps
    pstPacketStats->DownStreamMaxBitRate = (maxrxrate / 1000); //Kbps

    qmi_message_wds_get_channel_rates_output_unref (output);

NEXTSTEP:
    pNWPktStatsCtx->uiCurrentStep++;
    cellular_hal_qmi_get_network_packet_stats_step(task);
}

static void  cellular_hal_qmi_get_packet_statistics_cb ( QmiClientWds *wdsClient,
                                                            GAsyncResult *result,
                                                            gpointer  user_data )
{
    GError *error = NULL;
    QmiMessageWdsGetPacketStatisticsOutput *output;
    guint32 uiValue;
    guint64 ulValue;

    GTask *task = (GTask*)user_data;
    QMIContextStructPrivate *pstQMIContext  = NULL;
    ContextNWPacketStats    *pNWPktStatsCtx = NULL;
    ContextWDSInfo          *wdsCtx         = NULL;
    CellularPacketStatsStruct   *pstPacketStats = NULL;

    pNWPktStatsCtx   = g_task_get_task_data (task);
    pstQMIContext = (QMIContextStructPrivate*)pNWPktStatsCtx->vpPrivateData;
    wdsCtx = &(pstQMIContext->wdsCtx);
    pstPacketStats = &(wdsCtx->stPacketStats);

    output = qmi_client_wds_get_packet_statistics_finish (wdsClient, result, &error);
    if (!output) {
        CELLULAR_HAL_DBG_PRINT("%s Get packet statistics operation failed: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        goto NEXTSTEP;
    }

    if (!qmi_message_wds_get_packet_statistics_output_get_result (output, &error)) {
        CELLULAR_HAL_DBG_PRINT("%s Unable to get packet statistics: %s\n",__FUNCTION__,error->message);
        g_error_free (error);
        qmi_message_wds_get_packet_statistics_output_unref (output);
        goto NEXTSTEP;
    }

    if (qmi_message_wds_get_packet_statistics_output_get_tx_packets_ok (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        pstPacketStats->PacketsSent = uiValue;
    }
        
    if (qmi_message_wds_get_packet_statistics_output_get_rx_packets_ok (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        pstPacketStats->PacketsReceived = uiValue;
    }

    if (qmi_message_wds_get_packet_statistics_output_get_tx_packets_error (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        //NA
    }
       
    if (qmi_message_wds_get_packet_statistics_output_get_rx_packets_error (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        //NA
    }

    if (qmi_message_wds_get_packet_statistics_output_get_tx_overflows (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        //NA
    }

    if (qmi_message_wds_get_packet_statistics_output_get_rx_overflows (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        //NA
    }

    if (qmi_message_wds_get_packet_statistics_output_get_tx_packets_dropped (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        pstPacketStats->PacketsSentDrop = uiValue;
    }

    if (qmi_message_wds_get_packet_statistics_output_get_rx_packets_dropped (output, &uiValue, NULL) &&
        uiValue != CELLULAR_QMI_INVALID_PACKET_STATS_VALUE)
    {
        pstPacketStats->PacketsReceivedDrop = uiValue;
    }

    if (qmi_message_wds_get_packet_statistics_output_get_tx_bytes_ok (output, &ulValue, NULL))
    {
        pstPacketStats->BytesSent = ulValue;
    }

    if (qmi_message_wds_get_packet_statistics_output_get_rx_bytes_ok (output, &ulValue, NULL))
    {
        pstPacketStats->BytesReceived = ulValue;
    }

    if (qmi_message_wds_get_packet_statistics_output_get_last_call_tx_bytes_ok (output, &ulValue, NULL))
    {
        //NA
    }

    if (qmi_message_wds_get_packet_statistics_output_get_last_call_rx_bytes_ok (output, &ulValue, NULL))
    {
        //NA
    }

    qmi_message_wds_get_packet_statistics_output_unref (output);

NEXTSTEP:
    pNWPktStatsCtx->uiCurrentStep++;
    cellular_hal_qmi_get_network_packet_stats_step(task);
}

static void cellular_hal_qmi_get_network_packet_stats_step( GTask *task )
{
    QMIContextStructPrivate    *pstQMIContext = NULL;
    ContextNWPacketStats  *pNWPktStatsCtx = NULL;

    pNWPktStatsCtx = g_task_get_task_data (task);
    pstQMIContext  = (QMIContextStructPrivate*)pNWPktStatsCtx->vpPrivateData;

    switch ( pNWPktStatsCtx->uiCurrentStep ) 
    {
        case WDS_NW_PACKET_STATS_BEGIN:
        {
            pNWPktStatsCtx->uiCurrentStep++;
        }
        case WDS_NW_PACKET_STATS_GET_PACKET_STATISTICS:
        {
            QmiMessageWdsGetPacketStatisticsInput *input;
            ContextWDSInfo  *wdsCtx = &(pstQMIContext->wdsCtx); 

            input = qmi_message_wds_get_packet_statistics_input_new ();
            qmi_message_wds_get_packet_statistics_input_set_mask (  input,
                                                                    (QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_PACKETS_OK      |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_PACKETS_OK      |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_PACKETS_ERROR   |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_PACKETS_ERROR   |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_OVERFLOWS       |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_OVERFLOWS       |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_BYTES_OK        |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_BYTES_OK        |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_PACKETS_DROPPED |
                                                                    QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_PACKETS_DROPPED),
                                                                    NULL );

            qmi_client_wds_get_packet_statistics (  QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                    input,
                                                    10,
                                                    NULL,
                                                    (GAsyncReadyCallback)cellular_hal_qmi_get_packet_statistics_cb,
                                                    task);
            qmi_message_wds_get_packet_statistics_input_unref (input);
            return;
        }
        case WDS_NW_PACKET_STATS_GET_CHANNEL_RATES:
        {
            ContextWDSInfo  *wdsCtx = &(pstQMIContext->wdsCtx); 

            qmi_client_wds_get_channel_rates ( QMI_CLIENT_WDS(wdsCtx->wdsClient),
                                                NULL,
                                                10,
                                                NULL,
                                                (GAsyncReadyCallback)cellular_hal_qmi_get_channel_rates,
                                                task);
            return;
        }
        case WDS_NW_PACKET_STATS_END:
        default:
        {
            ContextWDSInfo  *wdsCtx = &(pstQMIContext->wdsCtx); 

            wdsCtx->IsPacketStatsCollectionDone = TRUE;
            g_object_unref (task);
        }
    }
}

int cellular_hal_qmi_get_current_radio_technology( char *current_rat)
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextWDSInfo  *wdsCtx = &(gpstQMIContext->wdsCtx); 
        if ((wdsCtx->client_ipv6 != NULL) || (wdsCtx->client_ipv4 != NULL))
        {
	    if( '\0' != wdsCtx->currentRAT[0] )
            {
               strncpy( current_rat, wdsCtx->currentRAT, strlen(wdsCtx->currentRAT) + 1 );
               CELLULAR_HAL_DBG_PRINT("%s -  current RAT '%s' Retrived via DML\n", __FUNCTION__,current_rat);
            }
        }
        else
        {
           CELLULAR_HAL_DBG_PRINT("%s No active bearer .. current RAT will be empty \n",__FUNCTION__);
	   return RETURN_ERROR;
        }
    }
    return RETURN_OK;
}

static void cellular_hal_qmi_get_network_packet_stats_context_free( ContextNWPacketStats *pNWPktStatsCtx )
{
    CELLULAR_HAL_DBG_PRINT("%s Freeing allocated resources\n",__FUNCTION__);

    if( NULL != pNWPktStatsCtx)
    {
        g_slice_free(ContextNWPacketStats, pNWPktStatsCtx);
        pNWPktStatsCtx  = NULL;
    }
}

int cellular_hal_qmi_get_packet_statistics( CellularPacketStatsStruct *packet_stats )
{
    //Check whether QMI ready or not
    if( ( NULL != gpstQMIContext ) && \
        ( NULL != gpstQMIContext->qmiDevice ) && \
        ( TRUE == qmi_device_is_open( gpstQMIContext->qmiDevice ) ) )
    {
        ContextWDSInfo  *wdsCtx = &(gpstQMIContext->wdsCtx); 

	    if ( ( cellular_hal_util_GetCurrentTimeInSeconds() - wdsCtx->last_packet_stats_updated_time ) < CELLULAR_QMI_GETPKTSTATS_VIA_DML_TTL_FOR_STATS_INFO )
        {
            memset(packet_stats, 0, sizeof(CellularPacketStatsStruct));
            memcpy(packet_stats, &(wdsCtx->stPacketStats), sizeof(CellularPacketStatsStruct));
            //CELLULAR_HAL_DBG_PRINT("%s - (C) Successfully got network packet statistics\n",__FUNCTION__);
        }
        else
        {
            ContextNWPacketStats  *pNWPktStatsCtx = NULL;
            GTask                 *task;
            int                   iLapsedSeconds = 0;

            pNWPktStatsCtx         = (ContextNWPacketStats*)g_slice_new0 (ContextNWPacketStats);
            memset(pNWPktStatsCtx, 0, sizeof(ContextNWPacketStats));
            pNWPktStatsCtx->uiCurrentStep   = WDS_NW_PACKET_STATS_BEGIN;
            pNWPktStatsCtx->vpPrivateData   = (void*) gpstQMIContext;

            wdsCtx->IsPacketStatsCollectionDone = FALSE;

            task = g_task_new (NULL, NULL, NULL, NULL);
            g_task_set_task_data (task, (gpointer)pNWPktStatsCtx, (GDestroyNotify)cellular_hal_qmi_get_network_packet_stats_context_free);

            //CELLULAR_HAL_DBG_PRINT("%s - Querying Network Packet Statistics Task:0x%p\n",__FUNCTION__,task);
            cellular_hal_qmi_get_network_packet_stats_step(task);

            //Wait here till stats collection
            while ( 1 )
            {
                if( TRUE == wdsCtx->IsPacketStatsCollectionDone )
                {
                    memset(packet_stats, 0, sizeof(CellularPacketStatsStruct));
                    memcpy(packet_stats, &(wdsCtx->stPacketStats), sizeof(CellularPacketStatsStruct));
                    wdsCtx->last_packet_stats_updated_time = cellular_hal_util_GetCurrentTimeInSeconds();

                    //CELLULAR_HAL_DBG_PRINT("%s - (O) Successfully got network packet statistics\n",__FUNCTION__);
                    break;
                }
                else if( iLapsedSeconds >= CELLULAR_QMI_GETIDS_VIA_DML_MAX_WAITIME )  
                {
                    CELLULAR_HAL_DBG_PRINT("%s - Timeout during IDs Query so sending empty packet stats\n",__FUNCTION__);
                    break;
                }

                sleep(1);
                iLapsedSeconds++;
            }
        }

        return RETURN_OK;
    }
    else
    {
        CELLULAR_HAL_DBG_PRINT("%s - QMI(%s) not ready so can't get modem statistics\n", __FUNCTION__, gpstQMIContext->modem_device_name);
        return RETURN_ERROR;
    }
}
