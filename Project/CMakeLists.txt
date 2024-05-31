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


set(SOURCES
    "main.cpp"
    "vulkanbase/VulkanBase.h" 
    "vulkanbase/VulkanUtil.h"
    "Engine/ShaderBase.h" 
    "Engine/ShaderBase.cpp" 
    "Engine/CommandPool.h" 
    "Engine/CommandPool.cpp"
    "Engine/DataBuffer.cpp"
    "Engine/DataBuffer.h"
    "Engine/DescriptorPool.h"
    "Engine/DescriptorPool.cpp"
    "Engine/Pipeline.h"
    "Engine/Pipeline.cpp"   

    # Add other source files here
    "labwork/Week01.cpp"
    "labwork/Week02.cpp"      
    "labwork/Week03.cpp"   
    "labwork/Week04.cpp" 
    "labwork/Week05.cpp" 
    "labwork/Week06.cpp"
 "Engine/CommandBuffer.cpp" "Engine/Mesh.h" "Engine/Mesh.cpp" "Engine/Scene.h"  "Engine/Objects/BaseObject.h" "Engine/Camera.h" "Engine/Camera.cpp")
  
  
   
# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${GLSL_SOURCE_FILES}  "vulkanbase/VulkanUtil.cpp" "labwork/Week03.cpp")
add_dependencies(${PROJECT_NAME} Shaders)
# Link libraries
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glm)