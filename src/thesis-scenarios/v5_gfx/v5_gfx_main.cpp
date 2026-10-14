
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

    using m_col           = thesis::manager::CollisionManager<GameObjectHandle, ParticleHandle>;
    using m_lft           = thesis::manager::LifetimeManager<ParticleHandle>;
    using s_beta_go       = thesis::systems::BetaSystem<GameObjectHandle>;
    using s_beta_prt      = thesis::systems::BetaSystem<GameObjectHandle>;
    using s_delta_det_go  = thesis::systems::DeltaDetSystem<GameObjectHandle>;
    using s_delta_det_prt = thesis::systems::DeltaDetSystem<ParticleHandle>;
    using s_delta_res_go  = thesis::systems::DeltaResSystem<GameObjectHandle>;
    using s_delta_res_prt = thesis::systems::DeltaResSystem<ParticleHandle>;
    using s_lambda        = thesis::systems::LambdaSystem<ParticleHandle>;

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

    // MAIN
    scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running))
    .endSchedule(
        Sequential(
            m_col(collisionCallback),
            m_lft
        )
    )
    .beginSchedule(GameWorld::sessionState(EngineState::Running))
        .add<s_beta_go, s_beta_prt>()
        .add(
            Sequential(
                s_delta_det_go(thesis::util::grid<GameObjectHandle>()),
                s_delta_res_go()
            ),
            Sequential(
                s_delta_det_prt(thesis::util::grid<ParticleHandle>()),
                s_delta_res_prt<ParticleHandle>(),
                s_lambda<ParticleHandle>()
            )
        )
    .endSchedule();

    #include "SchedulerPostSetup.inc"

    thesis::util::init(ecsDataContainer);

    while (true) {
        framePacer.beginFrame();

        thesis::util::updateFrameMetrics(ecsDataContainer, frameCount, frameTiming, totalTime);
        scheduler.update();

        frameTiming = framePacer.sync();
        fpsMetrics.addFrame(frameTiming);
    }

    return 0;
}