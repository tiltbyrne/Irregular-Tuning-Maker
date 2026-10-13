cmake_minimum_required(VERSION 3.19)

project(IrregularTuningMaker VERSION 1.0.3 LANGUAGES C CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Concurrent
)

qt_standard_project_setup()

set(APP_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    interval.h
    interval.cpp
    utilities.h
    scaleSpace.h
    scalespace.cpp
    weights.h
    weights.cpp
    scale.h
    scale.cpp

    settings.h
    dbUtils.h

    dbManager.h
    dbManager.cpp
    dbcurrnet.h
    dbcurrnet.cpp
    globals.h

    fractionParser.h
    toFraction.h
    scalespacemodel.h
    scalespacemodel.cpp
    scalespacedelegate.h
    scalespacedelegate.cpp

    customheaderview.h
    customheaderview.cpp
    customspinbox.h
    customspinbox.cpp
)

# A Windows .rc resource must not be passed to the macOS/Linux toolchain.
if(WIN32)
    list(APPEND APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
endif()

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "icon.icns")

    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/images/icon.icns")
    set_source_files_properties("${APP_ICON_MACOS}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    list(APPEND APP_SOURCES "${APP_ICON_MACOS}")
endif()

qt_add_executable(IrregularTuningMaker
    WIN32
    MACOSX_BUNDLE
    ${APP_SOURCES}
)

qt_add_resources(IrregularTuningMaker "app_resources"
    PREFIX "/"
    FILES
        images/icon.png
)

target_link_libraries(IrregularTuningMaker
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Concurrent
)

target_include_directories(IrregularTuningMaker
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

include(GNUInstallDirs)

if(APPLE)
    set_target_properties(IrregularTuningMaker PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "uk.org.unionz.jamesbyrne"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

install(TARGETS IrregularTuningMaker
    BUNDLE DESTINATION .
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# These are Linux desktop-integration files, not macOS bundle contents.
if(UNIX AND NOT APPLE)
    install(FILES IrregularTuningMaker.desktop
        DESTINATION share/applications
    )
    install(FILES images/icon-small.png
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME icon.png
    )
endif()

# Qt's deployment script copies the Qt frameworks/plugins needed by the app.
if(WIN32 OR APPLE)
    qt_generate_deploy_app_script(
        TARGET IrregularTuningMaker
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT "${deploy_script}")
endif()

set(CPACK_PACKAGE_NAME "IrregularTuningMaker")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "jamespatbyrne60@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Irregular Tuning Maker application")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${CPACK_PACKAGE_NAME}")
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")   # or "NSIS" if you want an installer
endif()

include(CPack)
