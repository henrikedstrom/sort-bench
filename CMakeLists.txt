cmake_minimum_required(VERSION 3.16)
project(sort-bench VERSION 1.0.0 LANGUAGES CXX)

# ------------------------------------------------------------------------------
# Project Configuration
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to enable hardware prefetch optimization
option(ENABLE_PREFETCH "Enable hardware prefetch optimization" OFF)
if(ENABLE_PREFETCH)
    add_compile_definitions(PREFETCH=1)
else()
    add_compile_definitions(PREFETCH=0)
endif()


# ------------------------------------------------------------------------------
# Source and Header Files
# ------------------------------------------------------------------------------
set(SOURCE_FILES
  src/main.cpp
  src/radix.cpp
)

set(HEADER_FILES
  src/radix.h
)


# ------------------------------------------------------------------------------
# Executable Target Setup
# ------------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

# Organize files for IDEs.
source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Header Files" FILES ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)


# ------------------------------------------------------------------------------
# Compiler flags and warnings
# ------------------------------------------------------------------------------
if(MSVC)

  # Common compiler optimization flags
  set(MSVC_OPT_FLAGS
    /O2        # optimize for maximum speed
    /Oi        # replace CRT calls with intrinsics
    /GL        # enable whole-program optimization
    /Gy        # enable function-level linking
    /GF        # enable read-only string pooling
    /Oy        # omit frame pointers for extra register
    /EHsc      # enable C++ exception handling (no SEH)
    /GR-       # disable RTTI
  )

  # Append platform-specific flags
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    list(APPEND MSVC_OPT_FLAGS
      /arch:AVX2   # target AVX2 + FMA on x64 platforms
    )
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64")
    list(APPEND MSVC_OPT_FLAGS
      /arch:ARM64 # use ARM64 (NEON) intrinsics on ARM64
    )
  endif()

  # Linker flags
  set(MSVC_LINK_FLAGS
    /LTCG      # enable link-time code generation
    /OPT:REF   # remove unreferenced functions/data
    /OPT:ICF   # fold identical COMDAT sections
  )


  # Attach optimization flags for the non-debug configurations
  foreach(flag IN LISTS MSVC_OPT_FLAGS)
    target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<NOT:$<CONFIG:Debug>>:${flag}>
    )
  endforeach()

  # Likewise for link-time flags
  foreach(lflag IN LISTS MSVC_LINK_FLAGS)
    target_link_options(${PROJECT_NAME} PRIVATE
      $<$<NOT:$<CONFIG:Debug>>:${lflag}>
    )
  endforeach()

  # Warnings
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()


# ------------------------------------------------------------------------------
# IDE Specific Settings
# ------------------------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES 
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

if(WIN32)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()