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
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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

#ifdef RBUS_BUILD_FLAG_ENABLE

#include "cellularmgr_rbus_events.h"

extern rbusHandle_t gBusHandle;
char componentName[32] = "CELLULARMANAGER";

/* RBUS Subscription Variables List */
CellularMGR_rbusSubListSt gRBUSSubListSt = {0}; 

static void CellularMgr_ConvertEnumToString(void* EnumValue, CellularMGR_EnumToString EnumType, char* String)
{
    char *Ptr = NULL;

    if (EnumType == ENUM_RADIOENVCONDITIONS)
    {
        CELLULAR_RADIO_ENV_CONDITIONS *Enum = (CELLULAR_RADIO_ENV_CONDITIONS*)EnumValue;

        Ptr = ((*Enum == RADIO_ENV_CONDITION_EXCELLENT)? "EXCELLENT":
               ((*Enum == RADIO_ENV_CONDITION_GOOD)? "GOOD":
               ((*Enum == RADIO_ENV_CONDITION_FAIR)? "FAIR":
               ((*Enum == RADIO_ENV_CONDITION_POOR)? "POOR":"UNAVAILABLE"))));
    }
    else if (EnumType == ENUM_X_RDK_STATUS)
    {
        CELLULAR_RDK_STATUS *Enum = (CELLULAR_RDK_STATUS*)EnumValue;

        Ptr = ((*Enum == RDK_STATUS_DOWN)? "Down":
               ((*Enum == RDK_STATUS_DEACTIVATED)? "Deactivated":
               ((*Enum == RDK_STATUS_DEREGISTERED)? "Deregistered":
               ((*Enum == RDK_STATUS_REGISTERED)? "Registered":
               ((*Enum == RDK_STATUS_CONNECTED)? "Connected":"Error")))));
    }
    if (Ptr != NULL)
    {
        AnscCopyString(String , Ptr);
    }
}

/* RBUS Subscription Monitor Thread */
void* CellularMgr_RBUS_Events_Monitor_Thread( void *arg )
{
    CELLULAR_INTERFACE_SERVING_INFO  stBackupServingInfo  = {0};
    CELLULAR_RADIO_ENV_CONDITIONS    enPrevValue = RADIO_ENV_CONDITION_UNAVAILABLE;
    CELLULAR_RDK_STATUS              enPrevRdkStatusValue = RDK_STATUS_DOWN;
    PCELLULARMGR_CELLULAR_DATA  pMyObject =  (PCELLULARMGR_CELLULAR_DATA) arg;
    PCELLULAR_DML_INFO  pstDmlCellular =  (PCELLULAR_DML_INFO) pMyObject->pstDmlCellular;

    //detach thread from caller stack
    pthread_detach(pthread_self());

    CcspTraceInfo(("%s %d - Entry\n",__FUNCTION__,__LINE__));
    memset(&stBackupServingInfo, 0, sizeof(CELLULAR_INTERFACE_SERVING_INFO));

    while( 1 )
    {
        sleep(RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL);

        /* Fetching Cellular Current State */
        CELLULAR_RDK_STATUS  X_RDK_Status = CellularMgrSMGetCurrentState( );
        int i;

        //When it is changing we need to publish if subscribed by others
        if( gRBUSSubListSt.stRadioSignal.RadioEnvCondSubFlag )
        {
            CELLULAR_RADIO_ENV_CONDITIONS enCurrentValue = CellularMgr_GetRadioEnvConditions( );
            BOOL bIsNeedToPublish = FALSE;

            if( gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval )
            {
                gRBUSSubListSt.stRadioSignal.RadioEnvCondLapsedCounterTimer += RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL;

                if( gRBUSSubListSt.stRadioSignal.RadioEnvCondLapsedCounterTimer >= gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval )
                {
                    gRBUSSubListSt.stRadioSignal.RadioEnvCondLapsedCounterTimer = 0;
                    bIsNeedToPublish = TRUE;
                }
            }
            else 
            {
                if ( enPrevValue != enCurrentValue  )
                {
                    bIsNeedToPublish = TRUE;
                }
            }

            if ( bIsNeedToPublish )
            {
                char acTmpPrevValue[32] = {0},
                     acTmpCurValue[32]  = {0},
                     acTmpParamName[256] = {0};

                CellularMgr_ConvertEnumToString((void*)&enPrevValue,ENUM_RADIOENVCONDITIONS,acTmpPrevValue);
                CellularMgr_ConvertEnumToString((void*)&enCurrentValue,ENUM_RADIOENVCONDITIONS,acTmpCurValue);
                enPrevValue = enCurrentValue;

                for( i = 0; i < pstDmlCellular->ulInterfaceNoEntries; i++ )
                {
                    snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_RADIOSIGNAL_RADIOENVCONDITIONS, i + 1);

                    CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s) Value(%d)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue,enCurrentValue));
                    CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_STRING);
                }
            }
        }

        if( gRBUSSubListSt.X_RDK_StatusSubFlag )
        {
            if ( enPrevRdkStatusValue != X_RDK_Status )
            {
                char acTmpPrevValue[32] = {0},
                     acTmpCurValue[32]  = {0},
                     acTmpParamName[256] = {0};

                CellularMgr_ConvertEnumToString((void*)&enPrevRdkStatusValue,ENUM_X_RDK_STATUS,acTmpPrevValue);
                CellularMgr_ConvertEnumToString((void*)&X_RDK_Status,ENUM_X_RDK_STATUS,acTmpCurValue);
                snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_X_RDK_STATUS);
                enPrevRdkStatusValue = X_RDK_Status;

                CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s) Value(%d)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue,X_RDK_Status));
                CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_STRING);
            }
        }
        else /** we need to store current value in each iteration when there is no subscription */
        {
            enPrevRdkStatusValue = X_RDK_Status;
        }

        /* If there is no subscription then no need to proceed further */
        if( ( !gRBUSSubListSt.stRadioSignal.RSSISubFlag ) &&
            ( !gRBUSSubListSt.stRadioSignal.SNRSubFlag ) &&
            ( !gRBUSSubListSt.stRadioSignal.RSRPSubFlag ) && 
            ( !gRBUSSubListSt.stRadioSignal.RSRQSubFlag ) && 
            ( !gRBUSSubListSt.stRadioSignal.TRXSubFlag )  )
        {
            continue;
        }

        if( ( X_RDK_Status != RDK_STATUS_DOWN ) &&
            ( X_RDK_Status != RDK_STATUS_DEACTIVATED ) )
        {
            CELLULAR_INTERFACE_SERVING_INFO stCurrentServingInfo  = {0};

            //Fetch latest signal information from HAL
            CellularMgr_RadioSignalGetSignalInfo( &stCurrentServingInfo );

            for( i = 0; i < pstDmlCellular->ulInterfaceNoEntries; i++ )
            {
                //When it is changing we need to publish if subscribed by others
                if( gRBUSSubListSt.stRadioSignal.RSSISubFlag )
                {
                    BOOL bIsNeedToPublish = FALSE;

                    if( gRBUSSubListSt.stRadioSignal.RSSISubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.RSSILapsedCounterTimer += RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL;

                        if( gRBUSSubListSt.stRadioSignal.RSSILapsedCounterTimer >= gRBUSSubListSt.stRadioSignal.RSSISubInterval )
                        {
                            gRBUSSubListSt.stRadioSignal.RSSILapsedCounterTimer = 0;
                            bIsNeedToPublish = TRUE;
                        }
                    }
                    else 
                    {
                        if ( stBackupServingInfo.Rssi != stCurrentServingInfo.Rssi )
                        {
                            bIsNeedToPublish = TRUE;
                        }
                    }

                    if( bIsNeedToPublish  )
                    {
                        char acTmpPrevValue[32]   = {0},
                            acTmpCurValue[32]    = {0},
                            acTmpParamName[256]  = {0};

                        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", stBackupServingInfo.Rssi);
                        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", stCurrentServingInfo.Rssi);
                        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_RADIOSIGNAL_RSSI, i + 1);

                        stBackupServingInfo.Rssi = stCurrentServingInfo.Rssi;

                        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
                        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_INT32);
                    }
                }

                if( gRBUSSubListSt.stRadioSignal.SNRSubFlag )
                {
                    BOOL bIsNeedToPublish = FALSE;

                    if( gRBUSSubListSt.stRadioSignal.SNRSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.SNRLapsedCounterTimer += RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL;

                        if( gRBUSSubListSt.stRadioSignal.SNRLapsedCounterTimer >= gRBUSSubListSt.stRadioSignal.SNRSubInterval )
                        {
                            gRBUSSubListSt.stRadioSignal.SNRLapsedCounterTimer = 0;
                            bIsNeedToPublish = TRUE;
                        }
                    }
                    else 
                    {
                        if ( stBackupServingInfo.Snr != stCurrentServingInfo.Snr )
                        {
                            bIsNeedToPublish = TRUE;
                        }
                    }

                    if( bIsNeedToPublish )
                    {
                        char acTmpPrevValue[32]   = {0},
                            acTmpCurValue[32]    = {0},
                            acTmpParamName[256]  = {0};

                        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", stBackupServingInfo.Snr);
                        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", stCurrentServingInfo.Snr);
                        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_RADIOSIGNAL_SNR, i + 1);

                        stBackupServingInfo.Snr = stCurrentServingInfo.Snr;

                        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
                        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_INT32);
                    }
                }

                if( gRBUSSubListSt.stRadioSignal.TRXSubFlag )
                {
                    BOOL bIsNeedToPublish = FALSE;

                    if( gRBUSSubListSt.stRadioSignal.TRXSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.TRXLapsedCounterTimer += RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL;

                        if( gRBUSSubListSt.stRadioSignal.TRXLapsedCounterTimer >= gRBUSSubListSt.stRadioSignal.TRXSubInterval )
                        {
                            gRBUSSubListSt.stRadioSignal.TRXLapsedCounterTimer = 0;
                            bIsNeedToPublish = TRUE;
                        }
                    }
                    else 
                    {
                        if ( stBackupServingInfo.Trx != stCurrentServingInfo.Trx )
                        {
                            bIsNeedToPublish = TRUE;
                        }
                    }

                    if( bIsNeedToPublish  )
                    {
                        char acTmpPrevValue[32]   = {0},
                            acTmpCurValue[32]    = {0},
                            acTmpParamName[256]  = {0};

                        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", stBackupServingInfo.Trx);
                        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", stCurrentServingInfo.Trx);
                        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_RADIOSIGNAL_TRX, i + 1);

                        stBackupServingInfo.Trx = stCurrentServingInfo.Trx;

                        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
                        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_INT32);
                    }
                }

                if( gRBUSSubListSt.stRadioSignal.RSRQSubFlag )
                {
                    BOOL bIsNeedToPublish = FALSE;

                    if( gRBUSSubListSt.stRadioSignal.RSRQSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.RSRQLapsedCounterTimer += RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL;

                        if( gRBUSSubListSt.stRadioSignal.RSRQLapsedCounterTimer >= gRBUSSubListSt.stRadioSignal.RSRQSubInterval )
                        {
                            gRBUSSubListSt.stRadioSignal.RSRQLapsedCounterTimer = 0;
                            bIsNeedToPublish = TRUE;
                        }
                    }
                    else 
                    {
                        if ( stBackupServingInfo.Rsrq != stCurrentServingInfo.Rsrq )
                        {
                            bIsNeedToPublish = TRUE;
                        }
                    }

                    if( bIsNeedToPublish  )
                    {
                        char acTmpPrevValue[32]   = {0},
                            acTmpCurValue[32]    = {0},
                            acTmpParamName[256]  = {0};

                        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", stBackupServingInfo.Rsrq);
                        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", stCurrentServingInfo.Rsrq);
                        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_RADIOSIGNAL_RSRQ, i + 1);

                        stBackupServingInfo.Rsrq = stCurrentServingInfo.Rsrq;

                        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
                        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_INT32);
                    }
                }

                if( gRBUSSubListSt.stRadioSignal.RSRPSubFlag )
                {
                    BOOL bIsNeedToPublish = FALSE;

                    if( gRBUSSubListSt.stRadioSignal.RSRPSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.RSRPLapsedCounterTimer += RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL;

                        if( gRBUSSubListSt.stRadioSignal.RSRPLapsedCounterTimer >= gRBUSSubListSt.stRadioSignal.RSRPSubInterval )
                        {
                            gRBUSSubListSt.stRadioSignal.RSRPLapsedCounterTimer = 0;
                            bIsNeedToPublish = TRUE;
                        }
                    }
                    else 
                    {
                        if ( stBackupServingInfo.Rsrp != stCurrentServingInfo.Rsrp )
                        {
                            bIsNeedToPublish = TRUE;
                        }
                    }

                    if( bIsNeedToPublish  )
                    {
                        char acTmpPrevValue[32]   = {0},
                            acTmpCurValue[32]    = {0},
                            acTmpParamName[256]  = {0};

                        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", stBackupServingInfo.Rsrp);
                        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", stCurrentServingInfo.Rsrp);
                        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_RADIOSIGNAL_RSRP, i + 1);

                        stBackupServingInfo.Rsrp = stCurrentServingInfo.Rsrp;

                        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
                        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_INT32);
                    }
                }
            }
        }
    }

    CcspTraceInfo(("%s %d - Exit\n",__FUNCTION__,__LINE__));

    //Cleanup current thread when exit
    pthread_exit(NULL);
}

int CellularMgr_RBUS_Events_PublishPhyConnectionStatus( unsigned char bPrevPhyState, unsigned char bCurrentPhyState )
{
    if((gRBUSSubListSt.stInterface.PhyConnectedStatusSubFlag) && (bPrevPhyState != bCurrentPhyState))
    {
        char acTmpPrevValue[32]   = {0},
             acTmpCurValue[32]    = {0},
             acTmpParamName[256]  = {0};

        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", (bPrevPhyState) ? 1 : 0);
        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", (bCurrentPhyState) ? 1 : 0);
        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_PHY_CONNECTION_STATUS, 1);

        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_BOOLEAN);         
    }

    return RETURN_OK;
}

int CellularMgr_RBUS_Events_PublishLinkAvailableStatus( unsigned char bPrevLinkState, unsigned char bCurrentLinkState )
{
    if((gRBUSSubListSt.stInterface.LinkAvailableStatusSubFlag) && (bPrevLinkState != bCurrentLinkState))
    {
        char acTmpPrevValue[32]   = {0},
             acTmpCurValue[32]    = {0},
             acTmpParamName[256]  = {0};

        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", (bPrevLinkState) ? 1 : 0);
        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", (bCurrentLinkState) ? 1 : 0);
        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_LINK_AVAILABLE_STATUS, 1);

        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_BOOLEAN);         
    }

    return RETURN_OK;
}

int CellularMgr_RBUS_Events_Publish_X_RDK_Enable( unsigned char bPreviousValue, unsigned char bCurrentValue )
{
    if(gRBUSSubListSt.X_RDK_EnableSubFlag)
    {
        char acTmpPrevValue[32]   = {0},
             acTmpCurValue[32]    = {0},
             acTmpParamName[256]  = {0};

        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%d", bPreviousValue);
        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%d", bCurrentValue);
        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_X_RDK_ENABLE);

        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue));
        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_BOOLEAN);         
    }
}

int CellularMgr_RBUS_Events_PublishInterfaceStatus( CellularInterfaceStatus_t PrevState, CellularInterfaceStatus_t CurrentState )
{
    if ( ( gRBUSSubListSt.stInterface.InterfaceStatusSubFlag ) && ( PrevState != CurrentState ) )
    {
        char acTmpPrevValue[32] = {0},
            acTmpCurValue[32]  = {0},
            acTmpParamName[256] = {0};

        snprintf(acTmpPrevValue, sizeof(acTmpPrevValue), "%s", (PrevState == IF_UP) ? "Up": "Down");
        snprintf(acTmpCurValue, sizeof(acTmpCurValue), "%s", (CurrentState == IF_UP) ? "Up": "Down");
        snprintf(acTmpParamName, sizeof(acTmpParamName), CELLULARMGR_INFACE_STATUS, 1);

        CcspTraceInfo(("%s-%d: Publish DM(%s) Prev(%s) Current(%s) Value(%d)\n",__FUNCTION__, __LINE__, acTmpParamName,acTmpPrevValue,acTmpCurValue,CurrentState));
        CellularMgr_Rbus_String_EventPublish_OnValueChange(acTmpParamName, acTmpPrevValue, acTmpCurValue, RBUS_STRING);
    }

    return RETURN_OK;
}

/****************************************************************************************************************
  CellularMgr_Rbus_String_EventPublish_OnValueChange(): publish rbus events on value change
 ****************************************************************************************************************/
ANSC_STATUS CellularMgr_Rbus_String_EventPublish_OnValueChange(char *dm_event, void *prev_dm_value, void *dm_value, rbusValueType_t rbus_type)
{
    rbusEvent_t event;
    rbusObject_t rdata;
    rbusValue_t Value, preValue, byVal;
    int rc = ANSC_STATUS_FAILURE;

    if(dm_event == NULL || dm_value == NULL)
    {
        CcspTraceInfo(("%s %d - Failed publishing\n", __FUNCTION__, __LINE__));
        return rc;
    }

    rbusValue_Init(&Value);
    rbusValue_Init(&preValue);

    switch(rbus_type)
    {
        case RBUS_BOOLEAN:
            rbusValue_SetBoolean(Value, atoi(dm_value));
            rbusValue_SetBoolean(preValue, atoi(prev_dm_value));
        break;

        case RBUS_INT32:
            rbusValue_SetInt32(Value, atoi(dm_value));
            rbusValue_SetInt32(preValue, atoi(prev_dm_value));
        break;

        case RBUS_STRING:
            rbusValue_SetString(Value, (char*)dm_value);
            rbusValue_SetString(preValue, (char*)prev_dm_value);
        break;

        default:
            rbusValue_Release(Value);
            rbusValue_Release(preValue);
            return ANSC_STATUS_FAILURE;
    }

    rbusValue_Init(&byVal);
    rbusValue_SetString(byVal, componentName);

    rbusObject_Init(&rdata, NULL);
    rbusObject_SetValue(rdata, "value", Value);
    rbusObject_SetValue(rdata, "oldValue", preValue);
    rbusObject_SetValue(rdata, "by", byVal);

    event.name = dm_event;
    event.data = rdata;
    event.type = RBUS_EVENT_VALUE_CHANGED;

    CcspTraceInfo(("%s %d - dm_event[%s],prev_dm_value[%s],dm_value[%s]\n", __FUNCTION__, __LINE__, dm_event, prev_dm_value, dm_value));

    if(rbusEvent_Publish(gBusHandle, &event) != RBUS_ERROR_SUCCESS)
    {
        CcspTraceInfo(("%s %d - event publishing failed for type\n", __FUNCTION__, __LINE__));
    }
    else
    {
        CcspTraceInfo(("%s %d - Successfully Published event for event %s \n", __FUNCTION__, __LINE__, dm_event));
        rc = ANSC_STATUS_SUCCESS;
    }

    rbusValue_Release(Value);
    rbusValue_Release(preValue);
    rbusValue_Release(byVal);
    rbusObject_Release(rdata);
    return rc;
}

/***********************************************************************
  Event subscribe handler API for objects:
 ***********************************************************************/
rbusError_t CellularMgrDmlPublishEventHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    char *subscribe_action = NULL;

    CcspTraceInfo(("%s %d - Event %s has been subscribed from subscribed\n", __FUNCTION__, __LINE__,eventName ));
    subscribe_action = action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe";
    CcspTraceInfo(("%s %d - action=%s \n", __FUNCTION__, __LINE__, subscribe_action ));

    if(eventName == NULL)
    {
        CcspTraceInfo(("%s %d - Property get name is NULL\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_BUS_ERROR;
    }

    *autoPublish = FALSE;

    /** 
     * We need to process publish based on least interval value for ON-INTERVAL change.
     * For example,
     * Initially one param is asking subscription for 60seconds interval. 
     * After sometime same param is asking subscription for 30seconds interval then we need to send 
     * event for least interval timer.
     * 
     * Return error when the proposed interval is not valid which means value not % RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL
    */

    if(strstr(eventName, ".X_RDK_RadioEnvConditions"))
    {
        int iProposedInterval = 0;

        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            if( 0 == gRBUSSubListSt.stRadioSignal.RadioEnvCondSubFlag )
            {
                gRBUSSubListSt.stRadioSignal.RadioEnvCondLapsedCounterTimer = 0;
                gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval = 0;
            }

            if( interval > 0 )
            {
                if( 0 == ( interval % RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL ) )
                {
                    iProposedInterval = interval;
                }
                else
                {
                    CcspTraceInfo(("%s-%d : Unable to subcribe X_RDK_RadioEnvConditions due to invalid interval(%d)\n", __FUNCTION__, __LINE__, interval));
                    return RBUS_ERROR_BUS_ERROR;
                }

                if( 0 == gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval )
                {
                    gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval = iProposedInterval;
                    gRBUSSubListSt.stRadioSignal.RadioEnvCondLapsedCounterTimer = 0;
                }
                else
                {
                    if( iProposedInterval < gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval = iProposedInterval;
                        gRBUSSubListSt.stRadioSignal.RadioEnvCondLapsedCounterTimer = 0;
                    }
                }
            }

            gRBUSSubListSt.stRadioSignal.RadioEnvCondSubFlag++;
            CcspTraceInfo(("%s-%d : X_RDK_RadioEnvConditions Sub(%d) Interval(%d)\n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RadioEnvCondSubFlag,gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval));
        }
        else
        {
            if (gRBUSSubListSt.stRadioSignal.RadioEnvCondSubFlag)
            {
                gRBUSSubListSt.stRadioSignal.RadioEnvCondSubFlag--;
            }
            else
            {
                gRBUSSubListSt.stRadioSignal.RadioEnvCondSubInterval = 0;
                gRBUSSubListSt.stRadioSignal.RadioEnvCondLapsedCounterTimer = 0;
            }

            CcspTraceInfo(("%s-%d : X_RDK_RadioEnvConditions UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RadioEnvCondSubFlag));
        }
    }
    else if(strstr(eventName, ".RSSI"))
    {
        int iProposedInterval = 0;

        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            if( 0 == gRBUSSubListSt.stRadioSignal.RSSISubFlag )
            {
                gRBUSSubListSt.stRadioSignal.RSSILapsedCounterTimer = 0;
                gRBUSSubListSt.stRadioSignal.RSSISubInterval = 0;
            }

            if( interval > 0 )
            {
                if( 0 == ( interval % RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL ) )
                {
                    iProposedInterval = interval;
                }
                else
                {
                    CcspTraceInfo(("%s-%d : Unable to subcribe RSSI due to invalid interval(%d)\n", __FUNCTION__, __LINE__, interval));
                    return RBUS_ERROR_BUS_ERROR;
                }

                if( 0 == gRBUSSubListSt.stRadioSignal.RSSISubInterval )
                {
                    gRBUSSubListSt.stRadioSignal.RSSISubInterval = iProposedInterval;
                    gRBUSSubListSt.stRadioSignal.RSSILapsedCounterTimer = 0;
                }
                else
                {
                    if( iProposedInterval < gRBUSSubListSt.stRadioSignal.RSSISubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.RSSISubInterval = iProposedInterval;
                        gRBUSSubListSt.stRadioSignal.RSSILapsedCounterTimer = 0;
                    }
                }
            }

            gRBUSSubListSt.stRadioSignal.RSSISubFlag++;
            CcspTraceInfo(("%s-%d : RSSI Sub(%d) Interval(%d)\n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RSSISubFlag,gRBUSSubListSt.stRadioSignal.RSSISubInterval));
        }
        else
        {
            if (gRBUSSubListSt.stRadioSignal.RSSISubFlag)
            {
                gRBUSSubListSt.stRadioSignal.RSSISubFlag--;
            }
            else
            {
                gRBUSSubListSt.stRadioSignal.RSSISubInterval = 0;
                gRBUSSubListSt.stRadioSignal.RSSILapsedCounterTimer = 0;
            }
            CcspTraceInfo(("%s-%d : RSSI UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RSSISubFlag));
        }
    }
    else if(strstr(eventName, ".X_RDK_SNR"))
    {
        int iProposedInterval = 0;

        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            if( 0 == gRBUSSubListSt.stRadioSignal.SNRSubFlag )
            {
                gRBUSSubListSt.stRadioSignal.SNRLapsedCounterTimer = 0;
                gRBUSSubListSt.stRadioSignal.SNRSubInterval = 0;
            }

            if( interval > 0 )
            {
                if( 0 == ( interval % RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL ) )
                {
                    iProposedInterval = interval;
                }
                else
                {
                    CcspTraceInfo(("%s-%d : Unable to subcribe X_RDK_SNR due to invalid interval(%d)\n", __FUNCTION__, __LINE__, interval));
                    return RBUS_ERROR_BUS_ERROR;
                }

                if( 0 == gRBUSSubListSt.stRadioSignal.SNRSubInterval )
                {
                    gRBUSSubListSt.stRadioSignal.SNRSubInterval = iProposedInterval;
                    gRBUSSubListSt.stRadioSignal.SNRLapsedCounterTimer = 0;
                }
                else
                {
                    if( iProposedInterval < gRBUSSubListSt.stRadioSignal.SNRSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.SNRSubInterval = iProposedInterval;
                        gRBUSSubListSt.stRadioSignal.SNRLapsedCounterTimer = 0;
                    }
                }
            }

            gRBUSSubListSt.stRadioSignal.SNRSubFlag++;
            CcspTraceInfo(("%s-%d : X_RDK_SNR Sub(%d) Interval(%d)\n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.SNRSubFlag,gRBUSSubListSt.stRadioSignal.SNRSubInterval));
        }
        else
        {
            if (gRBUSSubListSt.stRadioSignal.SNRSubFlag)
            {
                gRBUSSubListSt.stRadioSignal.SNRSubFlag--;
            }
            else
            {
                gRBUSSubListSt.stRadioSignal.SNRSubInterval = 0;
                gRBUSSubListSt.stRadioSignal.SNRLapsedCounterTimer = 0;
            }
            CcspTraceInfo(("%s-%d : X_RDK_SNR UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.SNRSubFlag));
        }
    }
    else if(strstr(eventName, ".RSRP"))
    {
        int iProposedInterval = 0;

        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            if( 0 == gRBUSSubListSt.stRadioSignal.RSRPSubFlag )
            {
                gRBUSSubListSt.stRadioSignal.RSRPLapsedCounterTimer = 0;
                gRBUSSubListSt.stRadioSignal.RSRPSubInterval = 0;
            }

            if( interval > 0 )
            {
                if( 0 == ( interval % RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL ) )
                {
                    iProposedInterval = interval;
                }
                else
                {
                    CcspTraceInfo(("%s-%d : Unable to subcribe RSRP due to invalid interval(%d)\n", __FUNCTION__, __LINE__, interval));
                    return RBUS_ERROR_BUS_ERROR;
                }

                if( 0 == gRBUSSubListSt.stRadioSignal.RSRPSubInterval )
                {
                    gRBUSSubListSt.stRadioSignal.RSRPSubInterval = iProposedInterval;
                    gRBUSSubListSt.stRadioSignal.RSRPLapsedCounterTimer = 0;
                }
                else
                {
                    if( iProposedInterval < gRBUSSubListSt.stRadioSignal.RSRPSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.RSRPSubInterval = iProposedInterval;
                        gRBUSSubListSt.stRadioSignal.RSRPLapsedCounterTimer = 0;
                    }
                }
            }

            gRBUSSubListSt.stRadioSignal.RSRPSubFlag++;
            CcspTraceInfo(("%s-%d : RSRP Sub(%d) Interval(%d)\n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RSRPSubFlag,gRBUSSubListSt.stRadioSignal.RSRPSubInterval));
        }
        else
        {
            if (gRBUSSubListSt.stRadioSignal.RSRPSubFlag)
            {
                gRBUSSubListSt.stRadioSignal.RSRPSubFlag--;
            }
            else
            {
                gRBUSSubListSt.stRadioSignal.RSRPSubInterval = 0;
                gRBUSSubListSt.stRadioSignal.RSRPLapsedCounterTimer = 0;
            }
            CcspTraceInfo(("%s-%d : RSRP UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RSRPSubFlag));
        }
    }
    else if(strstr(eventName, ".RSRQ"))
    {
        int iProposedInterval = 0;

        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            if( 0 == gRBUSSubListSt.stRadioSignal.RSRQSubFlag )
            {
                gRBUSSubListSt.stRadioSignal.RSRQLapsedCounterTimer = 0;
                gRBUSSubListSt.stRadioSignal.RSRQSubInterval = 0;
            }

            if( interval > 0 )
            {
                if( 0 == ( interval % RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL ) )
                {
                    iProposedInterval = interval;
                }
                else
                {
                    CcspTraceInfo(("%s-%d : Unable to subcribe RSRQ due to invalid interval(%d)\n", __FUNCTION__, __LINE__, interval));
                    return RBUS_ERROR_BUS_ERROR;
                }

                if( 0 == gRBUSSubListSt.stRadioSignal.RSRQSubInterval )
                {
                    gRBUSSubListSt.stRadioSignal.RSRQSubInterval = iProposedInterval;
                    gRBUSSubListSt.stRadioSignal.RSRQLapsedCounterTimer = 0;
                }
                else
                {
                    if( iProposedInterval < gRBUSSubListSt.stRadioSignal.RSRQSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.RSRQSubInterval = iProposedInterval;
                        gRBUSSubListSt.stRadioSignal.RSRQLapsedCounterTimer = 0;
                    }
                }
            }

            gRBUSSubListSt.stRadioSignal.RSRQSubFlag++;
            CcspTraceInfo(("%s-%d : RSRQ Sub(%d) Interval(%d)\n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RSRQSubFlag,gRBUSSubListSt.stRadioSignal.RSRQSubInterval));
        }
        else
        {
            if (gRBUSSubListSt.stRadioSignal.RSRQSubFlag)
            {
                gRBUSSubListSt.stRadioSignal.RSRQSubFlag--;
            }
            else
            {
                gRBUSSubListSt.stRadioSignal.RSRQSubInterval = 0;
                gRBUSSubListSt.stRadioSignal.RSRQLapsedCounterTimer = 0;
            }
            CcspTraceInfo(("%s-%d : RSRQ UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.RSRQSubFlag));
        }
    }
    else if(strstr(eventName, ".X_RDK_TRX"))
    {
        int iProposedInterval = 0;

        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            if( 0 == gRBUSSubListSt.stRadioSignal.TRXSubFlag )
            {
                gRBUSSubListSt.stRadioSignal.TRXLapsedCounterTimer = 0;
                gRBUSSubListSt.stRadioSignal.TRXSubInterval = 0;
            }

            if( interval > 0 )
            {
                if( 0 == ( interval % RBUS_SUBSCRIPTION_PUBLISH_POLLING_INTERVAL ) )
                {
                    iProposedInterval = interval;
                }
                else
                {
                    CcspTraceInfo(("%s-%d : Unable to subcribe X_RDK_TRX due to invalid interval(%d)\n", __FUNCTION__, __LINE__, interval));
                    return RBUS_ERROR_BUS_ERROR;
                }

                if( 0 == gRBUSSubListSt.stRadioSignal.TRXSubInterval )
                {
                    gRBUSSubListSt.stRadioSignal.TRXSubInterval = iProposedInterval;
                    gRBUSSubListSt.stRadioSignal.TRXLapsedCounterTimer = 0;
                }
                else
                {
                    if( iProposedInterval < gRBUSSubListSt.stRadioSignal.TRXSubInterval )
                    {
                        gRBUSSubListSt.stRadioSignal.TRXSubInterval = iProposedInterval;
                        gRBUSSubListSt.stRadioSignal.TRXLapsedCounterTimer = 0;
                    }
                }
            }

            gRBUSSubListSt.stRadioSignal.TRXSubFlag++;
            CcspTraceInfo(("%s-%d : X_RDK_TRX Sub(%d) Interval(%d)\n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.TRXSubFlag,gRBUSSubListSt.stRadioSignal.TRXSubInterval));
        }
        else
        {
            if (gRBUSSubListSt.stRadioSignal.TRXSubFlag)
            {
                gRBUSSubListSt.stRadioSignal.TRXSubFlag--;
            }
            else
            {
                gRBUSSubListSt.stRadioSignal.TRXSubInterval = 0;
                gRBUSSubListSt.stRadioSignal.TRXLapsedCounterTimer = 0;
            }
            CcspTraceInfo(("%s-%d : X_RDK_TRX UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stRadioSignal.TRXSubFlag));
        }
    }
    else if(strstr(eventName, ".Status"))
    {
        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            gRBUSSubListSt.stInterface.InterfaceStatusSubFlag++;
            CcspTraceInfo(("%s-%d : InterfaceStatus Sub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stInterface.InterfaceStatusSubFlag));
        }
        else
        {
            if (gRBUSSubListSt.stInterface.InterfaceStatusSubFlag)
                gRBUSSubListSt.stInterface.InterfaceStatusSubFlag--;
            CcspTraceInfo(("%s-%d : InterfaceStatus UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stInterface.InterfaceStatusSubFlag));
        }
    }
    else if(strstr(eventName, ".X_RDK_PhyConnectedStatus"))
    {
        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            gRBUSSubListSt.stInterface.PhyConnectedStatusSubFlag++;
            CcspTraceInfo(("%s-%d : PhyConnectedStatus Sub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stInterface.PhyConnectedStatusSubFlag));
        }
        else
        {
            if (gRBUSSubListSt.stInterface.PhyConnectedStatusSubFlag)
                gRBUSSubListSt.stInterface.PhyConnectedStatusSubFlag--;
            CcspTraceInfo(("%s-%d : PhyConnectedStatus UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stInterface.PhyConnectedStatusSubFlag));
        }
    }
    else if(strstr(eventName, ".X_RDK_LinkAvailableStatus"))
    {
        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            gRBUSSubListSt.stInterface.LinkAvailableStatusSubFlag++;
            CcspTraceInfo(("%s-%d : LinkAvailableStatus Sub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stInterface.LinkAvailableStatusSubFlag));
        }
        else
        {
            if (gRBUSSubListSt.stInterface.LinkAvailableStatusSubFlag)
                gRBUSSubListSt.stInterface.LinkAvailableStatusSubFlag--;
            CcspTraceInfo(("%s-%d : LinkAvailableStatus UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.stInterface.LinkAvailableStatusSubFlag));
        }
    }
    else if(strstr(eventName, ".X_RDK_Enable"))
    {
        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            gRBUSSubListSt.X_RDK_EnableSubFlag++;
            CcspTraceInfo(("%s-%d : X_RDK_Enable Sub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.X_RDK_EnableSubFlag));
        }
        else
        {
            if (gRBUSSubListSt.X_RDK_EnableSubFlag)
                gRBUSSubListSt.X_RDK_EnableSubFlag--;
            CcspTraceInfo(("%s-%d : X_RDK_Enable UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.X_RDK_EnableSubFlag));
        }
    }
    else if(strstr(eventName, ".X_RDK_Status"))
    {
        if (action == RBUS_EVENT_ACTION_SUBSCRIBE)
        {
            gRBUSSubListSt.X_RDK_StatusSubFlag++;
            CcspTraceInfo(("%s-%d : X_RDK_Status Sub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.X_RDK_StatusSubFlag));
        }
        else
        {
            if (gRBUSSubListSt.X_RDK_StatusSubFlag)
                gRBUSSubListSt.X_RDK_StatusSubFlag--;
            CcspTraceInfo(("%s-%d : X_RDK_Status UnSub(%d) \n", __FUNCTION__, __LINE__, gRBUSSubListSt.X_RDK_StatusSubFlag));
        }
    }

    return RBUS_ERROR_SUCCESS;
}


#endif /*RBUS_BUILD_FLAG_ENABLE*/
