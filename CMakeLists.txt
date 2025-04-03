cmake_minimum_required(VERSION 3.30)
project(serialPortTools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(.)

set(CMAKE_PREFIX_PATH "C:/Soft/Qt/6.4.3/msvc2019_64/lib/cmake")
set(ELA_LIB_PATH "C:\\Soft\\ElaWidgetTools\\ElaWidgetTools")
set(C_SERIAL_PORT_LIB_PATH "C:\\Soft\\CSerialPort")

include_directories(${ELA_LIB_PATH}/include)
link_directories(${ELA_LIB_PATH}/lib)

include_directories(${C_SERIAL_PORT_LIB_PATH}/include)
link_directories(${C_SERIAL_PORT_LIB_PATH}/lib)

include_directories("C:\\Project\\Cpp\\maddy\\include\\maddy")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

add_executable(serialPortTools main.cpp
        serialwindow.cpp
        serialwindow.h
        serialwindow.ui
        tabwindow.cpp
        tabwindow.h
        tabwindow.ui
        inputdialog.cpp
        inputdialog.h
        inputdialog.ui
        settingswindow.cpp
        settingswindow.h
        settingswindow.ui
)

target_link_libraries(serialPortTools
        Qt::Core
        Qt::Gui
        Qt::Widgets

        ElaWidgetToolsd.lib

        libcserialport.lib
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
