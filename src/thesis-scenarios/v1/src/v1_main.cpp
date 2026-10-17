
#include <thread>
#include <utility>
#include <numbers>
#include <cassert>


import helios.core;
import helios.ecs;
import helios.engine;
import helios.math;
import helios.physics;
import helios.gameplay;
import helios.opengl;
import helios.glfw;
import helios.imgui;

import thesis.handles;
import thesis.bootstrap;
import thesis.config;
import thesis.util;
import thesis.systems;
import thesis.manager;
import thesis.types;
import thesis.components;

#include <algorithm>
#include <cmath>
#include <optional>

#include "../../lib/include/Namespaces.h"

using namespace thesis::config;

#ifndef HEADLESS
#define HEADLESS false
#endif

#define THREAD_COUNT 0

int main(int argc, char** argv) {

    std::string PROGRAM_ID = "v1";

    #include "../../lib/include/Setup.inc"
    #include "../../lib/include/RenderSetup.inc"

    using CollisionManager = thesis::manager::CollisionManager<UniHandle, UniHandle>;
    using LifetimeManager = thesis::manager::LifetimeManager<UniHandle>;
    using BetaSystem = thesis::systems::BetaSystem<UniHandle>;
    using DeltaDetSystem = thesis::systems::UniDeltaDetSystem<UniHandle>;
    using DeltaResSystem = thesis::systems::UniDeltaResSystem<UniHandle>;
    using LambdaSystem = thesis::systems::LambdaSystem<UniHandle>;

    using CollisionDetectionResult = thesis::types::UniCollisionDetectionResult<UniHandle>;
    using DeltaResResult = thesis::types::UniDeltaResResult<UniHandle>;

    #if HEADLESS
        auto gameObjectRenderCallback = [&](auto& gameObject) {
            gameObject.template add<thesis::components::GameObjectTagComponent>();
        };
        auto borderRenderCallbackGameObject = [&](auto& border) {
            border.template add<thesis::components::GameObjectTagComponent>();
        };
        auto borderRenderCallbackParticle = [&](auto& border) {
            border.template add<thesis::components::ParticleTagComponent>();
        };
        auto particleRenderCallback = [&](auto& particle) {
            particle.template add<thesis::components::ParticleTagComponent>();
        };

    #else
        auto gameObjectRenderCallback = [&](auto& gameObject) {
            gameObject.setActive(true);
            gameObject.template add<thesis::components::GameObjectTagComponent>();
            gameObject.template add<DefaultSceneMemberComponent>(MainScene);
            gameObject.template add<DefaultRenderPrototypeComponent<Instanced>>(
                InstanceShader.handle(), GameObjectMaterial.handle(), RectMesh.handle()
            );
        };

        auto borderRenderCallbackGameObject = [&](auto& border) {
            border.setActive(true);
            border.template add<thesis::components::GameObjectTagComponent>();
            border.template add<DefaultSceneMemberComponent>(MainScene);
            border.template add<DefaultRenderPrototypeComponent<Instanced>>(
                InstanceShader.handle(), BoundaryMaterial.handle(), RectMesh.handle()
            );
        };

        auto borderRenderCallbackParticle = [&](auto& border) {
            border.setActive(true);
            border.template add<thesis::components::ParticleTagComponent>();
            border.template add<DefaultSceneMemberComponent>(MainScene);
            border.template add<DefaultRenderPrototypeComponent<Instanced>>(
                InstanceShader.handle(), BoundaryMaterial.handle(), RectMesh.handle()
            );
        };

        auto particleRenderCallback = [&](auto& particle) {
            particle.setActive(true);
            particle.template add<thesis::components::ParticleTagComponent>();
            particle.template add<DefaultSceneMemberComponent>(MainScene);
            particle.template add<DefaultRenderPrototypeComponent<Instanced>>(
                InstanceShader.handle(), ParticleMaterial.handle(), RectMesh.handle()
            );
        };
    #endif

    auto collisionCallback = thesis::util::collCb<UniHandle>(particleRenderCallback);

    auto m_col             = CollisionManager{collisionCallback};
    auto m_lft             = LifetimeManager();
    auto s_beta            = BetaSystem();
    auto s_delta_det       = DeltaDetSystem(thesis::util::grid<UniHandle>(), thesis::util::grid<UniHandle>());
    auto s_delta_res       = DeltaResSystem();
    auto s_lambda          = LambdaSystem();

    #include "../../lib/include/ToolingSetup.inc"
    #include "../../lib/include/SchedulerPreSetup.inc"

    thesis::util::createGameObjects<UniHandle>(
        ecsDataContainer.get<EntityManager<UniHandle>>(),
        gameObjectRenderCallback
    );
    thesis::util::createBoundaries<UniHandle>(
        ecsDataContainer.get<EntityManager<UniHandle>>(),
        borderRenderCallbackGameObject
    );
    thesis::util::createBoundaries<UniHandle>(
        ecsDataContainer.get<EntityManager<UniHandle>>(),
        borderRenderCallbackParticle
    );

    // +----------------------------------------
    // | THESIS SCHEDULE
    // +----------------------------------------
    managerSchedule.endSchedule(
        Sequential(m_col)
    );
    systemSchedule.add(
        Sequential(
            s_beta, s_delta_det, s_delta_res, s_lambda
        )
    ).endSchedule(m_lft);
    // +----------------------------------------
    // +----------------------------------------

     auto mColTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::manager::types::ManagerTypeId>{
        helios::ecs::manager::types::ManagerTypeId::template id<CollisionManager>(),
        managerSchedule.idx(), 0, 0, 0,
    };

    auto mLftTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::manager::types::ManagerTypeId>{
        helios::ecs::manager::types::ManagerTypeId::template id<LifetimeManager>(),
        systemSchedule.idx(), 0, 0, 0,
    };

    auto sBetaTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<BetaSystem>(),
        systemSchedule.idx(), 0, 0, 0,
    };

    auto sDeltaDetTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<DeltaDetSystem>(),
        systemSchedule.idx(), 0, 0, 1,
    };

    auto sDeltaResTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<DeltaResSystem>(),
        systemSchedule.idx(), 0, 0, 2,
    };

    auto sLambdaTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<LambdaSystem>(),
        systemSchedule.idx(), 0, 0, 3,
    };

    #if !HEADLESS
    scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running))
            .add(
                Sequential(
                    SceneMemberVisibilitySystem<
                        UniHandle, Instanced, NullCullingStrategy<UniHandle>, DefaultRenderHandles
                    >(NullCullingStrategy<UniHandle>()),
                    [&](EntityManager<UniHandle>& entityManager,
                       DefaultSceneMemberVisibilityRegistry<UniHandle, Instanced>& visibilityRegistry) {
                        // only entities with a LifetimeComponent (particles) fade with their age
                        auto* lifetimeSparseSet = entityManager.sparseSet<LifetimeComponent>();
                        visibilityRegistry.forEachVisibleMember([&lifetimeSparseSet = *lifetimeSparseSet](auto& visibleContext) {
                            if (auto* cmp = lifetimeSparseSet.get(visibleContext.memberHandle.entityId())) {
                                visibleContext.normalizedAge = cmp->value() / cmp->lifetime();
                            }
                        });
                    },
                    DefaultSceneRenderSystem<UniHandle, Instanced>()
                )
            )
        .endSchedule<Sequential<DefaultRenderManager>>()
    #endif

    #include "../../lib/include/SchedulerPostSetup.inc"

    thesis::util::init(ecsDataContainer);

    while (thesis::util::shouldbeRunning(frameCount)) {

        #if !HEADLESS
            framePacer.beginFrame();
        #endif

        scheduler.update(true, true);

        #if !HEADLESS
            frameTiming = framePacer.sync();
        #endif

        auto collisionManagerResult = rrr.item<thesis::types::CollisionManagerResult>();
        METRICS.COLLISION_COMMANDS_GO[frameCount] = collisionManagerResult ? collisionManagerResult->collisionCount : 0;

        auto lifetimeManagerResult = rrr.item<thesis::types::LifetimeManagerResult>();
        METRICS.LIFETIME_COMMANDS_PRT[frameCount] = lifetimeManagerResult ? lifetimeManagerResult->expiredCount : 0;

        auto collisionDetectionResult = rrr.item<CollisionDetectionResult>();

        METRICS.COLLISION_PAIRS_FOUND_GO[frameCount] = collisionDetectionResult
            ? collisionDetectionResult->collisionPairsGo().size() : 0;
        METRICS.COLLISION_PAIRS_FOUND_PRT[frameCount] = collisionDetectionResult
            ? collisionDetectionResult->collisionPairsPrt().size() : 0;

        auto collisionProcessedResult = rrr.item<DeltaResResult>();
        METRICS.COLLISION_PAIRS_PROCESSED_GO[frameCount] = collisionProcessedResult
            ? collisionProcessedResult->processedCollisionsGo : 0;
        METRICS.COLLISION_PAIRS_PROCESSED_PRT[frameCount] = collisionProcessedResult
            ? collisionProcessedResult->processedCollisionsPrt : 0;

        METRICS.PARTICLE_POPULATION[frameCount] = ecsDataContainer.get<EntityManager<UniHandle>>()
        .sparseSet<thesis::components::ParticleTagComponent>()->componentCount();

        METRICS.M_COL_TIMINGS[frameCount] = scheduler.timings(mColTimingHandle).updateDuration;
        METRICS.M_LFT_TIMINGS[frameCount] = scheduler.timings(mLftTimingHandle).updateDuration;
        METRICS.S_BETA_GO_TIMINGS[frameCount] = scheduler.timings(sBetaTimingHandle).updateDuration;
        METRICS.S_DELTA_DET_GO_TIMINGS[frameCount] = scheduler.timings(sDeltaDetTimingHandle).updateDuration;
        METRICS.S_DELTA_RES_GO_TIMINGS[frameCount] = scheduler.timings(sDeltaResTimingHandle).updateDuration;
        METRICS.S_LAMBDA_TIMINGS[frameCount] = scheduler.timings(sLambdaTimingHandle).updateDuration;

        METRICS.SCHEDULE_TIMINGS[frameCount] = (managerSchedule.timings() + systemSchedule.timings()).total();

        #if !HEADLESS
            fpsMetrics.addFrame(frameTiming);
        #endif

        thesis::util::updateFrameMetrics(ecsDataContainer, frameTiming, frameCount, totalTime);
    }

    auto sys = helios::ecs::system::System(
        thesis::systems::CollectTaggedSpatialResultsSystem<UniHandle>(METRICS)
    );
    sys.update(ecsDataContainer);

    thesis::util::writeMetricsToCSV(METRICS, PROGRAM_ID);

    return 0;
}