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
if !LE_ONLY
SUBDIRS = streamInOut
endif
INCLUDE_FILES =

if USE_RPC_RBUS
AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/src/main -I${top_srcdir}/include -I${top_srcdir}/include/safec ${RPC_FLAGS} $(SAFEC_CFLAGS) -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/wdmp-c -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}

if LE_ONLY
AM_CPPFLAGS += -I$(top_srcdir)/soc/include
endif

if WITH_PI_BUILD
AM_CPPFLAGS += $(PI_BUILD_FLAG)
endif

btrMgrTest_SOURCES = btrMgr_testRbus.c

AM_LDFLAGS = -ldbus-1  -lrbus $(SAFEC_LFLAGS)
btrMgrTest_LDADD     = $(top_builddir)/src/rpc/libBTMgr.la
if !LE_ONLY
AM_LDFLAGS += -lglib-2.0
endif

endif

if USE_RPC_IARM
AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/src/main -I${top_srcdir}/include -I${top_srcdir}/include/safec -I${top_srcdir}/include/common -I${top_srcdir}/include/streamOut ${GLIB_CFLAGS}  ${RPC_FLAGS} $(SAFEC_CFLAGS) -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/rdk/iarmbus -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/wdmp-c -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}

if WITH_PI_BUILD
AM_CPPFLAGS += $(PI_BUILD_FLAG)
endif

btrMgrTest_SOURCES = btrMgr_test.c

AM_LDFLAGS = -ldbus-1  -lIARMBus -lrfcapi $(SAFEC_LFLAGS)
btrMgrTest_LDADD     = $(top_builddir)/src/rpc/libBTMgr.la
if !LE_ONLY
AM_LDFLAGS += -lglib-2.0
endif

endif

if !USE_RPC_IARM
if !USE_RPC_RBUS
AM_CPPFLAGS =  -I${top_srcdir}/include -I${top_srcdir}/include/safec -I${top_srcdir}/include/common ${RPC_FLAGS} $(SAFEC_CFLAGS) -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/wdmp-c -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}

if LE_ONLY
AM_CPPFLAGS += -I$(top_srcdir)/soc/include
endif

if !LE_ONLY
AM_CPPFLAGS += ${GLIB_CFLAGS}
endif

btrMgrTest_SOURCES = btrMgr_test.c

if WITH_PI_BUILD
AM_CPPFLAGS += $(PI_BUILD_FLAG)
endif
AM_LDFLAGS = $(SAFEC_LFLAGS)
btrMgrTest_LDADD     = $(top_builddir)/src/ifce/libBTMgr.la
if !LE_ONLY
AM_LDFLAGS += -lglib-2.0
endif
endif

endif

bin_PROGRAMS = btrMgrTest

bin_PROGRAMS += btrMgrHidEvtMonitor
btrMgrHidEvtMonitor_SOURCES = btrMgr_inputMonitor.c
btrMgrHidEvtMonitor_LDFLAGS = -lglib-2.0 -lgio-2.0 -lgobject-2.0
btrMgrHidEvtMonitor_CFLAGS = $(GLIB_CFLAGS) $(SAFEC_CFLAGS)

bin_PROGRAMS += btrMgrAutoTestSuite
btrMgrAutoTestSuite_SOURCES = btrMgr_autoTests.c
btrMgrAutoTestSuite_LDFLAGS = -ldbus-1 -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lm
if WITH_PI_BUILD
btrMgrAutoTestSuite_LDADD = $(top_builddir)/src/ifce/libBTMgr.la
endif
if !WITH_PI_BUILD
btrMgrAutoTestSuite_LDADD     = $(top_builddir)/src/rpc/libBTMgr.la
endif
btrMgrAutoTestSuite_CFLAGS = $(GLIB_CFLAGS) $(SAFEC_CFLAGS) -I${top_srcdir}/include/common -ggdb3 -g

#bin_PROGRAMS += btrMgrHTMLControl
#btrMgrHTMLControl_SOURCES = btrMgr_testFcgi.c
#btrMgrHTMLControl_LDFLAGS = -lfcgi

