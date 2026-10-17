/**
 * @file BetaSystem.ixx
 * @brief Integrates local position using local velocity and frame delta time.
 */
module;

#include <iostream>
#include <ostream>

export module thesis.systems.CollectTaggedSpatialResultsSystem;

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

export namespace thesis::systems {


    /**
     * @brief Updates local position by applying local velocity each frame, including AABB bounds and world transform matrix.
     *
     * @tparam TMemberHandle ECS handle type.
     */
    template<typename TMemberHandle>
    class CollectTaggedSpatialResultsSystem {

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

        thesis::config::METRICS& metrics;

    public:

        using HandleType = TMemberHandle;

        explicit CollectTaggedSpatialResultsSystem(thesis::config::METRICS& metrics) : metrics(metrics) {}

        /**
         * @brief Updates an entity's position, including its AABB and world transform matrix.
         */
        void update(
            Query<
                Read<
                    helios::physics::motion::components::Velocity3DComponent<Local>,
                    helios::engine::spatial::components::Position3DComponent<Local>,
                    thesis::components::GameObjectTagComponent
                >,
                Write<>
            > queryGo,
            Query<
                Read<
                    helios::physics::motion::components::Velocity3DComponent<Local>,
                    helios::engine::spatial::components::Position3DComponent<Local>,
                    thesis::components::ParticleTagComponent
                >,
                Write<>
            > queryPrt
        ) noexcept {

            for (auto[
                entity,
                localVelocity,
                localPosition,
                tag
            ]: queryGo
            ) {
                metrics.POSITION_GO.push_back(localPosition->value());
                metrics.VELOCITY_GO.push_back(localVelocity->value());
            }

            for (auto[
                entity,
                localVelocity,
                localPosition,
                tag
            ]: queryPrt
            ) {
                metrics.POSITION_PRT.push_back(localPosition->value());
                metrics.VELOCITY_PRT.push_back(localVelocity->value());
            }

        }


    };


}
