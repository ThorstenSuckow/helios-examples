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
using namespace helios::engine::runtime::particle;
using namespace helios::engine::runtime::particle::types;
using namespace helios::engine::runtime::gameloop;
using namespace helios::ecs;
using namespace helios::ecs::command;
using namespace helios::engine::runtime::pooling::commands;
using namespace helios::engine::runtime::pooling::types;




export namespace helios::engine::bootstrap {


    struct GameObjectHandleDomain{};
    using GameObjectHandle = helios::ecs::common::types::EntityHandle<GameObjectHandleDomain>;


    template<typename ... THandles>
    struct WorldFactory {
        static EcsWorld makeEcsWorld() {
            return helios::ecs::EcsWorld::make<THandles...>();
        }

        static void updateResourceRegistry(GameWorld& gameWorld) {
            (gameWorld.resourceRegistry().bind(gameWorld.template entityManager<THandles>()), ... );
        }
    };

    struct RenderHandleList {
        using RenderTargetHandleType = rendering::renderTarget::types::RenderTargetHandle;
        using ViewportHandleType = rendering::viewport::types::ViewportHandle;
        using TextureHandleType = rendering::texture::types::TextureHandle;
        using ShaderHandleType = rendering::shader::types::ShaderHandle;
        using MaterialHandleType = rendering::material::types::MaterialHandle;
        using MeshHandleType = rendering::mesh::types::MeshHandle;
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
    using DefaultGLFWPlatformManager = helios::glfw::GLFWPlatformManager<WindowHandle>;
    using DefaultGameObjectMutationManager = helios::ecs::manager::EntityMutationManager<GameObjectHandle>;
    using DefaultParticleMutationManager = helios::ecs::manager::EntityMutationManager<ParticleHandle>;
    using DefaultRenderManager = helios::engine::rendering::RenderManager<ParticleHandle>;
    using DefaultTextureUploadManager = helios::opengl::OpenGLTextureUploadManager<>;
    using DefaultMeshUploadManager = helios::opengl::OpenGLMeshUploadManager<>;
    using DefaultShaderCompileManager = helios::opengl::OpenGLShaderCompileManager<rendering::shader::types::ShaderHandle>;


    struct EngineRuntime {
        GameWorld gameWorld;
        GameLoop gameLoop;

        EngineRuntime(const EngineRuntime&) = delete;
        EngineRuntime& operator=(const EngineRuntime&) = delete;
        EngineRuntime(EngineRuntime&&) = delete;
        EngineRuntime& operator=(EngineRuntime&&) = delete;


        explicit EngineRuntime(EcsWorld&& ecsWorld, JobSystem& jobSystem):
            gameWorld{std::move(ecsWorld), jobSystem},
            gameLoop{gameWorld}
        {}
    };

    /**
     * @brief Creates a pre-configured GameWorld and GameLoop pair.
     *
     * @return A EngineRuntime object.
     */
    [[nodiscard]] std::unique_ptr<EngineRuntime> bootstrapGameWorld(
        JobSystem& jobSystem,
        const size_t capacity = 1000
    ) {


        auto engineRuntime = std::make_unique<EngineRuntime>(
            EngineWorldFactory::makeEcsWorld(), jobSystem
        );

        auto& gameWorld = engineRuntime->gameWorld;

        EngineWorldFactory::updateResourceRegistry(gameWorld);

        gameWorld.registerManager<DefaultEngineStateManager>(
            runtime::enginestate::rules::DefaultEngineStateTransitionRules::rules());

        gameWorld.emplaceResource<rendering::RenderDataResolver>(
            rendering::RenderDataResolver{
                helios::opengl::OpenGLRenderDataResolver<RenderHandleList>{}
        });
        gameWorld.emplaceResource<rendering::RenderBackend>(
            rendering::RenderBackend{opengl::OpenGLBackend{}}
        );
        gameWorld.emplaceResource<SpawnPolicyRegistry>();


        return engineRuntime;
    }

}

