
#include <utility>
#include <functional>
import helios.engine;
import helios.math;
import helios.opengl;
import helios.glfw;
import helios.ecs;
import helios.imgui;


#include "Namespaces.h"


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


    // gameworld
    auto [gameWorldPtr, gameLoopPtr] = bootstrapGameWorld();
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
    MainRenderTarget.add<Size2DComponent<RenderTargetHandle>>();
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
    CullingCamera.add<PerspectiveCameraComponent<GameObjectHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM));
    CullingCamera.add<ProjectionMatrixComponent<GameObjectHandle>>();
    CullingCamera.add<ViewMatrixComponent<GameObjectHandle>>();
    CullingCamera.add<YawPitchRollComponent<GameObjectHandle>>();
    CullingCamera.add<Rotation3DComponent<GameObjectHandle, Local>>();
    CullingCamera.add<TransformComponent<GameObjectHandle, World>>(1.0f);
    CullingCamera.add<Position3DComponent<GameObjectHandle, Local>>(0.0f, 0.0f, -50.0f);
    CullingCamera.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
    // later on: rebuildHandleMultiMapFromSceneMembership(). SSoT w/ components, but systems get the
    // multimaps for faster access / querying?
    // or the view gets extended internally that it can fall back to a multimap, e.g. filter<> instead of view<>
    // or some other adequate semantic name
    CullingViewport.add<CameraBindingComponent<ViewportHandle>>(CullingCamera);

    auto CullingCamera_bottom = gameWorld.add<GameObjectHandle>(GameObjectId("CullingCamera_bottom"));
    CullingCamera_bottom.add<DebugNameComponent<GameObjectHandle>>("CullingCamera_bottom");
    CullingCamera_bottom.add<PerspectiveCameraComponent<GameObjectHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM));
    CullingCamera_bottom.add<ProjectionMatrixComponent<GameObjectHandle>>();
    CullingCamera_bottom.add<ViewMatrixComponent<GameObjectHandle>>();
    CullingCamera_bottom.add<YawPitchRollComponent<GameObjectHandle>>();
    CullingCamera_bottom.add<Rotation3DComponent<GameObjectHandle, Local>>();
    CullingCamera_bottom.add<Position3DComponent<GameObjectHandle, Local>>(0.0f, 0.0f, -110.0f);
    CullingCamera_bottom.add<TransformComponent<GameObjectHandle, World>>(1.0f);
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
    /*
    CubeShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Material>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"},
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );*/
    CubeShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Draw>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"},
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
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
        4, sizeof(InstanceData), offsetof(InstanceData, modelMatrix),1}
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
            cube.add<BoundsComponent<GameObjectHandle, Local>>(Triangle::boundsData());
            cube.add<BoundsComponent<GameObjectHandle, World>>();
            cube.add<Rotation3DComponent<GameObjectHandle, Local>>();
            cube.add<Position3DComponent<GameObjectHandle, Local>>(static_cast<float>(x), static_cast<float>(y), 0.0f);

            cube.add<TransformComponent<GameObjectHandle, World>>(1.0f);
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
                .addPass<EngineState>(EngineState::Any)
                .addSystem<EngineFlowSystem<EngineCommandBuffer>>()
                .addCommitPoint(CommitPoint::Structural)

                .addPass<EngineState>(EngineState::Booting)
                .addSystem<PlatformInitSystem<PlatformCommandBuffer>>()
                .addCommitPoint(CommitPoint::Structural)

                .addPass<EngineState>(EngineState::Booted | EngineState::Running)
                .addSystem<PollEventsSystem<PlatformCommandBuffer>>()
                .addSystem<WindowCreateSystem<WindowHandle, PlatformCommandBuffer>>()
                .addCommitPoint(CommitPoint::Structural)

                .addPass<EngineState>(EngineState::Warmup)
                .addSystem<MeshUploadSystem<MeshHandle, RenderCommandBuffer>>()
                .addSystem<ShaderCompileSystem<ShaderHandle, RenderCommandBuffer>>()
                .addSystem<WarmupDoneSystem<ShaderHandle, EngineCommandBuffer>>()
                .addCommitPoint(CommitPoint::Structural)

                .addPass<EngineState>(EngineState::Running);

            gameLoop.phase(PhaseType::Main)
                .addPass(EngineState::Running)
                //.addSystem<PerspectiveProjectionUpdateSystem<GameObjectHandle>>()
                //.addSystem<CameraLookAtSystem<GameObjectHandle>>()
                //.addSystem<ViewMatrixUpdateSystem<GameObjectHandle>>()
                ;

            gameLoop.phase(PhaseType::Post)
                 .addPass(EngineState::Running)

                .addSystem<YawPitchRollUpdateSystem<GameObjectHandle>>()
                .addSystem<WorldTransformSystem<GameObjectHandle>>()
                .addSystem<WorldBoundsUpdateSystem<GameObjectHandle>>()
                .addSystem<PerspectiveCameraUpdateSystem<GameObjectHandle>>()
                // this will produce render commands after scenes have been culled according to
                // their active viewports
                .addSystem<
                    SceneRenderSystem<
                        ViewportHandle,
                        GameObjectHandle,
                        AABBCullingStrategy<GameObjectHandle>,
                        RenderCommandBuffer
                    >
                >(AABBCullingStrategy<GameObjectHandle>(), sceneMemberVisibilityRegistry)
                .addSystem<LambdaSystem<GameObjectHandle>>(
                    [&](UpdateContext& updateContext) {
                        const auto viewport = CullingViewport.handle();

                        auto enableMemberMaterialOverride = [&](auto memberRange, const bool enable) {
                            for (const auto goHandle : memberRange) {
                                auto go = updateContext.find<GameObjectHandle>(goHandle);
                                if (!go) {
                                    continue;
                                }

                                auto* rpc = go->template get<RenderPrototypeComponent<GameObjectHandle, Instanced>>();
                                rpc->setMaterialHandle(enable ? CubeMaterialOverride.handle() : CubeMaterial.handle());
                            }
                        };

                        enableMemberMaterialOverride(sceneMemberVisibilityRegistry.visibleMembers(viewport), false);
                        enableMemberMaterialOverride(sceneMemberVisibilityRegistry.culledMembers(viewport), true);

                    }
                )
                .addCommitPoint(CommitPoint::Structural)

                 // Clear, bufferswapping
                .addPass<EngineState>(EngineState::Running)
                // WindowSizeUpdateSystem is not used right now:
                // it was mainly used for framebufefr resizing, which is now handled
                // directly in the GLFWPlatformManager
                //.addSystem<WindowSizeUpdateSystem<WindowHandle>>()
                //.addSystem<WindowSizeDirtyClearSystem<WindowHandle>>()
                .addSystem<GLFWWindowCloseSystem<WindowHandle, PlatformCommandBuffer>>()
                .addSystem<WindowBasedShutdownSystem<WindowHandle, PlatformCommandBuffer>>()
                .addSystem<ClearDirtySystem<
                    GameObjectHandle,
                    DirtyComponentSpec<PerspectiveCameraComponent>,
                    DirtyComponentSpec<Position3DComponent, Local>,
                    DirtyComponentSpec<TransformComponent, World>,
                    DirtyComponentSpec<BoundsComponent, Local>,
                    DirtyComponentSpec<BoundsComponent, World>,
                    DirtyComponentSpec<Rotation3DComponent, Local>,
                    DirtyComponentSpec<Direction3DComponent>
                >>()
                .addSystem<SceneMemberVisibilityRegistryClearSystem<GameObjectHandle>>(sceneMemberVisibilityRegistry)
                .addSystem<ImGuiOverlayRenderSystem>(imguiOverlay)
                .addSystem<SwapBuffersSystem<WindowHandle, PlatformCommandBuffer>>()
                .addCommitPoint(CommitPoint::Structural)

                .addPass<EngineState>(EngineState::Shutdown)
                .addSystem<DestroySessionSystem>()
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