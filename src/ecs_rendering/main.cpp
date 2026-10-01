
#include <thread>
#include <utility>

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

    return {rand.randomFloat(-1.0F, 1.0F), rand.randomFloat(-1.0F, 1.0F), rand.randomFloat(-1.0F, 1.0F)};
}

int main() {
    const auto& logger = helios::core::log::LogManager::loggerForScope("main");

    // ========================================
    // Constants
    // ========================================
    constexpr unsigned int SCREEN_WIDTH = 1280;
    constexpr unsigned int SCREEN_HEIGHT = 720;

    constexpr bool ENABLE_VSYNC = false;

    constexpr float WINDOW_ASPECT_RATIO_NUMER = 16.0F;
    constexpr float WINDOW_ASPECT_RATIO_DENOM = 9.0F;

    constexpr int OBJECT_COUNT = 143; // per axis
    constexpr std::size_t OBJECT_DISTANCE = 3;

    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================

    helios::core::thread::JobSystem jobSystem{std::max(1U, std::thread::hardware_concurrency() - 1)};
    // gameworld
    auto engineRuntime = helios::engine::bootstrap::bootstrapGameWorld(jobSystem);

    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;

    // ========================================
    // Window Setup
    // ========================================
    auto MainWindow = gameWorld.add<helios::engine::bootstrap::WindowHandle>();
    MainWindow.add<WindowCreateRequestComponent>(WindowConfig{
        .title = "helios - ECS Rendering Demo",
        .size = {SCREEN_WIDTH, SCREEN_HEIGHT},
        .aspectRatioNumer = WINDOW_ASPECT_RATIO_NUMER,
        .aspectRatioDenom = WINDOW_ASPECT_RATIO_DENOM,
        .vsyncEnabled = ENABLE_VSYNC
    });

    // ========================================
    // Scene and Viewport Setup
    // ========================================

    auto MainRenderTarget = gameWorld.add<helios::engine::bootstrap::RenderTargetHandle>();
    MainRenderTarget.add<OpenGLRenderTargetIdComponent>(0);
    MainRenderTarget.trackDirty<Size2DComponent>();
    MainRenderTarget.add<ClearComponent>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Black);

    auto CullingViewport = gameWorld.add<helios::engine::bootstrap::ViewportHandle>();
    CullingViewport.add<DebugNameComponent<ViewportHandle>>("CullingViewport");
    CullingViewport.add<ClearComponent>(ClearFlags::Color);
    CullingViewport.add<ColorComponent>(helios::engine::rendering::common::types::Colors::LightGray);
    // RenderTarget : Viewport (1:N)
    CullingViewport.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);
    CullingViewport.add<RectComponent<>>(helios::math::vec4f{0.0F, .5f, 1.0F, 0.5F});

    auto CullingViewport_bottom = gameWorld.add<helios::engine::bootstrap::ViewportHandle>();
    CullingViewport_bottom.add<DebugNameComponent<ViewportHandle>>("CullingViewport_bottom");
    CullingViewport_bottom.add<ClearComponent>(ClearFlags::Color);
    CullingViewport_bottom.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Gray);
    // RenderTarget : Viewport (1:N)
    CullingViewport_bottom.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);
    CullingViewport_bottom.add<RectComponent<>>(helios::math::vec4f{0.0F, .0f, 1.0F, 0.5F});

    auto MainScene = gameWorld.add<SceneHandle>();
    MainWindow.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);

    // Viewport : Scene (N:1)
    CullingViewport.add<DefaultSceneBindingComponent>(MainScene);
    CullingViewport_bottom.add<DefaultSceneBindingComponent>(MainScene);

    auto CullingCamera = gameWorld.add<CameraHandle>();
    CullingCamera.add<DebugNameComponent<CameraHandle>>("CullingCamera");
    CullingCamera.trackDirty<PerspectiveCameraComponent>(
        helios::math::radians(90.0F), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM)
    );
    CullingCamera.trackDirty<ProjectionMatrixComponent>();
    CullingCamera.trackDirty<ViewMatrixComponent>();
    CullingCamera.trackDirty<YawPitchRollComponent>();
    CullingCamera.trackDirty<Rotation3DComponent<Local>>();
    CullingCamera.trackDirty<TransformComponent<World>>(1.0F);
    CullingCamera.trackDirty<Position3DComponent<Local>>(0.0F, 0.0F, -50.0F);
    CullingCamera.add<DefaultSceneMemberComponent>(MainScene);
    // later on: rebuildHandleMultiMapFromSceneMembership(). SSoT w/ components, but systems get the
    // multimaps for faster access / querying?
    // or the view gets extended internally that it can fall back to a multimap, e.g. filter<> instead of view<>
    // or some other adequate semantic name
    CullingViewport.add<DefaultCameraBindingComponent>(CullingCamera);

    auto CullingCamera_bottom = gameWorld.add<CameraHandle>();
    CullingCamera_bottom.add<DebugNameComponent<CameraHandle>>("CullingCamera_bottom");
    CullingCamera_bottom.trackDirty<PerspectiveCameraComponent>(
        helios::math::radians(90.0F), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM)
    );
    CullingCamera_bottom.trackDirty<ProjectionMatrixComponent>();
    CullingCamera_bottom.trackDirty<ViewMatrixComponent>();
    CullingCamera_bottom.trackDirty<YawPitchRollComponent>();
    CullingCamera_bottom.trackDirty<Rotation3DComponent<Local>>();
    CullingCamera_bottom.trackDirty<Position3DComponent<Local>>(0.0F, 0.0F, -110.0F);
    CullingCamera_bottom.trackDirty<TransformComponent<World>>(1.0F);
    CullingCamera_bottom.add<DefaultSceneMemberComponent>(MainScene);
    CullingViewport_bottom.add<DefaultCameraBindingComponent>(CullingCamera_bottom);

    // ========================================
    // Rendering Management setup
    // ========================================

    // shader, mesh, material for cube
    auto CubeShader = gameWorld.add<ShaderHandle>();
    CubeShader.add<ShaderSourceComponent>("./resources/cube.vert", "./resources/cube.frag");
    CubeShader.add<UniformMappingsComponent<UniformScope::Pass>>(
        UniformMapping{.semantics = UniformSemantics::ProjectionMatrix, .name = "projectionMatrix"},
        UniformMapping{.semantics = UniformSemantics::ViewMatrix, .name = "viewMatrix"}
    );
    CubeShader.add<UniformMappingsComponent<UniformScope::Material>>(
        UniformMapping{.semantics = UniformSemantics::MaterialBaseColor, .name = "color"}
    );
    CubeShader.add<UniformMappingsComponent<UniformScope::Draw>>(
        UniformMapping{.semantics = UniformSemantics::ModelMatrix, .name = "modelMatrix"}
    );

    auto CubeMesh = gameWorld.add<MeshHandle>();
    CubeMesh.add<MeshDataComponent>(Triangle::meshData());
    CubeMesh.add<MeshUploadRequestComponent>();
    CubeMesh.add<VertexAttributeLayoutComponent<PerVertex>>(VertexAttributeLayout{
        .attribute =
            VertexAttribute{.semantics = VertexAttributeSemantics::Position, .type = VertexAttributeType::Vec3f},
        .location = 0,
        .stride = sizeof(Vertex),
        .offset = offsetof(Vertex, position),
        .divisor = 0
    });
    CubeMesh.add<VertexAttributeLayoutComponent<PerInstance>>(VertexAttributeLayout{
        .attribute =
            VertexAttribute{
                .semantics = VertexAttributeSemantics::InstancedModelMatrix, .type = VertexAttributeType::Mat4f
            },
        .location = 4,
        .stride = sizeof(InstanceData),
        .offset = offsetof(InstanceData, modelMatrix),
        .divisor = 1
    });

    auto CubeMaterial = gameWorld.add<MaterialHandle>();
    CubeMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Red);

    auto CubeMaterialOverride = gameWorld.add<MaterialHandle>();
    CubeMaterialOverride.add<ColorComponent>(helios::engine::rendering::common::types::Colors::White);

    // ========================================
    // Entity Setup
    // ========================================
    // cubes
    for (int x = -OBJECT_COUNT / 2; x < OBJECT_COUNT / 2; x += OBJECT_DISTANCE) {
        for (int y = -OBJECT_COUNT / 2; y < OBJECT_COUNT / 2; y += OBJECT_DISTANCE) {
            auto cube = gameWorld.add<GameObjectHandle>();
            cube.add<DefaultSceneMemberComponent>(MainScene);
            cube.trackDirty<BoundsComponent<Local>>(Triangle::boundsData());
            cube.trackDirty<BoundsComponent<World>>();
            cube.trackDirty<Rotation3DComponent<Local>>();

            cube.trackDirty<Position3DComponent<Local>>(
                static_cast<float>(x), static_cast<float>(y), 0.0F
            );
            cube.trackDirty<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
            cube.trackDirty<helios::physics::motion::components::Velocity3DComponent<Intent>>(
                randomVec3f(x * y).withZ(0.0F).normalize()
            );
            cube.trackDirty<helios::physics::motion::components::Velocity3DComponent<Local>>();

            cube.trackDirty<TransformComponent<World>>(1.0F);
            cube.add<DefaultRenderPrototypeComponent<Instanced>>(
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
    framePacer.setTargetFps(0.0F);
    FrameTiming frameTiming{};

    auto* menu = new MainMenuWidget();
    auto* fpsWidget = new FpsWidget(&fpsMetrics, &framePacer);
    auto* logWidget = new LogWidget();

    auto* cameraWidget = new CameraWidget<DefaultRenderHandles>(gameWorld);

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


    // painting function
    auto enableMemberMaterialOverride = [&](EntityManager<GameObjectHandle>& entityManager,
                                            auto memberContexts,
                                            const bool enable) {
        auto* renderPrototypeSet =
            entityManager.sparseSet<DefaultRenderPrototypeComponent<Instanced>>();

        auto handle = enable ? CubeMaterialOverride.handle() : CubeMaterial.handle();
        for (const auto member : memberContexts) {
            if (!entityManager.isValid(member.memberHandle)) {
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
    gameLoop.scheduler()
        // PRE
        .beginSchedule(GameWorld::sessionState(EngineState::Any))
            .add<EngineFlowSystem>()
        .endSchedule<EngineStateManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Booting))
            .add<PlatformInitSystem>()
        .endSchedule<DefaultGLFWPlatformManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Booted | EngineState::Running))
            .add<PollEventsSystem>()
            .add<WindowCreateSystem<WindowHandle>>()
        .endSchedule<DefaultGLFWPlatformManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Warmup))
            .add<MeshUploadSystem<MeshHandle>>()
            .add<ShaderCompileSystem<ShaderHandle>>()
            .add<DefaultWarmupDoneSystem>()
        .endSchedule<
            DefaultMeshUploadManager,
            DefaultShaderCompileManager,
            EngineStateManager
        >()

        // MAIN
        .beginSchedule(GameWorld::sessionState(EngineState::Running))

            .add(
                // replacement for systems that compute the local velocity from intended velocity,
                // such as component systems
                [&](Query<
                    GameObjectHandle,
                    ReadSet<Velocity3DComponent<Intent>, Velocity3DComponent<Local>>,
                    WriteSet<Velocity3DComponent<Local>>,
                    Filter<
                        IsActive,
                        AnyDirty<Active, Velocity3DComponent<Intent>>
                    >
                > query) {
                    for (auto [entity, intendedVelocity, localVelocity ] : query) {
                         entity.track<Velocity3DComponent<Local>>()
                            ->setValue(intendedVelocity->value());
                    }
                }
            )

            .add<
                Sequential<
                    YawPitchRollUpdateSystem<CameraHandle>,
                    WorldTransformSystem<CameraHandle>,
                    PerspectiveCameraUpdateSystem<CameraHandle>
                >,
                Sequential<
                    MotionIntegrationSystem<GameObjectHandle>,
                    WorldTransformSystem<GameObjectHandle>,
                    WorldBoundsUpdateSystem<GameObjectHandle>
                >
            >()

            // this will produce render commands after scenes have been culled according to
            // their active viewports
            .add<
                DefaultSceneMemberVisibilitySystem<GameObjectHandle, Instanced, AABBCullingStrategy<GameObjectHandle>>
            >(AABBCullingStrategy<GameObjectHandle>())
            .add(
                [&](EntityManager<GameObjectHandle>& entityManager,
                    DefaultSceneMemberVisibilityRegistry<GameObjectHandle, Instanced>& visibilityRegistry) {
                    const auto viewport = CullingViewport.handle();
                    enableMemberMaterialOverride(
                        entityManager, visibilityRegistry.culledMembers(viewport), true
                    );
                },
                [&](EntityManager<GameObjectHandle>& entityManager,
                    DefaultSceneMemberVisibilityRegistry<GameObjectHandle, Instanced>& visibilityRegistry) {
                    const auto viewport = CullingViewport.handle();
                    enableMemberMaterialOverride(
                        entityManager, visibilityRegistry.visibleMembers(viewport), false
                    );
                }
            )
            // consume the scenemember-registry
            .add<DefaultSceneRenderSystem<GameObjectHandle, Instanced>>()
        .endSchedule<DefaultRenderManager>()

        // POST
        // Clear, bufferswapping
        .beginSchedule(GameWorld::sessionState(EngineState::Running))
            .add<GLFWWindowCloseSystem<WindowHandle>>()
            .add<WindowBasedShutdownSystem<WindowHandle>>()
            .add<ClearAllDirtySetsSystem>()
            .add<ImGuiOverlayRenderSystem>(imguiOverlay)
            .add<SwapBuffersSystem<WindowHandle>>()
        .endSchedule<DefaultGLFWPlatformManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Shutdown))
            .add<DestroySessionSystem>()
        .endSchedule();


    gameWorld.init();
    gameLoop.init();

    gameWorld.session().template setStateFrom<EngineState>(StateTransitionContext<EngineState>(
        EngineState::Undefined, EngineState::Booting, EngineStateTransitionId::BootRequest
    ));

    while (gameLoop.isRunning()) {

        framePacer.beginFrame();

        // app->eventManager().dispatchAll();
        // inputManager.poll(0.0f);

        // Game Logic Update
        const GamepadState gamepadState = GamepadState(); // = {};//inputManager->gamepadState(Gamepad::ONE);
        const auto inputSnapshot = InputSnapshot(gamepadState);

        // const auto viewportSnapshots = renderTargetsRegistry.viewportSnapshots();

        // Frame Synchronization is now done via GLFWSwapBuffersSystems
        gameLoop.update(frameTiming, inputSnapshot); // inputSnapshot, viewportSnapshots);;

        frameTiming = framePacer.sync();
        fpsMetrics.addFrame(frameTiming);
    }

    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));

    return 0;
}