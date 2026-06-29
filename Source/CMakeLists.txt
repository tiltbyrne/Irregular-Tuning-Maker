cmake_minimum_required(VERSION 3.19)
project(TuningMakerGUI LANGUAGES C CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Widgets
    Sql
    Concurrent
    Gui)

qt_standard_project_setup()

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
    customheaderview.h customheaderview.cpp

    fractionParser.h
    toFraction.h
    scalespacemodel.h scalespacemodel.cpp
)

target_link_libraries(TuningMakerGUI
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Sql
        Qt6::Concurrent
        Qt6::Gui
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
