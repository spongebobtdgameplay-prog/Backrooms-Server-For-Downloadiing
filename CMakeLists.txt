cmake_minimum_required(VERSION 3.24)

project(BackroomsNative VERSION 0.3.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.20
)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl_core_41 REPRODUCIBLE API gl:core=4.1)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif()

find_package(OpenGL REQUIRED)

file(GLOB_RECURSE BACKROOMS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(WIN32)
    list(APPEND BACKROOMS_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Windows/Backrooms.rc"
    )
endif()

if(APPLE)
    set(BACKROOMS_MAC_ICON
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/Backrooms.icns"
    )
    set_source_files_properties(
        "${BACKROOMS_MAC_ICON}"
        PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    list(APPEND BACKROOMS_SOURCES "${BACKROOMS_MAC_ICON}")
endif()

add_executable(BackroomsNative ${BACKROOMS_SOURCES})

target_include_directories(BackroomsNative PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${miniaudio_SOURCE_DIR}"
)

target_link_libraries(BackroomsNative PRIVATE
    SDL3::SDL3
    glm::glm
    glad_gl_core_41
    OpenGL::GL
)

if(MSVC)
    target_compile_options(BackroomsNative PRIVATE /W4 /permissive-)
else()
    target_compile_options(BackroomsNative PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    set_target_properties(BackroomsNative PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

if(APPLE)
    set_target_properties(BackroomsNative PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "The Backrooms"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.spongebobtdgameplay.backrooms"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "Backrooms.icns"
    )
endif()

add_custom_command(
    TARGET BackroomsNative POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:BackroomsNative>/assets"
)
