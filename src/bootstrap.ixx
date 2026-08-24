/**
 * @file bootstrap.ixx
 * @brief Engine bootstrap: component registration and GameWorld/GameLoop factory.
 */
module;

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>

export module helios.engine.bootstrap;

import helios.ecs;
import helios.core.thread;
import helios.core.common.container;

import helios.core.io;

import helios.engine.runtime;
import helios.engine.runtime.world.UpdateContext;

import helios.engine.runtime.messaging;

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
using namespace helios::engine::runtime::world;
using namespace helios::engine::runtime::world::types;
using namespace helios::engine::runtime::particle;
using namespace helios::engine::runtime::particle::types;
using namespace helios::engine::runtime::gameloop;
using namespace helios::ecs;
using namespace helios::ecs::command;
using namespace helios::engine::runtime::pooling::commands;
using namespace helios::engine::runtime::pooling::types;




export namespace helios::engine::bootstrap {


    using DefaultEntityPoolRegistry = runtime::pooling::TypedEntityPoolRegistry<
        helios::core::common::container::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
    >;

    using DefaultSpawnPolicyRegistry = gameplay::spawning::TypedSpawnPolicyRegistry<
        helios::core::common::container::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
    >;


    template<typename ... THandles>
    struct WorldFactory {
        static EcsWorld makeEcsWorld() {
            return helios::ecs::EcsWorld::make<THandles...>();
        }

        static void updateResourceRegistry(GameWorld& gameWorld) {
            (gameWorld.resourceRegistry().bind(gameWorld.template entityManager<THandles>()), ... );
        }
    };

    using EngineWorldFactory = WorldFactory<
        GameObjectHandle,
        ParticleHandle,
        scene::types::SceneHandle,
        rendering::renderTarget::types::RenderTargetHandle,
        scene::types::CameraHandle,
        rendering::viewport::types::ViewportHandle,
        rendering::texture::types::TextureHandle,
        rendering::shader::types::ShaderHandle,
        rendering::material::types::MaterialHandle,
        rendering::mesh::types::MeshHandle,
        WindowHandle,
        platform::environment::types::PlatformHandle
    >;



    using DefaultEngineStateManager = helios::engine::runtime::enginestate::EngineStateManager;
    using DefaultTimerManager = helios::engine::runtime::timing::TimerManager;
    using DefaultGLFWPlatformManager = helios::glfw::GLFWPlatformManager<
        helios::opengl::OpenGLBackend, WindowHandle>;
    using DefaultEntityPoolManager = runtime::pooling::EntityPoolManager<DefaultEntityPoolRegistry>;
    using DefaultGameObjectMutationManager = helios::ecs::manager::EntityMutationManager<GameObjectHandle>;
    using DefaultParticleMutationManager = helios::ecs::manager::EntityMutationManager<ParticleHandle>;
    using DefaultSpawnManager = SpawnManager<DefaultSpawnPolicyRegistry, DefaultEntityPoolRegistry>;
    using DefaultRenderManager = helios::engine::rendering::RenderManager<helios::opengl::OpenGLBackend, ParticleHandle>;
    using DefaultTextureUploadManager = helios::opengl::OpenGLTextureUploadManager<>;
    using DefaultMeshUploadManager = helios::opengl::OpenGLMeshUploadManager<>;
    using DefaultShaderCompileManager = helios::opengl::OpenGLShaderCompileManager<helios::opengl::OpenGLUniformLocationCacheStrategy<>>;


    struct EngineRuntime {
        GameWorld gameWorld;
        GameLoop gameLoop;

        explicit EngineRuntime(GameWorld&& world):
            gameWorld{std::move(world)},
            gameLoop{gameWorld}
        {}
    };

    /**
     * @brief Creates a pre-configured GameWorld and GameLoop pair.
     *
     * @return A EngineRuntime object.
     */
    [[nodiscard]] EngineRuntime bootstrapGameWorld(
        JobSystem& jobSystem,
        const size_t capacity = 1000
    ) {


        auto engineRuntime = EngineRuntime(
            GameWorld{EngineWorldFactory::makeEcsWorld(), jobSystem}
        );

        auto& gameWorld = engineRuntime.gameWorld;

        EngineWorldFactory::updateResourceRegistry(gameWorld);

        gameWorld.registerManager<DefaultEngineStateManager>(
            helios::engine::runtime::enginestate::rules::DefaultEngineStateTransitionRules::rules());
        gameWorld.template registerManager<DefaultTimerManager>();
        gameWorld.template registerManager<DefaultGameObjectMutationManager>(jobSystem);
        gameWorld.template registerManager<DefaultParticleMutationManager>(jobSystem);

        auto& renderBackend = gameWorld.emplaceResource<helios::opengl::OpenGLBackend>(gameWorld.ecsWorld());
        auto& spawnPolicy = gameWorld.emplaceResource<DefaultSpawnPolicyRegistry>();
        auto& entityPoolRegistry = gameWorld.emplaceResource<DefaultEntityPoolRegistry>();

        gameWorld.registerManager<DefaultEntityPoolManager>(entityPoolRegistry, gameWorld.ecsWorld(), jobSystem);
        gameWorld.registerManager<DefaultSpawnManager>(spawnPolicy, entityPoolRegistry);
        gameWorld.registerManager<DefaultGLFWPlatformManager>(renderBackend, gameWorld.ecsWorld());
        gameWorld.registerManager<DefaultRenderManager>(renderBackend);

        gameWorld.registerManager<DefaultTextureUploadManager>(gameWorld.ecsWorld(), helios::core::io::ImageReader{});
        gameWorld.registerManager<DefaultMeshUploadManager>(gameWorld.ecsWorld());

        gameWorld.session().template trackState<helios::engine::runtime::enginestate::types::EngineState>();

        gameWorld.registerManager<DefaultShaderCompileManager>(
            gameWorld.ecsWorld(), helios::opengl::OpenGLUniformLocationCacheStrategy<>()
        );

        return engineRuntime;
    }

}

