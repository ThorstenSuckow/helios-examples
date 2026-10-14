module;

#include <functional>
#include <span>
#include <utility>
#include <vector>

export module thesis.manager.CollisionManager;

import helios.physics.collision.commands;
import helios.ecs.command;
import helios.ecs.entity.EntityManager;
import helios.engine.runtime.gameloop.types;

export namespace thesis::manager {

    template <typename TEmitterHandle, typename TTargetHandle>
    class CollisionManager {
    public:
        using CollisionCommand = helios::physics::collision::commands::CollisionCommand<TEmitterHandle>;
        using CollisionCommands = std::span<const CollisionCommand>;
        using ExecuteLambda = std::function<void(
            helios::ecs::entity::EntityManager<TTargetHandle>&,
            CollisionCommands,
            const helios::engine::runtime::gameloop::types::UpdateContext&
        )>;

    private:
        ExecuteLambda executeLambda_;
        std::vector<CollisionCommand> pendingCommands_;

        std::vector<std::size_t>& COLLISION_COMMANDS_;

        void measure(std::size_t count) noexcept {
            COLLISION_COMMANDS_.push_back(count);
        }

    public:
        CollisionManager() = delete;

        explicit CollisionManager(ExecuteLambda executeLambda, std::vector<std::size_t>& COLLISION_COMMANDS)
            : executeLambda_(std::move(executeLambda)),
              pendingCommands_{},
              COLLISION_COMMANDS_(COLLISION_COMMANDS) {
        }

        bool init(helios::ecs::command::CommandHandlerRegistry& commandHandlerRegistry) noexcept {
            commandHandlerRegistry.handleCommands<CollisionCommand>(*this);
            return true;
        }

        void reset() noexcept {
            pendingCommands_.clear();
        }

        bool submit(CollisionCommand&& command) noexcept {
            pendingCommands_.push_back(std::move(command));
            return true;
        }

        bool execute(
            helios::ecs::entity::EntityManager<TTargetHandle>& targetEntityManager,
            const helios::engine::runtime::gameloop::types::UpdateContext& updateContext
        ) noexcept {
            measure(pendingCommands_.size());
            executeLambda_(targetEntityManager, pendingCommands_, updateContext);
            pendingCommands_.clear();
            return true;
        }
    };

}





