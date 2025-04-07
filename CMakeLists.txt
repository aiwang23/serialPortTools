cmake_minimum_required(VERSION 3.30)
project(serialPortTools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#! 请把这里修改成你的 Qt 路径
set(CMAKE_PREFIX_PATH "C:/Soft/Qt/6.4.3/msvc2019_64/lib/cmake")
#! 请把这里修改成你的 ElaWidgetTools 路径
set(ELA_LIB_PATH "C:\\Soft\\ElaWidgetTools\\ElaWidgetTools")
#! 请把这里修改成你的 CSerialPort 路径
set(C_SERIAL_PORT_LIB_PATH "C:\\Soft\\CSerialPort")

include_directories(${ELA_LIB_PATH}/include)
link_directories(${ELA_LIB_PATH}/lib)

include_directories(${C_SERIAL_PORT_LIB_PATH}/include)
link_directories(${C_SERIAL_PORT_LIB_PATH}/lib)

include_directories(${CMAKE_SOURCE_DIR}/src/inc)
include_directories(.)
include_directories(src)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        LinguistTools
        REQUIRED)

add_executable(serialPortTools WIN32
        src/main.cpp
        src/serialwindow.cpp
        src/serialwindow.h
        src/serialwindow.ui
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/inputdialog.cpp
        src/inputdialog.h
        src/inputdialog.ui
        src/settingswindow.cpp
        src/settingswindow.h
        src/settingswindow.ui
        src/outTextEdit.cpp
        src/outTextEdit.h
        src/threadPool.h
        src/concurrentqueue.h

        src/cmdwidget.cpp
        src/cmdwidget.h
        src/cmdwidget.ui
        src/cmdlineedit.cpp
        src/cmdlineedit.h
        src/cmdlineedit.ui

        resources.qrc
)

# 添加资源文件
qt6_add_resources(serialPortTools "res/translations"
        PREFIX "/"
        FILES
        resources.qrc
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "当前是Debug构建")
    # 添加Debug特有的设置
    target_link_libraries(serialPortTools
            Qt::Core
            Qt::Gui
            Qt::Widgets
            ElaWidgetToolsd.lib
            libcserialportd.lib
    )
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "当前是Release构建")
    # 添加Release特有的设置
    target_link_libraries(serialPortTools
            Qt::Core
            Qt::Gui
            Qt::Widgets
            ElaWidgetTools.lib
            libcserialport.lib
    )
endif ()

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
