# Shared library without any Qt functionality
TEMPLATE = lib
QT -= gui core

CONFIG += warn_on plugin release
CONFIG -= thread exceptions qt rtti debug

VERSION = 1.0.0

INCLUDEPATH += ../src/SDK/CHeaders/XPLM
INCLUDEPATH += ../src/SDK/CHeaders/Wrappers
INCLUDEPATH += ../src/SDK/CHeaders/Widgets
INCLUDEPATH += ../src
INCLUDEPATH += ../src/Fmod_api/core/inc
INCLUDEPATH += ../src/Fmod_api/studio/inc


INCLUDEPATH += ..
VPATH = ..

# Defined to use X-Plane SDK 3.0.0 capabilities - no backward compatibility before 11.10
DEFINES +=  XPLM400 \
            XPLM303 \
            XPLM302 \
            XPLM301 \
            XPLM300 \
            XPLM210 \
            XPLM200

unix:!macx {
    DEFINES += APL=0 IBM=0 LIN=1 LINUX=1
    TARGET = lin.xpl
    # WARNING! This requires the latest version of the X-SDK !!!!
    QMAKE_CXXFLAGS += -fvisibility=hidden -O2 -Wall -Wextra -Wshadow -Wfloat-equal -Wformat -Wformat-security \
         --param ssp-buffer-size=4 -fstack-protector -D_FORTIFY_SOURCE=2 -std=gnu++11 -fpermissive
    QMAKE_CFLAGS += -fvisibility=hidden -O2 -Wall -Wextra -Wshadow -Wfloat-equal -Wformat -Wformat-security \
         --param ssp-buffer-size=4 -fstack-protector -D_FORTIFY_SOURCE=2
    LIBS += -ldl -Wl,--version-script -Wl,../src/exports.txt
    LIBS += -static-libgcc -static-libstdc++ -fPIC
}


HEADERS +=  ../src/SDK/CHeaders/XPLM/XPLMCamera.h \
            ../src/SDK/CHeaders/XPLM/XPLMDataAccess.h \
            ../src/SDK/CHeaders/XPLM/XPLMDefs.h \
            ../src/SDK/CHeaders/XPLM/XPLMDisplay.h \
            ../src/SDK/CHeaders/XPLM/XPLMGraphics.h \
            ../src/SDK/CHeaders/XPLM/XPLMInstance.h \
            ../src/SDK/CHeaders/XPLM/XPLMMap.h \
            ../src/SDK/CHeaders/XPLM/XPLMMenus.h \
            ../src/SDK/CHeaders/XPLM/XPLMNavigation.h \
            ../src/SDK/CHeaders/XPLM/XPLMPlanes.h \
            ../src/SDK/CHeaders/XPLM/XPLMPlugin.h \
            ../src/SDK/CHeaders/XPLM/XPLMProcessing.h \
            ../src/SDK/CHeaders/XPLM/XPLMScenery.h \
            ../src/SDK/CHeaders/XPLM/XPLMSound.h \
            ../src/SDK/CHeaders/XPLM/XPLMUtilities.h \
            ../src/SDK/CHeaders/XPLM/XPLMWeather.h \
            ../src/Fmod_api/core/inc/fmod.h \
            ../src/Fmod_api/core/inc/fmod_codec.h \
            ../src/Fmod_api/core/inc/fmod_common.h \
            ../src/Fmod_api/core/inc/fmod_dsp.h \
            ../src/Fmod_api/core/inc/fmod_dsp_effects.h \
            ../src/Fmod_api/core/inc/fmod_errors.h \
            ../src/Fmod_api/core/inc/fmod_output.h \
            ../src/Fmod_api/studio/inc/fmod_studio.h \
            ../src/Fmod_api/studio/inc/fmod_studio_common.h
            
            

SOURCES +=  ../src/xp12_fmod_sdk_demo.cpp
