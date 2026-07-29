cmake_minimum_required(VERSION 3.16)
project(wm_apps C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- System dependencies via pkg-config ----
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_EGL  REQUIRED wayland-egl)
pkg_check_modules(EGL          REQUIRED egl)
pkg_check_modules(IVI_APP      REQUIRED ivi-application)
pkg_check_modules(CAIRO        REQUIRED cairo)
pkg_check_modules(LIBJPEG      REQUIRED libjpeg)

find_library(GLESV2_LIB NAMES GLESv2 GLESv2_SCALAR REQUIRED)

# ---- Paths ----

# WM_SDKROOT: pre-built SDK directory (contains include/ and lib/)
# Set via env WM_SDKROOT or defaults to $REPO_ROOT/sdk/cvios
set(WM_SDKROOT "$ENV{WM_SDKROOT}")
if(NOT WM_SDKROOT)
    set(WM_SDKROOT "$ENV{REPO_ROOT}/sdk/cvios")
endif()
message(STATUS "WM_SDKROOT = ${WM_SDKROOT}")

# Set env vars for subdirectory CMake projects (cvi-service-radio reads these)
set(ENV{CVIOS_INCLUDES} "${WM_SDKROOT}/include")
set(ENV{CVIOS_LIBRARY_DIRECTORY} "${WM_SDKROOT}/lib")

# ---- Common include paths ----
# Includes needed by all app targets that use WmWindow / StandardProcessFramework
set(APP_CLIENT_INCLUDES
    ${WM_SDKROOT}/include                 # wm/, windows/, cvi/, spdlog/, etc.
    ${WM_SDKROOT}/include/cvi             # cvi-lib-servicefw/StandardProcessFramework.h
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
    ${WAYLAND_EGL_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${IVI_APP_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${LIBJPEG_INCLUDE_DIRS})

set(APP_COMMON_DEFS _GNU_SOURCE)

# ---- Link directories ----
link_directories(
    ${WM_SDKROOT}/lib                     # libwm_*, libcvi-lib-log, libspdlog, etc.
    ${WAYLAND_CLIENT_LIBRARY_DIRS}
    ${WAYLAND_EGL_LIBRARY_DIRS}
    ${IVI_APP_LIBRARY_DIRS})

# ---- Warning helper (same as core) ----
function(wm_warn_flags tgt)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${tgt} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wno-unused-parameter>)
    endif()
endfunction()

# =================================================================
#  App executables
# =================================================================

# ---- wm-bg: background chrome client ----
add_executable(wm-bg src/clients/wm_bg.cpp)
target_include_directories(wm-bg PRIVATE ${APP_CLIENT_INCLUDES})
target_compile_definitions(wm-bg PRIVATE ${APP_COMMON_DEFS})
target_link_libraries(wm-bg PRIVATE
    wm_window ${CAIRO_LIBRARIES} ${LIBJPEG_LIBRARIES} cvi-lib-log spdlog)
wm_warn_flags(wm-bg)

# ---- wm-statusbar: status bar chrome client ----
add_executable(wm-statusbar src/clients/wm_statusbar.cpp)
target_include_directories(wm-statusbar PRIVATE ${APP_CLIENT_INCLUDES})
target_compile_definitions(wm-statusbar PRIVATE ${APP_COMMON_DEFS})
target_link_libraries(wm-statusbar PRIVATE wm_window ${CAIRO_LIBRARIES} cvi-lib-log spdlog)
wm_warn_flags(wm-statusbar)

# ---- wm-testapp: minimal app-window test client ----
add_executable(wm-testapp src/clients/wm_testapp.cpp)
target_include_directories(wm-testapp PRIVATE ${APP_CLIENT_INCLUDES})
target_compile_definitions(wm-testapp PRIVATE ${APP_COMMON_DEFS})
target_link_libraries(wm-testapp PRIVATE wm_window ${CAIRO_LIBRARIES} cvi-lib-log spdlog)
wm_warn_flags(wm-testapp)

# ---- wm-window-test: StandardProcessFramework-based test for libwm_window ----
add_executable(wm-window-test src/clients/wm_window_test.cpp)
target_include_directories(wm-window-test PRIVATE ${APP_CLIENT_INCLUDES})
target_compile_definitions(wm-window-test PRIVATE ${APP_COMMON_DEFS})
target_link_libraries(wm-window-test PRIVATE wm_window ${CAIRO_LIBRARIES} cvi-lib-log spdlog)
wm_warn_flags(wm-window-test)

# ---- wm-navibar: left sidebar navigation bar ----
add_executable(wm-navibar src/clients/wm_navibar.cpp)
target_include_directories(wm-navibar PRIVATE ${APP_CLIENT_INCLUDES})
target_link_libraries(wm-navibar PRIVATE wm_window ${CAIRO_LIBRARIES} wm_common)
wm_warn_flags(wm-navibar)

# ---- wm-ctl: command-line IPC tool ----
add_executable(wm-ctl src/clients/wm_ctl.cpp)
target_include_directories(wm-ctl PRIVATE ${WM_SDKROOT}/include)
target_link_libraries(wm-ctl PRIVATE wm_common)
wm_warn_flags(wm-ctl)

# ---- wm-trans: standalone socket client (no WM libs, no SDK) ----
add_executable(wm-trans src/clients/wm_trans.cpp)
wm_warn_flags(wm-trans)

# ---- cvi-service-radio: standalone service process (separate CMake project) ----
add_subdirectory(src/cvi-service-radio)

# =================================================================
#  Install — merge into same output/windowsmanager/ tree as CORE
# =================================================================

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

install(TARGETS wm-bg wm-statusbar wm-testapp wm-window-test wm-navibar wm-ctl wm-trans
    RUNTIME DESTINATION bin)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts/
    DESTINATION scripts
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING PATTERN "*.sh")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res/
    DESTINATION res
    FILES_MATCHING PATTERN "*.png" PATTERN "*.jpeg")
