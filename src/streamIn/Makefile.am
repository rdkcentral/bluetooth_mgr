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

AM_CPPFLAGS = -Wall -Werror -I${top_srcdir}/include/common -I${top_srcdir}/include/streamIn -I${top_srcdir}/include/logger -I${top_srcdir}/include/safec
AM_LDFLAGS =

if USE_GSTREAMER1
AM_CPPFLAGS += $(GST_CFLAGS) -DUSE_GST1
AM_LDFLAGS += $(GST_LIBS) -lgstapp-1.0
endif

lib_LTLIBRARIES = libbtrMgrStreamIn.la
libbtrMgrStreamIn_la_SOURCES = btrMgr_streamIn.c
libbtrMgrStreamIn_la_CFLAGS = $(INCLUDE_FILES) $(GLIB_CFLAGS) $(SAFEC_CFLAGS)
libbtrMgrStreamIn_la_LIBADD = $(GLIB_LIBS) $(RDKLOGGER_LIBS) $(SAFEC_LFLAGS)

if USE_GSTREAMER1
libbtrMgrStreamIn_la_SOURCES += btrMgr_streamInGst.c
endif

if WITH_PI_BUILD
AM_CPPFLAGS += $(PI_BUILD_FLAG)
endif

if WITH_BRCM_BUILD
AM_CPPFLAGS += $(BRCM_BUILD_FLAG)
endif

include_HEADERS = $(top_srcdir)/include/streamIn/btrMgr_streamIn.h

