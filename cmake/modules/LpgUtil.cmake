###########################################################
# Options
###########################################################
function (option_if_not_defined name description default)
    if(NOT DEFINED ${name})
        option(${name} ${description} ${default})
    endif()
endfunction()


macro(cpplpg2_set_target_options_with_nuget_pkg target id version)
    if (CMAKE_GENERATOR MATCHES "Visual Studio.*")
        if(EXISTS ${CMAKE_BINARY_DIR}/packages/${id}.${version}/build/${id}.targets)
            target_link_libraries(${target} PRIVATE ${CMAKE_BINARY_DIR}/packages/${id}.${version}/build/${id}.targets)
        elseif(EXISTS ${CMAKE_BINARY_DIR}/packages/${id}.${version}/build/native/${id}.targets)
            target_link_libraries(${target} PRIVATE ${CMAKE_BINARY_DIR}/packages/${id}.${version}/build/native/${id}.targets)
        else()
            message(FATAL_ERROR "Can't find target of ${id}.${version}")
        endif()
    else()
        message(FATAL_ERROR "NUGET package only use in Visual Studio")
    endif()

endmacro()

macro(CPPLPG2_INSTALL_NUGET id version)
    if (CMAKE_GENERATOR MATCHES "Visual Studio.*")
        unset(nuget_cmd)
        list(APPEND nuget_cmd install ${id} -Prerelease -Version ${version} -OutputDirectory ${CMAKE_BINARY_DIR}/packages)
        message("excute nuget install:${nuget_cmd}")
        execute_process(COMMAND nuget ${nuget_cmd} ENCODING AUTO)
    else()
        message(FATAL_ERROR "CPPLPG2_INSTALL_NUGET only use in Visual Studio")
    endif()

endmacro()

# Libraries
###########################################################
# Functions
###########################################################


###########################################################
# icu4c library
###########################################################
function(load_library)
    if (CMAKE_GENERATOR MATCHES "Visual Studio.*")
        CPPLPG2_INSTALL_NUGET(icu4c.v140 59.1.1)
        CPPLPG2_INSTALL_NUGET(icu4c.v140.redist 59.1.1)
    else()
        find_package(PkgConfig)
        if (NOT PKG_CONFIG_FOUND)
            if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                message(STATUS "Can't find PKG-config,please install it with : brew install pkg-config")
            elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
                message(STATUS "Can't find PKG-config,please install it with : sudo apt-get install pkg-config")
            endif()
        endif()

        pkg_check_modules(my_icu4c_deps IMPORTED_TARGET icu-io icu-uc icu-i18n)
        if(NOT my_icu4c_deps_FOUND)
            if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                message(STATUS "Can't find icu4c,lease build icu4c and install it or install icu4c with :")
                message(FATAL_ERROR "brew install icu4c \nbrew link icu4c --force")
            elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
                message(STATUS "Can't find boost,please build icu4c and install it. or install icu4c with : sudo apt-get install libicu-dev")
            endif()
        endif()
    endif()

endfunction()


function(cpplpg2_set_target_options target)

    set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)

    # Enable C++17 (Required)
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 17)

    set_property(TARGET ${target} PROPERTY CXX_EXTENSIONS OFF)

    if (CMAKE_GENERATOR MATCHES "Visual Studio.*")
        cpplpg2_set_target_options_with_nuget_pkg(${target} icu4c.v140 59.1.1)
        cpplpg2_set_target_options_with_nuget_pkg(${target} icu4c.v140.redist 59.1.1)
    else()
        target_link_libraries(${target}
                PRIVATE
                PkgConfig::my_icu4c_deps )
        target_include_directories(${target}
                PRIVATE  PkgConfig::my_icu4c_deps)
    endif()

    # Enable all warnings
    if(MSVC)
        target_compile_options(${target} PRIVATE "-W4")
    else()
        target_compile_options(${target} PRIVATE "-Wall")
    endif()

    # Disable specific, pedantic warnings
    if(MSVC)
        target_compile_options(${target} PRIVATE
                "-D_CRT_SECURE_NO_WARNINGS"

                # Warnings from nlohmann/json headers.
                "/wd4267" # 'argument': conversion from 'size_t' to 'int', possible loss of data
                "/bigobj" # for visual studio 2022 x64 or later.
                )
    endif()


    # Treat all warnings as errors
    if(CPPLPG2_WARNINGS_AS_ERRORS)
        if(MSVC)
            target_compile_options(${target} PRIVATE "/WX")
        else()
            target_compile_options(${target} PRIVATE "-Werror")
        endif()
    endif(CPPLPG2_WARNINGS_AS_ERRORS)

    if(CPPLPG2_ASAN)
        target_compile_options(${target} PUBLIC "-fsanitize=address")
        target_link_libraries(${target} PRIVATE "-fsanitize=address")
    elseif(CPPLPG2_MSAN)
        target_compile_options(${target} PUBLIC "-fsanitize=memory")
        target_link_libraries(${target} PRIVATE "-fsanitize=memory")
    elseif(CPPLPG2_TSAN)
        target_compile_options(${target} PUBLIC "-fsanitize=thread")
        target_link_libraries(${target} PRIVATE "-fsanitize=thread")
    endif()

    # Error on undefined symbols
    # if(NOT MSVC)
    #     target_compile_options(${target} PRIVATE "-Wl,--no-undefined")
    # endif()

endfunction()

# Set each output directory according to ${CMAKE_CONFIGURATION_TYPES}.
# Note: Don't set variables CMAKE_*_OUTPUT_DIRECTORY any more,
# or a certain builder, for eaxample, msbuild.exe, would be confused.
function(set_output_directory target)
    cmake_parse_arguments(ARG "" "BINARY_DIR;LIBRARY_DIR" "" ${ARGN})

    # module_dir -- corresponding to LIBRARY_OUTPUT_DIRECTORY.
    # It affects output of add_library(MODULE).
    if(WIN32 OR CYGWIN)
        # DLL platform
        set(module_dir ${ARG_BINARY_DIR})
    else()
        set(module_dir ${ARG_LIBRARY_DIR})
    endif()
    if(NOT "${CMAKE_CFG_INTDIR}" STREQUAL ".")
        foreach(build_mode ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER "${build_mode}" CONFIG_SUFFIX)
            if(ARG_BINARY_DIR)
                string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} bi ${ARG_BINARY_DIR})
                set_target_properties(${target} PROPERTIES "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${bi})
            endif()
            if(ARG_LIBRARY_DIR)
                string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} li ${ARG_LIBRARY_DIR})
                set_target_properties(${target} PROPERTIES "ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${li})
            endif()
            if(module_dir)
                string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} mi ${module_dir})
                set_target_properties(${target} PROPERTIES "LIBRARY_OUTPUT_DIRECTORY_${CONFIG_SUFFIX}" ${mi})
            endif()
        endforeach()
    else()
        if(ARG_BINARY_DIR)
            set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ARG_BINARY_DIR})
        endif()
        if(ARG_LIBRARY_DIR)
            set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ARG_LIBRARY_DIR})
        endif()
        if(module_dir)
            set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${module_dir})
        endif()
    endif()
endfunction()

function(add_lpg_example target)

    add_executable(${target})
    cpplpg2_set_target_options(${target})
    target_include_directories(${target} PRIVATE cpplpg2)
    target_link_libraries(${target} PRIVATE cpplpg2)
    add_dependencies(${target} cpplpg2)
    set_output_directory(${target} BINARY_DIR ${LPG_RUNTIME_OUTPUT_INTDIR} LIBRARY_DIR ${LPG_LIBRARY_OUTPUT_INTDIR})
endfunction()