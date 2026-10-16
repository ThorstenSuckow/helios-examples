/**
 * @file bootstrap.ixx
 * @brief Engine bootstrap: component registration and GameWorld/GameLoop factory.
 */
module;

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

export module helios.engine.v1_bootstrap;

import helios.ecs;
import helios.core.thread;
import helios.core.common.container;

import helios.core.io;

import helios.engine.runtime;

import helios.engine.scene;
import helios.engine.rendering;
import helios.engine.state;

import helios.engine.platform;
import helios.engine.core;

import helios.gameplay;

import helios.glfw;
import helios.opengl;

using namespace helios::core::thread;
using namespace helios::engine::state::types;
using namespace helios::engine::runtime::enginestate::types;
using namespace helios::engine::platform::environment;
using namespace helios::engine::platform::window;
using namespace helios::engine::platform::window::types;
using namespace helios::gameplay::spawning;
using namespace helios::gameplay::spawning::types;
using namespace helios::engine::runtime;
using namespace helios::engine::runtime::gameloop;
using namespace helios::ecs;
using namespace helios::ecs::entity;
using namespace helios::ecs::command;
using namespace helios::engine::runtime::pooling::commands;
using namespace helios::engine::runtime::pooling::types;

export namespace helios::engine::v1_bootstrap {

struct UniHandleDomain {};
using UniHandle = helios::ecs::common::types::EntityHandle<UniHandleDomain>;

template <typename... THandles>
struct WorldFactory {
    static EntityWorld makeEcsWorld() {
        return helios::ecs::entity::EntityWorld::make<THandles...>();
    }

    static void initPermaResources(GameWorld& gameWorld) {
        (gameWorld.resourceRegistry().bind(gameWorld.template entityManager<THandles>()), ...);
    }
};


using EngineWorldFactory = WorldFactory<
    UniHandle
>;


struct EngineRuntime {
    GameWorld gameWorld;
    GameLoop gameLoop;

    EngineRuntime(const EngineRuntime&) = delete;
    EngineRuntime& operator=(const EngineRuntime&) = delete;
    EngineRuntime(EngineRuntime&&) = delete;
    EngineRuntime& operator=(EngineRuntime&&) = delete;

    explicit EngineRuntime(EntityWorld&& ecsWorld)
        : gameWorld{std::move(ecsWorld)}, gameLoop{gameWorld} {}
};


[[nodiscard]] std::unique_ptr<EngineRuntime> bootstrapGameWorld(size_t capacity = 1000) {

    auto engineRuntime = std::make_unique<EngineRuntime>(EngineWorldFactory::makeEcsWorld());

    auto& gameWorld = engineRuntime->gameWorld;

    EngineWorldFactory::initPermaResources(gameWorld);

    return engineRuntime;
}

} // namespace helios::engine::bootstrap
