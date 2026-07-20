cmake_minimum_required(VERSION 3.19)
project(TuningMakerGUI LANGUAGES C CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Widgets
    Sql
    Concurrent
    Gui)

qt_standard_project_setup()

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")

qt_add_executable(TuningMakerGUI
    WIN32 MACOSX_BUNDLE

    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    interval.h interval.cpp
    utilities.h
    scalespace.h scalespace.cpp
    weights.h weights.cpp
    scale.h scale.cpp

    settings.h
    dbUtils.h

    dbManager.h dbManager.cpp
    dbcurrnet.h dbcurrnet.cpp
    globals.h


    fractionParser.h
    toFraction.h
    scalespacemodel.h scalespacemodel.cpp
    scalespacedelegate.h scalespacedelegate.cpp

    customheaderview.h customheaderview.cpp
    customspinbox.h customspinbox.cpp
    ${APP_ICON_RESOURCE_WINDOWS}
)

qt_add_resources(TuningMakerGUI "app_resources"
    PREFIX "/"
    FILES
        images/icon.png
)

target_link_libraries(TuningMakerGUI
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Sql
        Qt6::Concurrent
        Qt6::Gui
)

target_include_directories(TuningMakerGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GNUInstallDirs)

install(TARGETS TuningMakerGUI
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET TuningMakerGUI
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
