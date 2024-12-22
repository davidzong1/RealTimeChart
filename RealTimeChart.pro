QT       += core gui widgets printsupport opengl
DEFINES  += QCUSTOMPLOT_USE_OPENGL

CONFIG   += c++17

INCLUDEPATH += /usr/include/qt5
INCLUDEPATH += /usr/include/eigen3
INCLUDEPATH += socket_share_mem
LIBS += -lQt5Core
LIBS += -lQt5Gui
LIBS += -lQt5Widgets
LIBS += -lQt5PrintSupport
LIBS += -lQt5OpenGL
LIBS += -L/usr/local/lib 
LIBS += -lboost_system -lboost_thread
LIBS += -lpthread
LIBS += -lrt 
LIBS += -ltbb -ltbbmalloc -ltbbmalloc_proxy
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

QMAKE_RPATHDIR += /usr/lib/x86_64-linux-gnu/qt5/plugins