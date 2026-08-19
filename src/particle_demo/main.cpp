
#include <utility>
#include <functional>
#include <format>
#include <algorithm>
#include <thread>
#include <cassert>

import helios.core;
import helios.ecs;
import helios.engine;
import helios.math;
import helios.physics;
import helios.gameplay;
import helios.opengl;
import helios.glfw;
import helios.imgui;

import helios.engine.bootstrap;

#include "Namespaces.h"


template<typename TEmitterHandle, typename TSpawnHandle = TEmitterHandle>
struct RadialSpawnPolicy {

    using EmitterHandle_type = TEmitterHandle;
    using SpawnHandle_type = TSpawnHandle;

    using SpawnContext = SpawnContext<TEmitterHandle, TSpawnHandle>;

    std::size_t spawnCount(UpdateContext& updateContext, const SpawnContext& spawnContext) {

        if (spawnContext.poolSnapshot.inactiveCount >= spawnContext.requiredAmount) {
            return spawnContext.requiredAmount;
        }

        return 0;
    }

    std::size_t spawn(UpdateContext& updateContext, const SpawnContext& spawnContext, std::span<const TSpawnHandle> spawnHandles) {

        auto frac = helios::math::radians(360.0f / spawnHandles.size());
       // assert(false && "use EntityAccessor");
        float i = 0.0f;
        float spread = 4.0f;
        for (auto& handle : spawnHandles) {

            auto entity = updateContext.find<TSpawnHandle>(handle);
            if (!entity) {
                continue;
            }

            auto* cmp = entity->template get<Velocity3DComponent<TSpawnHandle, Local>>();
            auto veloc = helios::math::vec3f{std::cos(frac * i) * spread, std::sin(frac * i) * spread, 0.0f}.normalize();
            cmp->setValue(veloc);
            entity->setActive(true);
            ++i;
        }

        return spawnHandles.size();
    }

    bool update(UpdateContext& updateContext, SpawnContext& spawnContext) {
        return true;
    }


};

template<typename THandle>
class DemoPoolPolicy {

    void onAcquire(UpdateContext& updateContext, std::span<THandle> handles) {

    }

    void onRelease(UpdateContext& updateContext, std::span<THandle> handles) {

    }

};


int main() {
    auto& logger = helios::core::log::LogManager::loggerForScope("main");

    // ========================================
    // Constants
    // ========================================
    constexpr unsigned int SCREEN_WIDTH  = 1280;
    constexpr unsigned int SCREEN_HEIGHT = 720;

    constexpr float WINDOW_ASPECT_RATIO_NUMER = 16.0f;
    constexpr float WINDOW_ASPECT_RATIO_DENOM = 9.0f;

    constexpr bool ENABLE_VSYNC = false;

    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================

    auto maxWorker = std::max(1u, std::thread::hardware_concurrency() - 1);
    JobSystem jobSystem(maxWorker);

    // gameworld
    auto engineRuntime = bootstrapGameWorld(jobSystem);
    //auto& [gameWorld, contextProvider, gameLoop] = bootstrapGameWorld(jobSystem);
    auto& gameWorld = engineRuntime.gameWorld;
    auto& contextProvider = engineRuntime.contextProvider;
    auto& gameLoop = engineRuntime.gameLoop;


    SceneMemberVisibilityRegistry<ParticleHandle, Instanced> visibilityRegistry{};


    // ========================================
    // Window Setup
    // ========================================
    auto MainWindow = gameWorld.add<WindowHandle>();
    MainWindow.add<WindowCreateRequestComponent<WindowHandle>>(WindowConfig{
        "helios - Game of Life",
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        WINDOW_ASPECT_RATIO_NUMER,
        WINDOW_ASPECT_RATIO_DENOM,
        ENABLE_VSYNC
    });

    
    // ========================================
    // Scene and Viewport Setup
    // ========================================

    auto MainRenderTarget = gameWorld.add<RenderTargetHandle>();
    MainRenderTarget.add<OpenGLRenderTargetIdComponent<RenderTargetHandle>>(0);
    MainRenderTarget.trackDirty<Size2DComponent<RenderTargetHandle>>();
    MainRenderTarget.add<ClearComponent<RenderTargetHandle>>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent<RenderTargetHandle>>(helios::engine::util::Colors::Black);

    auto MainViewport = gameWorld.add<ViewportHandle>();
    MainViewport.add<DebugNameComponent<ViewportHandle>>("MainViewport");
    MainViewport.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    MainViewport.add<ColorComponent<ViewportHandle>>(helios::engine::util::Colors::Black);
    // RenderTarget : Viewport (1:N)
    MainViewport.add<RenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    MainViewport.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0f, 0.0f, 1.0f, 1.0f});


    auto MainScene = gameWorld.add<SceneHandle>();
    MainWindow.add<RenderTargetBindingComponent<WindowHandle>>(MainRenderTarget);

    // Viewport : Scene (N:1)
    MainViewport.add<SceneBindingComponent<ViewportHandle>>(MainScene);

    auto MainCamera = gameWorld.add<CameraHandle>();
    MainCamera.add<DebugNameComponent<CameraHandle>>("MainCamera");
    MainCamera.trackDirty<PerspectiveCameraComponent<CameraHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / WINDOW_ASPECT_RATIO_DENOM);
    MainCamera.trackDirty<ProjectionMatrixComponent<CameraHandle>>();
    MainCamera.trackDirty<ViewMatrixComponent<CameraHandle>>();
    MainCamera.trackDirty<YawPitchRollComponent<CameraHandle>>();
    MainCamera.trackDirty<Rotation3DComponent<CameraHandle, Local>>();
    MainCamera.trackDirty<TransformComponent<CameraHandle, World>>(1.0f);
    MainCamera.trackDirty<Position3DComponent<CameraHandle, Local>>(0.0f, 0.0f, -50.0f);

    // camera does not need a scene binding, since there is no camera selection
    // using scenes. The SceneMemberVisibilitySystem will directly use the Viewport's mapping
    //MainCamera.add<SceneBindingComponent<CameraHandle>>(MainScene);
    MainViewport.add<CameraBindingComponent<ViewportHandle>>(MainCamera);


    // ========================================
    // Rendering Management setup
    // ========================================

    // Texture Setup
    auto ParticleShader = gameWorld.add<ShaderHandle>();
    ParticleShader.add<ShaderSourceComponent<ShaderHandle>>(
        "./resources/shader/particle.vert",
        "./resources/shader/particle.frag");
    ParticleShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Pass>>(
        UniformMapping{UniformSemantics::ProjectionMatrix, "projectionMatrix"},
        UniformMapping{UniformSemantics::ViewMatrix, "viewMatrix"}
    );
    ParticleShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Material>>(
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );

    // define Texture Entity
    auto ParticleTexture = gameWorld.add<TextureHandle>();
    ParticleTexture.add<TextureSourceComponent<TextureHandle>>("./resources/textures/particle.png");

    // TextureObject uses a Rect
    auto ParticleMesh = gameWorld.add<MeshHandle>();
    ParticleMesh.add<MeshDataComponent<MeshHandle>>(Rect::meshData());
    ParticleMesh.add<MeshUploadRequestComponent<MeshHandle>>();
    ParticleMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerVertex>>(
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::Position,  VertexAttributeType::Vec3f},
            0, sizeof(Vertex), offsetof(Vertex, position),0
        },
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::TextureCoordinates, VertexAttributeType::Vec2f},
            2, sizeof(Vertex), offsetof(Vertex, texCoords),0
    });

    ParticleMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerInstance>>(
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::InstancedModelMatrix,  VertexAttributeType::Mat4f},
            4, sizeof(InstanceData), offsetof(InstanceData, modelMatrix), 1
    });

    auto ParticleMaterial = gameWorld.add<MaterialHandle>();
    ParticleMaterial.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::Blue);
    

    // ========================================
    // Entity Setup
    // ========================================
    auto DEMO_ENTITY_POOL_KEY = gameWorld.resource<DefaultEntityPoolRegistry>().createPool(EntityPoolId<ParticleHandle>{"DemoPool"});
    auto DEMO_SPAWN_POLICY_KEY = gameWorld.resource<DefaultSpawnPolicyRegistry>().createPolicy<RadialSpawnPolicy<ParticleHandle>>(
        SpawnPolicyId<ParticleHandle>{"DemoSpawnPolicy"}
    );

    auto ParticlePrefab = gameWorld.add<ParticleHandle>(true);
    ParticlePrefab.add<SceneMemberComponent<ParticleHandle>>(MainScene);
    ParticlePrefab.add<PrefabEntityPoolRequestComponent<ParticleHandle>>(10);
    ParticlePrefab.add<EntityPoolKeyComponent<ParticleHandle>>(DEMO_ENTITY_POOL_KEY);

    ParticlePrefab.trackDirty<BoundsComponent<ParticleHandle, Local>>(Rect::boundsData());
    ParticlePrefab.trackDirty<BoundsComponent<ParticleHandle, World>>();
    ParticlePrefab.trackDirty<Velocity3DComponent<ParticleHandle, Local>>();
    //ParticlePrefab.trackDirty<LifetimeComponent<ParticleHandle>>(4.0f);
    ParticlePrefab.trackDirty<Rotation3DComponent<ParticleHandle, Local>>();
    ParticlePrefab.trackDirty<Position3DComponent<ParticleHandle, Local>>(1.0f, 0.0f, 0.0f);
    ParticlePrefab.trackDirty<Position3DComponent<ParticleHandle, World>>(0.0f, 0.0f, 0.0f);
    ParticlePrefab.trackDirty<TransformComponent<ParticleHandle, World>>(1.0f);

    ParticlePrefab.add<RenderPrototypeComponent<ParticleHandle, Instanced>>(
        ParticleShader.handle(), ParticleMaterial.handle(), ParticleMesh.handle(), ParticleTexture.handle()
    );


    auto ParticleEmitter = gameWorld.add<ParticleHandle>(true);
    ParticleEmitter.add<Position3DComponent<ParticleHandle, Local>>(0.0f, 0.0f, 0.0f);
    ParticleEmitter.add<SpawnRequestComponent<ParticleHandle>>(DEMO_ENTITY_POOL_KEY, DEMO_SPAWN_POLICY_KEY, 10);
    ParticleEmitter.add<SceneMemberComponent<ParticleHandle>>(MainScene);

    // ----------------------------------------
    // ImGui and Debug Tooling
    // ----------------------------------------
    auto imguiBackend = ImGuiGlfwOpenGLBackend(MainWindow.handle(), gameWorld.ecsWorld());
    auto imguiOverlay = ImGuiOverlay::forBackend(&imguiBackend);
    auto fpsMetrics = FpsMetrics();
    auto framePacer = FramePacer();
    framePacer.setTargetFps(0.0f);
    FrameStats frameStats{};

    auto menu = new MainMenuWidget();
    auto fpsWidget = new FpsWidget(&fpsMetrics, &framePacer);
    auto logWidget = new LogWidget();

    auto cameraWidget = new CameraWidget(gameWorld);

    imguiOverlay.addWidget(menu);
    imguiOverlay.addWidget(fpsWidget);
    imguiOverlay.addWidget(logWidget);
    imguiOverlay.addWidget(cameraWidget);

    // ----------------------------------------
    // Logger Configuration
    // ----------------------------------------
    LogManager::getInstance().enableLogging(true);
    //LogManager::getInstance().enableSink<ImGuiLogSink>(logWidget);


    // ========================================
    // Initialization of GameWorld and Game Loop
    // ========================================
    float DELTA_TIME = 0.0f;

    EngineCommandBuffer engineCommandBuffer{};

    // ----------------------------------------
    // GameLoop Config
    // ----------------------------------------
    gameLoop.phase(PhaseType::Pre)

            .beginPass(EngineState::Any)
                .addSystem(EngineFlowSystem{}, engineCommandBuffer)
            .executeCommands<DefaultEngineStateManager>()
            .endPass()

            .beginPass(EngineState::Booting)
                .addSystem(PlatformInitSystem{}, engineCommandBuffer)
            .executeCommands<DefaultGLFWPlatformManager>()
            .endPass()

            .beginPass(EngineState::Booted | EngineState::Running)
                .addSystem(PollEventsSystem{}, engineCommandBuffer)
                .addSystem(WindowCreateSystem<WindowHandle>{}, engineCommandBuffer)
            .executeCommands<DefaultGLFWPlatformManager>()
            .endPass()

            .beginPass(EngineState::Warmup)
                .addSystem(TextureUploadSystem<TextureHandle>{}, engineCommandBuffer)
                .addSystem(MeshUploadSystem<MeshHandle>{}, engineCommandBuffer)
                .addSystem(ShaderCompileSystem<ShaderHandle>{}, engineCommandBuffer)
                .addSystem(EntityPoolWarmupSystem<ParticleHandle>{}, engineCommandBuffer)
                .addSystem(WarmupDoneSystem{}, engineCommandBuffer)
            .executeCommands<
                DefaultTextureUploadManager,
                DefaultMeshUploadManager,
                DefaultShaderCompileManager,
                DefaultEngineStateManager
            >()
            .executeCommandsParallel<DefaultEntityPoolManager>()
            .endPass();

            // intentionally left empty
            gameLoop.phase(PhaseType::Main)
                .beginPass(EngineState::Running)
                .addSystem<EngineCommandBuffer>(Lambda<ParticleHandle>(
                    [&]<typename TUpdateContext, typename TCommandBuffer>
                    requires helios::engine::runtime::concepts::ProvidesUpdateContext<TUpdateContext, UpdateContext> &&
                    helios::ecs::command::concepts::IsCommandBufferLike<TCommandBuffer>
                            (TUpdateContext& updateCtx, TCommandBuffer& cmdBuffer) {
                            UpdateContext& updateContext = updateCtx.updateContext();

                            for (auto [entity, requestComponent] : updateContext.view<
                                ParticleHandle,
                                SpawnRequestComponent<ParticleHandle>
                            >().withActive()) {
                                cmdBuffer.template add<SpawnCommand<ParticleHandle>>(
                                    entity.handle(),
                                    requestComponent->entityPoolKey,
                                    requestComponent->spawnPolicyKey,
                                    requestComponent->amount
                                );

                                entity.remove<SpawnRequestComponent<ParticleHandle>>();
                            }

                            return true;

                        }), engineCommandBuffer)
                .executeCommands<DefaultSpawnManager>()
                .endPass()

                .beginPass(EngineState::Running)
                    .addSystem(MotionIntegrationSystem<ParticleHandle>{})
                .endPass();




            gameLoop.phase(PhaseType::Post)
                 .beginPass(EngineState::Running)

                    // create parallel groups
                    .addParallelSystems<
                        Serial<
                            WorldTransformSystem<GameObjectHandle>,
                            WorldBoundsUpdateSystem<GameObjectHandle>
                        >,
                        Serial<
                            YawPitchRollUpdateSystem<CameraHandle>,
                            WorldTransformSystem<CameraHandle>,
                            PerspectiveCameraUpdateSystem<CameraHandle>
                        >,
                        Serial<
                            WorldTransformSystem<ParticleHandle>,
                            WorldBoundsUpdateSystem<ParticleHandle>
                        >
                    >()

                    // this will produce render commands after scenes have been culled according to
                    // their active viewports
                    .addSystem(
                        SceneMemberVisibilitySystem<ParticleHandle, Instanced, AABBCullingStrategy<ParticleHandle>>
                        (AABBCullingStrategy<ParticleHandle>(), visibilityRegistry)
                    )
                    .addSystem<EngineCommandBuffer>(SceneRenderSystem<ParticleHandle, Instanced>{visibilityRegistry}, engineCommandBuffer)

                .executeCommands<DefaultRenderManager>()
                .endPass()

                 // Clear, bufferswapping, lifecycle
                .beginPass(EngineState::Running)
                    .addSystem(GLFWWindowCloseSystem<WindowHandle>{}, engineCommandBuffer)
                    .addSystem(WindowBasedShutdownSystem<WindowHandle>{}, engineCommandBuffer)
                    .addSystem(ClearAllDirtySetsSystem{})
                    .addSystem(Lambda<ParticleHandle>(
                        [&]<typename TUpdateContext, typename TCommandBuffer>
                        requires helios::engine::runtime::concepts::ProvidesUpdateContext<TUpdateContext, UpdateContext> &&
                                    helios::ecs::command::concepts::IsCommandBufferLike<TCommandBuffer>
                        (TUpdateContext& updateCtx, TCommandBuffer& cmdBuffer) {
                            UpdateContext& updateContext = updateCtx.updateContext();

                            for (auto [entity, lcc, keyCmp] : updateContext.view<
                                ParticleHandle,
                                LifetimeComponent<ParticleHandle>,
                                EntityPoolKeyComponent<ParticleHandle>>().withActive()) {
                                float rem = lcc->value() - updateContext.deltaTime();

                                if (rem <= 0) {
                                    cmdBuffer.template add<ReleaseEntityCommand<ParticleHandle>>(
                                        keyCmp->entityPoolKey,
                                        entity.handle()
                                    );
                                }
                                lcc->setValue(rem);
                            }

                            return true;
                    }), engineCommandBuffer)
                    .addSystem(ImGuiOverlayRenderSystem{imguiOverlay})
                    .addSystem(SwapBuffersSystem<WindowHandle>{}, engineCommandBuffer)
                .executeCommands<DefaultGLFWPlatformManager>()
                .executeCommandsParallel<DefaultEntityPoolManager>()
                .endPass()

                .beginPass(EngineState::Shutdown)
                    .addSystem(DestroySessionSystem{})
                .endPass()
            ;

    gameWorld.init(contextProvider);
    gameLoop.init();


    gameWorld.session().template setStateFrom<EngineState>(
        StateTransitionContext<EngineState>(
        EngineState::Undefined,
        EngineState::Booting,
        EngineStateTransitionId::BootRequest
    ));



    while (gameLoop.isRunning()) {

        framePacer.beginFrame();

        // Game Logic Update
        const GamepadState gamepadState = GamepadState();
        const auto inputSnapshot = InputSnapshot(gamepadState);

        // Frame Synchronization is now done via GLFWSwapBuffersSystems
        gameLoop.update(DELTA_TIME, inputSnapshot);//inputSnapshot, viewportSnapshots);;


        frameStats = framePacer.sync();
        fpsMetrics.addFrame(frameStats);
        DELTA_TIME = frameStats.totalFrameTime;
    }


    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));


    return 0;
}