# Source files
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

# Copy Resources folder to build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")


file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.vert" 
) 

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)


file(GLOB_RECURSE PROJECT_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
        
   
# Create the executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_FILES} ${GLSL_SOURCE_FILES}   )
add_dependencies(${PROJECT_NAME} Shaders)
# Link libraries
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include/physx")
target_include_directories(${PROJECT_NAME} PUBLIC ${stb_SOURCE_DIR})
# Link to GLFW + Vulkan; the backend needs both

target_link_libraries(${PROJECT_NAME} PRIVATE ${PhysX} ${PhysX_Common} ${PhysX_FOUNDATION} ${PhysX_COOKER} ${PhysX_EXTENSIONS} ${PhysX_PVD})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glm)
target_link_libraries(${PROJECT_NAME} PRIVATE fast_obj)
target_link_libraries(${PROJECT_NAME} PRIVATE ImGui nlohmann_json::nlohmann_json)
# This assumes vcpkg is in ${CMAKE_SOURCE_DIR}/vcpkg
set(PHYSX_BIN_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
set(PHYSX_DEBUG_BIN_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/debug/bin")


# For Debug
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/debug/bin/PhysX_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/debug/bin/PhysXCommon_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/debug/bin/PhysXFoundation_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/debug/bin/PhysXCooking_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/debug/bin/PhysXGpu_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/debug/bin/PhysXDevice64.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# For Release
if(CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/PhysX_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/PhysXCommon_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/PhysXFoundation_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/PhysXCooking_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/PhysXGpu_64.dll"
            "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/PhysXDevice64.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()