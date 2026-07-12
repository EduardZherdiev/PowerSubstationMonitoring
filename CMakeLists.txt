cmake_minimum_required(VERSION 3.19)
project(PowerSubstationMonitoring LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg PrintSupport)

qt_standard_project_setup()

set(QT_VERSION Qt6 CACHE STRING "The Qt version framework to use (Qt5 or Qt6).")
set(BREEZE_EXTENSIONS all CACHE STRING "The extensions to include in our stylesheets.")
set(BREEZE_STYLES all CACHE STRING "The styles to include in our stylesheets.")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Python COMPONENTS Interpreter REQUIRED)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF CACHE BOOL "Silence fetch content" FORCE)

FetchContent_Declare(
    breeze_stylesheets
    GIT_REPOSITORY https://github.com/Alexhuszagh/BreezeStyleSheets.git
    GIT_TAG origin/main
    GIT_PROGRESS ON
    GIT_SHALLOW 1
    USES_TERMINAL_DOWNLOAD TRUE)

FetchContent_GetProperties(breeze_stylesheets)
if(NOT TARGET breeze)
    if(NOT breeze_stylesheets_POPULATED)
    FetchContent_Populate(breeze_stylesheets)
    endif()

    add_library(breeze STATIC "${breeze_stylesheets_SOURCE_DIR}/dist/breeze.qrc")

    add_custom_target(
        run_python_breeze ALL
        COMMAND ${Python_EXECUTABLE} configure.py --extensions=${BREEZE_EXTENSIONS}
                --styles=${BREEZE_STYLES} --resource breeze.qrc
        WORKING_DIRECTORY ${breeze_stylesheets_SOURCE_DIR}
        BYPRODUCTS "${breeze_stylesheets_SOURCE_DIR}/dist/breeze.qrc"
        COMMENT "Generating themes")

    add_dependencies(breeze run_python_breeze)
    set_property(TARGET breeze PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

configure_file(data/substation_layout.json ${CMAKE_CURRENT_BINARY_DIR}/substation_layout.json COPYONLY)

qt_add_executable(PowerSubstationMonitoring
    WIN32 MACOSX_BUNDLE
    main.cpp
    appsettings.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    equipment.h equipment.cpp
    equipmenttreemodel.h equipmenttreemodel.cpp
    substationlayout.h substationlayout.cpp
    substationdiagramview.h substationdiagramview.cpp
    diagramnodeitem.h diagramnodeitem.cpp
    diagramlinkitem.h diagramlinkitem.cpp
    diagramtheme.h diagramtheme.cpp
    powerflowcalculator.h powerflowcalculator.cpp
    sensortelemetry.h sensortelemetry.cpp
    telemetryservice.h telemetryservice.cpp
    telemetrylayoutmapper.h telemetrylayoutmapper.cpp
    historypersistence.h historypersistence.cpp
    telemetryhistory.h telemetryhistory.cpp
    monitoringchart.h monitoringchart.cpp
    aboutdialog.h aboutdialog.cpp aboutdialog.ui
    eventlogger.h eventlogger.cpp
    reportdialog.h reportdialog.cpp reportdialog.ui
    settingsdialog.h settingsdialog.cpp settingsdialog.ui
    resources.qrc
)

target_include_directories(PowerSubstationMonitoring PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PowerSubstationMonitoring
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Svg
        Qt::PrintSupport
        breeze
)

include(GNUInstallDirs)

install(TARGETS PowerSubstationMonitoring
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET PowerSubstationMonitoring
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
