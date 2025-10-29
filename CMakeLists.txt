cmake_minimum_required(VERSION 3.16)
project(SDL3Defender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Collect sources
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/entities/*.cpp"
    "${CMAKE_SOURCE_DIR}/entities/opponents/*.cpp"
    "${CMAKE_SOURCE_DIR}/main.cpp"
)
add_executable(${PROJECT_NAME} ${SOURCES})

# SDL3
find_package(SDL3 REQUIRED CONFIG)

# Fix for missing SDL3::Headers target (found in some distros)
if (NOT TARGET SDL3::Headers)
    add_library(SDL3::Headers INTERFACE IMPORTED)
    if (DEFINED SDL3_INCLUDE_DIRS)
        target_include_directories(SDL3::Headers INTERFACE ${SDL3_INCLUDE_DIRS})
    endif()
    message(WARNING "SDL3::Headers target not provided by your SDL3 package — created manually.")
endif()

# SDL3_image & SDL3_ttf
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(SDL3_mixer REQUIRED CONFIG)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    SDL3_mixer::SDL3_mixer
)

# Filesystem support for older GCC versions
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <filesystem>
    int main() { std::filesystem::path p{\".\"}; return 0; }
" HAS_FILESYSTEM)

if (NOT HAS_FILESYSTEM)
    message(STATUS "Adding stdc++fs library for filesystem support")
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()

# Look for asset directories relative to project root
set(POSSIBLE_ASSET_DIRS
    "${CMAKE_SOURCE_DIR}/assets"
    "${CMAKE_SOURCE_DIR}/resources"
    "${CMAKE_SOURCE_DIR}/data"
)

set(FOUND_ASSET_DIRS "")
foreach(dir IN LISTS POSSIBLE_ASSET_DIRS)
    if (EXISTS "${dir}")
        list(APPEND FOUND_ASSET_DIRS "${dir}")
        message(STATUS "Asset dir found: ${dir}")
    endif()
endforeach()

if (FOUND_ASSET_DIRS)
    foreach(src_dir IN LISTS FOUND_ASSET_DIRS)
        get_filename_component(dir_name ${src_dir} NAME)
        set(dst_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${dir_name}")

        # Copy AFTER the executable is built (no cyclic dependency)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${src_dir}" "${dst_dir}"
            COMMENT "Copying ${dir_name} -> ${dst_dir}"
        )
    endforeach()
else()
    message(WARNING "No asset directories (assets/, resources/, data/) were found in the project root (${CMAKE_SOURCE_DIR}).")
endif()


# Platform tweaks
if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.sdl3defender"
    )
endif()
