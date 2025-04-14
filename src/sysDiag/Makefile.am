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

INCLUDE_FILES =

AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/include/common -I${top_srcdir}/include/sysDiag -I${top_srcdir}/include/logger -I${top_srcdir}/include/commonutilities -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/WPEFramework/powercontroller -I${top_srcdir}/include/safec $(SAFEC_CFLAGS)
AM_LDFLAGS = $(RDKLOGGER_LIBS) $(SAFEC_LFLAGS)

if USE_SYS_DIAG
AM_CPPFLAGS += -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/rdk/iarmbus -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/rdk/iarmmgrs-hal
AM_LDFLAGS += -lIARMBus
AM_LDFLAGS += -ldwnlutil -lfwutils -lsecure_wrapper -lparsejson
AM_CPPFLAGS += -DBTR_SYS_DIAG_IARM_ENABLE=1
endif

if USE_RPC_RBUS
AM_CPPFLAGS += -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus
AM_LDFLAGS += -lrbus -lsyscfg
AM_CPPFLAGS += -DBTR_SYS_DIAG_RBUS_ENABLE=1
endif

lib_LTLIBRARIES = libbtrMgrSysDiag.la
libbtrMgrSysDiag_la_SOURCES = btrMgr_SysDiag.c btrMgr_DeviceUtils.c
libbtrMgrSysDiag_la_CFLAGS = $(INCLUDE_FILES) -lWPEFrameworkPowerController

include_HEADERS = $(top_srcdir)/include/sysDiag/btrMgr_SysDiag.h $(top_srcdir)/include/sysDiag/btrMgr_DeviceUtils.h
