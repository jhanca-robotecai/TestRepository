o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_JHO3DETESTGEM_SUPPORTED)
    return()
endif()

# JHO3DETestGem loads and saves YAML Prefab Templates in the Editor; it has no runtime module.
if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

include(FetchContent)

# fkYAML is header-only; its minimal zip CMakeLists.txt references cmake/pkg-config.pc.in
# which is absent from that archive, so we populate manually and create the target ourselves.
FetchContent_Declare(fkYAML
    URL https://github.com/fktn-k/fkYAML/releases/download/v0.4.4/fkYAML_min.zip
)
FetchContent_GetProperties(fkYAML)
if(NOT fkYAML_POPULATED)
    FetchContent_Populate(fkYAML)
endif()
FetchContent_GetProperties(fkYAML SOURCE_DIR fkYAML_SRC_DIR)
if(NOT TARGET fkYAML::fkYAML)
    add_library(fkYAML INTERFACE)
    add_library(fkYAML::fkYAML ALIAS fkYAML)
    target_include_directories(fkYAML INTERFACE "${fkYAML_SRC_DIR}/include")
endif()

ly_add_target(
    NAME ${gem_name}.Editor.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        jho3detestgem_editor_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
            AZ::AzToolsFramework
)

ly_add_target(
    NAME ${gem_name}.Editor.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        jho3detestgem_editor_private_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzToolsFramework
            Gem::${gem_name}.Editor.API
        PRIVATE
            fkYAML::fkYAML
)

# fkYAML uses C++ exceptions. Re-enable them for this target;
# -fexceptions appears after the global -fno-exceptions, so it wins on GCC/Clang.
target_compile_options(${gem_name}.Editor.Private.Object PRIVATE -fexceptions)

ly_add_target(
    NAME ${gem_name}.Editor GEM_MODULE
    NAMESPACE Gem
    FILES_CMAKE
        jho3detestgem_editor_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.Editor.API
        PRIVATE
            Gem::${gem_name}.Editor.Private.Object
)

ly_add_source_properties(
    SOURCES
        Source/Tools/JHO3DETestGemEditorModule.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES
        O3DE_GEM_NAME=${gem_name}
        O3DE_GEM_VERSION=${gem_version})

ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
ly_create_alias(NAME ${gem_name}.Tools.API    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)
ly_create_alias(NAME ${gem_name}.Builders.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)

o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Tools Builders)
