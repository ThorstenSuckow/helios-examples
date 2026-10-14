/**
 * @file LambdaSystem.ixx
 * @brief Ticks lifetime components and emits LifetimeExpired commands.
 */
module;

export module thesis.systems.LambdaSystem;

import helios.ecs.command.TypedCommandBuffer;

import helios.ecs.entity.QueryAccessSet;
import helios.ecs.entity.query.Query;

import helios.engine.runtime.gameloop.types;

import helios.gameplay.lifecycle.components;
import helios.gameplay.lifecycle.commands;

export namespace thesis::systems {

    /**
     * @brief Advances the age of all entities with a LifetimeComponent and emits a
     * LifetimeCommand once the lifetime has expired.
     *
     * @tparam TMemberHandle ECS handle type.
     */
    template<typename TMemberHandle>
    class LambdaSystem {

        using UpdateContext = helios::engine::runtime::gameloop::types::UpdateContext;

        using LifetimeComponent = helios::gameplay::lifecycle::components::LifetimeComponent;
        using LifetimeCommand = helios::gameplay::lifecycle::commands::LifetimeCommand<TMemberHandle>;
        using LifetimeCommandType = helios::gameplay::lifecycle::commands::LifetimeCommandType;

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
         * @brief Ticks all lifetimes and emits LifetimeExpired commands for expired entities.
         *
         * @param commandBuffer Command buffer receiving LifetimeCommands.
         * @param query Frame-local query over LifetimeComponent.
         * @param updateContext Frame update context containing delta time.
         */
        void update(
            helios::ecs::command::TypedCommandBuffer<LifetimeCommand>& commandBuffer,
            Query<
                Read<LifetimeComponent>,
                Write<LifetimeComponent>
            > query,
            const UpdateContext& updateContext
        ) noexcept {

            for (auto [entity, lifetime] : query) {
                lifetime->tick(updateContext.deltaTime());

                if (lifetime->isExpired()) {
                    commandBuffer.template add<LifetimeCommand>(
                        LifetimeCommand{entity.handle(), LifetimeCommandType::LifetimeExpired}
                    );
                }
            }
        }
    };

}

