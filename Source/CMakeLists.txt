cmake_minimum_required(VERSION 3.19)

set(CMAKE_INSTALL_RPATH "$ORIGIN")

project(IrregularTuningMaker LANGUAGES C CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Widgets
    Sql
    Concurrent
    Gui)

qt_standard_project_setup()

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")

qt_add_executable(IrregularTuningMaker
    WIN32 MACOSX_BUNDLE

    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    interval.h interval.cpp
    utilities.h
    scaleSpace.h scalespace.cpp
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

qt_add_resources(IrregularTuningMaker "app_resources"
    PREFIX "/"
    FILES
        images/icon.png
)

target_link_libraries(IrregularTuningMaker
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Sql
        Qt6::Concurrent
        Qt6::Gui
)

target_include_directories(IrregularTuningMaker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GNUInstallDirs)

install(TARGETS IrregularTuningMaker
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(WIN32 OR APPLE)
    qt_generate_deploy_app_script(
        TARGET IrregularTuningMaker
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

set(CPACK_PACKAGE_NAME "IrregularTuningMaker")
set(CPACK_PACKAGE_VERSION "1.0.3")
set(CPACK_PACKAGE_CONTACT "jamespatbyrne60@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Irregular Tuning Maker application")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${CPACK_PACKAGE_NAME}")

elseif(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "James Byrne <jamespatbyrne60@gmail.com>")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
