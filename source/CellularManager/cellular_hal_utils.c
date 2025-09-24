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

#include "cellular_hal_utils.h"
#include <sys/stat.h>

/**********************************************************************
                STRUCTURE AND CONSTANT DEFINITIONS
**********************************************************************/

/**********************************************************************
                FUNCTION DEFINITION
**********************************************************************/

/* cellular_hal_util_IsDeviceFileExists() */
unsigned char cellular_hal_util_IsDeviceFileExists (const char *devname)
{
    char devfile[128]= {0};
    struct stat s;
    int result;

    snprintf(devfile, sizeof(devfile), "/dev/%s", devname);
    result = stat (devfile, &s);

    return (0 == result) ? TRUE : FALSE;
}

/* cellular_hal_util_GetCurrentTimeInSeconds() */
unsigned long cellular_hal_util_GetCurrentTimeInSeconds( void )
{
    struct timeval tv = {0};

    gettimeofday(&tv, NULL);

    return tv.tv_sec;
}

/* cellular_hal_util_GetUptime() */
int cellular_hal_util_GetUptime(unsigned long *time)
{
   FILE * fp = fopen("/proc/uptime", "r");
   if ( !fp )
   {
       return -1;
   }

   fscanf(fp,"%lu",time);
   fclose(fp);
   fp = NULL;
   
   return 0;
}

/* cellular_hal_util_PrintTime() */
void cellular_hal_util_PrintTime(char *time_value, int length) 
{
    time_t timer;
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(time_value, length, "%Y-%m-%d %H:%M:%S ", tm_info);
}