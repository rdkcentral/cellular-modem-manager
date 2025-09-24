#############################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Sky
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################

AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_LDFLAGS = -lccsp_common
AM_LDFLAGS +=

AM_LDFLAGS += -lulog
AM_LDFLAGS += -lsecure_wrapper
AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu
bin_PROGRAMS = cellularmanager
cellularmanager_CPPFLAGS = -I$(top_srcdir)/source/TR-181/include                   \
        -I$(top_srcdir)/../hal/include                                                \
        -I$(top_srcdir)/../CcspCommonLibrary/source/cosa/include/                  \
        -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include/       \
        -I$(top_srcdir)/source/TR-181/middle_layer_src $(CPPFLAGS) \
        -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus \
        -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rtmessage

cellularmanager_DEPENDENCIES= \
    $(EXTRA_DEPENDENCIES) \
    ${top_builddir}/source/TR-181/middle_layer_src/libcellularmanager_middle_layer_src.la

cellularmanager_CFLAGS = $(SYSTEMD_CFLAGS) $(NONROOT_CFLAGS)
cellularmanager_SOURCES = cellularmgr_sm.c cellularmgr_cellular_apis.c cellularmgr_cellular_internal.c cellularmgr_ssp_action.c cellularmgr_ssp_internal.h cellularmgr_main.c cellularmgr_messagebus_interface.c cellular_hal_utils.c cellular_hal.c cellularmgr_bus_utils.c dm_pack_datamodel.c
if HYBRID_SUPPORT
cellularmanager_SOURCES += cellular_hal_device_abstraction.c cellular_hal_device_manager.c cellular_hal_rndis_apis.c cellular_hal_modem_apis.c
cellularmanager_CFLAGS += -DHYBRID_SUPPORT
else
cellularmanager_SOURCES += cellular_hal_qmi_apis.c cellular_hal_mm_apis.c
cellularmanager_CFLAGS += -DMM_SUPPORT
endif
cellularmanager_LDFLAGS = -lccsp_common -ldl $(SYSTEMD_LDFLAGS) -lrdkloggers -lsyscfg -lnanomsg -lsysevent -lwebconfig_framework -lsecure_wrapper -lmsgpackc -lcurl -ltrower-base64
cellularmanager_LDADD =  $(cellularmanager_DEPENDENCIES)
