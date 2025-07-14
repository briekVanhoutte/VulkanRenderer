#pragma once

#include <vector>
#include <memory>
#include "Engine/DataBuffer.h"
#include <PxPhysicsAPI.h>
#include <extensions/PxParticleExt.h>

#include <vector>

#include <iostream>

#include "PxPhysicsAPI.h"
#include "cudamanager/PxCudaContext.h"
#include "cudamanager/PxCudaContextManager.h"

#include "engine/Singleton.h"
#define CUDA_SUCCESS 0
#define SHOW_SOLID_SDF_SLICE 0
#define IDX(i, j, k, offset) ((i) + dimX * ((j) + dimY * ((k) + dimZ * (offset))))
#define PVD_HOST "127.0.0.1"

using namespace physx;
using namespace ExtGpu;


static PxDefaultAllocator				gAllocator;
static PxDefaultErrorCallback			gErrorCallback;
static PxFoundation* gFoundation = NULL;
static PxPhysics* gPhysics = NULL;
static PxDefaultCpuDispatcher* gDispatcher = NULL;
static PxScene* gScene = NULL;
static PxMaterial* gMaterial = NULL;
static PxPvd* gPvd = NULL;
static PxPBDParticleSystem* gParticleSystem = NULL;
static PxParticleAndDiffuseBuffer* gParticleBuffer = NULL;
static bool								gIsRunning = true;
static bool								gStep = true;
static PxCudaContextManager* cudaContextManager = NULL;

static PxRigidDynamic* movingWall = nullptr;

static int								gMaxDiffuseParticles = 0;


static void initObstacles()
{
	PxShape* shape = gPhysics->createShape(PxCapsuleGeometry(1.0f, 2.5f), *gMaterial);
	PxRigidDynamic* body = gPhysics->createRigidDynamic(PxTransform(PxVec3(3.5f, 3.5f, 0), PxQuat(PxPi * -0.5f, PxVec3(0, 0, 1))));
	body->attachShape(*shape);
	body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	gScene->addActor(*body);
	shape->release();

	//shape = gPhysics->createShape(PxBoxGeometry(1.0f, 1.0f, 5.0f), *gMaterial);
	body = gPhysics->createRigidDynamic(PxTransform(PxVec3(3.5f, 0.75f, 0)));
	//body->attachShape(*shape);
	body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	gScene->addActor(*body);
	//shape->release();
}

static void initScene()
{
	
	if (PxGetSuggestedCudaDeviceOrdinal(gFoundation->getErrorCallback()) >= 0)
	{
		// initialize CUDA
		PxCudaContextManagerDesc cudaContextManagerDesc;
		cudaContextManager = PxCreateCudaContextManager(*gFoundation, cudaContextManagerDesc, PxGetProfilerCallback());
		if (cudaContextManager && !cudaContextManager->contextIsValid())
		{
			cudaContextManager->release();
			cudaContextManager = NULL;
		}
	}
	if (cudaContextManager == NULL)
	{
		PxGetFoundation().error(PxErrorCode::eINVALID_OPERATION, PX_FL, "Failed to initialize CUDA!\n");
	}

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.cudaContextManager = cudaContextManager;
	sceneDesc.staticStructure = PxPruningStructureType::eDYNAMIC_AABB_TREE;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
	sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;
	sceneDesc.solverType = PxSolverType::eTGS;
	gScene = gPhysics->createScene(sceneDesc);
}

static int getNumDiffuseParticles()
{
	return gMaxDiffuseParticles;
}

static void initParticles(const PxU32 numX, const PxU32 numY, const PxU32 numZ, const PxVec3& position = PxVec3(0, 0, 0), const PxReal particleSpacing = 0.2f, const PxReal fluidDensity = 1000.f, const PxU32 maxDiffuseParticles = 100000)
{
	PxCudaContextManager* cudaContextManager = gScene->getCudaContextManager();
	if (cudaContextManager == NULL)
		return;

	const PxU32 maxParticles = numX * numY * numZ;

	const PxReal restOffset = 0.5f * particleSpacing / 0.6f;

	// Material setup
	PxPBDMaterial* defaultMat = gPhysics->createPBDMaterial(0.05f, 0.05f, 0.f, 0.001f, 0.5f, 0.005f, 0.01f, 0.f, 0.f);

	defaultMat->setViscosity(0.001f);
	defaultMat->setSurfaceTension(0.00704f);
	defaultMat->setCohesion(0.0704f);
	defaultMat->setVorticityConfinement(10.f);

	PxPBDParticleSystem* particleSystem = gPhysics->createPBDParticleSystem(*cudaContextManager, 96);
	gParticleSystem = particleSystem;

	// General particle system setting

	const PxReal solidRestOffset = restOffset;
	const PxReal fluidRestOffset = restOffset * 0.6f;
	const PxReal particleMass = fluidDensity * 1.333f * 3.14159f * particleSpacing * particleSpacing * particleSpacing;
	particleSystem->setRestOffset(restOffset);
	particleSystem->setContactOffset(restOffset + 0.01f);
	particleSystem->setParticleContactOffset(fluidRestOffset / 0.6f);
	particleSystem->setSolidRestOffset(solidRestOffset);
	particleSystem->setFluidRestOffset(fluidRestOffset);
	particleSystem->enableCCD(false);
	particleSystem->setMaxVelocity(solidRestOffset * 100.f);
	
	gScene->addActor(*particleSystem);

	// Diffuse particles setting
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

	gMaxDiffuseParticles = maxDiffuseParticles;

	// Create particles and add them to the particle system
	const PxU32 particlePhase = particleSystem->createPhase(defaultMat, PxParticlePhaseFlags(PxParticlePhaseFlag::eParticlePhaseFluid | PxParticlePhaseFlag::eParticlePhaseSelfCollide));

	PxU32* phase = cudaContextManager->allocPinnedHostBuffer<PxU32>(maxParticles);
	PxVec4* positionInvMass = cudaContextManager->allocPinnedHostBuffer<PxVec4>(maxParticles);
	PxVec4* velocity = cudaContextManager->allocPinnedHostBuffer<PxVec4>(maxParticles);

	PxReal x = position.x;
	PxReal y = position.y;
	PxReal z = position.z;

	for (PxU32 i = 0; i < numX; ++i)
	{
		for (PxU32 j = 0; j < numY; ++j)
		{
			for (PxU32 k = 0; k < numZ; ++k)
			{
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

	gParticleBuffer = physx::ExtGpu::PxCreateAndPopulateParticleAndDiffuseBuffer(bufferDesc, cudaContextManager);
	gParticleSystem->addParticleBuffer(gParticleBuffer);

	cudaContextManager->freePinnedHostBuffer(positionInvMass);
	cudaContextManager->freePinnedHostBuffer(velocity);
	cudaContextManager->freePinnedHostBuffer(phase);
}



class PhysxBase : public Singleton<PhysxBase>
{
	

	public:
		PxPBDParticleSystem* getParticleSystem()
		{
			return gParticleSystem;
		}

		PxParticleAndDiffuseBuffer* getParticleBuffer()
		{
			return gParticleBuffer;
		}

		~PhysxBase() {
			cleanupPhysics(false);
		}

		void initPhysics(bool useLargeFluid)
		{
			gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

			gPvd = PxCreatePvd(*gFoundation);
			PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
			gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

			gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

			initScene();

			PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
			if (pvdClient)
			{
				pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
				pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
				pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
			}
			gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

			// Setup PBF
			bool useMovingWall = true;

			const PxReal fluidDensity = 1000.0f;

			const PxU32 maxDiffuseParticles = useLargeFluid ? 2000000 : 100000;
			initParticles(50, 120 * (useLargeFluid ? 5 : 1), 30, PxVec3(-2.5f, 3.f, 0.5f), 0.1f, fluidDensity, maxDiffuseParticles);

			//initObstacles();

			// Setup container
			gScene->addActor(*PxCreatePlane(*gPhysics, PxPlane(0.f, 1.f, 0.f, 0.0f), *gMaterial));
			gScene->addActor(*PxCreatePlane(*gPhysics, PxPlane(-1.f, 0.f, 0.f, 3.f), *gMaterial));
			gScene->addActor(*PxCreatePlane(*gPhysics, PxPlane(0.f, 0.f, 1.f, 3.f), *gMaterial));
			gScene->addActor(*PxCreatePlane(*gPhysics, PxPlane(0.f, 0.f, -1.f, 3.f), *gMaterial));

			if (!useMovingWall)
			{
				gScene->addActor(*PxCreatePlane(*gPhysics, PxPlane(1.f, 0.f, 0.f, 3.f), *gMaterial));
				movingWall = NULL;
			}
			else
			{
				PxTransform trans = PxTransformFromPlaneEquation(PxPlane(1.f, 0.f, 0.f, 5.f));
				movingWall = gPhysics->createRigidDynamic(trans);
				movingWall->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
				PxRigidActorExt::createExclusiveShape(*movingWall, PxPlaneGeometry(), *gMaterial);
				gScene->addActor(*movingWall);
			}

			// Setup rigid bodies
			const PxReal dynamicsDensity = fluidDensity * 0.5f;
			const PxReal boxSize = 1.0f;
			const PxReal boxMass = boxSize * boxSize * boxSize * dynamicsDensity;
			PxShape* shape = gPhysics->createShape(PxBoxGeometry(0.5f * boxSize, 0.5f * boxSize, 0.5f * boxSize), *gMaterial);
			for (int i = 0; i < 5; ++i)
			{
				PxRigidDynamic* body = gPhysics->createRigidDynamic(PxTransform(PxVec3(i - 3.0f, 10, 7.5f)));
				body->attachShape(*shape);
				PxRigidBodyExt::updateMassAndInertia(*body, boxMass);
				gScene->addActor(*body);
			}
			shape->release();
		}
		bool wallMoveLeft = false;
		bool wallMoveRight = false;

		float getRightWallLocation() {
			return movingWall->getGlobalPose().p.x;
		}

		void stepPhysics(bool interactive)
		{
			if (gIsRunning || gStep)
			{
				gStep = false;
				PxReal dt = 1.0f / 60.0f;
				//const PxReal dt = deltaTime;

				if (movingWall)
				{
					PxReal speed = 3.f;


					PxTransform pose = movingWall->getGlobalPose();
					if (wallMoveRight)
					{
						

						if (pose.p.x - dt * speed > -9.f) {
							pose.p.x -= dt * speed;
						}
					}
					if (wallMoveLeft)
					{
						if (pose.p.x + dt * speed < -3.f)
						{
							pose.p.x += dt * speed;
						}


					}
					wallMoveLeft = false;
					wallMoveRight = false;

					movingWall->setKinematicTarget(pose);
				}

				gScene->simulate(dt);
				gScene->fetchResults(true);
				gScene->fetchResultsParticleSystem();
				getParticles();
			}
		}

		void cleanupPhysics(bool /*interactive*/)
		{
			PX_RELEASE(gScene);
			PX_RELEASE(gDispatcher);
			PX_RELEASE(gPhysics);
			if (gPvd)
			{
				PxPvdTransport* transport = gPvd->getTransport();
				gPvd->release();	gPvd = NULL;
				PX_RELEASE(transport);
			}
			PX_RELEASE(gFoundation);

			printf("SnippetPBFFluid done.\n");
		}	

		void getParticles() {
			// Determine the size of the buffer
			auto pr = getParticleBuffer();

			if (pr == nullptr)return;

			size_t bufferSize = pr->getNbActiveParticles() * sizeof(PxVec4);

			// Allocate normal host memory for bufferLocation
			void* bufferLocation = malloc(bufferSize);
			if (bufferLocation == nullptr) {
				std::cerr << "Failed to allocate host memory" << std::endl;
				return;
			}

			// Get the positions of the particles
			PxVec4* bufferPos = pr->getPositionInvMasses();

			// Cast the bufferPos to CUdeviceptr
			CUdeviceptr ptr = reinterpret_cast<CUdeviceptr>(bufferPos);

			// Get the CUDA context manager and context
			auto cudaContextManager = gScene->getCudaContextManager();
			auto cudaContext = cudaContextManager->getCudaContext();

			// Perform the asynchronous memory copy from device to host
			cudaContext->memcpyDtoH(bufferLocation, ptr, bufferSize);
			
			// Cast the bufferLocation to PxVec4* to read the values
			PxVec4* positions = static_cast<PxVec4*>(bufferLocation);

			// Create a vector to hold the positions
			//std::vector<PxVec4> particlePositions(getParticleBuffer()->getNbActiveParticles());
			
			m_Particles.clear();
			m_Particles.resize(pr->getNbActiveParticles());

			// Copy the positions into the vector
			for (size_t i = 0; i < pr->getNbActiveParticles(); ++i) {
				m_Particles[i].pos.x = positions[i].x;
				m_Particles[i].pos.y = positions[i].y;
				m_Particles[i].pos.z = positions[i].z;
				m_Particles[i].pos.w = positions[i].w;
			}

			// Free the allocated host memory
			free(bufferLocation);
		};

		std::vector<Particle> m_Particles = { {} };
};
