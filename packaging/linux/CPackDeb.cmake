# CPack configuration for the Debian/Ubuntu/Raspberry Pi OS package.
#
#   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   cd build && cpack -G DEB
#
# Produces artilody-workout-creator_<version>_<arch>.deb (DEB-DEFAULT naming).
# Included from CMakeLists.txt only on Linux.

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "artilody-workout-creator")
set(CPACK_PACKAGE_VENDOR "Artilody")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Create Artilody (.awt) and Zwift (.zwo) cycling and rowing workouts")
set(CPACK_PACKAGE_DESCRIPTION
"Artilody Workout Creator is a small desktop editor for structured cycling
and rowing workouts with the same Workout Builder as Artilody Watts Trainer:
warm-up, steady, interval, recovery, ramp and cool-down blocks in watts or
% FTP. Workouts are kept as files in a library folder and can be exported as
Artilody .awt or Zwift .zwo files. Built with Qt 6 / QML.")
set(CPACK_PACKAGE_CONTACT "Artilody <noreply@artilody.app>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/shenphenchoedron-hue/Artilody-Workout-Creator")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_STRIP_FILES ON)
if(EXISTS "${PROJECT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
endif()

set(CPACK_DEBIAN_PACKAGE_NAME "artilody-workout-creator")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Artilody <noreply@artilody.app>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
# Library dependencies (libQt6*, libstdc++, ...) are computed with dpkg-shlibdeps.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Runtime pieces dpkg-shlibdeps cannot see: QML modules and platform plugins.
# Package names verified for Debian 12 (bookworm) and Ubuntu 24.04 (noble).
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-dialogs, qml6-module-qtquick-window, qml6-module-qtquick-templates, qml6-module-qtqml-workerscript, qml6-module-qtqml-models, qt6-qpa-plugins")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-wayland")

include(CPack)
