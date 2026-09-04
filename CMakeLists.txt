cmake_minimum_required(VERSION 3.24)

project(BackroomsOffical VERSION 0.3.8 LANGUAGES C CXX)

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

add_library(BackroomsGlad STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/gl.c"
)
target_include_directories(BackroomsGlad PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include"
)
target_link_libraries(BackroomsGlad PUBLIC
    ${CMAKE_DL_LIBS}
)

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

add_executable(BackroomsOffical ${BACKROOMS_SOURCES})

target_include_directories(BackroomsOffical PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${miniaudio_SOURCE_DIR}"
)

target_link_libraries(BackroomsOffical PRIVATE
    SDL3::SDL3
    glm::glm
    BackroomsGlad
    OpenGL::GL
)

if(WIN32)
    target_link_libraries(BackroomsOffical PRIVATE
        winhttp
        bcrypt
        shell32
    )
endif()

if(MSVC)
    target_compile_options(BackroomsOffical PRIVATE /W4 /permissive-)
else()
    target_compile_options(BackroomsOffical PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    set_target_properties(BackroomsOffical PROPERTIES
        WIN32_EXECUTABLE FALSE
        OUTPUT_NAME "Backrooms Offical"
    )
endif()

if(APPLE)
    set_target_properties(BackroomsOffical PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Backrooms Offical"
        OUTPUT_NAME "Backrooms Offical"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.spongebobtdgameplay.backrooms"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "Backrooms.icns"
    )
endif()

add_custom_command(
    TARGET BackroomsOffical POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:BackroomsOffical>/assets"
)

if(WIN32)
    add_custom_command(
        TARGET BackroomsOffical POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3>"
            "$<TARGET_FILE_DIR:BackroomsOffical>/SDL3.dll"
    )
endif()
