/**
 * @file bootstrap.ixx
 * @brief Engine bootstrap: component registration and GameWorld/GameLoop factory.
 */
module;

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

export module helios.engine.bootstrap;

import helios.ecs;

import helios.engine.runtime;
import helios.engine.runtime.world;
import helios.engine.runtime.messaging;

import helios.engine.scene;
import helios.engine.rendering;
import helios.engine.state;

import helios.engine.platform;
import helios.engine.core;

import helios.gameplay;

import helios.glfw;
import helios.opengl;

using namespace helios::engine::core::thread;
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
using namespace helios::engine::runtime::messaging::command;
using namespace helios::engine::runtime::pooling::commands;
using namespace helios::engine::runtime::pooling::types;




export namespace helios::engine::bootstrap {

    using RenderCommandBuffer  = runtime::messaging::command::TypedCommandBuffer<
            rendering::common::commands::RenderSceneCommand<GameObjectHandle>,
            rendering::common::commands::RenderSceneMemberCommand<GameObjectHandle>,
            rendering::common::commands::RenderInstanceBatchCommand<GameObjectHandle>,
            rendering::common::commands::RenderSceneCommand<ParticleHandle>,
            rendering::common::commands::RenderSceneMemberCommand<ParticleHandle>,
            rendering::common::commands::RenderInstanceBatchCommand<ParticleHandle>,
            rendering::shader::commands::ShaderCompileCommand<rendering::shader::types::ShaderHandle>,
            rendering::shader::commands::ShaderBatchCompileCommand<rendering::shader::types::ShaderHandle>,
            rendering::texture::commands::TextureBatchUploadCommand<rendering::texture::types::TextureHandle>,
            rendering::mesh::commands::MeshBatchUploadCommand<rendering::mesh::types::MeshHandle>
        >;

    using EngineCommandBuffer = helios::engine::runtime::messaging::command::TypedCommandBuffer<
        helios::engine::runtime::timing::commands::TimerControlCommand,
        helios::engine::runtime::lifecycle::commands::WorldLifecycleCommand,
        helios::engine::state::commands::StateCommand<helios::engine::runtime::enginestate::types::EngineState>,
        helios::engine::state::commands::DelayedStateCommand<helios::engine::runtime::enginestate::types::EngineState>
    >;

    using SpawnCommandBuffer  = helios::engine::runtime::messaging::command::TypedCommandBuffer<
        helios::gameplay::spawning::commands::SpawnCommand<helios::engine::runtime::particle::types::ParticleHandle>,
        helios::gameplay::spawning::commands::SpawnCommand<helios::engine::runtime::world::types::GameObjectHandle>,
        helios::gameplay::spawning::commands::SpawnCommand<
            helios::engine::runtime::world::types::GameObjectHandle,
            helios::engine::runtime::particle::types::ParticleHandle
        >
    >;

    using PlatformCommandBuffer = helios::engine::runtime::messaging::command::TypedCommandBuffer<
            // window
            helios::engine::platform::window::commands::WindowCreateCommand<WindowHandle>,
            helios::engine::platform::window::commands::WindowResizeCommand<WindowHandle>,
            helios::engine::platform::window::commands::SwapBuffersCommand<WindowHandle>,
            helios::engine::platform::window::commands::WindowCloseCommand<WindowHandle>,

            // runtime platform
            helios::engine::platform::lifecycle::commands::PlatformInitCommand,
            helios::engine::platform::environment::commands::PollEventsCommand,
            helios::engine::platform::lifecycle::commands::ShutdownCommand
        >;

    using EntityPoolCommandBuffer = TypedCommandBuffer<
        PrefabEntityPoolCommand<GameObjectHandle>,
        PrefabEntityPoolCommand<ParticleHandle>,
        ReleaseEntityCommand<ParticleHandle>
    >;

    using EngineGLFWPlatformManager = helios::glfw::GLFWPlatformManager<
        helios::opengl::OpenGLBackend, WindowHandle, EngineCommandBuffer, PlatformCommandBuffer>;

    using EngineEntityPoolRegistry = runtime::pooling::TypedEntityPoolRegistry<
        helios::ecs::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
    >;

    using EngineSpawnPolicyRegistry = gameplay::spawning::TypedSpawnPolicyRegistry<
        helios::ecs::strategies::HashedLookupStrategy, GameObjectHandle, ParticleHandle
    >;

    using EngineEntityPoolManager = runtime::pooling::EntityPoolManager<EngineEntityPoolRegistry>;

    using EngineWorld = helios::ecs::TypedHandleWorld<
        GameObjectHandle, ParticleHandle,
        scene::types::SceneHandle, rendering::texture::types::TextureHandle,
        rendering::shader::types::ShaderHandle,
        rendering::material::types::MaterialHandle,
        rendering::mesh::types::MeshHandle,
        WindowHandle,
        platform::environment::types::PlatformHandle,
        rendering::renderTarget::types::RenderTargetHandle,
        scene::types::SceneHandle,
        scene::types::CameraHandle, rendering::viewport::types::ViewportHandle
    >;


    using GameObjectEntityManager = ecs::EntityManager<GameObjectHandle>;
    using ParticleEntityManager = ecs::EntityManager<ParticleHandle>;

    /**
     * @brief Creates a pre-configured GameWorld and GameLoop pair.
     *
     * @param jobSystem Reference to the JobSystem used for parallel system execution.
     * @param capacity Initial capacity for the EntityManager's SparseSets.
     *                 Must be large enough to accommodate all entities including
     *                 pooled clones. Defaults to ENTITY_MANAGER_DEFAULT_CAPACITY.
     *
     * @return A pair of (GameWorld, GameLoop) unique pointers.
     *
     * @see GameWorld
     * @see GameLoop
     * @see EngineCommandBuffer
     * @see Session::trackState
     */
    inline std::pair<std::unique_ptr<GameWorld>, std::unique_ptr<GameLoop>> bootstrapGameWorld(
        JobSystem& jobSystem,
        const size_t capacity = 1000
    ) {
        auto gameWorld = std::make_unique<helios::engine::runtime::world::GameWorld>(jobSystem, capacity);

        auto gameLoop = std::make_unique<helios::engine::runtime::gameloop::GameLoop>(*gameWorld);


        // managers
        gameWorld->registerManager<helios::engine::runtime::lifecycle::WorldLifecycleManager>();

        gameWorld->registerManager<helios::engine::runtime::enginestate::EngineStateManager>(
            helios::engine::runtime::enginestate::rules::DefaultEngineStateTransitionRules::rules());

        gameWorld->registerManager<helios::engine::runtime::timing::TimerManager>();

        // mutation manager
        gameWorld->registerManager<helios::engine::runtime::world::EntityMutationManager<GameObjectEntityManager>>(
            gameWorld->entityManager<GameObjectHandle>(),
            jobSystem
        );
        gameWorld->registerManager<helios::engine::runtime::world::EntityMutationManager<ParticleEntityManager>>(
            gameWorld->entityManager<ParticleHandle>(),
            jobSystem
        );

        gameWorld->session().trackState<helios::engine::runtime::enginestate::types::EngineState>();

        gameWorld->registerCommandBuffer<RenderCommandBuffer>();
        gameWorld->registerCommandBuffer<PlatformCommandBuffer>();
        gameWorld->registerCommandBuffer<EngineCommandBuffer>();
        gameWorld->registerCommandBuffer<EntityPoolCommandBuffer>();
        gameWorld->registerCommandBuffer<SpawnCommandBuffer>();

        gameWorld->registerCommandBuffer<EntityMutationCommandBuffer<GameObjectEntityManager>>();
        gameWorld->registerCommandBuffer<EntityMutationCommandBuffer<ParticleEntityManager>>();

        gameWorld->session().setStateFrom<EngineState>(
            StateTransitionContext<EngineState>(
            EngineState::Undefined,
            EngineState::Booting,
            EngineStateTransitionId::BootRequest
        ));


        return std::make_pair(std::move(gameWorld), std::move(gameLoop));
    }

}

