cmake_minimum_required(VERSION 3.24)

project(ShipDesk
    VERSION 0.1.0
    DESCRIPTION "Native Desktop Release Workspace"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable AUTOMOC, AUTORCC, AUTOUIC for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt 6 components
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Test
)

# Global include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler warnings
if (MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------------------------------------------------------
# shipdesk_domain
# -----------------------------------------------------------------------------
add_library(shipdesk_domain STATIC
    src/domain/types.h
    src/domain/common/json_util.h
    src/domain/workspace/workspace.h
    src/domain/workspace/workspace.cpp
    src/domain/source/inventory_item.h
    src/domain/profile/release_profile.h
    src/domain/profile/release_profile.cpp
    src/domain/profile/build_profile.h
    src/domain/profile/build_profile.cpp
    src/domain/release/release_state_machine.h
    src/domain/release/release_state_machine.cpp
    src/domain/release/release_draft.h
    src/domain/release/release_draft.cpp
    src/domain/release/release_record.h
    src/domain/validation/validation_finding.h
    src/domain/validation/validator.h
    src/domain/validation/validator.cpp
    src/domain/artifact/manifest.h
    src/domain/artifact/manifest.cpp
    src/domain/artifact/verification_report.h
    src/domain/destination/destination.h
)
target_link_libraries(shipdesk_domain PUBLIC Qt6::Core)
target_include_directories(shipdesk_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# shipdesk_infrastructure
# -----------------------------------------------------------------------------
if(NOT MSVC)
    set_source_files_properties(
        src/infrastructure/archives/miniz.c
        src/infrastructure/archives/miniz_tdef.c
        src/infrastructure/archives/miniz_tinfl.c
        src/infrastructure/archives/miniz_zip.c
        PROPERTIES COMPILE_OPTIONS "-Wno-type-limits"
    )
endif()
add_library(shipdesk_infrastructure STATIC
    src/infrastructure/filesystem/path_utils.h
    src/infrastructure/filesystem/path_utils.cpp
    src/infrastructure/hashing/hasher.h
    src/infrastructure/hashing/hasher.cpp
    src/infrastructure/archives/miniz.h
    src/infrastructure/archives/miniz.c
    src/infrastructure/archives/miniz_tdef.c
    src/infrastructure/archives/miniz_tinfl.c
    src/infrastructure/archives/miniz_zip.c
    src/infrastructure/archives/zip_archive.h
    src/infrastructure/archives/zip_archive.cpp
    src/infrastructure/process/process_runner.h
    src/infrastructure/process/process_runner.cpp
    src/infrastructure/git/git_inspector.h
    src/infrastructure/git/git_inspector.cpp
    src/infrastructure/database/database_manager.h
    src/infrastructure/database/database_manager.cpp
)
target_link_libraries(shipdesk_infrastructure PUBLIC
    shipdesk_domain
    Qt6::Core
    Qt6::Sql
)
target_include_directories(shipdesk_infrastructure PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# shipdesk_adapters
# -----------------------------------------------------------------------------
add_library(shipdesk_adapters STATIC
    src/adapters/destinations/local_folder_destination.h
    src/adapters/destinations/local_folder_destination.cpp
    src/adapters/destinations/github_destination.h
    src/adapters/destinations/github_destination.cpp
)
target_link_libraries(shipdesk_adapters PUBLIC
    shipdesk_domain
    shipdesk_infrastructure
    Qt6::Core
)
target_include_directories(shipdesk_adapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# shipdesk_application
# -----------------------------------------------------------------------------
add_library(shipdesk_application STATIC
    src/application/workspace/workspace_service.h
    src/application/workspace/workspace_service.cpp
    src/application/inspection/source_inspection_service.h
    src/application/inspection/source_inspection_service.cpp
    src/application/staging/staging_service.h
    src/application/staging/staging_service.cpp
    src/application/packaging/packaging_service.h
    src/application/packaging/packaging_service.cpp
    src/application/verification/verification_service.h
    src/application/verification/verification_service.cpp
    src/application/publishing/publishing_service.h
    src/application/publishing/publishing_service.cpp
    src/application/release/release_service.h
    src/application/release/release_service.cpp
)
target_link_libraries(shipdesk_application PUBLIC
    shipdesk_domain
    shipdesk_infrastructure
    shipdesk_adapters
    Qt6::Core
)
target_include_directories(shipdesk_application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# shipdesk_ui
# -----------------------------------------------------------------------------
add_library(shipdesk_ui STATIC
    src/ui/themes/theme_manager.h
    src/ui/themes/theme_manager.cpp
    src/ui/pages/overview_page.h
    src/ui/pages/overview_page.cpp
    src/ui/pages/workspaces_page.h
    src/ui/pages/workspaces_page.cpp
    src/ui/pages/releases_page.h
    src/ui/pages/releases_page.cpp
    src/ui/pages/activity_page.h
    src/ui/pages/activity_page.cpp
    src/ui/pages/source_inspector_page.h
    src/ui/pages/source_inspector_page.cpp
    src/ui/pages/package_verifier_page.h
    src/ui/pages/package_verifier_page.cpp
    src/ui/pages/diagnostics_page.h
    src/ui/pages/diagnostics_page.cpp
    src/ui/pages/settings_page.h
    src/ui/pages/settings_page.cpp
    src/ui/dialogs/release_wizard_dialog.h
    src/ui/dialogs/release_wizard_dialog.cpp
    src/ui/dialogs/publish_dialog.h
    src/ui/dialogs/publish_dialog.cpp
    src/ui/windows/main_window.h
    src/ui/windows/main_window.cpp
)
target_link_libraries(shipdesk_ui PUBLIC
    shipdesk_application
    shipdesk_domain
    shipdesk_infrastructure
    shipdesk_adapters
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
target_include_directories(shipdesk_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# shipdesk (Main Executable)
# -----------------------------------------------------------------------------
set(SHIPDESK_EXTRA_SOURCES
    resources/shipdesk.qrc
)

if (WIN32)
    list(APPEND SHIPDESK_EXTRA_SOURCES resources/shipdesk.rc)
endif()

add_executable(shipdesk
    src/app/main.cpp
    ${SHIPDESK_EXTRA_SOURCES}
)
target_link_libraries(shipdesk PRIVATE
    shipdesk_ui
)

# -----------------------------------------------------------------------------
# shipdesk_tests (Automated Test Suite)
# -----------------------------------------------------------------------------
enable_testing()

add_executable(shipdesk_tests
    tests/test_main.cpp
)
target_link_libraries(shipdesk_tests PRIVATE
    shipdesk_application
    shipdesk_ui
    Qt6::Test
)
add_test(NAME ShipDeskAutomatedTests COMMAND shipdesk_tests)
