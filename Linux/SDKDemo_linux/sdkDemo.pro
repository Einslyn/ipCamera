#-------------------------------------------------
#
# Project created by QtCreator 2017-10-20T17:25:19
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = sdkDemo
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

#include(./PLATFORM_SUFFIX.pri)

unix: {
    DEFINES += __ENVIRONMENT_LINUX__
    DEFINES += ARM_LINUX
    LIBS += -ldl
}

CONFIG(debug, debug|release) {
    DESTDIR = ../bin
    OBJECTS_DIR = ./Debug
    UI_DIR = ./Debug
    RCC_DIR = ./Debug
    MOC_DIR = ./Debug
    LIBS += -L../bin
    LIBS += -L../SDKDemo_linux/project/lib
    LIBS += -L/usr/local/lib/
    LIBS += -L../../opensource/iniparser-master
    LIBS += -L../../opensource/zlog-master/src
} else {
    DESTDIR = ../bin
    OBJECTS_DIR = ./Release
    UI_DIR = ./Release
    RCC_DIR = ./Release
    MOC_DIR = ./Release
    LIBS += -L../bin
    LIBS += -L../SDKDemo_linux/project/lib
    LIBS += -L/usr/local/lib/
    LIBS += -L../../opensource/iniparser-master
    LIBS += -L../../opensource/zlog-master/src
}


#LIBS += -ldvrnetsdk
LIBS += -ldvrnetsdk -lasound -lmodbus -lmodbus2 -lrt -lpthread -liniparser -lzlog

INCLUDEPATH += project/include
INCLUDEPATH += ../../include
INCLUDEPATH += /usr/local/include/modbus
INCLUDEPATH += ../../opensource/iniparser-master/src
INCLUDEPATH += ../../opensource/zlog-master/src
SOURCES += \
#    project/source/TreeNode.cpp \
    project/source/SDKDemo.cpp \
#    project/source/RegionDeviceLog.cpp \
#    project/source/DeviceManager.cpp \
#    project/source/DDPublic.cpp \
    project/source/modbusThread.cpp \
    project/source/tvtcamera.cpp

HEADERS += \
    project/include/Typedef.h \
#    project/include/TreeNode.h \
    project/include/tinyxml.h \
#    project/include/RegionDeviceLog.h \
    project/include/RecordLog.h \
    project/include/lvfw.h \
    project/include/dvrprotocol.h \
#    project/include/DeviceManager.h \
    project/include/decoderdefine.h \
    ../../include/SDKTypeDef.h \
    ../../include/PTZ.h \
    ../../include/DVR_NET_SDK.h \
    ../../include/dvrdvstypedef.h \
    ../../include/dvrdvsdefine.h \
    ../../include/dvrdvsconfig.h \
    ../../include/DDPublic.h \
    ../../include/DDConfig.h \
    ../../include/DDCommon.h \
    ../../include/ConfigBlock.h \
    project/include/unit-test.h \
    project/include/camera.h \
    project/include/common.h
