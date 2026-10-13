
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

#include <algorithm>
#include <cmath>
#include <optional>

#include "./Namespaces.h"

using namespace thesis::config;


int main() {
    const auto& logger = helios::core::log::LogManager::loggerForScope("main");

    // gameworld
    auto engineRuntime = helios::engine::v5_gfx_bootstrap::bootstrapGameWorld();

    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;

    gameWorld.resourceRegistry().emplace<helios::core::thread::ThreadPool>(
        std::max(1U, std::thread::hardware_concurrency() - 1U)
    );

    #include "RenderSetup.inc"

    thesis::util::createGameObjects<GameObjectHandle>(
        gameWorld.resourceRegistry().get<EntityManager<GameObjectHandle>>(),
        gameObjectRenderCallback
    );
    thesis::util::createBoundaries<GameObjectHandle>(
        gameWorld.resourceRegistry().get<EntityManager<GameObjectHandle>>(),
        borderRenderCallback
    );
    thesis::util::createBoundaries<ParticleHandle>(
        gameWorld.resourceRegistry().get<EntityManager<ParticleHandle>>(),
        borderRenderCallback
    );

    // ----------------------------------------
    // ImGui and Debug Tooling
    // ----------------------------------------
    auto fpsMetrics = FpsMetrics();
    auto framePacer = FramePacer();


    #include "ToolingSetup.inc"

    framePacer.setFixedFrameTime(FIXED_FRAME_TIME);

    FrameTiming frameTiming{};
    std::size_t frameCount = 0;
    float totalTime = 0.0f;

    #include "RuntimeSetup.inc"

    gameWorld.managerRegistry().add<
        helios::physics::collision::CollisionManager<GameObjectHandle, ParticleHandle>>(
        collisionManagerCallback
    );

    // ----------------------------------------
    // GameLoop Config
    // ----------------------------------------

    auto scheduler = helios::ecs::scheduling::Scheduler();
    auto& ecsDataContainer = gameWorld.resourceRegistry();
    ecsDataContainer.reserve<UpdateContext>();
    scheduler.init(ecsDataContainer);

    #include "SchedulerPreSetup.inc"

        // MAIN
        scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running))
        .endSchedule<
            Sequential<
            helios::physics::collision::CollisionManager<GameObjectHandle, ParticleHandle>,
            helios::gameplay::lifecycle::LifetimeManager<ParticleHandle>
             >
        >()


        .beginSchedule(GameWorld::sessionState(EngineState::Running))


            .add<
                Sequential<
                    YawPitchRollUpdateSystem<CameraHandle>,
                    WorldTransformSystem<CameraHandle>,
                    PerspectiveCameraUpdateSystem<CameraHandle>
                >,
                Sequential<
                    MotionIntegrationSystem<GameObjectHandle>,
                    WorldTransformSystem<GameObjectHandle>,
                    WorldBoundsUpdateSystem<GameObjectHandle>
                >,
                Sequential<
                    MotionIntegrationSystem<ParticleHandle>,
                    WorldTransformSystem<ParticleHandle>,
                    WorldBoundsUpdateSystem<ParticleHandle>
                >
            >()
            .add(
                Sequential(
                    helios::physics::collision::systems::CollisionDetectionSystem<GameObjectHandle,
                    helios::physics::collision::strategies::GridCollisionDetection<GameObjectHandle>>(
                        helios::physics::collision::strategies::GridCollisionDetection<GameObjectHandle>(
                        SPATIAL_GRID, CELL_SIZE
                    )),
                    helios::physics::collision::systems::CollisionResponseSystem<
                        GameObjectHandle,
                        helios::physics::collision::strategies::CollisionImpulseResponse
                    >(
                        helios::physics::collision::strategies::CollisionImpulseResponse{}
                    )
                ),
                Sequential(
                    helios::physics::collision::systems::CollisionDetectionSystem<ParticleHandle,
                    helios::physics::collision::strategies::GridCollisionDetection<ParticleHandle>>(
                    helios::physics::collision::strategies::GridCollisionDetection<ParticleHandle>(
                        SPATIAL_GRID, CELL_SIZE
                    )),
                    helios::physics::collision::systems::CollisionResponseSystem<
                        ParticleHandle,
                        helios::physics::collision::strategies::CollisionImpulseResponse
                    >(
                        helios::physics::collision::strategies::CollisionImpulseResponse{}
                    ),
                    [](TypedCommandBuffer<LifetimeCommand<ParticleHandle>>& commandBuffer,
                        Query<ParticleHandle, ReadSet<LifetimeComponent>, WriteSet<LifetimeComponent>> query, const UpdateContext& updateContext) {

                        for (auto [entity, lifetime] : query) {
                            lifetime->tick(updateContext.deltaTime());

                            if (lifetime->isExpired())
                                commandBuffer.add<LifetimeCommand<ParticleHandle>>(
                                LifetimeCommand<ParticleHandle>(
                                    entity.handle(),
                                    LifetimeCommandType::LifetimeExpired
                                ));
                        }
                    }
                )
            )
        .endSchedule();

    #include "SchedulerPostSetup.inc"

    gameWorld.init();
    gameLoop.init();

    gameWorld.session().template setStateFrom<EngineState>(StateTransitionContext<EngineState>(
        EngineState::Undefined, EngineState::Booting, EngineStateTransitionId::BootRequest
    ));

    while (gameLoop.isRunning()) {
        framePacer.beginFrame();

        updateFrameMetrics(ecsDataContainer, frameCount, frameTiming, totalTime);
        scheduler.update();

        frameTiming = framePacer.sync();
        fpsMetrics.addFrame(frameTiming);
    }

    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));

    return 0;
}