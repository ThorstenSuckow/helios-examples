
#include <utility>
#include <functional>
#include <thread>

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


#include "../Namespaces.h"

helios::math::vec3f randomVec3f(const std::uint32_t seed) {

    auto rand = helios::core::common::Random(seed);

    return {
        rand.randomFloat(-1.0f, 1.0f),
        rand.randomFloat(-1.0f, 1.0f),
        rand.randomFloat(-1.0f, 1.0f)
    };
}


int main() {
    auto& logger = helios::core::log::LogManager::loggerForScope("main");

    // ========================================
    // Constants
    // ========================================
    constexpr unsigned int SCREEN_WIDTH  = 1280;
    constexpr unsigned int SCREEN_HEIGHT = 720;

    constexpr bool ENABLE_VSYNC = false;

    constexpr float WINDOW_ASPECT_RATIO_NUMER = 16.0f;
    constexpr float WINDOW_ASPECT_RATIO_DENOM = 9.0f;

    constexpr int OBJECT_COUNT = 143; // per axis
    constexpr std::size_t OBJECT_DISTANCE = 3;

    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================


    JobSystem jobSystem{std::max(1u, std::thread::hardware_concurrency() - 1)};
    // gameworld
    auto engineRuntime = bootstrapGameWorld(jobSystem);

    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;



    // ========================================
    // Window Setup
    // ========================================
    auto MainWindow = gameWorld.add<WindowHandle>();
    MainWindow.add<WindowCreateRequestComponent<WindowHandle>>(WindowConfig{
        "helios - ECS Rendering Demo",
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

    auto CullingViewport = gameWorld.add<ViewportHandle>();
    CullingViewport.add<DebugNameComponent<ViewportHandle>>("CullingViewport");
    CullingViewport.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    CullingViewport.add<ColorComponent<ViewportHandle>>(helios::engine::util::Colors::LightGray);
    // RenderTarget : Viewport (1:N)
    CullingViewport.add<DefaultRenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    CullingViewport.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0f, .5f, 1.0f, 0.5f});

    auto CullingViewport_bottom = gameWorld.add<ViewportHandle>();
    CullingViewport_bottom.add<DebugNameComponent<ViewportHandle>>("CullingViewport_bottom");
    CullingViewport_bottom.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    CullingViewport_bottom.add<ColorComponent<ViewportHandle>>(helios::engine::util::Colors::Gray);
    // RenderTarget : Viewport (1:N)
    CullingViewport_bottom.add<DefaultRenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    CullingViewport_bottom.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0f, .0f, 1.0f, 0.5f});

    auto MainScene = gameWorld.add<SceneHandle>();
    MainWindow.add<DefaultRenderTargetBindingComponent<WindowHandle>>(MainRenderTarget);

    // Viewport : Scene (N:1)
    CullingViewport.add<DefaultSceneBindingComponent<ViewportHandle>>(MainScene);
    CullingViewport_bottom.add<DefaultSceneBindingComponent<ViewportHandle>>(MainScene);

    auto CullingCamera = gameWorld.add<CameraHandle>();
    CullingCamera.add<DebugNameComponent<CameraHandle>>("CullingCamera");
    CullingCamera.trackDirty<PerspectiveCameraComponent<CameraHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM));
    CullingCamera.trackDirty<ProjectionMatrixComponent<CameraHandle>>();
    CullingCamera.trackDirty<ViewMatrixComponent<CameraHandle>>();
    CullingCamera.trackDirty<YawPitchRollComponent<CameraHandle>>();
    CullingCamera.trackDirty<Rotation3DComponent<CameraHandle, Local>>();
    CullingCamera.trackDirty<TransformComponent<CameraHandle, World>>(1.0f);
    CullingCamera.trackDirty<Position3DComponent<CameraHandle, Local>>(0.0f, 0.0f, -50.0f);
    CullingCamera.add<DefaultSceneMemberComponent<CameraHandle>>(MainScene);
    // later on: rebuildHandleMultiMapFromSceneMembership(). SSoT w/ components, but systems get the
    // multimaps for faster access / querying?
    // or the view gets extended internally that it can fall back to a multimap, e.g. filter<> instead of view<>
    // or some other adequate semantic name
    CullingViewport.add<DefaultCameraBindingComponent<ViewportHandle>>(CullingCamera);

    auto CullingCamera_bottom = gameWorld.add<CameraHandle>();
    CullingCamera_bottom.add<DebugNameComponent<CameraHandle>>("CullingCamera_bottom");
    CullingCamera_bottom.trackDirty<PerspectiveCameraComponent<CameraHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM));
    CullingCamera_bottom.trackDirty<ProjectionMatrixComponent<CameraHandle>>();
    CullingCamera_bottom.trackDirty<ViewMatrixComponent<CameraHandle>>();
    CullingCamera_bottom.trackDirty<YawPitchRollComponent<CameraHandle>>();
    CullingCamera_bottom.trackDirty<Rotation3DComponent<CameraHandle, Local>>();
    CullingCamera_bottom.trackDirty<Position3DComponent<CameraHandle, Local>>(0.0f, 0.0f, -110.0f);
    CullingCamera_bottom.trackDirty<TransformComponent<CameraHandle, World>>(1.0f);
    CullingCamera_bottom.add<DefaultSceneMemberComponent<CameraHandle>>(MainScene);
    CullingViewport_bottom.add<DefaultCameraBindingComponent<ViewportHandle>>(CullingCamera_bottom);


    // ========================================
    // Rendering Management setup
    // ========================================

    // shader, mesh, material for cube
    auto CubeShader = gameWorld.add<ShaderHandle>();
    CubeShader.add<ShaderSourceComponent<ShaderHandle>>("./resources/cube.vert", "./resources/cube.frag");
    CubeShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Pass>>(
        UniformMapping{UniformSemantics::ProjectionMatrix, "projectionMatrix"},
        UniformMapping{UniformSemantics::ViewMatrix, "viewMatrix"}
    );
    CubeShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Material>>(
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );
    CubeShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Draw>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"}
    );

    auto CubeMesh = gameWorld.add<MeshHandle>();
    CubeMesh.add<MeshDataComponent<MeshHandle>>(Triangle::meshData());
    CubeMesh.add<MeshUploadRequestComponent<MeshHandle>>();
    CubeMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerVertex>>(
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::Position,  VertexAttributeType::Vec3f},
            0, sizeof(Vertex), offsetof(Vertex, position),0
        }
    );
    CubeMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerInstance>>(
    VertexAttributeLayout{
        VertexAttribute{VertexAttributeSemantics::InstancedModelMatrix, VertexAttributeType::Mat4f},
        4, sizeof(InstanceData), offsetof(InstanceData, modelMatrix),1}
    );

    auto CubeMaterial = gameWorld.add<MaterialHandle>();
    CubeMaterial.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::Red);

    auto CubeMaterialOverride = gameWorld.add<MaterialHandle>();
    CubeMaterialOverride.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::White);

    // ========================================
    // Entity Setup
    // ========================================
    // cubes
    for (int x = -OBJECT_COUNT/2; x < OBJECT_COUNT/2; x+=OBJECT_DISTANCE) {
        for (int y = -OBJECT_COUNT/2; y < OBJECT_COUNT/2 ; y+=OBJECT_DISTANCE) {
            auto cube = gameWorld.add<GameObjectHandle>();
            cube.add<DefaultSceneMemberComponent<GameObjectHandle>>(MainScene);
            cube.trackDirty<BoundsComponent<GameObjectHandle, Local>>(Triangle::boundsData());
            cube.trackDirty<BoundsComponent<GameObjectHandle, World>>();
            cube.trackDirty<Rotation3DComponent<GameObjectHandle, Local>>();

            cube.trackDirty<Position3DComponent<GameObjectHandle, Local>>(static_cast<float>(x), static_cast<float>(y), 0.0f);
            cube.trackDirty<Position3DComponent<GameObjectHandle, World>>(0.0f, 0.0f, 0.0f);
            cube.trackDirty<helios::physics::motion::components::Velocity3DComponent<
                GameObjectHandle, Intent>
            >(randomVec3f(x * y).withZ(0.0f).normalize());
            cube.trackDirty<helios::physics::motion::components::Velocity3DComponent<
                GameObjectHandle, Local>
            >();

            cube.trackDirty<TransformComponent<GameObjectHandle, World>>(1.0f);
            cube.add<DefaultRenderPrototypeComponent<GameObjectHandle, Instanced>>(
                CubeShader.handle(), CubeMaterial.handle(), CubeMesh.handle()
            );
        }
    }


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

    auto cameraWidget = new CameraWidget<DefaultRenderHandles>(gameWorld);

    imguiOverlay.addWidget(menu);
    imguiOverlay.addWidget(fpsWidget);
    imguiOverlay.addWidget(logWidget);
    imguiOverlay.addWidget(cameraWidget);

    // ----------------------------------------
    // Logger Configuration
    // ----------------------------------------
    LogManager::getInstance().enableLogging(true);
    LogManager::getInstance().enableSink<ImGuiLogSink>(logWidget);
    LogManager::getInstance().enableSink<helios::core::log::ConsoleSink>(); // TEMP DIAGNOSTIC


    // ========================================
    // Initialization of GameWorld and Game Loop
    // ========================================
    float DELTA_TIME = 0.0f;

    // painting function
    auto enableMemberMaterialOverride = [&](const UpdateContext& updateContext, auto memberContexts, const bool enable) {

        auto* renderPrototypeSet = updateContext.sparseSet<GameObjectHandle, DefaultRenderPrototypeComponent<GameObjectHandle, Instanced>>();

        auto handle = enable ? CubeMaterialOverride.handle() : CubeMaterial.handle();
        for (const auto member : memberContexts) {
            if (!updateContext.isValid(member.memberHandle)) {
                continue;
            }

            if (auto* rpc = renderPrototypeSet->get(member.memberHandle.entityId())) {
                rpc->setMaterialHandle(handle);
            }

        }
    };

    // ----------------------------------------
    // GameLoop Config
    // ----------------------------------------
    gameLoop.phase(PhaseType::Pre)
                .beginPass(EngineState::Any)
                    .addSystem<EngineFlowSystem>()
                .executeCommands<DefaultEngineStateManager>()
                .endPass()

                .beginPass(EngineState::Booting)
                    .addSystem<PlatformInitSystem>()
                .executeCommands<DefaultGLFWPlatformManager>()
                .endPass()

                .beginPass(EngineState::Booted | EngineState::Running)
                    .addSystem<PollEventsSystem>()
                    .addSystem<WindowCreateSystem<WindowHandle>>()
                .executeCommands<DefaultGLFWPlatformManager>()
                .endPass()

                .beginPass(EngineState::Warmup)
                    .addSystem<MeshUploadSystem<MeshHandle>>()
                    .addSystem<ShaderCompileSystem<ShaderHandle>>()
                    .addSystem<DefaultWarmupDoneSystem>()
                .executeCommands<
                    DefaultMeshUploadManager,
                    DefaultShaderCompileManager,
                    DefaultEngineStateManager
                >()
                .endPass();


            gameLoop.phase(PhaseType::Main)
                .beginPass(EngineState::Running)
                .endPass();

            gameLoop.phase(PhaseType::Post)
                 .beginPass(EngineState::Running)

                    .addSystem(
                        // replacement for systems that compute the local velocity from intended velocity,
                        // such as component systems
                        [&](UpdateContext& updateContext) {
                            for (auto [entity, intendedVelocity, localVelocity] : updateContext.view<
                                GameObjectHandle,
                                Velocity3DComponent<GameObjectHandle, Intent>,
                                Velocity3DComponent<GameObjectHandle, Local>
                            >()
                                .withActive()
                               .template whereAnyDirty<Active<GameObjectHandle>, Velocity3DComponent<GameObjectHandle, Intent>>()) {
                                    entity.setTrackedValue(localVelocity, intendedVelocity->value());
                                //  intendedVelocity->setValue({0.0f, 0.0f, 0.0f});
                            }
                        }
                    )

                    .addParallelSystems<
                        Serial<
                            YawPitchRollUpdateSystem<CameraHandle>,
                            WorldTransformSystem<CameraHandle>,
                            PerspectiveCameraUpdateSystem<CameraHandle>
                        >,
                        Serial<
                            MotionIntegrationSystem<GameObjectHandle>,
                            WorldTransformSystem<GameObjectHandle>,
                            WorldBoundsUpdateSystem<GameObjectHandle>
                        >
                    >()

                    // this will produce render commands after scenes have been culled according to
                    // their active viewports
                    .addSystem<
                        DefaultSceneMemberVisibilitySystem<
                            GameObjectHandle,
                            Instanced,
                            AABBCullingStrategy<GameObjectHandle>
                        >>(AABBCullingStrategy<GameObjectHandle>())
                   .addParallelSystems(
                        [&](UpdateContext& updateContext,
                            DefaultSceneMemberVisibilityRegistry<GameObjectHandle, Instanced>& visibilityRegistry) {
                            const auto viewport = CullingViewport.handle();
                            enableMemberMaterialOverride(
                                updateContext, visibilityRegistry.culledMembers(viewport), true);
                        },
                        [&](UpdateContext& updateContext,
                            DefaultSceneMemberVisibilityRegistry<GameObjectHandle, Instanced>& visibilityRegistry) {
                                const auto viewport = CullingViewport.handle();
                                enableMemberMaterialOverride(
                                    updateContext, visibilityRegistry.visibleMembers(viewport), false);
                            }
                    )
                    // consume the scenemember-registry
                    .addSystem<DefaultSceneRenderSystem<GameObjectHandle, Instanced>>()
                .executeCommands<DefaultRenderManager>()
                .endPass()


                 // Clear, bufferswapping
                .beginPass(EngineState::Running)
                    .addSystem<GLFWWindowCloseSystem<WindowHandle>>()
                    .addSystem<WindowBasedShutdownSystem<WindowHandle>>()
                    .addSystem<ClearAllDirtySetsSystem>()
                    .addSystem<ImGuiOverlayRenderSystem>(imguiOverlay)
                    .addSystem<SwapBuffersSystem<WindowHandle>>()
                .executeCommands<DefaultGLFWPlatformManager>()
                .endPass()

                .beginPass(EngineState::Shutdown)
                    .addSystem<DestroySessionSystem>()
                .endPass()
            ;


    gameWorld.init();
    gameLoop.init();

    gameWorld.session().template setStateFrom<EngineState>(
        StateTransitionContext<EngineState>(
        EngineState::Undefined,
        EngineState::Booting,
        EngineStateTransitionId::BootRequest
    ));


    while (gameLoop.isRunning()) {

        framePacer.beginFrame();

        //app->eventManager().dispatchAll();
        //inputManager.poll(0.0f);

        // Game Logic Update
        const GamepadState gamepadState = GamepadState();// = {};//inputManager->gamepadState(Gamepad::ONE);
        const auto inputSnapshot = InputSnapshot(gamepadState);

        //const auto viewportSnapshots = renderTargetsRegistry.viewportSnapshots();


        // Frame Synchronization is now done via GLFWSwapBuffersSystems
        gameLoop.update(DELTA_TIME, inputSnapshot);//inputSnapshot, viewportSnapshots);;



        frameStats = framePacer.sync();
        fpsMetrics.addFrame(frameStats);
        DELTA_TIME = frameStats.totalFrameTime;
    }


    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));


    return 0;
}