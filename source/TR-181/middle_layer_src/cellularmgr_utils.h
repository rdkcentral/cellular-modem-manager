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
#ifndef _CELLULAR_MANAGER_DEV_INFO_H
#define _CELLULAR_MANAGER_DEV_INFO_H

#include "ansc_debug_wrapper_base.h"
#include "cellularmgr_plugin_main_apis.h"
#include "cellular_hal.h"
#include "ipc_msg.h"

#define CCSP_COMMON_FIFO "/tmp/ccsp_common_fifo"

int CellularMgr_Util_SendIPToWanMgr( CellularIPStruct *pstIPStruct );

#endif
