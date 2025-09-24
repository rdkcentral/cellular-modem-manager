##########################################################################
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
##########################################################################

AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_LDFLAGS = -lccsp_common

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

noinst_LTLIBRARIES = libcellularmanager_middle_layer_src.la
libcellularmanager_middle_layer_src_la_CPPFLAGS = -I$(top_srcdir)/source/TR-181/include -I$(top_srcdir)/source/TR-181/middle_layer_src -I$(top_srcdir)/source/CellularManager

libcellularmanager_middle_layer_src_la_SOURCES =  cellularmgr_plugin_main.c  cellularmgr_plugin_main_apis.c cellularmgr_cellular_dml.c cellularmgr_utils.c cellularmgr_cellular_param.c cellularmgr_cellular_helpers.c cellularmgr_cellular_webconfig_api.c
libcellularmanager_middle_layer_src_la_LDFLAGS = -lccsp_common -lrdkloggers $(DBUS_LIBS) $(SYSTEMD_LDFLAGS) -lnanomsg -lsysevent -lmsgpackc -lcurl -lwebconfig_framework -ltrower-base64

libcellularmanager_middle_layer_src_la_CPPFLAGS += -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rtmessage
libcellularmanager_middle_layer_src_la_SOURCES += cellularmgr_rbus_events.c cellularmgr_rbus_helpers.c cellularmgr_rbus_dml.c
if HYBRID_SUPPORT
libcellularmanager_middle_layer_src_la_CFLAGS = -DHYBRID_SUPPORT
endif
libcellularmanager_middle_layer_src_la_LDFLAGS += -lrbus -lrtMessage -lmsgpackc
