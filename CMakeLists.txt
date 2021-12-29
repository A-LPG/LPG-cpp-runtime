cmake_minimum_required(VERSION 3.8)

if(POLICY CMP0025)
    # detect Apple's Clang
    cmake_policy(SET CMP0025 NEW)
endif()
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(LPG_RUNTIME_OUTPUT_INTDIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(LPG_LIBRARY_OUTPUT_INTDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include(LpgUtil)
set(LIB_MAJOR_VERSION "1")
set(LIB_MINOR_VERSION "0")
set(LIB_PATCH_VERSION "0")
set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

cmake_policy(SET CMP0048 NEW)
project(cpplpg2 VERSION "${LIB_VERSION_STRING}" LANGUAGES CXX C)

# compile in Debug  mode by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()



option_if_not_defined(CPPLPG2_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option_if_not_defined(CPPLPG2_BUILD_EXAMPLES "Build example applications" OFF)
option_if_not_defined(CPPLPG2_ASAN "Build cpplpg2 with address sanitizer" OFF)
option_if_not_defined(CPPLPG2_MSAN "Build cpplpg2 with memory sanitizer" OFF)
option_if_not_defined(CPPLPG2_TSAN "Build cpplpg2 with thread sanitizer" OFF)
option_if_not_defined(CPPLPG2_INSTALL "Create cpplpg2 install target" OFF)

###########################################################
# Directories
###########################################################
function (set_if_not_defined name value)
    if(NOT DEFINED ${name})
        set(${name} ${value} PARENT_SCOPE)
    endif()
endfunction()

set(CPPLPG2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)




###########################################################
# Targets
###########################################################

# cpplpg2
add_library(cpplpg2 STATIC)

### Includes
target_include_directories(cpplpg2
        PUBLIC ${CPPLPG2_INCLUDE_DIR}
        PRIVATE third_party/utfcpp/source)

load_library()

### Sources
set(CPPLPG2_LIST
        src/AbstractToken.cpp
        src/Any.cpp
        src/BacktrackingParser.cpp
        src/CharOperation.cpp
        src/ConfigurationStack.cpp
        src/DeterministicParser.cpp
        src/Diagnose.cpp
        src/Differ.cpp
        src/DifferLines.cpp
        src/DifferTokens.cpp
        src/ErrorToken.cpp
        src/IcuUtil.cpp
        src/LexParser.cpp
        src/LexStream.cpp
        src/LpgRuntime.cpp
        src/LPGSourcePositionLocator.cpp
        src/PrsStream.cpp
        src/RecoveryParser.cpp
        src/Stacks.cpp
        src/Token.cpp
        src/Utf8LexStream.cpp

        )

target_sources(cpplpg2 PRIVATE
        ${CPPLPG2_LIST})

### Compile options

lspcpp_set_target_options(cpplpg2)

set_target_properties(cpplpg2 PROPERTIES POSITION_INDEPENDENT_CODE 1)

# install
if(CPPLPG2_INSTALL)
    include(GNUInstallDirs)

    install(DIRECTORY ${CPPLPG2_INCLUDE_DIR}/lpg2
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            USE_SOURCE_PERMISSIONS
            )

    install(TARGETS cpplpg2
            EXPORT cpplpg2-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            )

    install(EXPORT cpplpg2-targets
            FILE cpplpg2-config.cmake
            NAMESPACE cpplpg2::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpplpg2
            )
endif()


# examples
if(CPPLPG2_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
