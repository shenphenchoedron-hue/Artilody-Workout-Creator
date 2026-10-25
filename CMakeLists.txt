# Artilody Workout Creator – standalone workout editor (Qt 6 / QML).
#
#   cmake -S "Artilody Workout Creator" -B build-creator -DCMAKE_PREFIX_PATH=<Qt>/6.x/<platform>
#   cmake --build build-creator && ctest --test-dir build-creator
#
# It compiles the workout model, the Builder back-end and the .awt/.zwo code
# from Artilody Watts Trainer's src/core and reuses its Builder page and UI
# components (qml/), so both apps edit workouts the same way and write
# identical files. See ARTILODY_SHARED_DIR below for where they come from.
cmake_minimum_required(VERSION 3.21)

project(ArtilodyWorkoutCreator
    VERSION 1.1.0
    DESCRIPTION "Create Artilody Watts Trainer / Zwift workouts on any computer"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AWC_BUILD_TESTS "Build the unit tests" ON)

# Qt 6.4 is the minimum (same as Artilody Watts Trainer).
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup()
if(QT_KNOWN_POLICY_QTP0001)   # Qt >= 6.5: modules live under qrc:/qt/qml/
    qt_policy(SET QTP0001 NEW)
endif()

# Shared sources (src/core + qml) from Artilody Watts Trainer. Inside the
# Artilody Watts Trainer repository they are used straight from the parent
# folder; the standalone repository (Artilody-Workout-Creator) carries a copy
# in shared/ that is refreshed with tools/sync_shared.sh.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/core/WorkoutIO.h")
    get_filename_component(_awc_default_shared "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
else()
    set(_awc_default_shared "${CMAKE_CURRENT_SOURCE_DIR}/shared")
endif()
set(ARTILODY_SHARED_DIR "${_awc_default_shared}" CACHE PATH
    "Directory containing the shared src/core and qml folders of Artilody Watts Trainer")
if(NOT EXISTS "${ARTILODY_SHARED_DIR}/src/core/WorkoutIO.h")
    message(FATAL_ERROR "Shared Artilody sources not found in ARTILODY_SHARED_DIR=${ARTILODY_SHARED_DIR}")
endif()
message(STATUS "Artilody shared sources: ${ARTILODY_SHARED_DIR}")
set(WATT_ROOT ${ARTILODY_SHARED_DIR})
set(WATT_CORE ${WATT_ROOT}/src/core)
set(WATT_QML ${WATT_ROOT}/qml)
set(AWC_QML ${CMAKE_CURRENT_SOURCE_DIR}/qml)
set(AWC_ICONS ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons)

# --- back-end (no QML / UI code) -------------------------------------------------
# Shared with Artilody Watts Trainer: the database-free part of watt_core.
qt_add_library(awc_core STATIC
    ${WATT_CORE}/Interval.h ${WATT_CORE}/Interval.cpp
    ${WATT_CORE}/Workout.h ${WATT_CORE}/Workout.cpp
    ${WATT_CORE}/Zones.h ${WATT_CORE}/Zones.cpp
    ${WATT_CORE}/TimeFormat.h ${WATT_CORE}/TimeFormat.cpp
    ${WATT_CORE}/WorkoutIO.h ${WATT_CORE}/WorkoutIO.cpp
    ${WATT_CORE}/BuilderBackend.h
    ${WATT_CORE}/WorkoutBuilder.h ${WATT_CORE}/WorkoutBuilder.cpp
    # creator specific
    src/CreatorSettings.h src/CreatorSettings.cpp
    src/WorkoutFileLibrary.h src/WorkoutFileLibrary.cpp
    src/CreatorBackend.h src/CreatorBackend.cpp
    src/DemoWorkouts.h src/DemoWorkouts.cpp
)
target_include_directories(awc_core PUBLIC ${WATT_CORE} ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(awc_core PUBLIC Qt6::Core Qt6::Gui)

# --- QML ---------------------------------------------------------------------------
# Shared with Artilody Watts Trainer (unchanged files from ../qml).
set(SHARED_QML_FILES
    ${WATT_QML}/Theme.qml
    ${WATT_QML}/components/Icon.qml
    ${WATT_QML}/components/Card.qml
    ${WATT_QML}/components/PageHeader.qml
    ${WATT_QML}/components/AppButton.qml
    ${WATT_QML}/components/IconButton.qml
    ${WATT_QML}/components/AppTextField.qml
    ${WATT_QML}/components/AppComboBox.qml
    ${WATT_QML}/components/AppMenu.qml
    ${WATT_QML}/components/AppMenuItem.qml
    ${WATT_QML}/components/AppMenuSeparator.qml
    ${WATT_QML}/components/StepperField.qml
    ${WATT_QML}/components/UnitSpinBox.qml
    ${WATT_QML}/components/ToggleSwitch.qml
    ${WATT_QML}/components/CategoryBadge.qml
    ${WATT_QML}/components/SportBadge.qml
    ${WATT_QML}/components/StatTile.qml
    ${WATT_QML}/components/ZoneLegend.qml
    ${WATT_QML}/components/WorkoutChart.qml
    ${WATT_QML}/components/SegmentedControl.qml
    ${WATT_QML}/components/BlockPreview.qml
    ${WATT_QML}/components/AppDialog.qml
    ${WATT_QML}/components/Toast.qml
    ${WATT_QML}/dialogs/ConfirmDialog.qml
    ${WATT_QML}/dialogs/RampDialog.qml
    ${WATT_QML}/dialogs/IntervalSetDialog.qml
    ${WATT_QML}/pages/BuilderPage.qml
)
# Creator specific.
set(CREATOR_QML_FILES
    ${AWC_QML}/Main.qml
    ${AWC_QML}/LibraryPanel.qml
    ${AWC_QML}/LibraryItem.qml
    ${AWC_QML}/CreatorTopBar.qml
    ${AWC_QML}/UsageNote.qml
    ${AWC_QML}/ExportDialog.qml
    ${AWC_QML}/CreatorSettingsDialog.qml
    ${AWC_QML}/RenameDialog.qml
    ${AWC_QML}/HelpDialog.qml
)
# Every QML file sits flat in the module root (as in the main app) so the
# shared files find each other without imports.
foreach(qml_file ${SHARED_QML_FILES} ${CREATOR_QML_FILES})
    get_filename_component(qml_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${qml_name})
endforeach()
set_source_files_properties(${WATT_QML}/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(ArtilodyWorkoutCreator src/main.cpp)

# --- application icon ------------------------------------------------------------
if(APPLE)
    set(AWC_ICON_MACOS ${AWC_ICONS}/AppIcon.icns)
    target_sources(ArtilodyWorkoutCreator PRIVATE ${AWC_ICON_MACOS})
    set_source_files_properties(${AWC_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()
if(WIN32)
    target_sources(ArtilodyWorkoutCreator PRIVATE packaging/windows/app-icon.rc)
endif()
qt_add_resources(ArtilodyWorkoutCreator "appicon"
    PREFIX "/icons"
    BASE ${AWC_ICONS}
    FILES ${AWC_ICONS}/app-icon-256.png
)

qt_add_qml_module(ArtilodyWorkoutCreator
    URI ArtilodyCreator
    VERSION 1.0
    QML_FILES ${SHARED_QML_FILES} ${CREATOR_QML_FILES}
)

target_compile_definitions(ArtilodyWorkoutCreator PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(ArtilodyWorkoutCreator PRIVATE
    awc_core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

set_target_properties(ArtilodyWorkoutCreator PROPERTIES
    OUTPUT_NAME "Artilody Workout Creator"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER app.artilody.workoutcreator
    MACOSX_BUNDLE_BUNDLE_NAME "Artilody Workout Creator"
    MACOSX_BUNDLE_ICON_FILE AppIcon.icns
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in
    WIN32_EXECUTABLE TRUE
)
if(UNIX AND NOT APPLE)
    set_target_properties(ArtilodyWorkoutCreator PROPERTIES OUTPUT_NAME "artilody-workout-creator")
endif()

include(GNUInstallDirs)
install(TARGETS ArtilodyWorkoutCreator
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/artilody-workout-creator.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    foreach(icon_size 48 64 128 256 512)
        install(FILES packaging/linux/icons/${icon_size}x${icon_size}/artilody-workout-creator.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${icon_size}x${icon_size}/apps)
    endforeach()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(packaging/linux/CPackDeb.cmake)
endif()

# --- tests -------------------------------------------------------------------------
if(AWC_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(tst_creator tests/tst_creator.cpp)
    target_link_libraries(tst_creator PRIVATE awc_core Qt6::Test)
    add_test(NAME tst_creator COMMAND tst_creator)
    set_tests_properties(tst_creator PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
