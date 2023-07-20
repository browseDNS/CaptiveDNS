#-------------------------------------------------
#
# Project created by QtCreator 2018-04-06T03:09:54
#
#-------------------------------------------------

QT       += core gui network

CONFIG +=  c++14 openssl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CaptiveDNS
TEMPLATE = app

VERSION = 2.1.3
QMAKE_TARGET_COMPANY = BrowseDNS
QMAKE_TARGET_PRODUCT = CaptiveDNS
QMAKE_TARGET_DESCRIPTION = captive portal redirector
QMAKE_TARGET_COPYRIGHT = 2023
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
#comment this next line out if you need to debug it if somethings not quite right (you'll get the qDebug() output then [and on macOS it has to be a debug build too for some reason])
DEFINES += QT_NO_DEBUG_OUTPUT

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        dnsserverwindow.cpp \
    smalldnsserver.cpp \
    settingswindow.cpp \
    initialresponse.cpp \
    indexhtml.cpp \
    smallhttpserver.cpp \
    messagesthread.cpp \
    cacheviewer.cpp

HEADERS += \
        dnsserverwindow.h \
    smalldnsserver.h \
    settingswindow.h \
    initialresponse.h \
    indexhtml.h \
    smallhttpserver.h \
    androidsuop.h \
    messagesthread.h \
    cacheviewer.h \
    dnsinfo.h \
    buffer.h

FORMS += \
        dnsserverwindow.ui \
    settingswindow.ui \
    indexhtml.ui \
    cacheviewer.ui

CONFIG += mobility

MOBILITY = 

DISTFILES += \
    GPLv2.txt \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

RESOURCES += \
    images.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

