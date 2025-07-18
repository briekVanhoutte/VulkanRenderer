#pragma once

#include <vector>
#include <PxPhysicsAPI.h>
#include <extensions/PxParticleExt.h>
#include <Engine/Graphics/Particle.h>
#include <Engine/Core/Singleton.h>
#include <Engine/Graphics/DataBuffer.h>


using namespace physx;
using namespace ExtGpu;

// Forward declarations
class PxCudaContextManager;

class PhysxBase : public Singleton<PhysxBase>
{
public:
    PhysxBase();
    ~PhysxBase();

    void initPhysics(bool useLargeFluid);
    void stepPhysics(bool interactive, float deltaTime);
    void cleanupPhysics(bool interactive);
    float getRightWallLocation();
    std::vector<Particle>& getParticles();

    PxPBDParticleSystem* getParticleSystem();
    PxParticleAndDiffuseBuffer* getParticleBuffer();

    // Wall movement controls
    bool wallMoveLeft = false;
    bool wallMoveRight = false;

private:
    void initObstacles();
    void initScene();
    void initParticles(PxU32 numX, PxU32 numY, PxU32 numZ, const PxVec3& position, PxReal particleSpacing, PxReal fluidDensity, PxU32 maxDiffuseParticles);
    void getParticlesInternal();

    // PhysX SDK objects
    physx::PxFoundation* mFoundation = nullptr;
    physx::PxPhysics* mPhysics = nullptr;
    physx::PxDefaultCpuDispatcher* mDispatcher = nullptr;
    physx::PxScene* mScene = nullptr;
    physx::PxMaterial* mMaterial = nullptr;
    physx::PxPvd* mPvd = nullptr;
    physx::PxCudaContextManager* mCudaContextManager = nullptr; // <-- FULLY QUALIFIED
    physx::PxPBDParticleSystem* mParticleSystem = nullptr;
    physx::PxParticleAndDiffuseBuffer* mParticleBuffer = nullptr;
    physx::PxRigidDynamic* mMovingWall = nullptr;

    int mMaxDiffuseParticles = 0;
    bool mIsRunning = true;
    bool mStep = true;

    std::vector<Particle> m_Particles;
};
