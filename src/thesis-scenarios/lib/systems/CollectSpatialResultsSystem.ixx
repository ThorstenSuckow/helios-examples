/**
 * @file BetaSystem.ixx
 * @brief Integrates local position using local velocity and frame delta time.
 */
module;

#include <iostream>
#include <ostream>

export module thesis.systems.CollectSpatialResultsSystem;

import helios.physics.motion.components;

import helios.ecs.entity.QueryAccessSet;
import helios.ecs.entity.query.Query;

import helios.math;
import helios.engine.core.types;

import helios.engine.runtime.gameloop.types;

import helios.engine.spatial.components;
import thesis.components;
import thesis.types;

import thesis.config;

import thesis.handles;

export namespace thesis::systems {


    /**
     * @brief Updates local position by applying local velocity each frame, including AABB bounds and world transform matrix.
     *
     * @tparam TMemberHandle ECS handle type.
     */
    template<typename TMemberHandle>
    class CollectSpatialResultsSystem {

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

        std::vector<helios::math::vec3f>& positionSink_;
        std::vector<helios::math::vec3f>& velocitySink_;

    public:

        using HandleType = TMemberHandle;

        explicit CollectSpatialResultsSystem(
            std::vector<helios::math::vec3f>& positionSink,
            std::vector<helios::math::vec3f>& velocitySink
        ) : positionSink_(positionSink), velocitySink_(velocitySink) {}

        /**
         * @brief Updates an entity's position, including its AABB and world transform matrix.
         */
        void update(
            Query<
                Read<
                    helios::physics::motion::components::Velocity3DComponent<Local>,
                    helios::engine::spatial::components::Position3DComponent<Local>
                >,
                Write<>
            > query
        ) noexcept {

            for (auto[
                entity,
                localVelocity,
                localPosition
            ]: query
            ) {
                positionSink_.push_back(localPosition->value());
                velocitySink_.push_back(localVelocity->value());
            }
        }


    };


}
