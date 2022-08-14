# Read CMake documentation at https://cmake.org/cmake/help/latest/

# Sets the minimum required version of cmake for a project. Call the
# cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command. Calling
# cmake_minimum_required() inside a function() limits some effects to the
# function scope when invoked.
cmake_minimum_required(VERSION 3.10)

# Use -std=gnu++17 (the options are 98, 11, 14, 17, 20, and 23).
set(CMAKE_CXX_STANDARD 17)

# Use ISO standard without GNU extensions.
# For example, necessary for using -std=c++17 instead of -std=gnu++17.
set(CMAKE_CXX_EXTENSIONS OFF)

# If this property is OFF or unset, the CXX_STANDARD target property is treated
# as optional and may "decay" to a previous standard if the requested is not
# available.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set PROJECT_NAME variable.
project(imprimis_gameserver
    VERSION 1.0.0     # VERSION <major>[.<minor>[.<patch>]]
    DESCRIPTION "Imprimis Dedicated Server"
    HOMEPAGE_URL "https://github.com/project-imprimis/imprimis-gameserver"
    LANGUAGES C CXX)

# Show help message.
message(NOTICE "
# Go into Makefile directory.
cd \"${CMAKE_BINARY_DIR}\"

  # To build program:
  make

  # To build program while showing compilation flags.
  make VERBOSE=1

  # To clean all compiled files.
  make clean

  # To compile the program without compiling subprojects (enet).
  make ${PROJECT_NAME}/fast

  # To install the ${PROJECT_NAME}.
  make install

  # To uninstall the ${PROJECT_NAME}.
  make uninstall

# Go into CMakeLists.txt directory.
cd \"${CMAKE_CURRENT_LIST_DIR}\"

  # To re-generate build system in 'build' directory:
  cmake -S . -B build

  # To activate Release profile (the default):
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

  # To activate Debug profile (and turn off Release profile):
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
")

# Add compiler flags.
if(MSVC) # Windows flags.
  add_compile_options(/Wall /fp:fast)
else() # Linux flags.
  add_compile_options(-Wall           # Turn on warnings.
                      -ffast-math     # Faster code; doesn't follow ISO specs.
                      -fno-exceptions # Do not throw exceptions: abort instead.
                      -fno-rtti       # Do not generate virtual function info.
                      -fsigned-char   # Allow char to be signed.
                      )

  # The -ggdb and -O flags are incompatible. Add -ggdb3 to Debug builds
  # and -O3 to Release builds.
  if(CMAKE_BUILD_TYPE MATCHES Debug)  # Debug profile.
      add_definitions(-DDEBUG=1)      # Enable DEBUG macros.
      add_compile_options(-ggdb3)     # Produce GDB debug info.
  else()                              # Release profile.
      set(CMAKE_BUILD_TYPE Release)   # Set build type to report it to user.
      add_compile_options(-O3)        # Turn on speed optimizations.
  endif()

  # Report the build type (Debug, Release, or None).
  message(STATUS "Generating ${CMAKE_BUILD_TYPE} build files.")
endif()

# Adds a subdirectory to the build. The source_dir specifies the directory in
# which the source CMakeLists.txt and code files are located.
add_subdirectory(enet)      # Compile CMakeLists.txt in enet subdirectory.

# Add the given directories to those the compiler uses to search for #include
# files. Relative paths are interpreted as relative to the current source
# directory.
include_directories(enet/include)   # -I flag for compiler.

# The first argument is the executable output name, and the remaining arguments
# are the source files required to build the executable.

# File manipulation command (READ, TIMESTAMP, WRITE, TOUCH, GLOB,
# MAKE_DIRECTORY, REMOVE, RENAME, COPY, SIZE, CHMOD, DOWNLOAD, UPLOAD, etc).
file(GLOB SRC_FILES *.cpp) # Collect the source files.

# Adds an executable target to be built from the listed source files.
add_executable(${PROJECT_NAME} ${SRC_FILES})
    # Specifies the paths in which the linker should search for libraries when
    # linking a given target.
    #
    # PRIVATE - use only for specified build target
    # PUBLIC - use for specified target and for targets which link with project
    # INTERFACE - use only for targets which link with project
    target_link_directories(${PROJECT_NAME} PRIVATE enet) # -L flag for linker.

    # Specify libraries or flags to use when linking a given target and/or its
    # dependents.
    target_link_libraries(${PROJECT_NAME} enet) # -l flag for linker.

# Install targets in /usr/local on UNIX and c:/Program Files/${PROJECT_NAME} on
# Windows. Change the default path using CMAKE_INSTALL_PREFIX.
install(TARGETS ${PROJECT_NAME})

# Copies an <input>.in file to an <output> file and substitutes variable values
# referenced as @VAR@. The @ONLY keywords means to only replace variables
# surrounded by @ signs.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  @ONLY)

# Add "make uninstall" support.
add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

