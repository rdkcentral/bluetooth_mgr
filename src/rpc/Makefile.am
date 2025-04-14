##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
SUBDIRS =
if USE_RPC_IARM
AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/include -I${top_srcdir}/include/logger -I${top_srcdir}/include/rpc ${RPC_FLAGS} -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/rdk/iarmbus -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/WPEFramework/powercontroller -I${top_srcdir}/include/safec $(SAFEC_CFLAGS)
AM_LDFLAGS = -ldbus-1  -lIARMBus -lrdkloggers $(SAFEC_LFLAGS)

libBTMgr_la_SOURCES = btmgr_iarm_external_interface.c
include_HEADERS = ${top_srcdir}/include/rpc/btmgr_iarm_interface.h
endif

if USE_RPC_RBUS
AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/include -I${top_srcdir}/include/logger -I${top_srcdir}/include/rpc -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/WPEFramework/powercontroller -I${top_srcdir}/include/safec $(SAFEC_CFLAGS)
if LE_ONLY
AM_CPPFLAGS += -I$(top_srcdir)/soc/include
endif
AM_LDFLAGS = -ldbus-1  -lrbus -lrdkloggers -lWPEFrameworkPowerController $(SAFEC_LFLAGS)

libBTMgr_la_SOURCES = btmgr_rbus_external_interface.c
include_HEADERS = ${top_srcdir}/include/rpc/btmgr_rbus_interface.h
endif

lib_LTLIBRARIES = libBTMgr.la
libBTMgr_la_includedir = $(includedir) -lWPEFrameworkPowerController
libBTMgr_la_include_HEADERS = ${top_srcdir}/include/btmgr.h
