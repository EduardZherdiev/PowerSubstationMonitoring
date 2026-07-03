cmake_minimum_required(VERSION 3.19)
project(PowerSubstationMonitoring LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

configure_file(data/substation_layout.json ${CMAKE_CURRENT_BINARY_DIR}/substation_layout.json COPYONLY)

qt_add_executable(PowerSubstationMonitoring
    WIN32 MACOSX_BUNDLE
    main.cpp
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
    aboutdialog.h aboutdialog.cpp aboutdialog.ui
    eventlogger.h eventlogger.cpp
    reportdialog.h reportdialog.cpp reportdialog.ui
    settingsdialog.h settingsdialog.cpp settingsdialog.ui
)

target_include_directories(PowerSubstationMonitoring PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PowerSubstationMonitoring
    PRIVATE
        Qt::Core
        Qt::Widgets
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
