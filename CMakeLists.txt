cmake_minimum_required(VERSION 4.0)
project(helios-examples LANGUAGES C CXX)

include(FetchContent)

set(HELIOS_USE_DEVTOOLS_DEFAULT OFF)
if(PROJECT_IS_TOP_LEVEL)
    set(HELIOS_USE_DEVTOOLS_DEFAULT ON)
endif()

option(HELIOS_USE_DEVTOOLS "Load shared tooling from helios-devtools" ${HELIOS_USE_DEVTOOLS_DEFAULT})
set(HELIOS_DEVTOOLS_GIT_REPOSITORY "https://github.com/thorstensuckow/helios-devtools.git" CACHE STRING "helios-devtools git repository")
set(HELIOS_DEVTOOLS_GIT_TAG "v1.0.0" CACHE STRING "helios-devtools git tag or commit")
set(HELIOS_DEVTOOLS_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-devtools" CACHE PATH "Optional local path to helios-devtools")

if(HELIOS_USE_DEVTOOLS)
    if(HELIOS_DEVTOOLS_LOCAL_PATH)
        FetchContent_Declare(
                helios_devtools
                SOURCE_DIR "${HELIOS_DEVTOOLS_LOCAL_PATH}"
        )
    else()
        FetchContent_Declare(
                helios_devtools
                GIT_REPOSITORY "${HELIOS_DEVTOOLS_GIT_REPOSITORY}"
                GIT_TAG "${HELIOS_DEVTOOLS_GIT_TAG}"
        )
    endif()

    FetchContent_MakeAvailable(helios_devtools)
    include("${helios_devtools_SOURCE_DIR}/cmake/HeliosDevtools.cmake")
endif()

function(helios_enable_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
                /W4
                /we4715
        )
    else()
        target_compile_options(${target} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wreturn-type
                -Werror=return-type
        )
    endif()
endfunction()

set(HELIOS_ECS_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-ecs" CACHE PATH "Local path to helios-ecs")
set(HELIOS_ENGINE_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-engine" CACHE PATH "Local path to helios-engine")
set(HELIOS_OPENGL_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-opengl" CACHE PATH "Local path to helios-opengl")
set(HELIOS_GLFW_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-glfw" CACHE PATH "Local path to helios-glfw")
set(HELIOS_IMGUI_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-imgui" CACHE PATH "Local path to helios-imgui")
set(HELIOS_PHYSICS_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-physics" CACHE PATH "Local path to helios-physics")
set(HELIOS_GAMEPLAY_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-gameplay" CACHE PATH "Local path to helios-gameplay")

if(NOT TARGET helios::ecs)
    if(EXISTS "${HELIOS_ECS_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_ECS_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-ecs-build")
    else()
        FetchContent_Declare(helios_ecs
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-ecs.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_ecs)
    endif()
endif()


if(NOT TARGET helios::engine)
    if(EXISTS "${HELIOS_ENGINE_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_ENGINE_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-engine-build")
    else()
        FetchContent_Declare(helios_engine
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-engine.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_engine)
    endif()
endif()

if(NOT TARGET helios::opengl)
    if(EXISTS "${HELIOS_OPENGL_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_OPENGL_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-opengl-build")
    else()
        FetchContent_Declare(helios_opengl
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-opengl.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_opengl)
    endif()
endif()

if(NOT TARGET helios::glfw)
    if(EXISTS "${HELIOS_GLFW_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_GLFW_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-glfw-build")
    else()
        FetchContent_Declare(helios_glfw
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-glfw.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_glfw)
    endif()
endif()

if(NOT TARGET helios::gameplay)
    if(EXISTS "${HELIOS_GAMEPLAY_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_GAMEPLAY_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-gameplay-build")
    else()
        FetchContent_Declare(helios_gameplay
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-gameplay.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_gameplay)
    endif()
endif()

if(NOT TARGET helios::imgui)
    if(EXISTS "${HELIOS_IMGUI_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_IMGUI_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-imgui-build")
    else()
        FetchContent_Declare(helios_imgui
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-imgui.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_imgui)
    endif()
endif()

if(NOT TARGET helios::physics)
    if(EXISTS "${HELIOS_PHYSICS_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_PHYSICS_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-physics-build")
    else()
        FetchContent_Declare(helios_physics
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-physics.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_physics)
    endif()
endif()

add_library(helios_examples_bootstrap STATIC)
target_sources(helios_examples_bootstrap
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/bootstrap.ixx"
)

if(HELIOS_USE_DEVTOOLS)
    helios_configure_target(helios_examples_bootstrap)
endif()


target_link_libraries(
        helios_examples_bootstrap
        PUBLIC
        helios::ecs
        helios::engine
        helios::opengl
        helios::glfw
        helios::imgui
        helios::gameplay
        helios::physics
)

file(GLOB_RECURSE EXAMPLE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${EXAMPLE_FILE})
    get_filename_component(SUBDIR ${REL_PATH} DIRECTORY)
    get_filename_component(TARGET_NAME ${EXAMPLE_FILE} NAME_WE)

    string(REPLACE "/" "" UNIQUE_TARGET ${REL_PATH})
    string(REPLACE ".cpp" "" UNIQUE_TARGET ${UNIQUE_TARGET})

    add_executable(${UNIQUE_TARGET} ${EXAMPLE_FILE})

    helios_enable_strict_warnings(${UNIQUE_TARGET})

    if(MSVC)
        target_compile_options(${UNIQUE_TARGET} PRIVATE /bigobj)
    endif()

    target_link_libraries(${UNIQUE_TARGET} PRIVATE
            helios_examples_bootstrap
            helios::ecs
            helios::engine
            helios::opengl
            helios::glfw
            helios::imgui
            helios::gameplay
            helios::physics
    )

    target_compile_definitions(${UNIQUE_TARGET} PRIVATE GLFW_INCLUDE_NONE)

    get_filename_component(EXAMPLE_DIR ${EXAMPLE_FILE} DIRECTORY)
    file(GLOB_RECURSE EXAMPLE_MODULES
            "${EXAMPLE_DIR}/*.ixx"
    )

    if(EXAMPLE_MODULES)
        target_sources(${UNIQUE_TARGET}
                PRIVATE
                FILE_SET cxx_modules TYPE CXX_MODULES FILES
                ${EXAMPLE_MODULES}
        )
    endif()

    set_target_properties(${UNIQUE_TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/${SUBDIR}
            OUTPUT_NAME ${TARGET_NAME}
    )

    set(RESOURCES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}/resources")
    set(RESOURCES_DEST_DIR "${CMAKE_BINARY_DIR}/examples/${SUBDIR}/resources")

    if(EXISTS "${RESOURCES_SOURCE_DIR}")
        add_custom_command(
                TARGET ${UNIQUE_TARGET}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${RESOURCES_SOURCE_DIR}"
                "${RESOURCES_DEST_DIR}"
        )
    endif()
endforeach()