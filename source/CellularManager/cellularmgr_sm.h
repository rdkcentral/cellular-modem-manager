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

#ifndef _CELLULARMGR_SM_H_
#define _CELLULARMGR_SM_H_

#include "cellularmgr_cellular_apis.h"
#include "cellular_hal.h"

/**********************************************************************
                MACRO DECLARATION
**********************************************************************/

/**********************************************************************
                ENUM, STRUCTURE AND CONSTANT DEFINITIONS
**********************************************************************/

typedef enum 
{
   CELLULAR_STATE_DOWN                   = RDK_STATUS_DOWN,        //Device is not open, WWAN status is Down
   CELLULAR_STATE_DEACTIVATED,                                     //Device is open, Able to communicate with Modem
   CELLULAR_STATE_DEREGISTERED,                                    //Device is open, Selecting Valid Slot, NAS detached
   CELLULAR_STATE_REGISTERED,                                      //Device is open, NAS attached, Selecting Profile, Attaching PDN context, Start Network
   CELLULAR_STATE_CONNECTED,                                       //Device is open, Network Started and WWAN status up
   CELLULAR_STATUS_ERROR,                                          //State Machine error state

} CellularPolicySmState_t;

typedef  struct                                     
{
    unsigned char                  bModemEnable;                   //Needs to pass modem state
    CellularProfileStruct          stContextProfile;               //This profile information used to start network
    PCELLULAR_INTERFACE_INFO       pCmIfData;                      //This is actual interface data
    PCELLULAR_DML_INFO             pstDmlCellular;                 //This is actual cellular DML info struct data
} 
CellularMgrSMInputStruct;

/**********************************************************************
    GLOBAL or LOCAL DEFINITIONS and STRUCTURE or ENUM DECLARATION
**********************************************************************/

/**********************************************************************
                FUNCTION PROTOTYPES
**********************************************************************/

int
CellularMgr_Start_State_Machine
    (
        CellularMgrSMInputStruct    *pstInput
    );

CellularPolicySmState_t CellularMgrSMGetCurrentState( void );

void CellularMgrSMSetCellularEnable( unsigned char bRDKEnable );

unsigned char CellularMgrSMGetCellularEnable( void );

int CellularMgrGetCurrentPDPContextStatusInformation( PCELLULAR_INTERFACE_CONTEXTPROFILE_INFO  pstContextProfileInfo );

int cellular_get_serving_info(int *registration_status, int *roaming_status,  int *attach_status);

int CellularMgrGetNetworkRegisteredService( CELLULAR_INTERFACE_REGISTERED_SERVICE_TYPE  *penRegisteredService );

#endif //_CELLULARMGR_SM_H_
