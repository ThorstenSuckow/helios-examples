
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

import helios.engine.v5_gfx_bootstrap;
import thesis.config;
import thesis.util;
import thesis.systems;
import thesis.manager;
import thesis.types;

#include <algorithm>
#include <cmath>
#include <optional>

#include "./Namespaces.h"

using namespace thesis::config;


int main() {
;
    auto METRICS = thesis::config::METRICS{};

    auto engineRuntime = helios::engine::v5_gfx_bootstrap::bootstrapGameWorld();
    auto& ecsDataContainer = engineRuntime->gameWorld.resourceRegistry();
    auto& rrr = ecsDataContainer.get<helios::ecs::common::RuntimeResultRegistry>();

    auto scheduler = helios::ecs::scheduling::Scheduler();
    ecsDataContainer.emplace<UpdateContext>(0.0f, 0.0f, 0, helios::engine::input::InputSnapshot{});
    scheduler.init(ecsDataContainer);

    FrameTiming frameTiming{};
    std::size_t frameCount = 0;
    float totalTime = 0.0f;

    thesis::util::prepare(ecsDataContainer);

    #include "RenderSetup.inc"

    using GameObject = GameObjectHandle;
    using Particle = ParticleHandle;

    using CollisionManager = thesis::manager::CollisionManager<GameObject, Particle>;
    using LifetimeManager = thesis::manager::LifetimeManager<Particle>;
    using BetaSystemGo = thesis::systems::BetaSystem<GameObject>;
    using BetaSystemPrt = thesis::systems::BetaSystem<Particle>;
    using DeltaDetSystemGo = thesis::systems::DeltaDetSystem<GameObject>;
    using DeltaDetSystemPrt = thesis::systems::DeltaDetSystem<Particle>;
    using DeltaResSystemGo = thesis::systems::DeltaResSystem<GameObject>;
    using DeltaResSystemPrt = thesis::systems::DeltaResSystem<Particle>;
    using LambdaSystem = thesis::systems::LambdaSystem<Particle>;

    using CollisionDetectionResultGo = helios::physics::collision::types::CollisionDetectionResult<GameObject>;
    using CollisionDetectionResultPrt = helios::physics::collision::types::CollisionDetectionResult<Particle>;

    using DeltaResResultGo = thesis::types::DeltaResResult<GameObject>;
    using DeltaResResultPrt = thesis::types::DeltaResResult<Particle>;

    auto collisionCallback = thesis::util::collCb<Particle>(particleRenderCallback);
    auto m_col             = CollisionManager{collisionCallback};
    auto m_lft             = LifetimeManager();
    auto s_beta_go         = BetaSystemGo();
    auto s_beta_prt        = BetaSystemPrt();
    auto s_delta_det_go    = DeltaDetSystemGo(thesis::util::grid<GameObject>());
    auto s_delta_det_prt   = DeltaDetSystemPrt(thesis::util::grid<Particle>());
    auto s_delta_res_go    = DeltaResSystemGo();
    auto s_delta_res_prt   = DeltaResSystemPrt();
    auto s_lambda          = LambdaSystem();

    thesis::util::createGameObjects<GameObject>(
        ecsDataContainer.get<EntityManager<GameObject>>(),
        gameObjectRenderCallback
    );
    thesis::util::createBoundaries<GameObject>(
        ecsDataContainer.get<EntityManager<GameObject>>(),
        borderRenderCallback
    );
    thesis::util::createBoundaries<Particle>(
        ecsDataContainer.get<EntityManager<Particle>>(),
        borderRenderCallback
    );

    #include "ToolingSetup.inc"
    #include "SchedulerPreSetup.inc"

    // +----------------------------------------
    // | THESIS SCHEDULE
    // +----------------------------------------
    auto& managerSchedule = scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running));
    auto& systemSchedule = scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running));

    managerSchedule.endSchedule(
        Sequential(m_col, m_lft)
    );
    systemSchedule
        .add(s_beta_go, s_beta_prt)
        .add(s_delta_det_go, s_delta_det_prt)
        .add(s_delta_res_go, s_delta_res_prt)
        .add(s_lambda)
    .endSchedule();
    // +----------------------------------------
    // +----------------------------------------

     auto mColTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::manager::types::ManagerTypeId>{
        helios::ecs::manager::types::ManagerTypeId::template id<CollisionManager>(),
        managerSchedule.idx(), 0, 0, 0,
    };

    auto mLftTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::manager::types::ManagerTypeId>{
        helios::ecs::manager::types::ManagerTypeId::template id<LifetimeManager>(),
        managerSchedule.idx(), 0, 0, 1,
    };

    auto sBetaGoTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<BetaSystemGo>(),
        systemSchedule.idx(), 0, 0, 0,
    };

    auto sBetaPrtTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<BetaSystemPrt>(),
        systemSchedule.idx(), 0, 1, 0,
    };

    auto sDeltaDetGoTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<DeltaDetSystemGo>(),
        systemSchedule.idx(), 1, 0, 0,
    };

    auto sDeltaDetPrtTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<DeltaDetSystemPrt>(),
        systemSchedule.idx(), 1, 1, 0,
    };

    auto sDeltaResGoTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<DeltaResSystemGo>(),
        systemSchedule.idx(), 2, 0, 0,
    };

    auto sDeltaResPrtTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<DeltaResSystemPrt>(),
        systemSchedule.idx(), 2, 1, 0,
    };

    auto sLambdaTimingHandle = helios::ecs::scheduling::types::RuntimeSystemEntryHandle<helios::ecs::system::types::SystemTypeId>{
        helios::ecs::system::types::SystemTypeId::template id<LambdaSystem>(),
        systemSchedule.idx(), 3, 0, 0,
    };


    #include "SchedulerPostSetup.inc"

    thesis::util::init(ecsDataContainer);

    while (thesis::util::shouldbeRunning(frameCount)) {
        framePacer.beginFrame();

        scheduler.update(true, false);

        frameTiming = framePacer.sync();

        auto collisionManagerResult = rrr.item<thesis::types::CollisionManagerResult>();
        METRICS.COLLISION_COMMANDS_GO[frameCount] = collisionManagerResult ? collisionManagerResult->collisionCount : 0;

        auto lifetimeManagerResult = rrr.item<thesis::types::LifetimeManagerResult>();
        METRICS.LIFETIME_COMMANDS_PRT[frameCount] = lifetimeManagerResult ? lifetimeManagerResult->expiredCount : 0;

        auto collisionDetectionResult_GO = rrr.item<CollisionDetectionResultGo>();
        METRICS.COLLISION_PAIRS_FOUND_GO[frameCount] = collisionDetectionResult_GO ? collisionDetectionResult_GO->collisionPairs().size() : 0;

        auto collisionDetectionResult_PRT = rrr.item<CollisionDetectionResultPrt>();
        METRICS.COLLISION_PAIRS_FOUND_PRT[frameCount] = collisionDetectionResult_PRT ? collisionDetectionResult_PRT->collisionPairs().size() : 0;

        auto collisionProcessedResult_GO = rrr.item<DeltaResResultGo>();
        METRICS.COLLISION_PAIRS_PROCESSED_GO[frameCount] = collisionProcessedResult_GO ? collisionProcessedResult_GO->processedCollisions : 0;

        auto collisionProcessedResult_PRT = rrr.item<DeltaResResultPrt>();
        METRICS.COLLISION_PAIRS_PROCESSED_PRT[frameCount] = collisionProcessedResult_PRT ? collisionProcessedResult_PRT->processedCollisions : 0;

        METRICS.PARTICLE_POPULATION[frameCount] = ecsDataContainer.get<EntityManager<Particle>>()
        .sparseSet<helios::physics::collision::components::CollisionComponent>()->componentCount();

        METRICS.M_COL_TIMINGS[frameCount] = scheduler.timings(mColTimingHandle).updateDuration;
        METRICS.M_LFT_TIMINGS[frameCount] = scheduler.timings(mLftTimingHandle).updateDuration;
        METRICS.S_BETA_GO_TIMINGS[frameCount] = scheduler.timings(sBetaGoTimingHandle).updateDuration;
        METRICS.S_BETA_PRT_TIMINGS[frameCount] = scheduler.timings(sBetaPrtTimingHandle).updateDuration;
        METRICS.S_DELTA_DET_GO_TIMINGS[frameCount] = scheduler.timings(sDeltaDetGoTimingHandle).updateDuration;
        METRICS.S_DELTA_DET_PRT_TIMINGS[frameCount] = scheduler.timings(sDeltaDetPrtTimingHandle).updateDuration;
        METRICS.S_DELTA_RES_GO_TIMINGS[frameCount] = scheduler.timings(sDeltaResGoTimingHandle).updateDuration;
        METRICS.S_DELTA_RES_PRT_TIMINGS[frameCount] = scheduler.timings(sDeltaResPrtTimingHandle).updateDuration;
        METRICS.S_LAMBDA_TIMINGS[frameCount] = scheduler.timings(sLambdaTimingHandle).updateDuration;

        METRICS.SCHEDULE_TIMINGS[frameCount] = (managerSchedule.timings() + systemSchedule.timings()).total();

        fpsMetrics.addFrame(frameTiming);
        thesis::util::updateFrameMetrics(ecsDataContainer, frameTiming, frameCount, totalTime);
    }

    thesis::util::writeMetricsToCSV(METRICS);

    return 0;
}