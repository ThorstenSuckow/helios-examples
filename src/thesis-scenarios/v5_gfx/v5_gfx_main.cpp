
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
    auto& managerRegistry = ecsDataContainer.get<ManagerRegistry>();
    auto& rrr = ecsDataContainer.get<helios::ecs::common::RuntimeResultRegistry>();

    auto scheduler = helios::ecs::scheduling::Scheduler();
    ecsDataContainer.emplace<UpdateContext>(0.0f, 0.0f, 0, helios::engine::input::InputSnapshot{});
    scheduler.init(ecsDataContainer);

    FrameTiming frameTiming{};
    std::size_t frameCount = 0;
    float totalTime = 0.0f;

    thesis::util::prepare(ecsDataContainer);

    #include "RenderSetup.inc"

    auto collisionCallback = thesis::util::collCb<ParticleHandle>(particleRenderCallback);
    auto m_col             = thesis::manager::CollisionManager<GameObjectHandle, ParticleHandle>{collisionCallback};
    auto m_lft             = thesis::manager::LifetimeManager<ParticleHandle>();
    auto s_beta_go         = thesis::systems::BetaSystem<GameObjectHandle>();
    auto s_beta_prt        = thesis::systems::BetaSystem<ParticleHandle>();
    auto s_delta_det_go    = thesis::systems::DeltaDetSystem<GameObjectHandle>(thesis::util::grid<GameObjectHandle>());
    auto s_delta_det_prt   = thesis::systems::DeltaDetSystem<ParticleHandle>(thesis::util::grid<ParticleHandle>());
    auto s_delta_res_go    = thesis::systems::DeltaResSystem<GameObjectHandle>();
    auto s_delta_res_prt   = thesis::systems::DeltaResSystem<ParticleHandle>();
    auto s_lambda          = thesis::systems::LambdaSystem<ParticleHandle>();

    thesis::util::createGameObjects<GameObjectHandle>(
        ecsDataContainer.get<EntityManager<GameObjectHandle>>(),
        gameObjectRenderCallback
    );
    thesis::util::createBoundaries<GameObjectHandle>(
        ecsDataContainer.get<EntityManager<GameObjectHandle>>(),
        borderRenderCallback
    );
    thesis::util::createBoundaries<ParticleHandle>(
        ecsDataContainer.get<EntityManager<ParticleHandle>>(),
        borderRenderCallback
    );

    #include "ToolingSetup.inc"
    #include "SchedulerPreSetup.inc"

    // +----------------------------------------
    // | THESIS SCHEDULE
    // +----------------------------------------
    scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running))
    .endSchedule(
        Sequential(m_col, m_lft)
    )
    .beginSchedule(GameWorld::sessionState(EngineState::Running))
        .add(s_beta_go, s_beta_prt)
        .add(s_delta_det_go, s_delta_det_prt)
        .add(s_delta_res_go, s_delta_res_prt)
        .add(s_lambda)
    .endSchedule();
    // +----------------------------------------
    // +----------------------------------------

    #include "SchedulerPostSetup.inc"

    thesis::util::init(ecsDataContainer);

    while (thesis::util::shouldbeRunning(frameCount)) {
        framePacer.beginFrame();

        scheduler.update();

        frameTiming = framePacer.sync();

        METRICS.FRAME_TIMINGS[frameCount] = frameTiming.workTime;

        auto collisionManagerResult = rrr.item<thesis::types::CollisionManagerResult>();
        METRICS.COLLISION_COMMANDS_GO[frameCount] = collisionManagerResult ? collisionManagerResult->collisionCount : 0;

        auto lifetimeManagerResult = rrr.item<thesis::types::LifetimeManagerResult>();
        METRICS.LIFETIME_COMMANDS_PRT[frameCount] = lifetimeManagerResult ? lifetimeManagerResult->expiredCount : 0;

        auto collisionDetectionResult_GO = rrr.item<helios::physics::collision::types::CollisionDetectionResult<GameObjectHandle>>();
        METRICS.COLLISION_PAIRS_FOUND_GO[frameCount] = collisionDetectionResult_GO ? collisionDetectionResult_GO->collisionPairs().size() : 0;

        auto collisionDetectionResult_PRT = rrr.item<helios::physics::collision::types::CollisionDetectionResult<ParticleHandle>>();
        METRICS.COLLISION_PAIRS_FOUND_PRT[frameCount] = collisionDetectionResult_PRT ? collisionDetectionResult_PRT->collisionPairs().size() : 0;

        auto collisionProcessedResult_GO = rrr.item<thesis::types::DeltaResResult<GameObjectHandle>>();
        METRICS.COLLISION_PAIRS_PROCESSED_GO[frameCount] = collisionProcessedResult_GO ? collisionProcessedResult_GO->processedCollisions : 0;

        auto collisionProcessedResult_PRT = rrr.item<thesis::types::DeltaResResult<ParticleHandle>>();
        METRICS.COLLISION_PAIRS_PROCESSED_PRT[frameCount] = collisionProcessedResult_PRT ? collisionProcessedResult_PRT->processedCollisions : 0;


        fpsMetrics.addFrame(frameTiming);
        thesis::util::updateFrameMetrics(ecsDataContainer, frameTiming, frameCount, totalTime);
    }

    thesis::util::writeMetricsToCSV(METRICS);

    return 0;
}