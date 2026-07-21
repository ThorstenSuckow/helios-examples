
#include <utility>
#include <functional>
#include <thread>

import helios.ecs;
import helios.engine;
import helios.math;
import helios.physics;
import helios.opengl;
import helios.glfw;
import helios.imgui;


#include "Namespaces.h"

helios::math::vec3f randomVec3f(const std::uint32_t seed) {

    auto rand = helios::engine::util::Random(seed);

    return {
        rand.randomFloat(-1.0f, 1.0f),
        rand.randomFloat(-1.0f, 1.0f),
        rand.randomFloat(-1.0f, 1.0f)
    };
}


int main() {
    auto& logger = helios::engine::util::log::LogManager::loggerForScope("main");

    // ========================================
    // Constants
    // ========================================
    constexpr unsigned int SCREEN_WIDTH  = 1280;
    constexpr unsigned int SCREEN_HEIGHT = 720;

    constexpr float WINDOW_ASPECT_RATIO_NUMER = 16.0f;
    constexpr float WINDOW_ASPECT_RATIO_DENOM = 9.0f;


    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================


    JobSystem jobSystem{std::max(1u, std::thread::hardware_concurrency() - 1)};
    // gameworld
    auto [gameWorldPtr, gameLoopPtr] = bootstrapGameWorld(jobSystem);
    auto& gameWorld = *gameWorldPtr;
    auto& gameLoop = *gameLoopPtr;


    // Renderbackend
    auto renderBackend = OpenGLBackend(gameWorld.engineWorld());

    // register additional managers
    gameWorld.registerManager<helios::engine::rendering::RenderManager<OpenGLBackend, GameObjectHandle>>(renderBackend);

    gameWorld.registerManager<GLFWPlatformManager<
        OpenGLBackend,
        WindowHandle,
        EngineCommandBuffer,
        PlatformCommandBuffer>>(
        renderBackend,
        gameWorld.platformWorld(), gameWorld.resourceRegistry().commandBufferRegistry()
    );

    SceneMemberVisibilityRegistry<GameObjectHandle> sceneMemberVisibilityRegistry{};


    gameWorld.registerManager<OpenGLMeshUploadManager<MeshHandle>>(gameWorld.renderResourceWorld());
    gameWorld.registerManager<OpenGLShaderCompileManager<ShaderHandle, OpenGLUniformLocationCacheStrategy<ShaderHandle>>>(
        gameWorld.renderResourceWorld(),
        OpenGLUniformLocationCacheStrategy<ShaderHandle>()
    );


    // ========================================
    // Window Setup
    // ========================================
    auto MainWindow = gameWorld.add<WindowHandle>(WindowId("MainWindow"));
    MainWindow.add<WindowCreateRequestComponent<WindowHandle>>(WindowConfig{
        "helios - ECS Rendering Demo",
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        WINDOW_ASPECT_RATIO_NUMER,
        WINDOW_ASPECT_RATIO_DENOM
    });

    
    // ========================================
    // Scene and Viewport Setup
    // ========================================

    auto MainRenderTarget = gameWorld.add<RenderTargetHandle>(RenderTargetId{"MainRenderTarget"});
    MainRenderTarget.add<OpenGLRenderTargetIdComponent<RenderTargetHandle>>(0);
    MainRenderTarget.trackDirty<Size2DComponent<RenderTargetHandle>>();
    MainRenderTarget.add<ClearComponent<RenderTargetHandle>>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent<RenderTargetHandle>>(helios::engine::util::Colors::Black);

    auto CullingViewport = gameWorld.add<ViewportHandle>(ViewportId{"CullingViewport"});
    CullingViewport.add<DebugNameComponent<ViewportHandle>>("CullingViewport");
    CullingViewport.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    CullingViewport.add<ColorComponent<ViewportHandle>>(helios::engine::util::Colors::LightGray);
    // RenderTarget : Viewport (1:N)
    CullingViewport.add<RenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    CullingViewport.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0f, .5f, 1.0f, 0.5f});

    auto CullingViewport_bottom = gameWorld.add<ViewportHandle>(ViewportId{"CullingViewport_bottom"});
    CullingViewport_bottom.add<DebugNameComponent<ViewportHandle>>("CullingViewport_bottom");
    CullingViewport_bottom.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    CullingViewport_bottom.add<ColorComponent<ViewportHandle>>(helios::engine::util::Colors::Gray);
    // RenderTarget : Viewport (1:N)
    CullingViewport_bottom.add<RenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    CullingViewport_bottom.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0f, .0f, 1.0f, 0.5f});

    auto MainScene = gameWorld.add<SceneHandle>(SceneId("MainScene"));
    MainWindow.add<RenderTargetBindingComponent<WindowHandle>>(MainRenderTarget);

    // Viewport : Scene (N:1)
    CullingViewport.add<SceneBindingComponent<ViewportHandle>>(MainScene);
    CullingViewport_bottom.add<SceneBindingComponent<ViewportHandle>>(MainScene);

    auto CullingCamera = gameWorld.add<GameObjectHandle>(GameObjectId("CullingCamera"));
    CullingCamera.add<DebugNameComponent<GameObjectHandle>>("CullingCamera");
    CullingCamera.trackDirty<PerspectiveCameraComponent<GameObjectHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM));
    CullingCamera.trackDirty<ProjectionMatrixComponent<GameObjectHandle>>();
    CullingCamera.trackDirty<ViewMatrixComponent<GameObjectHandle>>();
    CullingCamera.trackDirty<YawPitchRollComponent<GameObjectHandle>>();
    CullingCamera.trackDirty<Rotation3DComponent<GameObjectHandle, Local>>();
    CullingCamera.trackDirty<TransformComponent<GameObjectHandle, World>>(1.0f);
    CullingCamera.trackDirty<Position3DComponent<GameObjectHandle, Local>>(0.0f, 0.0f, -50.0f);
    CullingCamera.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
    // later on: rebuildHandleMultiMapFromSceneMembership(). SSoT w/ components, but systems get the
    // multimaps for faster access / querying?
    // or the view gets extended internally that it can fall back to a multimap, e.g. filter<> instead of view<>
    // or some other adequate semantic name
    CullingViewport.add<CameraBindingComponent<ViewportHandle>>(CullingCamera);

    auto CullingCamera_bottom = gameWorld.add<GameObjectHandle>(GameObjectId("CullingCamera_bottom"));
    CullingCamera_bottom.add<DebugNameComponent<GameObjectHandle>>("CullingCamera_bottom");
    CullingCamera_bottom.trackDirty<PerspectiveCameraComponent<GameObjectHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM));
    CullingCamera_bottom.trackDirty<ProjectionMatrixComponent<GameObjectHandle>>();
    CullingCamera_bottom.trackDirty<ViewMatrixComponent<GameObjectHandle>>();
    CullingCamera_bottom.trackDirty<YawPitchRollComponent<GameObjectHandle>>();
    CullingCamera_bottom.trackDirty<Rotation3DComponent<GameObjectHandle, Local>>();
    CullingCamera_bottom.trackDirty<Position3DComponent<GameObjectHandle, Local>>(0.0f, 0.0f, -110.0f);
    CullingCamera_bottom.trackDirty<TransformComponent<GameObjectHandle, World>>(1.0f);
    CullingCamera_bottom.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
    CullingViewport_bottom.add<CameraBindingComponent<ViewportHandle>>(CullingCamera_bottom);


    // ========================================
    // Rendering Management setup
    // ========================================

    // shader, mesh, material for cube
    auto CubeShader = gameWorld.add<ShaderHandle>(ShaderId("CubeShader"));
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

    auto CubeMesh = gameWorld.add<MeshHandle>(MeshId("CubeMesh"));
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
        4, sizeof(InstanceData<GameObjectHandle>), offsetof(InstanceData<GameObjectHandle>, modelMatrix),1}
    );

    auto CubeMaterial = gameWorld.add<MaterialHandle>(MaterialId("CubeMaterial"));
    CubeMaterial.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::Red);

    auto CubeMaterialOverride = gameWorld.add<MaterialHandle>(MaterialId("CubeMaterialOverride"));
    CubeMaterialOverride.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::White);

    // ========================================
    // Entity Setup
    // ========================================
    // cubes
    for (int x = -142; x < 143; x+=3) {
        for (int y = -142; y < 143 ; y+=3) {
            auto cube = gameWorld.add<GameObjectHandle>();
            cube.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
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
            cube.add<RenderPrototypeComponent<GameObjectHandle, Instanced>>(
                CubeShader.handle(), CubeMaterial.handle(), CubeMesh.handle()
            );
        }
    }



    // ==============================================
    // Map Scenes to Viewports, Cameras with Viewports.
    // ==============================================
    //auto mainViewportEntity = gameWorld.addGameObject();
    //mainViewportEntity.add<ViewportComponent>(CullingViewportHandle, MainSceneHandle, camera.entityHandle());
    // entities are described through the sum of their parts.
    // if we have a ViewportComponent and a UniformValueMapComponent, those values are treated per frame.



    // ----------------------------------------
    // ImGui and Debug Tooling
    // ----------------------------------------
    auto imguiBackend = ImGuiGlfwOpenGLBackend(MainWindow.handle(), gameWorld.platformWorld());
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
    LogManager::getInstance().enableSink<ImGuiLogSink>(logWidget);


    // ========================================
    // Initialization of GameWorld and Game Loop
    // ========================================
    float DELTA_TIME = 0.0f;



    // ----------------------------------------
    // GameLoop Config
    // ----------------------------------------
    gameLoop.phase(PhaseType::Pre)
                .beginPass<EngineState>(EngineState::Any)
                    .addSystem<EngineFlowSystem<EngineCommandBuffer>>()
                .submit<EngineCommandBuffer>()
                .flush<EngineStateManager>()
                .endPass()

                .beginPass<EngineState>(EngineState::Booting)
                    .addSystem<PlatformInitSystem<PlatformCommandBuffer>>()
                .submit<PlatformCommandBuffer>()
                .flush<GLFWPlatformManager<
                    OpenGLBackend,
                    WindowHandle,
                    EngineCommandBuffer,
                    PlatformCommandBuffer>>()
                .endPass()

                .beginPass<EngineState>(EngineState::Booted | EngineState::Running)
                    .addSystem<PollEventsSystem<PlatformCommandBuffer>>()
                    .addSystem<WindowCreateSystem<WindowHandle, PlatformCommandBuffer>>()
                .submit<PlatformCommandBuffer>()
                .flush<GLFWPlatformManager<
                    OpenGLBackend,
                    WindowHandle,
                    EngineCommandBuffer,
                    PlatformCommandBuffer>>()
                .endPass()

                .beginPass<EngineState>(EngineState::Warmup)
                    .addSystem<MeshUploadSystem<MeshHandle, RenderCommandBuffer>>()
                    .addSystem<ShaderCompileSystem<ShaderHandle, RenderCommandBuffer>>()
                    .addSystem<WarmupDoneSystem<ShaderHandle, EngineCommandBuffer>>()
                .submit<RenderCommandBuffer, EngineCommandBuffer>()
                .flush<
                    OpenGLMeshUploadManager<MeshHandle>,
                    OpenGLShaderCompileManager<ShaderHandle, OpenGLUniformLocationCacheStrategy<ShaderHandle>>,
                    EngineStateManager
                >()
                .endPass();


            gameLoop.phase(PhaseType::Main)
                .beginPass(EngineState::Running)
                .endPass();

            gameLoop.phase(PhaseType::Post)
                 .beginPass(EngineState::Running)

                    .addSystem<YawPitchRollUpdateSystem<GameObjectHandle>>()
                    .addSystem(callableSystemForLambda<GameObjectHandle>(
                        // replacement for systems that compute the local velocity from intended velocity,
                        // such as component systems
                        [&](UpdateContext& updateContext) {
                            for (auto [
                                entity,
                                intendedVelocity,
                                localVelocity
                            ] : updateContext.view<
                                GameObjectHandle,
                                helios::physics::motion::components::Velocity3DComponent<GameObjectHandle, Intent>,
                                helios::physics::motion::components::Velocity3DComponent<GameObjectHandle, Local>
                            >()
                                .withActive()
                               .template whereAnyDirty<
                                   Active<GameObjectHandle>,
                                   helios::physics::motion::components::Velocity3DComponent<GameObjectHandle, Intent>
                               >()) {
                                entity.setTrackedValue(localVelocity, intendedVelocity->value());
                              //  intendedVelocity->setValue({0.0f, 0.0f, 0.0f});
                            }
                        }
                    ))
                    .addSystem<MotionIntegrationSystem<GameObjectHandle>>()
                    .addSystem<WorldTransformSystem<GameObjectHandle>>()
                    .addSystem<WorldBoundsUpdateSystem<GameObjectHandle>>()
                    .addSystem<PerspectiveCameraUpdateSystem<GameObjectHandle>>()
                    // this will produce render commands after scenes have been culled according to
                    // their active viewports
                    .addSystem<
                        SceneMemberVisibilitySystem<
                            ViewportHandle,
                            GameObjectHandle,
                            AABBCullingStrategy<GameObjectHandle>
                        >
                    >(AABBCullingStrategy<GameObjectHandle>(), sceneMemberVisibilityRegistry)
                    .addSystem(callableSystemForLambda<GameObjectHandle>(
                        [&](UpdateContext& updateContext) {
                            const auto viewport = CullingViewport.handle();

                            auto enableMemberMaterialOverride = [&](auto memberContexts, const bool enable) {
                                for (const auto member : memberContexts) {
                                    auto go = updateContext.find<GameObjectHandle>(member.memberHandle);
                                    if (!go) {
                                        continue;
                                    }

                                    auto* rpc = go->template get<RenderPrototypeComponent<GameObjectHandle, Instanced>>();
                                    rpc->setMaterialHandle(enable ? CubeMaterialOverride.handle() : CubeMaterial.handle());
                                }
                            };

                            enableMemberMaterialOverride(sceneMemberVisibilityRegistry.visibleMembers<Instanced>(viewport), false);
                            enableMemberMaterialOverride(sceneMemberVisibilityRegistry.culledMembers<Instanced>(viewport), true);

                        }
                    ))
                    .addSystem<
                        SceneRenderSystem<
                            ViewportHandle,
                            GameObjectHandle,
                            RenderCommandBuffer
                        >
                    >(sceneMemberVisibilityRegistry)
                .submit<RenderCommandBuffer>()
                .flush<RenderManager<OpenGLBackend, GameObjectHandle>>()// buffer -> manager
                .endPass()


                 // Clear, bufferswapping
                .beginPass<EngineState>(EngineState::Running)
                    .addSystem<GLFWWindowCloseSystem<WindowHandle, PlatformCommandBuffer>>()
                    .addSystem<WindowBasedShutdownSystem<WindowHandle, PlatformCommandBuffer>>()
                    .addSystem<ClearAllDirtySetsSystem>()
                    .addSystem<ImGuiOverlayRenderSystem>(imguiOverlay)
                    .addSystem<SwapBuffersSystem<WindowHandle, PlatformCommandBuffer>>()
                .submit<PlatformCommandBuffer>()
                .flush<GLFWPlatformManager<
                    OpenGLBackend,
                    WindowHandle,
                    EngineCommandBuffer,
                    PlatformCommandBuffer>>()
                .endPass()

                .beginPass<EngineState>(EngineState::Shutdown)
                    .addSystem<DestroySessionSystem>()
                .endPass()
            ;


    gameLoop.init(gameWorld.init());


    while (gameLoop.isRunning(gameWorld)) {

        framePacer.beginFrame();

        //app->eventManager().dispatchAll();
        //inputManager.poll(0.0f);

        // Game Logic Update
        const GamepadState gamepadState = GamepadState();// = {};//inputManager->gamepadState(Gamepad::ONE);
        const auto inputSnapshot = InputSnapshot(gamepadState);

        //const auto viewportSnapshots = renderTargetsRegistry.viewportSnapshots();


        // Frame Synchronization is now done via GLFWSwapBuffersSystems
        gameLoop.update(gameWorld, DELTA_TIME, inputSnapshot);//inputSnapshot, viewportSnapshots);;



        frameStats = framePacer.sync();
        fpsMetrics.addFrame(frameStats);
        DELTA_TIME = frameStats.totalFrameTime;
    }


    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));


    return 0;
}