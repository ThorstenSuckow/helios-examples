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
import helios.core.container;

import helios.core.io;

import helios.engine.runtime;
import helios.engine.runtime.world.UpdateContext;
import helios.engine.runtime.world.ContextProvider;

import helios.engine.runtime.world.types.Contexts;

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


    using CommandBufferFlushContext = helios::engine::runtime::world::types::CommandBufferFlushContext;
    using DefaultInitContext = helios::engine::runtime::world::types::DefaultInitContext;
    using ManagerExecutionContext = helios::engine::runtime::world::types::ManagerExecutionContext;

    using EngineCommandBuffer  = helios::ecs::command::TypedCommandBuffer<
        DefaultInitContext, CommandBufferFlushContext,
        rendering::common::commands::RenderSceneCommand<GameObjectHandle>,
        rendering::common::commands::RenderSceneMemberCommand<GameObjectHandle>,
        rendering::common::commands::RenderInstanceBatchCommand<GameObjectHandle>,
        rendering::common::commands::RenderSceneCommand<ParticleHandle>,
        rendering::common::commands::RenderSceneMemberCommand<ParticleHandle>,
        rendering::common::commands::RenderInstanceBatchCommand<ParticleHandle>,
        rendering::shader::commands::ShaderCompileCommand<rendering::shader::types::ShaderHandle>,
        rendering::shader::commands::ShaderBatchCompileCommand<rendering::shader::types::ShaderHandle>,
        rendering::texture::commands::TextureBatchUploadCommand<rendering::texture::types::TextureHandle>,
        rendering::mesh::commands::MeshBatchUploadCommand<rendering::mesh::types::MeshHandle>,
        helios::engine::runtime::timing::commands::TimerControlCommand,
        helios::engine::state::commands::StateCommand<helios::engine::runtime::enginestate::types::EngineState>,
        helios::engine::state::commands::DelayedStateCommand<helios::engine::runtime::enginestate::types::EngineState>,
        helios::gameplay::spawning::commands::SpawnCommand<helios::engine::runtime::particle::types::ParticleHandle>,
        helios::gameplay::spawning::commands::SpawnCommand<helios::engine::runtime::world::types::GameObjectHandle>,
        helios::gameplay::spawning::commands::SpawnCommand<
            helios::engine::runtime::world::types::GameObjectHandle,
            helios::engine::runtime::particle::types::ParticleHandle
        >,
        // window
        helios::engine::platform::window::commands::WindowCreateCommand<WindowHandle>,
        helios::engine::platform::window::commands::WindowResizeCommand<WindowHandle>,
        helios::engine::platform::window::commands::SwapBuffersCommand<WindowHandle>,
        helios::engine::platform::window::commands::WindowCloseCommand<WindowHandle>,

        // runtime platform
        helios::engine::platform::lifecycle::commands::PlatformInitCommand,
        helios::engine::platform::environment::commands::PollEventsCommand,
        helios::engine::platform::lifecycle::commands::ShutdownCommand,
        PrefabEntityPoolCommand<GameObjectHandle>,
        PrefabEntityPoolCommand<ParticleHandle>,
        ReleaseEntityCommand<ParticleHandle>
    >;

    using DefaultEntityPoolRegistry = runtime::pooling::TypedEntityPoolRegistry<
        helios::core::container::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
    >;

    using DefaultSpawnPolicyRegistry = gameplay::spawning::TypedSpawnPolicyRegistry<
        helios::core::container::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
    >;


    auto EngineEcsWorld = helios::ecs::EcsWorld::make<
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
    >();



    using DefaultEngineStateManager = helios::engine::runtime::enginestate::EngineStateManager<DefaultInitContext, ManagerExecutionContext>;
    using DefaultTimerManager = helios::engine::runtime::timing::TimerManager<DefaultInitContext, ManagerExecutionContext>;
    using DefaultGLFWPlatformManager = helios::glfw::GLFWPlatformManager<
        helios::opengl::OpenGLBackend, WindowHandle, DefaultInitContext, ManagerExecutionContext, EngineCommandBuffer>;
    using DefaultEntityPoolManager = runtime::pooling::EntityPoolManager<DefaultEntityPoolRegistry, DefaultInitContext, ManagerExecutionContext>;
    using DefaultGameObjectMutationManager = helios::ecs::manager::EntityMutationManager<
            GameObjectHandle, DefaultInitContext, ManagerExecutionContext
    >;
    using DefaultParticleMutationManager = helios::ecs::manager::EntityMutationManager<
            ParticleHandle, DefaultInitContext, ManagerExecutionContext
    >;
    using DefaultSpawnManager = SpawnManager<DefaultSpawnPolicyRegistry, DefaultEntityPoolRegistry, DefaultInitContext,
        ManagerExecutionContext
    >;
    using DefaultRenderManager = helios::engine::rendering::RenderManager<
        helios::opengl::OpenGLBackend, DefaultInitContext, ManagerExecutionContext,
        ParticleHandle
    >;
    using DefaultTextureUploadManager = helios::opengl::OpenGLTextureUploadManager<DefaultInitContext, ManagerExecutionContext>;
    using DefaultMeshUploadManager = helios::opengl::OpenGLMeshUploadManager<DefaultInitContext, ManagerExecutionContext>;
    using DefaultShaderCompileManager = helios::opengl::OpenGLShaderCompileManager<DefaultInitContext, ManagerExecutionContext,
        helios::opengl::OpenGLUniformLocationCacheStrategy<>
    >;


    class DefaultContextProvider {
        GameWorld& gameWorld_;

        helios::core::container::TypeMap<ecs::common::types::ContextTypeId::DomainType> typeMap_;

        [[nodiscard]] ecs::common::types::ContextRef getImpl(const ecs::common::types::ContextTypeId typeId, UpdateContext* updateContext) {
            const auto idx = typeId.value();

            if (typeId == ecs::common::types::ContextTypeId::template id<common::types::NullFlushContext>()) {
                return common::types::ContextRef{typeMap_.getOrEmplace<common::types::NullFlushContext>()};
            }
            if (typeId == ecs::common::types::ContextTypeId::template id<common::types::NullInitContext>()) {
                return common::types::ContextRef{typeMap_.getOrEmplace<common::types::NullInitContext>()};
            }

            if (typeId == ecs::common::types::ContextTypeId::template id<CommandBufferFlushContext>()) {
                return common::types::ContextRef{typeMap_.getOrEmplace<CommandBufferFlushContext>(
                    gameWorld_.commandHandlerRegistry(),
                    gameWorld_.managerRegistry()
                )};
            }


            if (!updateContext) {
                if (typeId == ecs::common::types::ContextTypeId::template id<DefaultInitContext>()) {
                    return common::types::ContextRef{typeMap_.getOrEmplace<DefaultInitContext>(
                        gameWorld_.commandHandlerRegistry()
                    )};
                }
            } else {
                if (typeId == ecs::common::types::ContextTypeId::template id<ManagerExecutionContext>()) {
                    return common::types::ContextRef{typeMap_.getOrEmplace<ManagerExecutionContext>(
                        *updateContext,
                       gameWorld_.ecsWorld()
                    )};
                }
                if (typeId == ecs::common::types::ContextTypeId::template id<SystemUpdateContext>()) {
                    return common::types::ContextRef{typeMap_.getOrEmplace<SystemUpdateContext>(
                        *updateContext
                    )};
                }
            }
            #if !NDEBUG
            std::cerr << "Requested context type not supported: " << typeid(ecs::common::types::ContextTypeId::DomainType).name() << '\n';
            assert(false && "Requested context type is not supported by DefaultContextProvider.");
            #endif
            std::unreachable();
        }


    public:
        explicit DefaultContextProvider(GameWorld& gameWorld): gameWorld_(gameWorld) {}


        [[nodiscard]] ecs::common::types::ContextRef get(const ecs::common::types::ContextTypeId typeId) {
            return getImpl(typeId, nullptr);
        }

        [[nodiscard]] ecs::common::types::ContextRef get(const ecs::common::types::ContextTypeId typeId, UpdateContext& updateContext) {
            return getImpl(typeId, &updateContext);
        }

        bool clear() {
            typeMap_.clear();
            return true;
        }
    };

    struct EngineRuntime {
        GameWorld gameWorld;
        ContextProvider contextProvider;
        GameLoop gameLoop;

        explicit EngineRuntime(GameWorld&& world):
            gameWorld{std::move(world)},
            contextProvider{ContextProvider(DefaultContextProvider(gameWorld))},
            gameLoop{gameWorld, contextProvider}
        {
        }
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
            GameWorld{std::move(EngineEcsWorld), jobSystem}
        );

        auto& gameWorld = engineRuntime.gameWorld;

        gameWorld.registerManager<DefaultEngineStateManager>(
            helios::engine::runtime::enginestate::rules::DefaultEngineStateTransitionRules::rules());
        gameWorld.template registerManager<DefaultTimerManager>();
        gameWorld.template registerManager<DefaultGameObjectMutationManager>(jobSystem);
        gameWorld.template registerManager<DefaultParticleMutationManager>(jobSystem);

        auto& renderBackend = gameWorld.addResource<helios::opengl::OpenGLBackend>(gameWorld.ecsWorld());
        auto& spawnPolicy = gameWorld.addResource<DefaultSpawnPolicyRegistry>();
        auto& entityPoolRegistry = gameWorld.addResource<DefaultEntityPoolRegistry>();

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

