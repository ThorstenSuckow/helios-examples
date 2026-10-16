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


export module thesis.systems.UniDeltaDetSystem;

import thesis.strategies;

import helios.physics.collision.components;
import helios.physics.collision.types;
import helios.physics.motion.components;

import helios.engine.core.types;

import helios.ecs.component;
import helios.ecs.entity;
import helios.ecs.common;

import helios.core.thread;


import helios.math;

import thesis.components;
import thesis.types;

import helios.engine.spatial.components;

export namespace thesis::systems {

    template<
        typename THandle,
        typename TStrategy = thesis::strategies::GridCollisionDetection<THandle>
    >
    class UniDeltaDetSystem {

        using DetectionStrategy = TStrategy;

        using CollisionComponent = helios::physics::collision::components::CollisionComponent;
        using WorldBoundsComponent = helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::World>;

        DetectionStrategy strategyGo_;
        DetectionStrategy strategyPrt_;

    public:



        explicit UniDeltaDetSystem(TStrategy&& strategyGo, TStrategy&& strategyPrt)
        : strategyGo_(std::move(strategyGo)), strategyPrt_(std::move(strategyPrt)) {
        }


        thesis::types::UniCollisionDetectionResult<THandle> update(
            helios::ecs::entity::query::Query<
                THandle,
                helios::ecs::entity::ReadSet<
                    CollisionComponent,
                    WorldBoundsComponent,
                    thesis::components::GameObjectTagComponent
                >
            > queryGo,
            helios::ecs::entity::query::Query<
                THandle,
                helios::ecs::entity::ReadSet<
                    CollisionComponent,
                    WorldBoundsComponent,
                    thesis::components::ParticleTagComponent
                >
            > queryPrt,
            helios::core::thread::ThreadPool& threadPool
        ) noexcept {

            strategyGo_.reset();
            for (auto [entity, cc, wb, tag] : queryGo) {
                strategyGo_.addCollisionCandidate(entity.handle(), wb->value());
            }

            strategyPrt_.reset();
            for (auto [entity, cc, wb, tag] : queryPrt) {
                strategyPrt_.addCollisionCandidate(entity.handle(), wb->value());
            }

            return thesis::types::UniCollisionDetectionResult<THandle>(
                std::move(strategyGo_.solveCollisions(threadPool)),
                std::move(strategyPrt_.solveCollisions(threadPool))
            );
        }



    };


}
