cmake_minimum_required(VERSION 3.10)

project(VulkanEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1 
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# --- fast_obj (MIT) ---------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    fast_obj
    GIT_REPOSITORY https://github.com/thisistherk/fast_obj.git
    GIT_TAG        v1.3         # or a commit hash
)

# Some repos already provide a CMake target; to be robust, just compile the .c ourselves:
FetchContent_GetProperties(fast_obj)
if(NOT fast_obj_POPULATED)
  FetchContent_Populate(fast_obj)
  add_library(fast_obj STATIC
      ${fast_obj_SOURCE_DIR}/fast_obj.c
  )
  target_include_directories(fast_obj PUBLIC ${fast_obj_SOURCE_DIR})
endif()


find_package(Vulkan REQUIRED)

include_directories(${Vulkan_INCLUDE_DIRS})

add_subdirectory(Project)

function(install_vcpkg)
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/vcpkg")
    message("Installing vcpkg...")
    execute_process(COMMAND git clone https://github.com/microsoft/vcpkg.git "${CMAKE_SOURCE_DIR}/vcpkg")
    if(WIN32)
      execute_process(COMMAND "${CMAKE_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat"
                      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/vcpkg")
    else()
      execute_process(COMMAND "${CMAKE_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.sh"
                      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/vcpkg")
    endif()
  endif()

  execute_process(COMMAND "${CMAKE_SOURCE_DIR}/vcpkg/vcpkg" list
                  RESULT_VARIABLE result
                  OUTPUT_VARIABLE vcpkg_output)
  string(FIND "${vcpkg_output}" "physx" physx_found)
  if("${physx_found}" EQUAL "-1")
    message("PhysX not found, installing via vcpkg...")
    execute_process(COMMAND "${CMAKE_SOURCE_DIR}/vcpkg/vcpkg" install physx
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/vcpkg")
  endif()

  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
endfunction()

install_vcpkg()

find_library(PhysX NAMES PhysX_64.lib HINTS "./vcpkg/packages/x64-windows/lib")
find_library(PhysX_Common NAMES PhysXCommon_64.lib HINTS "./vcpkg/packages/x64-windows/lib")
find_library(PhysX_FOUNDATION NAMES PhysXFoundation_64.lib HINTS "./vcpkg/packages/x64-windows/lib")
find_library(PhysX_EXTENSIONS NAMES PhysXExtensions_static_64.lib HINTS "./vcpkg/packages/x64-windows/lib")
find_library(PhysX_COOKER NAMES PhysXCooking_64.lib HINTS "./vcpkg/packages/x64-windows/lib")
find_library(PhysX_PVD NAMES PhysXPvdSDK_Static_64.lib HINTS "./vcpkg/packages/x64-windows/lib")

# Check if PhysX library is found
if(NOT PhysX)
    message(FATAL_ERROR "PhysX library not found")
endif()

