#include <Engine/Scene/MeshScene.h>
#include <Engine/Graphics/vulkanVars.h>
#include <iostream>
#include <Engine/ObjUtils/DebugPrint.h>


// MeshScene.cpp
DataBuffer* MeshScene::getOrGrowInstanceBuffer(const MeshKey& key, size_t neededCount) {
    auto& vk = vulkanVars::GetInstance();
    size_t frame = vk.currentFrame % MAX_FRAMES_IN_FLIGHT;
    auto& slot = m_instanceBuffers[key][frame];

    VkDeviceSize needBytes = sizeof(InstanceData) * neededCount;
    if (!slot || slot->getSizeInBytes() < needBytes) {
        // (Re)create host-visible VB for instances
        slot = std::make_unique<DataBuffer>(
            vk.physicalDevice, vk.device,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // host-updated, no staging
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            needBytes
        );
    }
    return slot.get();
}

void MeshScene::flushPendingRuntime() {
    if (m_pendingToRegister.empty()) return;

    auto& vk = vulkanVars::GetInstance();
    // Move pending into a local list so we can clear and avoid re-processing
    std::vector<BaseObject*> pending;
    pending.swap(m_pendingToRegister);

    for (BaseObject* o : pending) {
        if (!o) continue;

        // Initialize GPU buffers if needed
        if (!o->isInitialized()) {
            o->init(vk.physicalDevice,
                vk.device,
                vk.commandPoolModelPipeline.m_CommandPool,
                vk.graphicsQueue);
        }

        // Register into chunk grid (pipelineIndex = 0 for your 3D pipeline)
        if (o->isInitialized()) {
            m_chunks.add(o, MakeMeshKey(o, 0));
        }
    }
}

void MeshScene::rebuildAllInstanceBuffersFromCurrentTransforms()
{
    m_chunks.forVisibleBatches([&](const MeshKey& key, const std::vector<BaseObject*>& batch) {
        getOrGrowInstanceBuffer(key, batch.size());
        });
}

void MeshScene::drawScene(VkPipelineLayout& pipelineLayout, VkCommandBuffer& cmd) {
    auto& vk = vulkanVars::GetInstance();


    if (m_instancesDirty) {
        rebuildAllInstanceBuffersFromCurrentTransforms(); // fill per-key InstanceData.model = bo->getModelMatrix()
        m_instancesDirty = false;
    }

    // Draw one instanced batch (existing logic pulled into a lambda)
    auto drawBatch = [&](const MeshKey& key, const std::vector<BaseObject*>& batch) {
        if (batch.empty()) return;

        // 1) Pack instance data
        std::vector<InstanceData> instances;
        instances.reserve(batch.size());
        for (BaseObject* o : batch) {
            if (!o) continue;
            InstanceData id{};
            id.model = o->getModelMatrix();
            auto& mat = o->getMaterial();
            id.texIds0 = glm::uvec4(
                mat ? mat->getAlbedoMapID() : 0u,
                mat ? mat->getNormalMapID() : 0u,
                mat ? mat->getMetalnessMapID() : 0u,
                mat ? mat->getRoughnessMapID() : 0u
            );
            id.heightId = mat ? mat->getHeightMapID() : 0u;
            instances.push_back(id);
        }
        if (instances.empty()) return;

        // 2) Upload per-instance buffer (per key, per frame)
        DataBuffer* instBuf = getOrGrowInstanceBuffer(key, instances.size());
        instBuf->upload(sizeof(InstanceData) * instances.size(), instances.data());

        // 3) Bind mesh VB/IB + instance VB, then draw
        VkBuffer bufs[2] = { key.vertexBuffer, instBuf->getVkBuffer() };
        VkDeviceSize offs[2] = { key.vbOffset, 0 };

        if (key.indexBuffer == VK_NULL_HANDLE || key.indexCount == 0) {
            // In debug builds you can log once to find the offender.
            fprintf(stderr, "Skipped batch with missing index buffer.\n");
            return;
        }

        vkCmdBindVertexBuffers(cmd, 0, 2, bufs, offs);
        vkCmdBindIndexBuffer(cmd, key.indexBuffer, key.ibOffset, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, key.indexCount,
            static_cast<uint32_t>(instances.size()),
            /*firstIndex*/0, /*vertexOffset*/0, /*firstInstance*/0);
        };

    if (m_chunksEnabled) {
        // 1) Collect once, draw once per MeshKey
        std::unordered_map<MeshKey, std::vector<BaseObject*>, MeshKeyHash> visible;
        m_chunks.forVisibleBatches([&](const MeshKey& key, const std::vector<BaseObject*>& batch) {
            auto& dst = visible[key];
            dst.insert(dst.end(), batch.begin(), batch.end()); // already dedup'ed by forVisibleBatches()
            });

        for (auto& kv : visible)
            drawBatch(kv.first, kv.second);
    }
    else {
        // Debug path: render EVERYTHING (no culling), grouped by MeshKey
        std::unordered_map<MeshKey, std::vector<BaseObject*>, MeshKeyHash> all;
        all.reserve(m_BaseObjects.size());

        for (BaseObject* o : m_BaseObjects) {
            if (!o || !o->isInitialized()) continue;
            all[MakeMeshKey(o, /*pipelineIndex*/0)].push_back(o);
        }

        for (auto& kv : all) {
            drawBatch(kv.first, kv.second);
        }
    }
}

static std::string keyStr(const MeshKey& k) {
    std::ostringstream ss;
    ss << "{VB=" << ptrStr((void*)k.vertexBuffer)
        << ", IB=" << ptrStr((void*)k.indexBuffer)
        << ", idx=" << k.indexCount
        << ", pipe=" << k.pipelineIndex
        << ", group=" << k.logicalId
        << "}";
    return ss.str();
}

void MeshScene::debugPrintVisibleBatches(std::ostream& os) {
    size_t drawCalls = 0, totalInstances = 0;
    std::unordered_map<size_t, size_t> histogram; // instances -> how many batches

    os << "=== Visible batches (for current camera) ===\n";
    m_chunks.forVisibleBatches([&](const MeshKey& key, const std::vector<BaseObject*>& batch) {
        os << "Key " << keyStr(key) << " -> instances=" << batch.size() << "\n";
        drawCalls++;
        totalInstances += batch.size();
        histogram[batch.size()]++;
        });

    os << "Draw calls (instanced) = " << drawCalls
        << ", total instances = " << totalInstances << "\n";
    os << "Instance-count histogram:\n";
    for (auto& kv : histogram)
        os << "  " << kv.first << " instance(s): " << kv.second << " batch(es)\n";
}

