#include "ImGuiLayer.h"

#include <array>
#include <stdexcept>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "Engine/Core/Settings.h"
#include <Engine/Scene/GameSceneManager.h>
#include <Engine/Scene/SceneModelManager.h>

ImGuiLayer::~ImGuiLayer() {
    Shutdown();
}

void ImGuiLayer::Initialize(VkInstance instance,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    VkRenderPass renderPass,
    uint32_t minImageCount,
    uint32_t imageCount,
    GLFWwindow* window,
    VkCommandPool uploadCmdPool)
{
    if (m_Initialized) return;

    m_Instance = instance;
    m_PhysicalDevice = physicalDevice;
    m_Device = device;
    m_GraphicsQueueFamily = graphicsQueueFamily;
    m_GraphicsQueue = graphicsQueue;
    m_RenderPass = renderPass;
    m_MinImageCount = minImageCount;
    m_ImageCount = imageCount;
    m_Window = window;

    createDescriptorPool();

    IMGUI_CHECKVERSION();
    m_Context = ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Backend init
    ImGui_ImplGlfw_InitForVulkan(m_Window, true);

    ImGui_ImplVulkan_InitInfo init{};
    init.Instance = m_Instance;
    init.PhysicalDevice = m_PhysicalDevice;
    init.Device = m_Device;
    init.QueueFamily = m_GraphicsQueueFamily;
    init.Queue = m_GraphicsQueue;
    init.DescriptorPool = m_DescriptorPool;
    init.MinImageCount = m_MinImageCount;
    init.ImageCount = m_ImageCount;
    init.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init.UseDynamicRendering = false;

#if IMGUI_VERSION_NUM >= 19000
    init.RenderPass = m_RenderPass;                // new API: pass via InitInfo
    if (!ImGui_ImplVulkan_Init(&init))
        throw std::runtime_error("ImGui_ImplVulkan_Init failed");
#else
    // old API: render pass is a separate parameter
    if (!ImGui_ImplVulkan_Init(&init, m_RenderPass))
        throw std::runtime_error("ImGui_ImplVulkan_Init failed");
#endif

    uploadFonts(uploadCmdPool);

    m_Initialized = true;
}

void ImGuiLayer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 11> poolSizes{ {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    } };

    VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.maxSets = 1000 * (uint32_t)poolSizes.size();
    info.poolSizeCount = (uint32_t)poolSizes.size();
    info.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(m_Device, &info, nullptr, &m_DescriptorPool) != VK_SUCCESS)
        throw std::runtime_error("ImGui descriptor pool creation failed");
}

void ImGuiLayer::uploadFonts(VkCommandPool uploadPool)
{
#if IMGUI_VERSION_NUM >= 19000
    // Newer backends: fonts are uploaded automatically on first NewFrame().
    // Optional: force upload now (no arguments).
    ImGui_ImplVulkan_CreateFontsTexture();
    // A short wait makes sure the upload completes before first frame.
    vkQueueWaitIdle(m_GraphicsQueue);
    (void)uploadPool; // unused in new path
#else
    // Older backends: record upload into a one-time command buffer.
    VkCommandBufferAllocateInfo alloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc.commandPool = uploadPool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cmd{};
    vkAllocateCommandBuffers(m_Device, &alloc, &cmd);

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;

    vkQueueSubmit(m_GraphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    ImGui_ImplVulkan_DestroyFontUploadObjects(); // exists only in old backend
    vkFreeCommandBuffers(m_Device, uploadPool, 1, &cmd);
#endif
}

void ImGuiLayer::BeginFrame() {
    if (!m_Initialized) return;
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::DrawSettingsWindow(bool* pOpen)
{
    auto& S = Settings::GetInstance();
    if (!ImGui::Begin("Engine Settings", pOpen)) { ImGui::End(); return; }

    // --- Renderer group -------------------------------------------------------
    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool showNormals = S.Get<bool>("renderer.showNormals", false);
        bool chunkDebug = S.Get<bool>("renderer.chunkDebug", true);
        float renderDistance = S.Get<float>("renderer.renderDistance", 200.f);
        float chunkRange = S.Get<float>("renderer.chunkRange", 100.f);

        if (ImGui::Checkbox("Show Normals Pass", &showNormals))
            S.Set("renderer.showNormals", showNormals);

        if (ImGui::Checkbox("Chunk Debug", &chunkDebug))
            S.Set("renderer.chunkDebug", chunkDebug);

        if (ImGui::SliderFloat("Render Distance", &renderDistance, 25.f, 2000.f))
            S.Set("renderer.renderDistance", renderDistance);

        if (ImGui::SliderFloat("Chunk Range", &chunkRange, 10.f, 1000.f))
            S.Set("renderer.chunkRange", chunkRange);
    }

    // --- Camera group ---------------------------------------------------------
    if (ImGui::CollapsingHeader("Camera")) {
        float fov = S.Get<float>("camera.fov", 90.f);
        if (ImGui::SliderFloat("FOV", &fov, 30.f, 120.f))
            S.Set("camera.fov", fov);

        float nearP = S.Get<float>("camera.near", 0.1f);
        float farP = S.Get<float>("camera.far", 1000.f);
        if (ImGui::DragFloat("Near", &nearP, 0.01f, 0.01f, 10.f))  S.Set("camera.near", nearP);
        if (ImGui::DragFloat("Far", &farP, 1.f, 10.f, 10000.f)) S.Set("camera.far", farP);
    }

    // --- General --------------------------------------------------------------
    if (ImGui::CollapsingHeader("General")) {
        bool capFps = S.Get<bool>("general.capFps", false);
        int  fpsCap = S.Get<int>("general.fpsCap", 60);
        if (ImGui::Checkbox("Cap FPS", &capFps)) S.Set("general.capFps", capFps);
        if (ImGui::SliderInt("FPS Cap", &fpsCap, 30, 240)) S.Set("general.fpsCap", fpsCap);

        if (ImGui::Button("Save Now")) S.Save();
        ImGui::SameLine();
        if (ImGui::Button("Dump to Console")) {
            // dump nice-formatted json to stdout (or your logger)
            fprintf(stdout, "%s\n", S.Data().dump(2).c_str());
        }
    }

    ImGui::End();
}

void ImGuiLayer::EndFrame(VkCommandBuffer commandBuffer) {
    if (!m_Initialized) return;
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiLayer::RecreateForNewRenderPass(VkRenderPass renderPass,
    uint32_t minImageCount,
    uint32_t imageCount)
{
    if (!m_Initialized) return;
    // The simplest, safest path: reinit the Vulkan backend with the new pass.
    ImGui_ImplVulkan_Shutdown();

    m_RenderPass = renderPass;
    m_MinImageCount = minImageCount;
    m_ImageCount = imageCount;

    ImGui_ImplVulkan_InitInfo init{};
    init.Instance = m_Instance;
    init.PhysicalDevice = m_PhysicalDevice;
    init.Device = m_Device;
    init.QueueFamily = m_GraphicsQueueFamily;
    init.Queue = m_GraphicsQueue;
    init.DescriptorPool = m_DescriptorPool;
    init.MinImageCount = m_MinImageCount;
    init.ImageCount = m_ImageCount;
    init.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init.UseDynamicRendering = false;
    init.RenderPass = m_RenderPass;

    if (!ImGui_ImplVulkan_Init(&init))
        throw std::runtime_error("ImGui_ImplVulkan_Init (recreate) failed");
}

void ImGuiLayer::Shutdown() {
    if (!m_Initialized) return;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    if (m_Context) { ImGui::DestroyContext(m_Context); m_Context = nullptr; }
    if (m_DescriptorPool) { vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr); m_DescriptorPool = VK_NULL_HANDLE; }
    m_Initialized = false;
}

void ImGuiLayer::DrawMainUI() {
    // 1) Main window with two buttons
    DrawMainWindow_();

    // 2) Settings window (toggled by main window)
    if (m_ShowSettings)
        DrawSettingsWindow(&m_ShowSettings);

    // 3) Spawner window (toggled by main window)
    if (m_ShowSpawner)
        DrawSpawnerWindow_(&m_ShowSpawner);
}

void ImGuiLayer::SetMaterialChoices(const std::vector<std::shared_ptr<Material>>& mats) {
    m_MaterialChoices = mats;
    if (m_SelectedMaterialIndex >= (int)m_MaterialChoices.size())
        m_SelectedMaterialIndex = (m_MaterialChoices.empty() ? 0 : (int)m_MaterialChoices.size() - 1);
}
void ImGuiLayer::ClearMaterialChoices() {
    m_MaterialChoices.clear();
    m_SelectedMaterialIndex = 0;
}

void ImGuiLayer::DrawMainWindow_() {
    // Dockable small main window
    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::Button(m_ShowSettings ? "Hide Settings" : "Show Settings"))
        m_ShowSettings = !m_ShowSettings;

    if (ImGui::Button(m_ShowSpawner ? "Hide Spawner" : "Show Spawner"))
        m_ShowSpawner = !m_ShowSpawner;

    ImGui::End();
}

// ------------- Spawner -----------------

static const char* kTypeNames[] = { "Cube", "Cat Mesh" };

void ImGuiLayer::DrawSpawnerWindow_(bool* pOpen) {
    using ST = SpawnType;
    if (!ImGui::Begin("Spawner", pOpen)) { ImGui::End(); return; }

    // --- Type -------------------------------------------------------
    static const char* kTypeNames[] = { "Cube", "Cat Mesh" };
    ImGui::TextUnformatted("Object Type");
    ImGui::Combo("##type", &m_SpawnTypeIndex, kTypeNames, IM_ARRAYSIZE(kTypeNames));

    // --- Materials (choices -> fallback to MaterialManager) ---------
    const std::vector<std::shared_ptr<Material>>* matsPtr = nullptr;
    if (!m_MaterialChoices.empty()) matsPtr = &m_MaterialChoices;
    else                            matsPtr = &MaterialManager::GetInstance().getActiveMaterials();

    const auto& mats = *matsPtr;
    if (mats.empty()) {
        ImGui::TextDisabled("No materials available. Use SetMaterialChoices() or fill MaterialManager.");
    }
    else {
        if (m_SelectedMaterialIndex < 0 || m_SelectedMaterialIndex >= (int)mats.size())
            m_SelectedMaterialIndex = 0;
        auto itemsGetter = [](void* data, int idx, const char** out)->bool {
            const auto* v = static_cast<const std::vector<std::shared_ptr<Material>>*>(data);
            if (idx < 0 || idx >= (int)v->size()) return false;
            static thread_local char buf[64];
            snprintf(buf, sizeof(buf), "Material #%d", idx);
            *out = buf;
            return true;
            };
        ImGui::TextUnformatted("Material");
        ImGui::Combo("##mat", &m_SelectedMaterialIndex, itemsGetter, (void*)&mats, (int)mats.size());
    }

    // --- Transform --------------------------------------------------
    ImGui::SeparatorText("Transform");
    ImGui::DragFloat3("Position", m_Pos, 0.1f);
    ImGui::DragFloat3("Rotation", m_Rot, 0.5f);   // degrees
    ImGui::DragFloat3("Scale", m_Scale, 0.01f, 0.001f, 1000.f);

    // --- Size (cube only) ------------------------------------------
    if (m_SpawnTypeIndex == (int)ST::Cube) {
        ImGui::DragFloat3("Cube Size (W,H,D)", m_Size, 0.05f, 0.01f, 1000.f);
    }

    // --- Randomize (pos/rot/scale/size) -----------------------------
    if (ImGui::Button("Randomize")) {
        std::uniform_real_distribution<float> pos(-10.f, 10.f);       // X/Y/Z in [-10, 10]
        std::uniform_real_distribution<float> rot(0.f, 360.f);        // degrees
        std::uniform_real_distribution<float> scl(0.1f, 2.0f);        // uniform scale range
        std::uniform_real_distribution<float> size(0.2f, 3.0f);       // cube w/h/d

        m_Pos[0] = pos(m_Rng); m_Pos[1] = pos(m_Rng); m_Pos[2] = pos(m_Rng);
        m_Rot[0] = rot(m_Rng); m_Rot[1] = rot(m_Rng); m_Rot[2] = rot(m_Rng);

        float u = scl(m_Rng);
        m_Scale[0] = m_Scale[1] = m_Scale[2] = u;

        if (m_SpawnTypeIndex == (int)ST::Cube) {
            m_Size[0] = size(m_Rng); m_Size[1] = size(m_Rng); m_Size[2] = size(m_Rng);
        }
    }

    // --- Spawn ------------------------------------------------------
    ImGui::Separator();
    if (ImGui::Button("Spawn")) {
        auto& GS = GameSceneManager::getInstance();
        GameObject* go = GS.addGameObject();
        if (go) {
            go->getTransform()->position = { m_Pos[0], m_Pos[1], m_Pos[2] };
            go->getTransform()->rotation = { m_Rot[0], m_Rot[1], m_Rot[2] };
            go->getTransform()->scale = { m_Scale[0], m_Scale[1], m_Scale[2] };

            std::shared_ptr<Material> chosenMat;
            if (!mats.empty()) chosenMat = mats[(size_t)m_SelectedMaterialIndex];
            else               chosenMat = MaterialManager::GetInstance().getStandardMaterial();

            if (m_SpawnTypeIndex == (int)ST::Cube) {
                float w = m_Size[0], h = m_Size[1], d = m_Size[2];
                auto* comp = go->addComponent<PrimitiveMeshComponent>(go, PrimitiveType::Cube, w, h, d, chosenMat);

                SpawnedItem si{};
                si.object = go;
                si.id = ++m_SpawnCounter;
                si.type = ST::Cube;
                m_Spawned.push_back(si);
            }
            else {
                auto* comp = go->addComponent<ModelMeshComponent>(go, "Resources/Models/cat.obj", chosenMat, false);

                SpawnedItem si{};
                si.object = go;
                si.id = ++m_SpawnCounter;
                si.type = ST::CatMesh;
                m_Spawned.push_back(si);
            }

            // IMPORTANT: initialize/register newly spawned BaseObjects right away
            SceneModelManager::getInstance().flushRuntimeAdds();

            m_SelectedSpawned = (int)m_Spawned.size() - 1;
        }
    }

    // --- Edit spawned objects --------------------------------------
    ImGui::SeparatorText("Spawned Objects");
    if (m_Spawned.empty()) {
        ImGui::TextDisabled("None yet.");
    }
    else {
        ImGui::BeginChild("spawned_list", ImVec2(160, 180), true);
        for (int i = 0; i < (int)m_Spawned.size(); ++i) {
            const auto& it = m_Spawned[i];
            char label[64];
            snprintf(label, sizeof(label), "%s %d",
                (it.type == ST::Cube ? "Cube" : "Cat"), it.id);
            if (ImGui::Selectable(label, m_SelectedSpawned == i))
                m_SelectedSpawned = i;
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("spawned_edit", ImVec2(0, 180), true);
        if (m_SelectedSpawned >= 0 && m_SelectedSpawned < (int)m_Spawned.size()) {
            auto& it = m_Spawned[m_SelectedSpawned];
            auto* go = m_Spawned[m_SelectedSpawned].object;
            if (go) {
                auto* tr = go->getTransform();
                float p[3] = { tr->position.x, tr->position.y, tr->position.z };
                float r[3] = { tr->rotation.x, tr->rotation.y, tr->rotation.z };
                float s[3] = { tr->scale.x, tr->scale.y, tr->scale.z };

                bool changed = false;
                if (ImGui::DragFloat3("Pos", p, 0.1f)) { tr->position = { p[0], p[1], p[2] }; changed = true; }
                if (ImGui::DragFloat3("Rot", r, 0.5f)) { tr->rotation = { r[0], r[1], r[2] }; changed = true; }
                if (ImGui::DragFloat3("Scale", s, 0.01f, 0.001f, 1000.f)) { tr->scale = { s[0], s[1], s[2] }; changed = true; }

                // No need to poke BaseObject directly—broadcast and let components handle it.
                if (changed) tr->applyNow();

                if (ImGui::Button("Remove from list")) {
                    m_Spawned.erase(m_Spawned.begin() + m_SelectedSpawned);
                    if (m_SelectedSpawned >= (int)m_Spawned.size()) m_SelectedSpawned = (int)m_Spawned.size() - 1;
                }
            }
            else {
                ImGui::TextDisabled("Object destroyed.");
            }
        }
        else {
            ImGui::TextDisabled("Select an object.");
        }
        ImGui::EndChild();
    }

    ImGui::End();
}
