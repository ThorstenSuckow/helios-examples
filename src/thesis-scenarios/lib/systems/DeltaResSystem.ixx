/**
* @file CollisionResponseSystem.ixx
 * @brief Applies collision response functions sequentially to collision pairs.
 */
module;

#include <utility>

export module thesis.systems.DeltaResSystem;

import :CollisionImpulseResponse;

import thesis.types;

import helios.physics.collision.components;
import helios.physics.collision.types;
import helios.physics.motion.components;

import helios.engine.core.types;
import helios.engine.runtime;

import helios.ecs.component;
import helios.ecs.entity;
import helios.ecs.common;

import helios.math;

import helios.engine.spatial.components;

import helios.physics.collision.commands;

import helios.ecs.command.TypedCommandBuffer;

export namespace thesis::systems {


    template<
        typename THandle,
        typename TResponseStrategy = thesis::systems::CollisionImpulseResponse
    >
    class DeltaResSystem {

        using ResponseStrategy = TResponseStrategy;

        using CollisionCommand = helios::physics::collision::commands::CollisionCommand<THandle>;

        using CommandBuffer = helios::ecs::command::TypedCommandBuffer<CollisionCommand>;

        ResponseStrategy responseStrategy_;

    public:

        DeltaResSystem() = default;

        explicit DeltaResSystem(TResponseStrategy&& responseStrategy)
            : responseStrategy_(std::move(responseStrategy)) {
        }


         types::DeltaResResult<THandle> update(
            CommandBuffer& commandBuffer,
            const helios::physics::collision::types::CollisionDetectionResult<THandle>& collisionResult,
            helios::ecs::entity::query::Query<
                THandle,
                helios::ecs::entity::ReadSet<helios::physics::motion::components::Velocity3DComponent<helios::engine::core::types::Local>>,
                helios::ecs::entity::WriteSet<helios::physics::motion::components::Velocity3DComponent<helios::engine::core::types::Local>>
            > query

        ) noexcept {

            const auto& collisions = collisionResult.collisionPairs();

            std::size_t processedCollisions = 0;

            for (const auto& collisionPair : collisions) {

                auto lftResult = query.get(collisionPair.leftHandle);
                auto rgtResult = query.get(collisionPair.rightHandle);

                // stationary collision (most unlikely)
                if (!lftResult && !rgtResult) [[unlikely]] {
                    continue;
                }

                helios::math::vec3f leftVelocity{0.0f, 0.0f, 0.0f};
                helios::math::vec3f rightVelocity{0.0f, 0.0f, 0.0f};

                if (lftResult) {
                    auto [entity, velocity] = *lftResult;
                    leftVelocity = velocity->value();
                }

                if (rgtResult) {
                    auto [entity, velocity] = *rgtResult;
                    rightVelocity = velocity->value();
                }

                auto collisionSnapshot = collisionPair.collisionSnapshot;
                collisionSnapshot.leftDynamic = static_cast<bool>(lftResult);
                collisionSnapshot.rightDynamic = static_cast<bool>(rgtResult);

                auto collisionResponse = responseStrategy_.process(
                    collisionSnapshot,
                    leftVelocity,
                    rightVelocity
                );

                if (!collisionResponse) {
                    continue;
                }

                leftVelocity = collisionResponse->leftVelocity;
                rightVelocity = collisionResponse->rightVelocity;

                if (lftResult) {
                    auto [entity, velocity] = *lftResult;

                    velocity->setValue(
                        leftVelocity
                    );
                }

                if (rgtResult) {
                    auto [entity, velocity] = *rgtResult;
                    velocity->setValue(rightVelocity);
                }

                processedCollisions++;

                commandBuffer.template add<CollisionCommand>(CollisionCommand{
                    .leftHandle = collisionPair.leftHandle,
                    .rightHandle = collisionPair.rightHandle,
                    .type = helios::physics::collision::commands::CollisionCommandType::Impact,
                    .location = collisionPair.collisionSnapshot.overlapCenter
                });
            }
            return types::DeltaResResult<THandle>{processedCollisions};
        }
    };
}