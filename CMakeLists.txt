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

# MSVC (Windows) ------------------------
if(MSVC)
  set(MSVC_OPT_FLAGS
    /O2        # optimize for speed
    /Oi        # use intrinsics
    /GL        # whole program optimization
    /Gy        # function-level linking
    /GF        # string pooling
    /Oy        # omit frame pointer
    /EHsc      # C++ exceptions only
    /GR-       # disable RTTI
  )

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
    list(APPEND MSVC_OPT_FLAGS /arch:AVX2)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64")
    list(APPEND MSVC_OPT_FLAGS /arch:ARM64)
  endif()

  set(MSVC_LINK_FLAGS
    /LTCG       # link-time code generation
    /OPT:REF    # eliminate unused code
    /OPT:ICF    # fold identical functions
  )

  foreach(flag IN LISTS MSVC_OPT_FLAGS)
    target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<NOT:$<CONFIG:Debug>>:${flag}>
    )
  endforeach()

  foreach(flag IN LISTS MSVC_LINK_FLAGS)
    target_link_options(${PROJECT_NAME} PRIVATE
      $<$<NOT:$<CONFIG:Debug>>:${flag}>
    )
  endforeach()

  # Warnings
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
  
endif()

# Clang ------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(GCC_CLANG_OPT_FLAGS
    -O3                  # optimize for speed
    -ffast-math          # aggressive FP optimization
    -march=native        # use all local CPU instructions
    -flto                # link-time optimization
    -fstrict-aliasing    # assume strict aliasing rules
    -fno-rtti            # disable RTTI (if not used)
    -fno-exceptions      # disable exceptions (if not used)
    -fvisibility=hidden  # hidden by default
    -fdata-sections      # allow section GC
    -ffunction-sections  # allow section GC
  )

  set(GCC_CLANG_LINK_FLAGS
    -flto
    -Wl
  )

  foreach(flag IN LISTS GCC_CLANG_OPT_FLAGS)
    target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<NOT:$<CONFIG:Debug>>:${flag}>
    )
  endforeach()

  foreach(flag IN LISTS GCC_CLANG_LINK_FLAGS)
    target_link_options(${PROJECT_NAME} PRIVATE
      $<$<NOT:$<CONFIG:Debug>>:${flag}>
    )
  endforeach()

  # Warnings
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