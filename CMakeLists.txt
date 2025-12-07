cmake_minimum_required(VERSION 3.16)

# ----------------------------------------
# Project
# ----------------------------------------
project(flv-parser VERSION 1.0.4 LANGUAGES CXX)

# Qt build helpers
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------
# Dependencies (Qt5/Qt6 Widgets)
# ----------------------------------------
# Try Qt6 first, fallback to Qt5 if not found
find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# ----------------------------------------
# Sources
# ----------------------------------------
file(GLOB MODEL_SOURCES "src/model/*.h" "src/model/*.cpp")
file(GLOB VIEW_SOURCES "src/view/*.h" "src/view/*.cpp" "src/view/*.ui")
file(GLOB UTILS_SOURCES "src/utils/*.h" "src/utils/*.cpp")

set(PROJECT_SOURCES
    src/main.cpp
    ${MODEL_SOURCES}
    ${VIEW_SOURCES}
    ${UTILS_SOURCES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(flv-parser MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
    add_executable(flv-parser ${PROJECT_SOURCES})
endif()

target_include_directories(flv-parser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/view
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

target_link_libraries(flv-parser PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# ----------------------------------------
# Windows: embed application icon into exe (static rc) and add Qt resource
# ----------------------------------------
if (WIN32)
    if (EXISTS "${CMAKE_SOURCE_DIR}/res/windows/app.rc")
        target_sources(flv-parser PRIVATE "${CMAKE_SOURCE_DIR}/res/windows/app.rc")
    endif()
    if (EXISTS "${CMAKE_SOURCE_DIR}/res/app.qrc")
        target_sources(flv-parser PRIVATE "${CMAKE_SOURCE_DIR}/res/app.qrc")
    endif()
endif()

# ----------------------------------------
# Windows: deploy Qt runtime at build time (windeployqt)
# ----------------------------------------
if (WIN32)
    find_program(_qt_windeploy NAMES windeployqt windeployqt.exe)
    if (_qt_windeploy)
        set(DEPLOY_DIR "${CMAKE_CURRENT_BINARY_DIR}/deploy")
        add_custom_command(TARGET flv-parser POST_BUILD
            COMMAND "${_qt_windeploy}" --release --compiler-runtime --no-quick-import --no-translations
                    --dir "${DEPLOY_DIR}" "$<TARGET_FILE:flv-parser>"
            VERBATIM
            COMMENT "Running windeployqt -> ${DEPLOY_DIR}"
        )
        # Install deployed runtime next to the exe at install root
        install(DIRECTORY "${DEPLOY_DIR}/" DESTINATION .)
    else()
        message(WARNING "windeployqt not found; Qt DLLs will not be bundled automatically.")
    endif()
endif()

set_target_properties(flv-parser PROPERTIES
        WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS flv-parser
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION .
)

# Install license materials at install root: <prefix>/license
if (WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/license")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/license/ DESTINATION license)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(flv-parser)
endif()

# ----------------------------------------
# Packaging (NSIS via CPack)
# ----------------------------------------
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "flv-parser")
set(CPACK_PACKAGE_VENDOR "Q-Q-Q")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "flv-parser")
set(CPACK_NSIS_DISPLAY_NAME "flv-parser")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_GENERATOR "NSIS")

# Start Menu shortcuts (exe installed at install root)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_PACKAGE_EXECUTABLES "flv-parser;FLV Parser")

# Use app icon for installer/uninstaller if provided
if (EXISTS "${CMAKE_SOURCE_DIR}/res/windows/app.ico")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/res/windows/app.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/res/windows/app.ico")
endif()

include(CPack)
