# Compilation
# ---------------------------------------------------------------------------- #

add_library(alpha-lib SHARED
    src/overlay_alpha.cpp
)
target_include_directories(alpha-lib 
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# Options and dependencies
# ---------------------------------------------------------------------------- #

if (TARGET OpenMP::OpenMP_CXX)
    target_link_libraries(alpha-lib PRIVATE OpenMP::OpenMP_CXX)
endif()
if (TARGET NEON_2_SSE::NEON_2_SSE)
    target_link_libraries(alpha-lib PRIVATE NEON_2_SSE::NEON_2_SSE)
    target_compile_options(alpha-lib PRIVATE -Wno-deprecated-declarations)
endif()
if (TARGET SSE4)
    target_link_libraries(alpha-lib PRIVATE SSE4)
endif()

# Tests
# ---------------------------------------------------------------------------- #

add_subdirectory(test)

# Installation
# ---------------------------------------------------------------------------- #

add_library(ARM-NEON-Compositor::alpha-lib ALIAS alpha-lib)
set_target_properties(alpha-lib PROPERTIES 
    EXPORT_NAME ARM-NEON-Compositor::alpha-lib)

include(GNUInstallDirs)

set(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ARM-NEON-Compositor")

# Add the alpha-lib library to the "export-set", install the library files
install(TARGETS alpha-lib
    EXPORT ARM-NEON-CompositorTargets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT shlib
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" 
        COMPONENT lib
)

# Install the header files
INSTALL (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT dev
    FILES_MATCHING PATTERN "*.h*"
)

# Install the export set for use with the install-tree
install(EXPORT ARM-NEON-CompositorTargets 
    FILE ARM-NEON-CompositorTargets.cmake
    DESTINATION "${INSTALL_CMAKE_DIR}" 
        COMPONENT dev
)

# Generate the config file that includes the exports
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${PROJECT_BINARY_DIR}/ARM-NEON-CompositorConfig.cmake"
    INSTALL_DESTINATION "${INSTALL_CMAKE_DIR}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/ARM-NEON-CompositorConfigVersion.cmake"
    VERSION "${ARM-NEON-Compositor_VERSION}"
    COMPATIBILITY SameMajorVersion
)

# Install the ARM-NEON-CompositorConfig.cmake and 
# ARM-NEON-CompositorConfigVersion.cmake
install(FILES
    "${PROJECT_BINARY_DIR}/ARM-NEON-CompositorConfig.cmake"
    "${PROJECT_BINARY_DIR}/ARM-NEON-CompositorConfigVersion.cmake"
    DESTINATION "${INSTALL_CMAKE_DIR}" 
        COMPONENT dev)

# Add all targets to the build tree export set
export(EXPORT ARM-NEON-CompositorTargets
    FILE "${PROJECT_BINARY_DIR}/ARM-NEON-CompositorTargets.cmake")

# Export the package for use from the build tree
# (this registers the build tree with a global CMake-registry, so you can use
# find_package(ARM-NEON-Compositor) to find the package in the build tree
export(PACKAGE ARM-NEON-Compositor)
