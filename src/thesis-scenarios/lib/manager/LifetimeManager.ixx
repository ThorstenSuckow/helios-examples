module;

#include <vector>

export module thesis.manager.LifetimeManager;

import helios.gameplay.lifecycle.commands;
import helios.ecs.command;
import helios.ecs.entity;

export namespace thesis::manager {

    template <typename THandle>
    class LifetimeManager {
    public:
        using LifetimeCommand = helios::gameplay::lifecycle::commands::LifetimeCommand<THandle>;
        using EntityManager = helios::ecs::entity::EntityManager<THandle>;

    private:
        std::vector<LifetimeCommand> pendingCommands_;

    public:
        LifetimeManager() = default;

        bool init(helios::ecs::command::CommandHandlerRegistry& commandHandlerRegistry) noexcept {
            commandHandlerRegistry.handleCommands<LifetimeCommand>(*this);
            return true;
        }

        void reset() noexcept {
            pendingCommands_.clear();
        }

        bool submit(LifetimeCommand&& command) noexcept {
            pendingCommands_.push_back(std::move(command));
            return true;
        }

        /**
         * @note Not thread-safe!
         */
        bool execute(EntityManager& entityManager) noexcept {
            bool result = true;

            for (const auto& command : pendingCommands_) {
                result = entityManager.destroy(command.handle) && result;
            }

            pendingCommands_.clear();
            return result;
        }
    };

}





