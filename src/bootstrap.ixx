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

export namespace helios::engine::bootstrap {

struct GameObjectHandleDomain {};
using GameObjectHandle = helios::ecs::common::types::EntityHandle<GameObjectHandleDomain>;

struct PlatformHandleDomain {};
using PlatformHandle = helios::ecs::common::types::EntityHandle<PlatformHandleDomain>;

struct WindowHandleDomain {};
using WindowHandle = helios::ecs::common::types::EntityHandle<WindowHandleDomain>;

struct RenderTargetHandleDomain {};
using RenderTargetHandle = helios::ecs::common::types::EntityHandle<RenderTargetHandleDomain>;

struct ViewportHandleDomain {};
using ViewportHandle = helios::ecs::common::types::EntityHandle<ViewportHandleDomain>;

struct TextureHandleDomain {};
using TextureHandle = helios::ecs::common::types::EntityHandle<TextureHandleDomain>;

struct ShaderHandleDomain {};
using ShaderHandle = helios::ecs::common::types::EntityHandle<ShaderHandleDomain>;

struct MaterialHandleDomain {};
using MaterialHandle = helios::ecs::common::types::EntityHandle<MaterialHandleDomain>;

struct MeshHandleDomain {};
using MeshHandle = helios::ecs::common::types::EntityHandle<MeshHandleDomain>;

struct SceneHandleDomain {};
using SceneHandle = helios::ecs::common::types::EntityHandle<SceneHandleDomain>;

struct ParticleHandleDomain {};
using ParticleHandle = helios::ecs::common::types::EntityHandle<ParticleHandleDomain>;

struct CameraHandleDomain {};
using CameraHandle = helios::ecs::common::types::EntityHandle<CameraHandleDomain>;

template <typename... THandles>
struct WorldFactory {
    static EntityWorld makeEcsWorld() {
        return helios::ecs::entity::EntityWorld::make<THandles...>();
    }

    static void updateResourceRegistry(GameWorld& gameWorld) {
        (gameWorld.resourceRegistry().bind(gameWorld.template entityManager<THandles>()), ...);
    }
};

using DefaultRenderHandles = helios::engine::rendering::RenderHandles<
    RenderTargetHandle,
    ViewportHandle,
    SceneHandle,
    CameraHandle,
    ShaderHandle,
    MaterialHandle,
    TextureHandle,
    MeshHandle
>;

using EngineWorldFactory = WorldFactory<
    GameObjectHandle,
    ParticleHandle,
    WindowHandle,
    PlatformHandle,

    SceneHandle,
    RenderTargetHandle,
    CameraHandle,
    ViewportHandle,
    TextureHandle,
    ShaderHandle,
    MaterialHandle,
    MeshHandle
>;

using DefaultTimerManager = helios::engine::runtime::timing::TimerManager;
using DefaultGLFWPlatformManager = helios::glfw::GLFWPlatformManager<WindowHandle, DefaultRenderHandles>;
using DefaultGameObjectMutationManager = helios::ecs::manager::EntityMutationManager<GameObjectHandle>;
using DefaultParticleMutationManager = helios::ecs::manager::EntityMutationManager<ParticleHandle>;
using DefaultRenderManager =
    helios::engine::rendering::RenderManager<DefaultRenderHandles, ParticleHandle, GameObjectHandle>;
using DefaultTextureUploadManager = helios::opengl::OpenGLTextureUploadManager<TextureHandle>;
using DefaultMeshUploadManager = helios::opengl::OpenGLMeshUploadManager<MeshHandle>;
using DefaultShaderCompileManager = helios::opengl::OpenGLShaderCompileManager<ShaderHandle>;

using DefaultWarmupDoneSystem = platform::lifecycle::systems::WarmupDoneSystem<DefaultRenderHandles>;

template <typename TMemberHandle, typename TSubmissionMode>
using DefaultSceneRenderSystem =
    scene::systems::SceneRenderSystem<TMemberHandle, TSubmissionMode, DefaultRenderHandles>;

template <typename TMemberHandle, typename TSubmissionMode, typename TCullingStrategy>
using DefaultSceneMemberVisibilitySystem = scene::systems::SceneMemberVisibilitySystem<
    TMemberHandle,
    TSubmissionMode,
    scene::AABBCullingStrategy<TMemberHandle>,
    DefaultRenderHandles
>;

template <typename TMemberHandle, typename TSubmissionMode>
using DefaultSceneMemberVisibilityRegistry =
    scene::SceneMemberVisibilityRegistry<TMemberHandle, TSubmissionMode, DefaultRenderHandles>;

template <typename TMemberHandle, typename TSubmissionMode>
using DefaultRenderPrototypeComponent =
    rendering::common::components::RenderPrototypeComponent<TMemberHandle, TSubmissionMode, DefaultRenderHandles>;

template <typename TMemberHandle>
using DefaultRenderTargetBindingComponent =
    helios::engine::rendering::common::components::RenderTargetBindingComponent<TMemberHandle, DefaultRenderHandles>;

template <typename TMemberHandle>
using DefaultSceneBindingComponent =
    helios::engine::scene::components::SceneBindingComponent<TMemberHandle, DefaultRenderHandles>;

template <typename TMemberHandle>
using DefaultCameraBindingComponent =
    helios::engine::scene::components::CameraBindingComponent<TMemberHandle, DefaultRenderHandles>;

template <typename TMemberHandle>
using DefaultSceneMemberComponent =
    helios::engine::scene::components::SceneMemberComponent<TMemberHandle, DefaultRenderHandles>;

struct EngineRuntime {
    GameWorld gameWorld;
    GameLoop gameLoop;

    EngineRuntime(const EngineRuntime&) = delete;
    EngineRuntime& operator=(const EngineRuntime&) = delete;
    EngineRuntime(EngineRuntime&&) = delete;
    EngineRuntime& operator=(EngineRuntime&&) = delete;

    explicit EngineRuntime(EntityWorld&& ecsWorld, JobSystem& jobSystem)
        : gameWorld{std::move(ecsWorld), jobSystem}, gameLoop{gameWorld} {}
};

/**
 * @brief Creates a pre-configured GameWorld and GameLoop pair.
 *
 * @return A EngineRuntime object.
 */
[[nodiscard]] std::unique_ptr<EngineRuntime> bootstrapGameWorld(JobSystem& jobSystem, size_t capacity = 1000) {

    auto engineRuntime = std::make_unique<EngineRuntime>(EngineWorldFactory::makeEcsWorld(), jobSystem);

    auto& gameWorld = engineRuntime->gameWorld;

    EngineWorldFactory::updateResourceRegistry(gameWorld);

    gameWorld.emplaceResource<helios::engine::state::StateTransitionRules<EngineState>>(
        runtime::enginestate::rules::DefaultEngineStateTransitionRules{}
    );

    gameWorld.emplaceResource<rendering::RenderDataResolver>(
        helios::opengl::OpenGLRenderDataResolver<DefaultRenderHandles>{}
    );
    gameWorld.emplaceResource<rendering::RenderBackend>(rendering::RenderBackend{opengl::OpenGLBackend{}});
    gameWorld.emplaceResource<SpawnPolicyRegistry>();

    return engineRuntime;
}

} // namespace helios::engine::bootstrap
