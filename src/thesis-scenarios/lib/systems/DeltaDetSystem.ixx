/**
 * @file GridCollisionDetectionSystem.ixx
 * @brief Spatial-partitioning based collision detection using a uniform 3D grid.
 */
module;

#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <helios-physics-config.h>
#include <unordered_set>
#include <vector>
#include <cstddef>


export module thesis.systems.DeltaDetSystem;

export import :GridCollisionDetection;

import helios.physics.collision.components;
import helios.physics.collision.types;
import helios.physics.motion.components;

import helios.engine.core.types;

import helios.ecs.component;
import helios.ecs.entity;
import helios.ecs.common;

import helios.core.thread;


import helios.math;

import helios.engine.spatial.components;

export namespace thesis::systems {

    template<
        typename THandle,
        typename TStrategy = thesis::systems::GridCollisionDetection<THandle>
    >
    class DeltaDetSystem {

        using DetectionStrategy = TStrategy;

        using CollisionComponent = helios::physics::collision::components::CollisionComponent;
        using WorldBoundsComponent = helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::World>;

        DetectionStrategy strategy_;

    public:



        explicit DeltaDetSystem(TStrategy&& strategy)
        : strategy_(std::move(strategy)) {
        }


        helios::physics::collision::types::CollisionDetectionResult<THandle> update(
            helios::ecs::entity::query::Query<
                THandle,
                helios::ecs::entity::ReadSet<
                    CollisionComponent,
                    WorldBoundsComponent
                >
            > query,
            helios::core::thread::ThreadPool& threadPool
        ) noexcept {

            strategy_.reset();

            for (auto [entity, cc, wb] : query) {
                strategy_.addCollisionCandidate(entity.handle(), wb->value());
            }

            return strategy_.solveCollisions(threadPool);
        }



    };


}
