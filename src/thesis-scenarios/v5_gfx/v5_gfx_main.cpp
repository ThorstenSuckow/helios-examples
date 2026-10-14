
#include <thread>
#include <utility>
#include <numbers>


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

#include <algorithm>
#include <cmath>
#include <optional>

#include "./Namespaces.h"

using namespace thesis::config;


int main() {
;

    auto engineRuntime = helios::engine::v5_gfx_bootstrap::bootstrapGameWorld();
    auto& ecsDataContainer = engineRuntime->gameWorld.resourceRegistry();
    auto& managerRegistry = ecsDataContainer.get<ManagerRegistry>();

    auto scheduler = helios::ecs::scheduling::Scheduler();
    ecsDataContainer.reserve<UpdateContext>();
    scheduler.init(ecsDataContainer);

    FrameTiming frameTiming{};
    std::size_t frameCount = 0;
    float totalTime = 0.0f;

    thesis::util::prepare(ecsDataContainer);

    #include "RenderSetup.inc"

    auto collisionCallback = thesis::util::collCb<ParticleHandle>(particleRenderCallback);
    auto m_col             = thesis::manager::CollisionManager<
        GameObjectHandle, ParticleHandle>{collisionCallback, thesis::config::COLLISION_COMMANDS_GO};
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

    while (thesis::util::shouldRun(frameCount)) {
        framePacer.beginFrame();

        thesis::util::updateFrameMetrics(ecsDataContainer, frameTiming, frameCount, totalTime);
        scheduler.update();

        frameTiming = framePacer.sync();
        fpsMetrics.addFrame(frameTiming);
    }

    thesis::util::writeMetricsToCSV();

    return 0;
}