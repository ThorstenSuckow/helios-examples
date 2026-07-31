
#include <utility>
#include <functional>
#include <format>
#include <algorithm>
#include <thread>

import helios.ecs;
import helios.engine;
import helios.math;
import helios.physics;
import helios.opengl;
import helios.glfw;
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

    constexpr bool ENABLE_VSYNC = false;

    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================

    auto maxWorker = std::max(1u, std::thread::hardware_concurrency() - 1);
    JobSystem jobSystem(maxWorker);

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

    SceneMemberVisibilityRegistry<GameObjectHandle, NonInstanced> visibilityRegistry{};

    gameWorld.registerManager<OpenGLTextureUploadManager<TextureHandle>>(gameWorld.renderResourceWorld(), ImageReader{});
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
        "helios - Game of Life",
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        WINDOW_ASPECT_RATIO_NUMER,
        WINDOW_ASPECT_RATIO_DENOM,
        ENABLE_VSYNC
    });

    
    // ========================================
    // Scene and Viewport Setup
    // ========================================

    auto MainRenderTarget = gameWorld.add<RenderTargetHandle>(RenderTargetId{"MainRenderTarget"});
    MainRenderTarget.add<OpenGLRenderTargetIdComponent<RenderTargetHandle>>(0);
    MainRenderTarget.trackDirty<Size2DComponent<RenderTargetHandle>>();
    MainRenderTarget.add<ClearComponent<RenderTargetHandle>>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent<RenderTargetHandle>>(helios::engine::util::Colors::Black);

    auto MainViewport = gameWorld.add<ViewportHandle>(ViewportId{"MainViewport"});
    MainViewport.add<DebugNameComponent<ViewportHandle>>("MainViewport");
    MainViewport.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    MainViewport.add<ColorComponent<ViewportHandle>>(helios::engine::util::Colors::Black);
    // RenderTarget : Viewport (1:N)
    MainViewport.add<RenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    MainViewport.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0f, 0.0f, 1.0f, 1.0f});


    auto MainScene = gameWorld.add<SceneHandle>(SceneId("MainScene"));
    MainWindow.add<RenderTargetBindingComponent<WindowHandle>>(MainRenderTarget);

    // Viewport : Scene (N:1)
    MainViewport.add<SceneBindingComponent<ViewportHandle>>(MainScene);

    auto MainCamera = gameWorld.add<CameraHandle>(CameraId("MainCamera"));
    MainCamera.add<DebugNameComponent<CameraHandle>>("MainCamera");
    MainCamera.trackDirty<PerspectiveCameraComponent<CameraHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / WINDOW_ASPECT_RATIO_DENOM);
    MainCamera.trackDirty<ProjectionMatrixComponent<CameraHandle>>();
    MainCamera.trackDirty<ViewMatrixComponent<CameraHandle>>();
    MainCamera.trackDirty<YawPitchRollComponent<CameraHandle>>();
    MainCamera.trackDirty<Rotation3DComponent<CameraHandle, Local>>();
    MainCamera.trackDirty<TransformComponent<CameraHandle, World>>(1.0f);
    MainCamera.trackDirty<Position3DComponent<CameraHandle, Local>>(0.0f, 0.0f, -5.0f);

    // camera does not need a scene binding, since there is no camera selection
    // using scenes. The SceneMemberVisibilitySystem will directly use the Viewport's mapping
    //MainCamera.add<SceneBindingComponent<CameraHandle>>(MainScene);
    MainViewport.add<CameraBindingComponent<ViewportHandle>>(MainCamera);


    // ========================================
    // Rendering Management setup
    // ========================================

    // shader, mesh, material for cell
    auto DefaultShader = gameWorld.add<ShaderHandle>(ShaderId("DefaultShader"));
    DefaultShader.add<ShaderSourceComponent<ShaderHandle>>("./resources/shader/cell.vert", "./resources/shader/cell.frag");
    DefaultShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Pass>>(
        UniformMapping{UniformSemantics::ProjectionMatrix, "projectionMatrix"},
        UniformMapping{UniformSemantics::ViewMatrix, "viewMatrix"}
    );
    DefaultShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Material>>(
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );
    DefaultShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Draw>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"}
    );

    auto Texture = gameWorld.add<TextureHandle>(TextureId{"Texture"});
    Texture.add<TextureSourceComponent<TextureHandle>>("./resources/textures/garagecraft_games.png");

    auto TextureMesh = gameWorld.add<MeshHandle>(MeshId("TextureMesh"));
    TextureMesh.add<MeshDataComponent<MeshHandle>>(Rect::meshData());
    TextureMesh.add<MeshUploadRequestComponent<MeshHandle>>();
    TextureMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerVertex>>(
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::Position,  VertexAttributeType::Vec3f},
            0, sizeof(Vertex), offsetof(Vertex, position),0
        },
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::TextureCoordinates, VertexAttributeType::Vec2f},
            2, sizeof(Vertex), offsetof(Vertex, texCoords),0}
        );

    auto TriangleMaterial = gameWorld.add<MaterialHandle>(MaterialId("TriangleMaterial"));
    TriangleMaterial.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::Blue);
    TriangleMaterial.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::Blue);


    // ========================================
    // Entity Setup
    // ========================================
    auto triangle = gameWorld.add<GameObjectHandle>(GameObjectId{}, true);
    triangle.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
    triangle.trackDirty<BoundsComponent<GameObjectHandle, Local>>(Triangle::boundsData());
    triangle.trackDirty<BoundsComponent<GameObjectHandle, World>>();
    triangle.trackDirty<Rotation3DComponent<GameObjectHandle, Local>>();
    triangle.trackDirty<Position3DComponent<GameObjectHandle, Local>>(0.0f, 0.0f, 0.0f);
    triangle.trackDirty<Position3DComponent<GameObjectHandle, World>>(0.0f, 0.0f, 0.0f);
    triangle.trackDirty<TransformComponent<GameObjectHandle, World>>(1.0f);
    triangle.add<RenderPrototypeComponent<GameObjectHandle, NonInstanced>>(
        DefaultShader.handle(), TriangleMaterial.handle(), TextureMesh.handle(), Texture.handle()
    );

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
    LogManager::getInstance().enableLogging(false);
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
            .flush<EngineStateManager>()
            .endPass()

            .beginPass<EngineState>(EngineState::Booting)
                .addSystem<PlatformInitSystem<PlatformCommandBuffer>>()
            .flush<GLFWPlatformManager<OpenGLBackend, WindowHandle, EngineCommandBuffer, PlatformCommandBuffer>>()
            .endPass()

            .beginPass<EngineState>(EngineState::Booted | EngineState::Running)
                .addSystem<PollEventsSystem<PlatformCommandBuffer>>()
                .addSystem<WindowCreateSystem<WindowHandle, PlatformCommandBuffer>>()
            .flush<GLFWPlatformManager<OpenGLBackend, WindowHandle, EngineCommandBuffer, PlatformCommandBuffer>>()
            .endPass()

            .beginPass<EngineState>(EngineState::Warmup)
                .addSystem<TextureUploadSystem<TextureHandle, RenderCommandBuffer>>()
                .addSystem<MeshUploadSystem<MeshHandle, RenderCommandBuffer>>()
                .addSystem<ShaderCompileSystem<ShaderHandle, RenderCommandBuffer>>()
                .addSystem<WarmupDoneSystem<EngineCommandBuffer>>()

            .flush<
                OpenGLTextureUploadManager<TextureHandle>,
                OpenGLMeshUploadManager<MeshHandle>,
                OpenGLShaderCompileManager<ShaderHandle, OpenGLUniformLocationCacheStrategy<ShaderHandle>>,
                EngineStateManager
            >()
            .endPass();

            // intentionally left empty
            gameLoop.phase(PhaseType::Main).beginPass(EngineState::Running).endPass();

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
                    .addSystem<
                        SceneMemberVisibilitySystem<GameObjectHandle, NonInstanced, AABBCullingStrategy<GameObjectHandle>>
                        >(AABBCullingStrategy<GameObjectHandle>(), visibilityRegistry)

                    .addSystem<
                        SceneRenderSystem<GameObjectHandle, NonInstanced, RenderCommandBuffer>
                    >(visibilityRegistry)

                .flush<RenderManager<OpenGLBackend, GameObjectHandle>>()
                .endPass()

                 // Clear, bufferswapping
                .beginPass<EngineState>(EngineState::Running)
                    .addSystem<GLFWWindowCloseSystem<WindowHandle, PlatformCommandBuffer>>()
                    .addSystem<WindowBasedShutdownSystem<WindowHandle, PlatformCommandBuffer>>()
                    .addSystem<ClearAllDirtySetsSystem>()
                    .addSystem<ImGuiOverlayRenderSystem>(imguiOverlay)
                    .addSystem<SwapBuffersSystem<WindowHandle, PlatformCommandBuffer>>()
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