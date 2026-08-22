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

    using DefaultEntityPoolRegistry = runtime::pooling::TypedEntityPoolRegistry<
        helios::core::common::container::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
    >;

    using DefaultSpawnPolicyRegistry = gameplay::spawning::TypedSpawnPolicyRegistry<
        helios::core::common::container::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
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


    class DefaultContextProvider {
        GameWorld& gameWorld_;

        helios::core::common::container::TypeMap<ecs::common::types::ContextTypeId::DomainType> typeMap_;

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

