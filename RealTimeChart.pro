QT       += core gui
QT +=printsupport

# use Opengl
DEFINES += QCUSTOMPLOT_USE_OPENGL
# 添加 OpenGL 模块
QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += /usr/include/eigen3
INCLUDEPATH += /path/to/your/boost/include
INCLUDEPATH += socket_share_mem
LIBS += -L/path/to/your/boost/lib
LIBS += -lboost_system -lboost_thread
LIBS += -lpthread
LIBS += -lrt


SOURCES += \
    CsvLoaderWindow.cc \
    main.cpp \
    mainwindow.cpp \
    qcustomplot.cpp \
    socket_share_mem/IOopperator.cc \
    socket_share_mem/dz_sm_socket.cc \
    socket_share_mem/dz_sm_socket_top.cc \
    socket_share_mem/dzsem.cc

HEADERS += \
    dziostream.h \
    mainwindow.h \
    qcustomplot.h \
    socket_share_mem/IOopperator.h \
    socket_share_mem/dz_sm_socket.h \
    socket_share_mem/dz_sm_socket_top.h \
    socket_share_mem/dz_sm_socket_top_queue.hpp \
    socket_share_mem/dzsem.hpp

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    RealTimeChart_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
