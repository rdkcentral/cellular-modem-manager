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

#ifndef _CELLULAR_HAL_UTILS_H_
#define _CELLULAR_HAL_UTILS_H_

#include "cellular_hal.h"
#include <sys/time.h>

/**********************************************************************
                STRUCTURE AND CONSTANT DEFINITIONS
**********************************************************************/

#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

#define CELLULAR_HAL_LOG_FILE    "/rdklogs/logs/CellularManagerLog.txt.0"

#define CELLULAR_HAL_DBG_PRINT(fmt ...)     {\
                                                FILE     *fp        = NULL;\
                                                fp = fopen ( CELLULAR_HAL_LOG_FILE, "a+");\
                                                if (fp)\
                                                {\
                                                    char buf[128] = {0};\
                                                    cellular_hal_util_PrintTime(buf,sizeof(buf));\
                                                    fprintf(fp,buf);\
                                                    fprintf(fp,fmt);\
                                                    fclose(fp);\
                                                }\
                                            }\

/**********************************************************************
                FUNCTION PROTOTYPES
**********************************************************************/

unsigned char cellular_hal_util_IsDeviceFileExists (const char *devname);
unsigned long cellular_hal_util_GetCurrentTimeInSeconds( void );
int cellular_hal_util_GetUptime(unsigned long *time);

#endif //_CELLULAR_HAL_UTILS_H_
