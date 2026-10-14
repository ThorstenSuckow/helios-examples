/**
 * @file BetaSystem.ixx
 * @brief Integrates local position using local velocity and frame delta time.
 */
module;

#include <iostream>
#include <ostream>

export module thesis.systems.BetaSystem;

import helios.physics.motion.components;

import helios.ecs.entity.QueryAccessSet;
import helios.ecs.entity.query.Query;

import helios.math;
import helios.engine.core.types;

import helios.engine.runtime.gameloop.types;

import helios.engine.spatial.components;

export namespace thesis::systems {

    /**
     * @brief Updates local position by applying local velocity each frame, including AABB bounds and world transform matrix.
     *
     * @tparam TMemberHandle ECS handle type.
     */
    template<typename TMemberHandle>
    class BetaSystem {

        using UpdateContext = helios::engine::runtime::gameloop::types::UpdateContext;

        using Local = helios::engine::core::types::Local;
        using World = helios::engine::core::types::World;

        template<
            typename TRead,
            typename TWrite,
            typename TFilter = helios::ecs::entity::query::Filter<helios::ecs::entity::query::AnyDirty<>>,
            typename TOptional = helios::ecs::entity::query::Optional<>
        >
        using Query = helios::ecs::entity::query::Query<TMemberHandle, TRead, TWrite, TFilter, TOptional>;

        template<typename ... TReads>
        using Read = helios::ecs::entity::ReadSet<TReads...>;

        template<typename ... TWrites>
        using Write = helios::ecs::entity::WriteSet<TWrites...>;

    public:

        using HandleType = TMemberHandle;

        /**
         * @brief Updates an entity's position, including its AABB and world transform matrix.
         */
        void update(
            Query<
                Read<
                    helios::physics::motion::components::Velocity3DComponent<Local>,
                    helios::engine::spatial::components::Position3DComponent<Local>,
                    helios::engine::spatial::components::BoundsComponent<Local>,
                    helios::engine::spatial::components::BoundsComponent<World>,
                    helios::engine::spatial::components::TransformComponent<World>
                >,
                Write<
                    helios::engine::spatial::components::Position3DComponent<Local>,
                    helios::engine::spatial::components::BoundsComponent<World>,
                    helios::engine::spatial::components::TransformComponent<World>
                >
            > query,
            const UpdateContext& updateContext
        ) noexcept {

            for (auto[
                entity,
                localVelocity,
                localPosition,
                boundsLocal,
                boundsWorld,
                worldTransform
            ]: query
            ) {

                const auto position = localPosition->value() + localVelocity->value() * updateContext.deltaTime();
                entity.template track<helios::engine::spatial::components::Position3DComponent<Local>>()
                    ->setValue(position);

                const auto transform = helios::math::mat4f::identity().withTranslation(position);
                entity.template track<helios::engine::spatial::components::TransformComponent<World>>()
                    ->setValue(transform);

                entity.template track<helios::engine::spatial::components::BoundsComponent<World>>()
                    ->setValue(boundsLocal->value().applyTransform(transform));
            }

        }


    };


}