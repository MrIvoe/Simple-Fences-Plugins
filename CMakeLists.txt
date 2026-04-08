cmake_minimum_required(VERSION 3.20)

project(SpacesPlugins VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# HOST_SDK_PATH must point to the Spaces host SDK directory — the
# directory that directly contains the 'extensions/' header folder.
# Pass it at configure time:
#   cmake -DHOST_SDK_PATH="C:/Dev/Spaces/sdk" ...
# ---------------------------------------------------------------------------
if(NOT DEFINED HOST_SDK_PATH OR HOST_SDK_PATH STREQUAL "")
    message(FATAL_ERROR
        "\n"
        "  HOST_SDK_PATH is required.\n"
        "  It must point to the Spaces host SDK directory that\n"
        "  contains the 'extensions/' header folder.\n"
        "\n"
        "  Example:\n"
        "    cmake -DHOST_SDK_PATH=\"C:/Dev/Spaces/sdk\" ..\n")
endif()

cmake_path(SET _sdk_path NORMALIZE "${HOST_SDK_PATH}")
if(NOT IS_DIRECTORY "${_sdk_path}")
    message(FATAL_ERROR "HOST_SDK_PATH does not exist or is not a directory: ${_sdk_path}")
endif()
if(NOT IS_DIRECTORY "${_sdk_path}/extensions")
    message(FATAL_ERROR
        "HOST_SDK_PATH does not contain an 'extensions/' subdirectory.\n"
        "  Expected: ${_sdk_path}/extensions\n"
        "  Verify that HOST_SDK_PATH points to the right folder.")
endif()

message(STATUS "Host SDK: ${_sdk_path}")

# ---------------------------------------------------------------------------
# Common compile definitions for every plugin target
# ---------------------------------------------------------------------------
set(PLUGIN_COMPILE_DEFS UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

# ---------------------------------------------------------------------------
# add_plugin(<TARGET_NAME> <PLUGIN_DIR> SOURCES <file1> [file2 ...])
#
#   TARGET_NAME  — CMake target identifier (underscores, no hyphens)
#   PLUGIN_DIR   — directory name under plugins/ (may contain hyphens)
#   SOURCES      — list of .cpp files relative to repo root
#
# Each plugin builds as a Windows DLL.  Output lands in:
#   <BUILD_DIR>/output/plugins/<PLUGIN_DIR>/
# alongside a copy of its plugin.json.
# ---------------------------------------------------------------------------
function(add_plugin TARGET_NAME PLUGIN_DIR PLUGIN_HEADER PLUGIN_CLASS)
    cmake_parse_arguments(_P "" "" "SOURCES" ${ARGN})

    if(NOT _P_SOURCES)
        message(FATAL_ERROR "add_plugin(${TARGET_NAME}): no SOURCES provided")
    endif()

    set(_export_src "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_exports.cpp")
    set(PLUGIN_HEADER "${PLUGIN_HEADER}")
    set(PLUGIN_CLASS "${PLUGIN_CLASS}")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PluginExports.cpp.in" "${_export_src}" @ONLY)

    add_library(${TARGET_NAME} SHARED
        ${_P_SOURCES}
        "${_export_src}"
        "${CMAKE_CURRENT_SOURCE_DIR}/plugins/shared/HostSdkRuntimeShim.cpp"
    )

    target_include_directories(${TARGET_NAME} PRIVATE
        "${_sdk_path}"                                                        # extensions/
        "${CMAKE_CURRENT_SOURCE_DIR}/plugins/${PLUGIN_DIR}/src"               # plugin-own headers
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE ${PLUGIN_COMPILE_DEFS})

    # Force UTF-8 source encoding on MSVC
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /utf-8 /W3)
    endif()

    set(_out_dir "${CMAKE_BINARY_DIR}/output/plugins/${PLUGIN_DIR}")

    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME             "${PLUGIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${_out_dir}"
        LIBRARY_OUTPUT_DIRECTORY "${_out_dir}"
        PDB_OUTPUT_DIRECTORY     "${_out_dir}"
    )

    # Multi-config generators (VS) place outputs in a <config> subfolder by
    # default — override to keep our flat structure.
    foreach(_cfg Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER ${_cfg} _CFG)
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${_out_dir}"
            LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${_out_dir}"
            PDB_OUTPUT_DIRECTORY_${_CFG}     "${_out_dir}"
        )
    endforeach()

    # Copy plugin.json next to the DLL after each build
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/plugins/${PLUGIN_DIR}/plugin.json"
            "${_out_dir}/plugin.json"
        COMMENT "  [${PLUGIN_DIR}] Copying plugin.json"
    )
endfunction()

# ---------------------------------------------------------------------------
# Plugin targets — one per plugin subdirectory
# ---------------------------------------------------------------------------

add_plugin(plugin_clock_widget          clock-widget         ClockWidgetPlugin.h         ClockWidgetPlugin         SOURCES plugins/clock-widget/src/ClockWidgetPlugin.cpp)
add_plugin(plugin_context_actions       context-actions      ContextActionsPlugin.h      ContextActionsPlugin      SOURCES plugins/context-actions/src/ContextActionsPlugin.cpp)
add_plugin(plugin_external_provider     external-provider    ExternalProviderPlugin.h    ExternalProviderPlugin    SOURCES plugins/external-provider/src/ExternalProviderPlugin.cpp)
add_plugin(plugin_space_organizer       space-organizer      SpaceOrganizerPlugin.h      SpaceOrganizerPlugin      SOURCES plugins/space-organizer/src/SpaceOrganizerPlugin.cpp)
add_plugin(plugin_folder_portal         folder-portal        FolderPortalPlugin.h        FolderPortalPlugin        SOURCES plugins/folder-portal/src/FolderPortalPlugin.cpp)
add_plugin(plugin_network_drive_space   network-drive-space  NetworkDriveSpacePlugin.h   NetworkDriveSpacePlugin   SOURCES plugins/network-drive-space/src/NetworkDriveSpacePlugin.cpp)
add_plugin(plugin_powershell_space      powershell-space     PowerShellSpacePlugin.h     PowerShellSpacePlugin     SOURCES plugins/powershell-space/src/PowerShellSpacePlugin.cpp)
add_plugin(plugin_productivity_actions  productivity-actions ProductivityActionsPlugin.h ProductivityActionsPlugin SOURCES plugins/productivity-actions/src/ProductivityActionsPlugin.cpp)
add_plugin(plugin_rules_engine          rules-engine         RulesEnginePlugin.h         RulesEnginePlugin         SOURCES plugins/rules-engine/src/RulesEnginePlugin.cpp)
add_plugin(plugin_sorting_cleanup       sorting-clean-up     SortingCleanupPlugin.h      SortingCleanupPlugin      SOURCES plugins/sorting-clean-up/src/SortingCleanupPlugin.cpp)
add_plugin(plugin_visual_modes          visual-modes         VisualModesPlugin.h         VisualModesPlugin         SOURCES plugins/visual-modes/src/VisualModesPlugin.cpp)
add_plugin(plugin_widgets_plus          widgets-plus         WidgetsPlusPlugin.h         WidgetsPlusPlugin         SOURCES plugins/widgets-plus/src/WidgetsPlusPlugin.cpp)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "Configured 12 plugin targets.")
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/output/plugins/")
