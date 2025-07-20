#include "PhysxBase.h"

#include <iostream>
#include <PxPhysicsAPI.h>
#include <extensions/PxParticleExt.h>
#include "cudamanager/PxCudaContext.h"
#include "cudamanager/PxCudaContextManager.h"

#define PVD_HOST "127.0.0.1"

using namespace physx;
using namespace ExtGpu;

namespace {
    PxDefaultAllocator gAllocator;
    PxDefaultErrorCallback gErrorCallback;
}

// --------------- Constructor/Destructor ----------------

PhysxBase::PhysxBase() {}

PhysxBase::~PhysxBase() {
    cleanupPhysics(false);
}

// --------------- Public API ----------------

void PhysxBase::initPhysics(bool useLargeFluid)
{
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

    mPvd = PxCreatePvd(*mFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    mPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, mPvd);

    initScene();

    PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
    if (pvdClient) {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    const PxReal fluidDensity = 1000.0f;
    const PxU32 maxDiffuseParticles = useLargeFluid ? 20000 : 1;
    initParticles(1, 1 * (useLargeFluid ? 5 : 1), 1, PxVec3(-2.5f, 3.f, 0.5f), 0.1f, fluidDensity, maxDiffuseParticles);

    // Add boundary planes and walls
    mScene->addActor(*PxCreatePlane(*mPhysics, PxPlane(0.f, 1.f, 0.f, 0.0f), *mMaterial));
    mScene->addActor(*PxCreatePlane(*mPhysics, PxPlane(-1.f, 0.f, 0.f, 3.f), *mMaterial));
    mScene->addActor(*PxCreatePlane(*mPhysics, PxPlane(0.f, 0.f, 1.f, 3.f), *mMaterial));
    mScene->addActor(*PxCreatePlane(*mPhysics, PxPlane(0.f, 0.f, -1.f, 3.f), *mMaterial));

    bool useMovingWall = true;
    if (!useMovingWall) {
        mScene->addActor(*PxCreatePlane(*mPhysics, PxPlane(1.f, 0.f, 0.f, 3.f), *mMaterial));
        mMovingWall = nullptr;
    }
    else {
        PxTransform trans = PxTransformFromPlaneEquation(PxPlane(1.f, 0.f, 0.f, 5.f));
        mMovingWall = mPhysics->createRigidDynamic(trans);
        mMovingWall->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        PxRigidActorExt::createExclusiveShape(*mMovingWall, PxPlaneGeometry(), *mMaterial);
        mScene->addActor(*mMovingWall);
    }

    // Add boxes
    const PxReal dynamicsDensity = fluidDensity * 0.5f;
    const PxReal boxSize = 1.0f;
    const PxReal boxMass = boxSize * boxSize * boxSize * dynamicsDensity;
    PxShape* shape = mPhysics->createShape(PxBoxGeometry(0.5f * boxSize, 0.5f * boxSize, 0.5f * boxSize), *mMaterial);
    for (int i = 0; i < 5; ++i) {
        PxRigidDynamic* body = mPhysics->createRigidDynamic(PxTransform(PxVec3(i - 3.0f, 10, 7.5f)));
        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, boxMass);
        mScene->addActor(*body);
    }
    shape->release();

    m_Particles = {};
    Particle par = {};
    m_Particles.push_back(par);
}

void PhysxBase::stepPhysics(bool interactive, float deltaTime)
{
    if (mIsRunning || mStep) {
        mStep = false;

        if (mMovingWall) {
            PxReal speed = 3.f;
            PxTransform pose = mMovingWall->getGlobalPose();
            if (wallMoveRight && pose.p.x - deltaTime * speed > -9.f) {
                pose.p.x -= deltaTime * speed;
            }
            if (wallMoveLeft && pose.p.x + deltaTime * speed < -3.f) {
                pose.p.x += deltaTime * speed;
            }
            wallMoveLeft = false;
            wallMoveRight = false;
            mMovingWall->setKinematicTarget(pose);
        }

        mScene->simulate(deltaTime);
        mScene->fetchResults(true);
        mScene->fetchResultsParticleSystem();
        getParticlesInternal();
    }
}

void PhysxBase::cleanupPhysics(bool /*interactive*/)
{
    PX_RELEASE(mScene);
    PX_RELEASE(mDispatcher);
    PX_RELEASE(mPhysics);
    if (mPvd) {
        PxPvdTransport* transport = mPvd->getTransport();
        mPvd->release(); mPvd = nullptr;
        PX_RELEASE(transport);
    }
    PX_RELEASE(mFoundation);
    printf("PhysxBase cleanup done.\n");
}

float PhysxBase::getRightWallLocation() {
    return mMovingWall ? mMovingWall->getGlobalPose().p.x : 0.0f;
}

std::vector<Particle>& PhysxBase::getParticles() {
    return m_Particles;
}

PxPBDParticleSystem* PhysxBase::getParticleSystem() {
    return mParticleSystem;
}

PxParticleAndDiffuseBuffer* PhysxBase::getParticleBuffer() {
    return mParticleBuffer;
}

// --------------- Private ----------------

void PhysxBase::initObstacles()
{
    PxShape* shape = mPhysics->createShape(PxCapsuleGeometry(1.0f, 2.5f), *mMaterial);
    PxRigidDynamic* body = mPhysics->createRigidDynamic(PxTransform(PxVec3(3.5f, 3.5f, 0), PxQuat(PxPi * -0.5f, PxVec3(0, 0, 1))));
    body->attachShape(*shape);
    body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    mScene->addActor(*body);
    shape->release();

    body = mPhysics->createRigidDynamic(PxTransform(PxVec3(3.5f, 0.75f, 0)));
    body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    mScene->addActor(*body);
}

void PhysxBase::initScene()
{
    if (PxGetSuggestedCudaDeviceOrdinal(mFoundation->getErrorCallback()) >= 0) {
        PxCudaContextManagerDesc cudaContextManagerDesc;
        mCudaContextManager = PxCreateCudaContextManager(*mFoundation, cudaContextManagerDesc, PxGetProfilerCallback());
        if (mCudaContextManager && !mCudaContextManager->contextIsValid()) {
            mCudaContextManager->release();
            mCudaContextManager = nullptr;
        }
    }
    if (mCudaContextManager == nullptr) {
        PxGetFoundation().error(PxErrorCode::eINVALID_OPERATION, PX_FL, "Failed to initialize CUDA!\n");
    }

    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    mDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    sceneDesc.cudaContextManager = mCudaContextManager;
    sceneDesc.staticStructure = PxPruningStructureType::eDYNAMIC_AABB_TREE;
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
    sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;
    sceneDesc.solverType = PxSolverType::eTGS;
    mScene = mPhysics->createScene(sceneDesc);
}

void PhysxBase::initParticles(PxU32 numX, PxU32 numY, PxU32 numZ, const PxVec3& position, PxReal particleSpacing, PxReal fluidDensity, PxU32 maxDiffuseParticles)
{
    if (mScene == nullptr || mPhysics == nullptr)
        return;

    physx::PxCudaContextManager* cudaContextManager = mScene->getCudaContextManager();
    if (cudaContextManager == nullptr)
        return;

    const PxU32 maxParticles = numX * numY * numZ;
    const PxReal restOffset = 0.5f * particleSpacing / 0.6f;

    PxPBDMaterial* defaultMat = mPhysics->createPBDMaterial(0.05f, 0.05f, 0.f, 0.001f, 0.5f, 0.005f, 0.01f, 0.f, 0.f);
    defaultMat->setViscosity(0.001f);
    defaultMat->setSurfaceTension(0.00704f);
    defaultMat->setCohesion(0.0704f);
    defaultMat->setVorticityConfinement(10.f);

    mParticleSystem = mPhysics->createPBDParticleSystem(*cudaContextManager, 96);

    const PxReal solidRestOffset = restOffset;
    const PxReal fluidRestOffset = restOffset * 0.6f;
    const PxReal particleMass = fluidDensity * 1.333f * 3.14159f * particleSpacing * particleSpacing * particleSpacing;
    mParticleSystem->setRestOffset(restOffset);
    mParticleSystem->setContactOffset(restOffset + 0.01f);
    mParticleSystem->setParticleContactOffset(fluidRestOffset / 0.6f);
    mParticleSystem->setSolidRestOffset(solidRestOffset);
    mParticleSystem->setFluidRestOffset(fluidRestOffset);
    mParticleSystem->enableCCD(false);
    mParticleSystem->setMaxVelocity(solidRestOffset * 100.f);

    mScene->addActor(*mParticleSystem);

    PxDiffuseParticleParams dpParams;
    dpParams.threshold = 300.0f;
    dpParams.bubbleDrag = 0.9f;
    dpParams.buoyancy = 0.9f;
    dpParams.airDrag = 0.0f;
    dpParams.kineticEnergyWeight = 0.01f;
    dpParams.pressureWeight = 1.0f;
    dpParams.divergenceWeight = 10.f;
    dpParams.lifetime = 1.0f;
    dpParams.useAccurateVelocity = false;

    mMaxDiffuseParticles = maxDiffuseParticles;

    const PxU32 particlePhase = mParticleSystem->createPhase(defaultMat, PxParticlePhaseFlags(PxParticlePhaseFlag::eParticlePhaseFluid | PxParticlePhaseFlag::eParticlePhaseSelfCollide));

    PxU32* phase = cudaContextManager->allocPinnedHostBuffer<PxU32>(maxParticles);
    PxVec4* positionInvMass = cudaContextManager->allocPinnedHostBuffer<PxVec4>(maxParticles);
    PxVec4* velocity = cudaContextManager->allocPinnedHostBuffer<PxVec4>(maxParticles);

    PxReal x = position.x;
    PxReal y = position.y;
    PxReal z = position.z;

    for (PxU32 i = 0; i < numX; ++i) {
        for (PxU32 j = 0; j < numY; ++j) {
            for (PxU32 k = 0; k < numZ; ++k) {
                const PxU32 index = i * (numY * numZ) + j * numZ + k;
                PxVec4 pos(x, y, z, 1.0f / particleMass);
                phase[index] = particlePhase;
                positionInvMass[index] = pos;
                velocity[index] = PxVec4(0.0f);
                z += particleSpacing;
            }
            z = position.z;
            y += particleSpacing;
        }
        y = position.y;
        x += particleSpacing;
    }

    ExtGpu::PxParticleAndDiffuseBufferDesc bufferDesc;
    bufferDesc.maxParticles = maxParticles;
    bufferDesc.numActiveParticles = maxParticles;
    bufferDesc.maxDiffuseParticles = maxDiffuseParticles;
    bufferDesc.maxActiveDiffuseParticles = maxDiffuseParticles;
    bufferDesc.diffuseParams = dpParams;
    bufferDesc.positions = positionInvMass;
    bufferDesc.velocities = velocity;
    bufferDesc.phases = phase;

    mParticleBuffer = physx::ExtGpu::PxCreateAndPopulateParticleAndDiffuseBuffer(bufferDesc, cudaContextManager);
    mParticleSystem->addParticleBuffer(mParticleBuffer);

    cudaContextManager->freePinnedHostBuffer(positionInvMass);
    cudaContextManager->freePinnedHostBuffer(velocity);
    cudaContextManager->freePinnedHostBuffer(phase);
}

void PhysxBase::getParticlesInternal()
{
    if (!mParticleBuffer) return;

    size_t bufferSize = mParticleBuffer->getNbActiveParticles() * sizeof(PxVec4);
    if (bufferSize == 0) return;

    void* bufferLocation = malloc(bufferSize);
    if (!bufferLocation) {
        std::cerr << "Failed to allocate host memory" << std::endl;
        return;
    }

    PxVec4* bufferPos = mParticleBuffer->getPositionInvMasses();
    CUdeviceptr ptr = reinterpret_cast<CUdeviceptr>(bufferPos);

    auto cudaContextManager = mScene->getCudaContextManager();
    auto cudaContext = cudaContextManager->getCudaContext();
    cudaContext->memcpyDtoH(bufferLocation, ptr, bufferSize);

    PxVec4* positions = static_cast<PxVec4*>(bufferLocation);

    m_Particles.clear();
    m_Particles.resize(mParticleBuffer->getNbActiveParticles());

    for (size_t i = 0; i < mParticleBuffer->getNbActiveParticles(); ++i) {
        m_Particles[i].pos.x = positions[i].x;
        m_Particles[i].pos.y = positions[i].y;
        m_Particles[i].pos.z = positions[i].z;
        m_Particles[i].pos.w = positions[i].w;
    }

    free(bufferLocation);
}
