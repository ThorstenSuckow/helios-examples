cmake_minimum_required(VERSION 4.0)
project(helios-examples LANGUAGES C CXX)

include(FetchContent)

set(HELIOS_ENGINE_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-engine" CACHE PATH "Local path to helios-engine")
set(HELIOS_OPENGL_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-opengl" CACHE PATH "Local path to helios-opengl")

if(EXISTS "${HELIOS_ENGINE_LOCAL_PATH}/CMakeLists.txt")
    add_subdirectory("${HELIOS_ENGINE_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-engine-build")
else()
    FetchContent_Declare(helios_engine
            GIT_REPOSITORY https://github.com/thorstensuckow/helios-engine.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(helios_engine)
endif()

if(EXISTS "${HELIOS_OPENGL_LOCAL_PATH}/CMakeLists.txt")
    add_subdirectory("${HELIOS_OPENGL_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-opengl-build")
else()
    FetchContent_Declare(helios_opengl
            GIT_REPOSITORY https://github.com/thorstensuckow/helios-opengl.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(helios_opengl)
endif()

file(GLOB_RECURSE EXAMPLE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${EXAMPLE_FILE})
    get_filename_component(SUBDIR ${REL_PATH} DIRECTORY)
    get_filename_component(TARGET_NAME ${EXAMPLE_FILE} NAME_WE)

    string(REPLACE "/" "" UNIQUE_TARGET ${REL_PATH})
    string(REPLACE ".cpp" "" UNIQUE_TARGET ${UNIQUE_TARGET})

    add_executable(${UNIQUE_TARGET} ${EXAMPLE_FILE})

    if (MSVC)
        target_compile_options(${UNIQUE_TARGET} PRIVATE /bigobj)
    endif()

    target_link_libraries(${UNIQUE_TARGET} PRIVATE
            helios
            helios::opengl
    )

    target_compile_definitions(${UNIQUE_TARGET} PRIVATE GLFW_INCLUDE_NONE)

    get_filename_component(EXAMPLE_DIR ${EXAMPLE_FILE} DIRECTORY)
    file(GLOB_RECURSE EXAMPLE_MODULES
            "${EXAMPLE_DIR}/*.ixx"
    )

    if (EXAMPLE_MODULES)
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


