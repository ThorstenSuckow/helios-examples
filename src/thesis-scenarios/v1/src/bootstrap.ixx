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


export namespace helios::engine::v1_bootstrap {

struct UniHandleDomain {};
using UniHandle = helios::ecs::common::types::EntityHandle<UniHandleDomain>;

// All roles share the one unified handle domain.
using WindowHandle       = UniHandle;
using RenderTargetHandle = UniHandle;
using ViewportHandle     = UniHandle;
using TextureHandle      = UniHandle;
using ShaderHandle       = UniHandle;
using MaterialHandle     = UniHandle;
using MeshHandle         = UniHandle;
using SceneHandle        = UniHandle;
using CameraHandle       = UniHandle;

template <typename... THandles>
struct WorldFactory {
    static EntityWorld makeEcsWorld() {
        return helios::ecs::entity::EntityWorld::make<THandles...>();
    }

    static void initPermaResources(GameWorld& gameWorld) {
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

using EngineWorldFactory = WorldFactory<UniHandle>;

using DefaultTimerManager = helios::engine::runtime::timing::TimerManager;
using DefaultGLFWPlatformManager = helios::glfw::GLFWPlatformManager<WindowHandle, DefaultRenderHandles>;
using DefaultRenderManager =
    helios::engine::rendering::RenderManager<DefaultRenderHandles, UniHandle>;
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

template <typename TSubmissionMode>
using DefaultRenderPrototypeComponent =
    rendering::common::components::RenderPrototypeComponent<TSubmissionMode, DefaultRenderHandles>;

using DefaultRenderTargetBindingComponent =
    helios::engine::rendering::common::components::RenderTargetBindingComponent<DefaultRenderHandles>;

using DefaultSceneBindingComponent =
    helios::engine::scene::components::SceneBindingComponent<DefaultRenderHandles>;

using DefaultCameraBindingComponent =
    helios::engine::scene::components::CameraBindingComponent<DefaultRenderHandles>;

using DefaultSceneMemberComponent =
    helios::engine::scene::components::SceneMemberComponent<DefaultRenderHandles>;

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

/**
 * @brief Creates a pre-configured GameWorld and GameLoop pair.
 *
 * @return A EngineRuntime object.
 */
[[nodiscard]] std::unique_ptr<EngineRuntime> bootstrapGameWorld(size_t capacity = 1000) {

    auto engineRuntime = std::make_unique<EngineRuntime>(EngineWorldFactory::makeEcsWorld());

    auto& gameWorld = engineRuntime->gameWorld;

    EngineWorldFactory::initPermaResources(gameWorld);

    gameWorld.emplaceResource<helios::engine::state::StateTransitionRules<EngineState>>(
        runtime::enginestate::rules::DefaultEngineStateTransitionRules{}
    );

    gameWorld.emplaceResource<rendering::RenderDataResolver>(
        helios::opengl::OpenGLRenderDataResolver<DefaultRenderHandles>{}
    );
    gameWorld.emplaceResource<rendering::RenderBackend>(rendering::RenderBackend{opengl::OpenGLBackend{}});


    return engineRuntime;
}

} // namespace helios::engine::v1_gfx_bootstrap

