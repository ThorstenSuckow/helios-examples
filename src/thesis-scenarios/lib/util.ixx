module;

#include <cstdint>
#include <functional>
#include <numbers>

export module thesis.util;

import helios.core.common;
import helios.ecs.entity;
import helios.engine.core;
import helios.math;
import helios.physics;
import helios.engine.spatial;
import thesis.config;
import helios.engine.rendering.mesh;
import helios.gameplay.lifecycle;

export namespace thesis::util {

    helios::math::vec3f randomVec3f(const std::uint64_t seed) {
        auto rand = helios::core::common::Random(seed);
        return {
            rand.randomFloat(-1.0F, 1.0F), 
            rand.randomFloat(-1.0F, 1.0F), 
            rand.randomFloat(-1.0F, 1.0F)
        };
    }

    template<typename THandle>
    void createObjects(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        const int randomizerOffset = 0
    ) {
        createObjects<THandle>(entityManager, [](auto&){}, randomizerOffset);
    }

    template<typename THandle, typename TFunc>
    void createGameObjects(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        TFunc&& func,
        const int randomizerOffset = 0
    ) {
        int NUM_OBJECTS = 0;

        float borderAccount  = thesis::config::BORDER_SCALE[0]/2.0f;
        float startLeft = -thesis::config::AXIS_LENGTH / 2.0F;
        float startBottom = -thesis::config::AXIS_LENGTH / 2.0F - borderAccount;
        float x = startLeft + thesis::config::TILE_SIZE - borderAccount;

        for (int i = 0; i < thesis::config::OBJECTS_PER_AXIS; i++) {

            float y = startBottom + thesis::config::TILE_SIZE;

            for (int j = 0; j < thesis::config::OBJECTS_PER_AXIS; j++) {
                auto handle = entityManager.create();
                auto gameObject = helios::ecs::entity::Entity{handle, &entityManager};
                gameObject.setActive(true);

                gameObject.template add<helios::physics::collision::components::CollisionComponent>();
                gameObject.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>>(
                    helios::engine::rendering::mesh::assets::WireframeCube::boundsData()
                );
                gameObject.template add<helios::engine::spatial::components::BoundsComponent<
                    helios::engine::core::types::World>>();
                gameObject.template add<helios::engine::spatial::components::Rotation3DComponent<
                    helios::engine::core::types::Local>>();
                gameObject.template add<helios::engine::spatial::components::Scale3DComponent<
                    helios::engine::core::types::Local>>(1.0F, 1.0F, 1.0F);
                gameObject.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(
                    static_cast<float>(x), static_cast<float>(y), 0.0F
                );
                gameObject.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::World>>(0.0F, 0.0F, 0.0F);
                gameObject.template add<helios::physics::motion::components::Velocity3DComponent<helios::engine::core::types::Local>>(
                    randomVec3f(NUM_OBJECTS + randomizerOffset).withZ(0.0F).normalize() * thesis::config::GAME_OBJECT_SPEED
                );

                gameObject.template add<helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>>(1.0F);

                std::invoke(func, gameObject);

                NUM_OBJECTS++;

                y += thesis::config::TILE_SIZE;
            }
            x += thesis::config::TILE_SIZE;

        }
    };

    template<typename THandle>
    void createParticles(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        const std::uint64_t key,
        const int count,
        const helios::math::vec3f location
    ) {
        createParticles(entityManager, key, count, location, [](auto&){});
    }
    
    template<typename THandle, typename TFunc>
    void createParticles(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        const std::uint64_t key, 
        const int count, 
        const helios::math::vec3f location,
        TFunc&& func
    ) {

        for (int i = 0; i < count; i++) {
            auto handle = entityManager.create();
            auto particle = helios::ecs::entity::Entity{handle, &entityManager};
            particle.setActive(true);

            particle.template add<helios::physics::collision::components::CollisionComponent>();

            particle.template add<helios::gameplay::lifecycle::components::LifetimeComponent>(thesis::config::PARTICLE_LIFETIME);

            particle.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>>(
                helios::engine::rendering::mesh::assets::WireframeCube::boundsData()
            );
            particle.template add<helios::engine::spatial::components::BoundsComponent<
                helios::engine::core::types::World>>();
            particle.template add<helios::engine::spatial::components::Rotation3DComponent<
                helios::engine::core::types::Local>>();
            particle.template add<helios::engine::spatial::components::Scale3DComponent<
                helios::engine::core::types::Local>>(1.0F, 1.0F, 1.0F);
            particle.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(location);
            particle.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::World>>(0.0F, 0.0F, 0.0F);
            particle.template add<helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>>(1.0F);

            particle.template add<helios::physics::motion::components::Velocity3DComponent<helios::engine::core::types::Local>>(
               randomVec3f(key + i).withZ(0.0F).normalize()*thesis::config::PARTICLE_SPEED
            );

            std::invoke(func, particle);
        }
    };

    template<typename THandle>
    void createBoundaries(helios::ecs::entity::EntityManager<THandle>& entityManager) {
        createBoundaries<THandle>(entityManager, [](auto&){});
    }   
    
    template<typename THandle, typename TFunc>
    void createBoundaries(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        TFunc&& func
    ) {

        constexpr auto borderAccount = config::BORDER_SCALE[0]/2.0f;

        for (int i = 0; i < 4; i++) {
            auto handle = entityManager.create();
            auto border = helios::ecs::entity::Entity{handle, &entityManager};
            border.setActive(true);

            switch (i) {
                case 0: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(0.0F, -config::AXIS_LENGTH/2.0f - borderAccount , 0.0F);
                    break;
                case 1: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(config::AXIS_LENGTH/2.0f + borderAccount , 0.0F, 0.0F);
                    break;
                case 2: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(0.0F, config::AXIS_LENGTH/2.0f + borderAccount , 0.0F);
                    break;
                case 3: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(-config::AXIS_LENGTH/2.0f - borderAccount , 0.0F, 0.0F);
                    break;
            }
            border.template add<helios::engine::spatial::components::Scale3DComponent<helios::engine::core::types::Local>>(config::AXIS_LENGTH , config::BORDER_SCALE[1], config::BORDER_SCALE[2]);
            border.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>>(helios::engine::rendering::mesh::assets::WireframeCube::boundsData());
            border.template add<helios::engine::spatial::components::Rotation3DComponent<helios::engine::core::types::Local>>(
                helios::math::quatf::fromEulerAngles<helios::math::Intrinsic>(
                    0.0f, 0.0f, i* (std::numbers::pi / 2)
                )
            );
            border.template add<helios::physics::collision::components::CollisionComponent>();
            border.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::World>>();
            border.template add<helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>>(1.0F);

            // update fixed component values so we do not require systems
            const auto boundsLocal = border.template get<
                helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>
                >();

            const auto localRotation = border.template get<
                helios::engine::spatial::components::Rotation3DComponent<helios::engine::core::types::Local>
            >();
            const auto localPosition = border.template get<
                helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>
            >();
            const auto worldTransform =  border.template get<
                helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>
            >();

            const auto worldBounds = border.template get<
                helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::World>
            >();

            worldTransform->setValue(localRotation->value().rotationMatrix().withTranslation(
                            localPosition->value())
            );

            worldTransform->setValue(localRotation->value()
                .rotationMatrix()
                .withScaling(helios::math::vec3f{config::AXIS_LENGTH , config::BORDER_SCALE[1], config::BORDER_SCALE[2]})
                .withTranslation(localPosition->value()));

            worldBounds->setValue(boundsLocal->value().applyTransform(worldTransform->value()));


            std::invoke(func, border);

        }
    };
    
    
}
