module;

#include <cassert>
#include <cstddef>
#include <memory>
#include <tuple>
#include <utility>

export module thesis.bootstrap;

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

import thesis.handles;

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
using namespace thesis::handles;

namespace platform  = helios::engine::platform;
namespace scene     = helios::engine::scene;
namespace rendering = helios::engine::rendering;
namespace runtime   = helios::engine::runtime;
namespace opengl    = helios::opengl;


export namespace thesis::bootstrap {

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

template <typename THandleList>
struct WorldFactoryFor;

template <typename... THandles>
struct WorldFactoryFor<std::tuple<THandles...>> {
    using type = WorldFactory<THandles...>;
};

template <typename THandleList>
struct RenderManagerFor;

template <typename... TMemberHandles>
struct RenderManagerFor<std::tuple<TMemberHandles...>> {
    using type = helios::engine::rendering::RenderManager<DefaultRenderHandles, TMemberHandles...>;
};

using EngineWorldFactory = typename WorldFactoryFor<thesis::handles::WorldHandles>::type;

using DefaultTimerManager = helios::engine::runtime::timing::TimerManager;
using DefaultGLFWPlatformManager = helios::glfw::GLFWPlatformManager<WindowHandle, DefaultRenderHandles>;
using DefaultRenderManager = typename RenderManagerFor<thesis::handles::RenderMemberHandles>::type;
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
[[nodiscard]] std::unique_ptr<EngineRuntime> bootstrapGameWorld() {

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

} // namespace thesis::bootstrap
