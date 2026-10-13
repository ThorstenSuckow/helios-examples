        auto collisionManagerCallback = [&](
            EntityManager<GameObjectHandle>& entityManager,
            EntityManager<ParticleHandle>& targetEntityManager,
            std::span<const helios::physics::collision::commands::CollisionCommand<GameObjectHandle>> collisionCommands
            ) {
                for (const auto& collision : collisionCommands) {
                    thesis::util::createParticles<ParticleHandle>(
                        targetEntityManager,
                        ((collision.leftHandle.entityId() << 32) |  collision.rightHandle.entityId()),
                    PARTICLE_SPAWN_COUNT,
                    collision.location,
                    particleRenderCallback
                    );
                }
            };

    auto updateFrameMetrics = [](auto& ecsDataContainer, auto&frameCount, auto& frameTiming, auto &totalTime) {
        totalTime += frameTiming.totalFrameTime;
        frameCount++;
        auto updateContext = UpdateContext(
              frameTiming.totalFrameTime,
              totalTime,
              frameCount,
              {}
        );
        ecsDataContainer.template replace<UpdateContext>(updateContext);
    };