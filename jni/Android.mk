# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := frodoc64

LOCAL_LDLIBS	+= -L${SYSROOT}/usr/lib -llog

#non thumb:
LOCAL_ARM_MODE := arm

LOCAL_CFLAGS	+= -DANDROID -DOS_ANDROID 
LOCAL_SRC_FILES := main.cpp Prefs.cpp C64.cpp CPU1541.cpp CPUC64.cpp VIC.cpp CIA.cpp CPU_common.cpp SID.cpp SAM.cpp REU.cpp IEC.cpp Display.cpp 1541d64.cpp 1541fs.cpp 1541job.cpp CmdPipe.cpp 1541t64.cpp

#LOCAL_CFLAGS	+= -Istlport/stlport -D__NEW__ -D__SGI_STL_INTERNAL_PAIR_H -DANDROID -DOS_ANDROID -DPRECISE_CPU_CYCLES=1 -DPRECISE_CIA_CYCLES=1 -DPC_IS_POINTER=0
#LOCAL_SRC_FILES := main.cpp Prefs.cpp C64_PC.cpp CPU1541_PC.cpp CPUC64_PC.cpp VIC.cpp CIA.cpp CPU_common.cpp SID.cpp SAM.cpp REU.cpp IEC.cpp Display.cpp 1541d64.cpp 1541fs.cpp 1541job.cpp CmdPipe.cpp 1541t64.cpp

#LOCAL_CFLAGS	+= -Istlport/stlport -D__NEW__ -D__SGI_STL_INTERNAL_PAIR_H -DANDROID -DOS_ANDROID -DFRODO_SC
#LOCAL_SRC_FILES := main.cpp Prefs.cpp C64_SC.cpp CPU1541_SC.cpp CPUC64_SC.cpp VIC_SC.cpp CIA_SC.cpp CPU_common.cpp SID.cpp SAM.cpp REU.cpp IEC.cpp Display.cpp 1541d64.cpp 1541fs.cpp 1541job.cpp CmdPipe.cpp 1541t64.cpp

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := frodoc65

LOCAL_LDLIBS	+= -L${SYSROOT}/usr/lib -llog

#non thumb:
LOCAL_ARM_MODE := arm

#LOCAL_CFLAGS	+= -Istlport/stlport -D__NEW__ -D__SGI_STL_INTERNAL_PAIR_H -DANDROID -DOS_ANDROID 
#LOCAL_SRC_FILES := main.cpp Prefs.cpp C64.cpp CPU1541.cpp CPUC64.cpp VIC.cpp CIA.cpp CPU_common.cpp SID.cpp SAM.cpp REU.cpp IEC.cpp Display.cpp 1541d64.cpp 1541fs.cpp 1541job.cpp CmdPipe.cpp 1541t64.cpp

LOCAL_CFLAGS	+= -DANDROID -DOS_ANDROID -DPRECISE_CPU_CYCLES=1 -DPRECISE_CIA_CYCLES=1 -DPC_IS_POINTER=0
LOCAL_SRC_FILES := main.cpp Prefs.cpp C64_PC.cpp CPU1541_PC.cpp CPUC64_PC.cpp VIC.cpp CIA.cpp CPU_common.cpp SID.cpp SAM.cpp REU.cpp IEC.cpp Display.cpp 1541d64.cpp 1541fs.cpp 1541job.cpp CmdPipe.cpp 1541t64.cpp

#LOCAL_CFLAGS	+= -Istlport/stlport -D__NEW__ -D__SGI_STL_INTERNAL_PAIR_H -DANDROID -DOS_ANDROID -DFRODO_SC
#LOCAL_SRC_FILES := main.cpp Prefs.cpp C64_SC.cpp CPU1541_SC.cpp CPUC64_SC.cpp VIC_SC.cpp CIA_SC.cpp CPU_common.cpp SID.cpp SAM.cpp REU.cpp IEC.cpp Display.cpp 1541d64.cpp 1541fs.cpp 1541job.cpp CmdPipe.cpp 1541t64.cpp

include $(BUILD_SHARED_LIBRARY)
