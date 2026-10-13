
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

import helios.engine.v1_bootstrap;



#include <algorithm>
#include <cmath>
#include <optional>

#include "./Namespaces.h"

helios::math::vec3f randomVec3f(const std::uint64_t seed) {

    auto rand = helios::core::common::Random(seed);

    return {rand.randomFloat(-1.0F, 1.0F), rand.randomFloat(-1.0F, 1.0F), rand.randomFloat(-1.0F, 1.0F)};
}

int main() {
    const auto& logger = helios::core::log::LogManager::loggerForScope("main");

    // ========================================
    // Constants
    // ========================================
    constexpr auto OBJECTS_PER_AXIS      = 300;
    constexpr auto BORDER_SCALE          = helios::math::vec3f{1.0F, 1.0F, 1.0F};
    constexpr auto TILE_SIZE             = 2;
    constexpr auto PARTICLE_SPAWN_COUNT  = 2;
    constexpr auto PARTICLE_SPEED        = 32.0F;
    constexpr auto GAME_OBJECT_SPEED     = 1.0f;
    constexpr auto PARTICLE_LIFETIME     = 1.0f;
    constexpr auto AXIS_LENGTH           = TILE_SIZE * OBJECTS_PER_AXIS + TILE_SIZE - 1;
    constexpr auto CELL_SIZE             = AXIS_LENGTH / OBJECTS_PER_AXIS;

    // gameworld
    auto engineRuntime = helios::engine::v1_bootstrap::bootstrapGameWorld();
    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;

    gameWorld.resourceRegistry().emplace<helios::core::thread::ThreadPool>(
        std::max(1U, std::thread::hardware_concurrency() - 1U)
    );

    // boundaries
    auto createBoundaries = [&]<typename THandle>() {

        float borderAccount  = BORDER_SCALE[0]/2.0f;
        for (int i = 0; i < 4; i++) {
            auto bndLeft =  gameWorld.add<THandle>();

            switch (i) {
                case 0: bndLeft.template add<Position3DComponent<Local>>(0.0F, -AXIS_LENGTH/2.0f - borderAccount , 0.0F);
                    break;
                case 1: bndLeft.template add<Position3DComponent<Local>>(AXIS_LENGTH/2.0f + borderAccount , 0.0F, 0.0F);
                    break;
                case 2: bndLeft.template add<Position3DComponent<Local>>(0.0F, AXIS_LENGTH/2.0f + borderAccount , 0.0F);
                    break;
                case 3: bndLeft.template add<Position3DComponent<Local>>(-AXIS_LENGTH/2.0f - borderAccount , 0.0F, 0.0F);
                    break;
            }
            bndLeft.template add<Scale3DComponent<Local>>(AXIS_LENGTH , BORDER_SCALE[1], BORDER_SCALE[2]);
            bndLeft.template add<BoundsComponent<Local>>(WireframeCube::boundsData());
            bndLeft.template add<Rotation3DComponent<Local>>(
                helios::math::quatf::fromEulerAngles<Intrinsic>(
                    0.0f, 0.0f, i* (std::numbers::pi / 2)
                )
            );
            bndLeft.template add<helios::physics::collision::components::CollisionComponent>();
            bndLeft.template add<BoundsComponent<World>>();
            bndLeft.template add<TransformComponent<World>>(1.0F);

        }
    };

    auto createObjects = [&]<typename THandle>(const int randomizerOffset = 0) {
        int NUM_OBJECTS = 0;

        float borderAccount  = BORDER_SCALE[0]/2.0f;
        float startLeft = -AXIS_LENGTH / 2.0F;
        float startBottom = -AXIS_LENGTH / 2.0F - borderAccount;
        float x = startLeft + TILE_SIZE - borderAccount;

        for (int i = 0; i < OBJECTS_PER_AXIS; i++) {

            float y = startBottom + TILE_SIZE;

            for (int j = 0; j < OBJECTS_PER_AXIS; j++) {
                auto cube = gameWorld.add<THandle>();
                cube.template add<helios::physics::collision::components::CollisionComponent>();
                cube.template add<BoundsComponent<Local>>(WireframeCube::boundsData());
                cube.template add<BoundsComponent<World>>();
                cube.template add<Rotation3DComponent<Local>>();
                cube.template add<Scale3DComponent<Local>>(1.0F, 1.0F, 1.0F);
                cube.template add<Position3DComponent<Local>>(
                    static_cast<float>(x), static_cast<float>(y), 0.0F
                );
                cube.template add<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
                cube.template add<helios::physics::motion::components::Velocity3DComponent<Local>>(
                    randomVec3f(NUM_OBJECTS + randomizerOffset).withZ(0.0F).normalize() * GAME_OBJECT_SPEED
                );

                cube.template add<TransformComponent<World>>(1.0F);
                NUM_OBJECTS++;

                y += TILE_SIZE;
            }
            x += TILE_SIZE;

        }

        logger.info("Created {} objects.", NUM_OBJECTS);
    };

    auto createParticles = [&]<typename THandle>(
        EntityManager<THandle>& entityManager,
        std::uint64_t key, int count, helios::math::vec3f location) {

        for (int i = 0; i < count; i++) {
            auto handle = entityManager.create();
            auto particle = entity::Entity{handle, &entityManager};
            particle.setActive(true);
            particle.template add<helios::physics::collision::components::CollisionComponent>();
            particle.template add<BoundsComponent<Local>>(WireframeCube::boundsData());
            particle.template add<BoundsComponent<World>>();
            particle.template add<LifetimeComponent>(PARTICLE_LIFETIME);
            particle.template add<Rotation3DComponent<Local>>();
            particle.template add<Scale3DComponent<Local>>(1.0F, 1.0F, 1.0F);
            particle.template add<Position3DComponent<Local>>(location);
            particle.template add<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
            particle.template add<helios::physics::motion::components::Velocity3DComponent<Local>>(
                randomVec3f(key + i).withZ(0.0F).normalize()*PARTICLE_SPEED
            );

            particle.template add<TransformComponent<World>>(1.0F);
        }
    };


    createObjects.operator()<UniHandle>();
    createBoundaries.operator()<UniHandle>();

    auto spatialGrid = helios::math::aabbf{
        -AXIS_LENGTH / 2.0F, -AXIS_LENGTH / 2.0F, 0.0f,
        AXIS_LENGTH / 2.0F, AXIS_LENGTH / 2.0F, 0.0f
    };

    // ----------------------------------------
    // Logger Configuration
    // ----------------------------------------
    LogManager::getInstance().enableLogging(false);
    LogManager::getInstance().enableSink<helios::core::log::ConsoleSink>();

    auto s_beta = [](
        helios::ecs::entity::query::Query<
            UniHandle,
            helios::ecs::entity::ReadSet<
                Velocity3DComponent<Local>,
                Position3DComponent<Local>
            >,
            helios::ecs::entity::WriteSet<
                Position3DComponent<Local>
            >
        > query,
        const UpdateContext& updateContext) {

        for (auto[
            entity,
            localVelocity,
            localPosition
        ]: query
        ) {

            entity.template track<Position3DComponent<Local>>()
                ->setValue(
                localPosition->value() + localVelocity->value() * updateContext.deltaTime()
            );
        }

    };


    auto s_delta_det = helios::physics::collision::systems::CollisionDetectionSystem<UniHandle,
        helios::physics::collision::strategies::GridCollisionDetection<UniHandle>>(
            helios::physics::collision::strategies::GridCollisionDetection<UniHandle>(
            spatialGrid, CELL_SIZE
        ));
    auto s_delta_res = helios::physics::collision::systems::CollisionResponseSystem<
        UniHandle,
        helios::physics::collision::strategies::CollisionImpulseResponse
    >(
        helios::physics::collision::strategies::CollisionImpulseResponse{}
    );
    
    using m_col = helios::physics::collision::CollisionManager<UniHandle, UniHandle>;
    using m_lft = helios::gameplay::lifecycle::LifetimeManager<UniHandle>;

    auto m_col_inst = [&](
        EntityManager<UniHandle>& entityManager, 
        EntityManager<UniHandle>& targetEntityManager,
        std::span<const helios::physics::collision::commands::CollisionCommand<UniHandle>> collisionCommands) {
        for (const auto& collision : collisionCommands) {
            createParticles.operator()<UniHandle>(
                targetEntityManager,
                ((collision.leftHandle.entityId() << 32) |  collision.rightHandle.entityId()),
                PARTICLE_SPAWN_COUNT,
                collision.location
            );
        }
    };

    auto s_lambda = [](TypedCommandBuffer<LifetimeCommand<UniHandle>>& commandBuffer,
                        Query<UniHandle, ReadSet<LifetimeComponent>, WriteSet<LifetimeComponent>> query, const UpdateContext& updateContext) {

        for (auto [entity, lifetime] : query) {
            lifetime->tick(updateContext.deltaTime());

            if (lifetime->isExpired())
                commandBuffer.add<LifetimeCommand<UniHandle>>(
                LifetimeCommand<UniHandle>(
                    entity.handle(),
                    LifetimeCommandType::LifetimeExpired
                ));
        }
    };


    gameWorld.managerRegistry().add<helios::physics::collision::CollisionManager<UniHandle, UniHandle>>(
        m_col_inst
    );

    helios::ecs::scheduling::Scheduler scheduler(gameWorld.resourceRegistry());
        // PRE

        //
    scheduler.beginSchedule()
        .endSchedule<Sequential<m_col, m_lft>>()
        .beginSchedule()
            .add(s_beta, s_delta_det, s_delta_res, s_lambda);


    gameWorld.init();
    gameLoop.init();

    gameWorld.session().template setStateFrom<EngineState>(StateTransitionContext<EngineState>(
        EngineState::Undefined, EngineState::Booting, EngineStateTransitionId::BootRequest
    ));

    float totalTime = 0.0f;
    std::size_t frameCount = 0;
    auto updateFameMetrics = [&]() {
      
        const GamepadState gamepadState = GamepadState();
        const auto inputSnapshot = InputSnapshot(gamepadState);
        
        totalTime += frameTiming.totalFrameTime;
        frameCount++;
        auto updateContext = UpdateContext(
              frameTiming.totalFrameTime,
              totalTime,
              frameCount,
              {}
        );
        gameWorld.resourceRegistry().emplace<UpdateContext>(updateContext);
    };
    
    while (gameLoop.isRunning()) {
        framePacer.beginFrame();
        updateFameMetrics();
        // Frame Synchronization is now done via GLFWSwapBuffersSystems
        scheduler.update();

        frameTiming = framePacer.sync();
    }

    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));

    return 0;
}