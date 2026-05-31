
#include <utility>

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

    constexpr float FOVY               = radians(90.0f);
    constexpr float ASPECT_RATIO_NUMER = 16.0f;
    constexpr float ASPECT_RATIO_DENOM = 9.0f;

    constexpr auto SHADER_POOL_CAPACITY   = 10;
    constexpr auto MATERIAL_POOL_CAPACITY = 10;
    constexpr auto MESH_POOL_CAPACITY     = 10;
    constexpr auto FRAMEBUFFER_POOL_CAPACITY = 10;
    constexpr auto VIEWPORT_POOL_CAPACITY    = 10;


    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================

    // inputmanager
    auto deadzoneStrategy = RadialDeadzoneStrategy();
    /*const auto inputManager = std::make_unique<InputManager>(
        std::make_unique<helios::ext::glfw::input::GLFWInputAdapter>(std::move(deadzoneStrategy))
    );*/

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
        ASPECT_RATIO_NUMER,
        ASPECT_RATIO_DENOM
    });

    
    // ========================================
    // Scene and Viewport Setup
    // ========================================

    auto WindowRenderTarget = gameWorld.add<RenderTargetHandle>(RenderTargetId{"WindowRenderTarget"});
    WindowRenderTarget.add<OpenGLRenderTargetIdComponent<RenderTargetHandle>>(0);
    WindowRenderTarget.add<Size2DComponent<RenderTargetHandle>>();
    WindowRenderTarget.add<ClearComponent<RenderTargetHandle>>(ClearFlags::Color);
    WindowRenderTarget.add<ColorComponent<RenderTargetHandle>>(0.0f);
    auto MainViewport = gameWorld.add<ViewportHandle>(ViewportId{"MainViewport"});
    MainViewport.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    MainViewport.add<ColorComponent<ViewportHandle>>(0.5f);

    auto MainScene = gameWorld.add<SceneHandle>(SceneId("MainScene"));

    MainWindow.add<RenderTargetBindingComponent<WindowHandle>>(WindowRenderTarget);

    // RenderTarget : Viewport (1:N)
    MainViewport.add<RenderTargetBindingComponent<ViewportHandle>>(WindowRenderTarget);
    MainViewport.add<BoundsComponent<ViewportHandle>>(helios::math::vec4f{.5f, .5f, .5f, .5f});

    // Viewport : Scene (N:1)
    MainViewport.add<SceneBindingComponent<ViewportHandle>>(MainScene);

    auto player = gameWorld.add<GameObjectHandle>();
    player.add<SceneMemberComponent<GameObjectHandle>>(MainScene);

    auto MainCamera = gameWorld.add<GameObjectHandle>(GameObjectId("MainCamera"));
    MainCamera.add<PerspectiveCameraComponent<GameObjectHandle>>();
    MainCamera.add<ProjectionMatrixComponent<GameObjectHandle>>();
    MainCamera.add<ViewMatrixComponent<GameObjectHandle>>();
    MainCamera.add<UpVector3DComponent<GameObjectHandle>>(0.0f, 1.0f, 0.0f);
    MainCamera.add<TargetPosition3DComponent<GameObjectHandle>>(0.0f, 0.0f, 0.0f);
    MainCamera.add<Position3DComponent<GameObjectHandle>>(0.0f, 0.0f, 1.0f);
    MainCamera.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
    // later on: rebuildHandleMultiMapFromSceneMembership(). SSoT w/ components, but systems get the
    // multimaps for faster access / querying?
    // or the view gets extended internally that it can fall back to a multimap, e.g. filter<> instead of view<>
    // or some other adequate semantic name

    MainViewport.add<CameraBindingComponent<ViewportHandle>>(MainCamera);


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
    CubeShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Draw>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"},
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );

    auto CubeMesh = gameWorld.add<MeshHandle>(MeshId("CubeMesh"));
    CubeMesh.add<MeshDataComponent<MeshHandle>>(Triangle::meshData());
    CubeMesh.add<MeshUploadRequestComponent<MeshHandle>>();

    auto CubeMaterial = gameWorld.add<MaterialHandle>(MaterialId("CubeMaterial"));
    CubeMaterial.add<ColorComponent<MaterialHandle>>(1.0f, 0.0f, 0.0f, 1.0f);

    // ========================================
    // Entity Setup
    // ========================================
    // cube
    auto cube = gameWorld.add<GameObjectHandle>();
    cube.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
    cube.add<WorldBoundsComponent<GameObjectHandle>>();
    cube.add<WorldMatrixComponent<GameObjectHandle>>(1.0f);
    cube.add<RenderPrototypeComponent<GameObjectHandle>>(CubeShader, CubeMaterial, CubeMesh);


    // ==============================================
    // Map Scenes to Viewports, Cameras with Viewports.
    // ==============================================
    //auto mainViewportEntity = gameWorld.addGameObject();
    //mainViewportEntity.add<ViewportComponent>(MainViewportHandle, MainSceneHandle, camera.entityHandle());
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
    imguiOverlay.addWidget(menu);
    imguiOverlay.addWidget(fpsWidget);
    imguiOverlay.addWidget(logWidget);

    // ----------------------------------------
    // 2.5 Logger Configuration
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

                 //.addSystem<LocalComposeTransformSystem>()
                 //.addSystem<WorldTransformSystem>()

                .addSystem<PerspectiveCameraUpdateSystem<GameObjectHandle>>()
                // this will produce render commands after scenes have been culled according to
                // their active viewports
                .addSystem<
                    SceneMemberRenderContextExtractionSystem<
                        ViewportHandle,
                        GameObjectHandle,
                        NoCullingStrategy<GameObjectHandle>,
                        RenderCommandBuffer
                    >
                >(NoCullingStrategy<GameObjectHandle>())
                .addCommitPoint(CommitPoint::Structural)

                 // Clear, bufferswapping
                .addPass<EngineState>(EngineState::Running)
                .addSystem<ImGuiOverlayRenderSystem>(imguiOverlay)
                // WindowSizeUpdateSystem is not used right now:
                // it was mainly used for framebufefr resizing, which is now handled
                // directly in the GLFWPlatformManager
                //.addSystem<WindowSizeUpdateSystem<WindowHandle>>()
                //.addSystem<WindowSizeDirtyClearSystem<WindowHandle>>()
                .addSystem<SwapBuffersSystem<WindowHandle, PlatformCommandBuffer>>()
                .addSystem<GLFWWindowCloseSystem<WindowHandle, PlatformCommandBuffer>>()
                .addSystem<WindowBasedShutdownSystem<WindowHandle, PlatformCommandBuffer>>()
                .addSystem<ClearDirtySystem<
                    GameObjectHandle,
                    PerspectiveCameraComponent,
                    TargetPosition3DComponent,
                    Position3DComponent,
                    Direction3DComponent,
                    UpVector3DComponent>
                >()
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