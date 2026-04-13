cmake_minimum_required(VERSION 3.16)
project(ClusterApp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SOURCES
    src/main.cpp
    src/ClusterWindow.cpp
    src/GaugeWidget.cpp
    src/CenterPanel.cpp
    src/PillBadge.cpp
    src/CanWorker.cpp
    src/DisplayController.cpp
    src/resources.qrc
)

set(HEADERS
    src/ClusterWindow.h
    src/GaugeWidget.h
    src/CenterPanel.h
    src/PillBadge.h
    src/CanWorker.h
    src/DisplayController.h
)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    SerialPort
    SerialBus
)

add_executable(ClusterApp ${SOURCES} ${HEADERS})

target_link_libraries(ClusterApp
    Qt5::Core
    Qt5::Widgets
    Qt5::SerialPort
    Qt5::SerialBus
)

# ── Desktop: mock CAN so it compiles without hardware ─────────────────────
if(NOT CMAKE_CROSSCOMPILING)
    target_compile_definitions(ClusterApp PRIVATE DESKTOP_MOCK)
endif()
