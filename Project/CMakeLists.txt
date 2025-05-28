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
    "vulkanbase/VulkanUtil.h"
    "Engine/ShaderBase.h" 
    "Engine/ShaderBase.cpp" 
    "Engine/CommandPool.h" 
    "Engine/PhysxBase.h" 
    "Engine/PhysxBase.cpp" 
    "Engine/Singleton.h" 
    "Engine/CommandPool.cpp"
    "Engine/DataBuffer.cpp"
    "Engine/DataBuffer.h"
    "Engine/DescriptorPool.h"
    "Engine/DescriptorPool.cpp"
    "Engine/Pipeline.h"
    "Engine/Pipeline.cpp"   
    "Engine/vulkanVars.h"
    "Engine/vulkanVars.cpp"
    "Engine/ParticleScene.h"
    "Engine/ParticleGroup.h"
    "Engine/ParticleGroup.cpp"
    "Engine/MeshScene.h"
    "Engine/InputManager.h"
    "Engine/InputManager.cpp"
    "Engine/RendererManager.h"
    "Engine/RendererManager.cpp"
    "Engine/WindowManager.h"
    "Engine/WindowManager.cpp"
 "Engine/CommandBuffer.cpp" "Engine/Mesh.h" "Engine/Mesh.cpp" "Engine/Scene.h"  "Engine/Objects/BaseObject.h" "Engine/Camera.h" "Engine/Camera.cpp" "Engine/SceneModelManager.h" "Engine/SceneModelManager.cpp" "Engine/GameObjects/Component.h" "Engine/GameObjects/GameObject.h"  "Engine/GameObjects/TransformComponent.h"  "Engine/GameObjects/ModelMeshComponent.h" "Engine/GameObjects/ModelMeshComponent.cpp" "Engine/GameObjects/PrimitiveMeshComponent.h" "Engine/GameObjects/PrimitiveMeshComponent.cpp" "Engine/GameSceneManager.h")
  
        
   
# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${GLSL_SOURCE_FILES}  "vulkanbase/VulkanUtil.cpp" "labwork/Week03.cpp")
add_dependencies(${PROJECT_NAME} Shaders)
# Link libraries
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include/physx")

target_link_libraries(${PROJECT_NAME} PRIVATE ${PhysX} ${PhysX_Common} ${PhysX_FOUNDATION} ${PhysX_COOKER} $<$<CONFIG:DEBUG>:${PhysX_EXTENSIONS}> $<$<CONFIG:DEBUG>:${PhysX_PVD}>)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glm)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/vcpkg/packages/physx_x64-windows/debug/bin"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()