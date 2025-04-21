cmake_minimum_required(VERSION 3.30)
project(serialPortTools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#! 请把这里修改成你的 Qt 路径
set(CMAKE_PREFIX_PATH "C:/Soft/Qt/6.4.3/msvc2019_64/lib/cmake")

# 子模块 ElaWidgetTools CSerialPort
add_subdirectory(3rd/ElaWidgetTools)
add_subdirectory(3rd/CSerialPort)

include_directories(3rd/CSerialPort/include)

include_directories(${CMAKE_SOURCE_DIR}/src/inc)
include_directories(${CMAKE_SOURCE_DIR}/src/serialWindow)
include_directories(${CMAKE_SOURCE_DIR}/src/serialServer)
include_directories(${CMAKE_SOURCE_DIR}/src/settingsWindow)
include_directories(.)
include_directories(src)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        LinguistTools
        Network
        REQUIRED)

add_executable(serialPortTools WIN32
        src/main.cpp
        src/serialWindow/serialWindow.cpp
        src/serialWindow/serialWindow.h
        src/serialWindow/serialWindow.ui
        src/mainWindow.cpp
        src/mainWindow.h
        src/mainWindow.ui
        src/inputDialog.cpp
        src/inputDialog.h
        src/inputDialog.ui
        src/settingsWindow/settingsWindow.cpp
        src/settingsWindow/settingsWindow.h
        src/settingsWindow/settingsWindow.ui
        src/serialWindow/outTextEdit.cpp
        src/serialWindow/outTextEdit.h
        src/inc/threadPool.h
        src/inc/moodycamel/concurrentqueue.h

        src/serialWindow/cmdWidget.cpp
        src/serialWindow/cmdWidget.h
        src/serialWindow/cmdWidget.ui
        src/serialWindow/cmdLineEdit.cpp
        src/serialWindow/cmdLineEdit.h
        src/serialWindow/cmdLineEdit.ui

        resources.qrc
        src/serialServer/serialServer.cpp
        src/serialServer/serialServer.h
        src/serialServer/serialServer.ui
        src/serialServer/tcpServer.h
        src/settings.cpp
        src/settings.h
)

# 添加资源文件
qt6_add_resources(serialPortTools "res/translations"
        PREFIX "/"
        FILES
        resources.qrc
)

target_link_libraries(serialPortTools
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt6::Network

        libcserialport
        ElaWidgetTools
)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
