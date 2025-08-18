#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <Engine/Graphics/Material.h>
#include <string>
#include <random>
struct GLFWwindow;
struct ImGuiContext;

class GameObject; // fwd
class BaseObject;



class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    void Initialize(VkInstance instance,
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t graphicsQueueFamily,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        uint32_t minImageCount,
        uint32_t imageCount,
        GLFWwindow* window,
        VkCommandPool uploadCmdPool);

    void BeginFrame();
    void EndFrame(VkCommandBuffer commandBuffer);

    // NEW: single call from renderer
    void DrawMainUI();

    void SetMaterialChoices(const std::vector<std::shared_ptr<Material>>& mats);
    void ClearMaterialChoices();

    // You still have the settings window (used internally)
    void DrawSettingsWindow(bool* pOpen = nullptr);

    // Swapchain/render pass changes
    void RecreateForNewRenderPass(VkRenderPass renderPass,
        uint32_t minImageCount,
        uint32_t imageCount);
    void Shutdown();

private:
    enum class SpawnType { Cube = 0, CatMesh = 1 };

    struct SpawnedItem {
        GameObject* object = nullptr;
        int         id = 0;
        SpawnType   type{};
    };

    void createDescriptorPool();
    void uploadFonts(VkCommandPool uploadPool);

    // -------- UI state --------
    bool m_ShowSettings = false;
    bool m_ShowSpawner = false;

    // Spawner sources its materials from here if non-empty:
    std::vector<std::shared_ptr<Material>> m_MaterialChoices;

    // Random engine for "Randomize" button
    std::mt19937 m_Rng{ std::random_device{}() };

    int   m_SpawnTypeIndex = 0; // 0: Cube, 1: Cat Mesh
    int   m_SelectedMaterialIndex = 0;

    float m_Pos[3] = { 0.f, 0.f, 0.f };
    float m_Rot[3] = { 0.f, 0.f, 0.f };
    float m_Scale[3] = { 1.f, 1.f, 1.f };
    float m_Size[3] = { 1.f, 1.f, 1.f }; // Cube W,H,D


    std::vector<SpawnedItem> m_Spawned;
    int   m_SelectedSpawned = -1;
    int   m_SpawnCounter = 0;

    // -------- cached handles / params --------
    VkInstance       m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice         m_Device = VK_NULL_HANDLE;
    uint32_t         m_GraphicsQueueFamily = 0;
    VkQueue          m_GraphicsQueue = VK_NULL_HANDLE;
    GLFWwindow* m_Window = nullptr;

    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkRenderPass     m_RenderPass = VK_NULL_HANDLE;
    uint32_t         m_MinImageCount = 2;
    uint32_t         m_ImageCount = 2;

    bool             m_Initialized = false;
    ImGuiContext* m_Context = nullptr;

private:
    // internal helpers
    void DrawMainWindow_();
    void DrawSpawnerWindow_(bool* pOpen);
};
