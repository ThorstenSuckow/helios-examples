
#include <utility>
#include <functional>
#include <format>

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

    constexpr int GRID_WIDTH = 150;
    constexpr int GRID_HEIGHT = 150;

    constexpr uint32_t SEED = 1234;

    auto RAND = helios::engine::util::Random(SEED);


    std::array<GameObjectHandle, GRID_WIDTH * GRID_HEIGHT> CELLS;
    std::array<bool, GRID_WIDTH * GRID_HEIGHT> CELL_GEN_CURRENT;
    std::array<bool, GRID_WIDTH * GRID_HEIGHT> CELL_GEN_NEXT;

    auto toGridIndex = [&](int x, int y)-> unsigned int {

        x = (x % GRID_WIDTH + GRID_WIDTH) % GRID_WIDTH;
        y = (y % GRID_HEIGHT + GRID_HEIGHT) % GRID_HEIGHT;

        return y * GRID_WIDTH + x;
    };

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
        "helios - Game of Life",
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

    auto CellViewport = gameWorld.add<ViewportHandle>(ViewportId{"CellViewport"});
    CellViewport.add<DebugNameComponent<ViewportHandle>>("CellViewport");
    CellViewport.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    CellViewport.add<ColorComponent<ViewportHandle>>(helios::engine::util::Colors::Black);
    // RenderTarget : Viewport (1:N)
    CellViewport.add<RenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    CellViewport.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0f, 0.0f, 1.0f, 1.0f});


    auto MainScene = gameWorld.add<SceneHandle>(SceneId("MainScene"));
    MainWindow.add<RenderTargetBindingComponent<WindowHandle>>(MainRenderTarget);

    // Viewport : Scene (N:1)
    CellViewport.add<SceneBindingComponent<ViewportHandle>>(MainScene);

    auto CellCamera = gameWorld.add<GameObjectHandle>(GameObjectId("CellCamera"));
    CellCamera.add<DebugNameComponent<GameObjectHandle>>("CellCamera");
    CellCamera.add<PerspectiveCameraComponent<GameObjectHandle>>(helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / WINDOW_ASPECT_RATIO_DENOM);
    CellCamera.add<ProjectionMatrixComponent<GameObjectHandle>>();
    CellCamera.add<ViewMatrixComponent<GameObjectHandle>>();
    CellCamera.add<YawPitchRollComponent<GameObjectHandle>>();
    CellCamera.add<Rotation3DComponent<GameObjectHandle, Local>>();
    CellCamera.add<TransformComponent<GameObjectHandle, World>>(1.0f);
    CellCamera.add<Position3DComponent<GameObjectHandle, Local>>(0.0f, 0.0f, -50.0f);
    CellCamera.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
    CellViewport.add<CameraBindingComponent<ViewportHandle>>(CellCamera);


    // ========================================
    // Rendering Management setup
    // ========================================

    // shader, mesh, material for cell
    auto CellShader = gameWorld.add<ShaderHandle>(ShaderId("CellShader"));
    CellShader.add<ShaderSourceComponent<ShaderHandle>>("./resources/cell.vert", "./resources/cell.frag");
    CellShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Pass>>(
        UniformMapping{UniformSemantics::ProjectionMatrix, "projectionMatrix"},
        UniformMapping{UniformSemantics::ViewMatrix, "viewMatrix"}
    );
    CellShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Material>>(
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );
    CellShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Draw>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"}
    );

    auto CellMesh = gameWorld.add<MeshHandle>(MeshId("CellMesh"));
    CellMesh.add<MeshDataComponent<MeshHandle>>(WireframeSphere::meshData());
    CellMesh.add<MeshUploadRequestComponent<MeshHandle>>();
    CellMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerVertex>>(
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::Position,  VertexAttributeType::Vec3f},
            0, sizeof(Vertex), offsetof(Vertex, position),0
        }

    );
    CellMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerInstance>>(
    VertexAttributeLayout{
        VertexAttribute{VertexAttributeSemantics::InstancedModelMatrix, VertexAttributeType::Mat4f},
        4, sizeof(InstanceData<GameObjectHandle>), offsetof(InstanceData<GameObjectHandle>, modelMatrix),1}
    );

    auto CellMaterial = gameWorld.add<MaterialHandle>(MaterialId("CellMaterial"));
    CellMaterial.add<ColorComponent<MaterialHandle>>(helios::engine::util::Colors::Gray);


    // ========================================
    // Entity Setup
    // ========================================
    // cubes
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT ; y++) {

            const bool isAlive = RAND.randomInt(0, 2) == 1;

            const auto cellIdx = toGridIndex(x, y);

            auto cell = gameWorld.add<GameObjectHandle>(
                GameObjectId(std::format("({0},{1})", x, y)), isAlive
            );
            cell.add<SceneMemberComponent<GameObjectHandle>>(MainScene);
            cell.add<BoundsComponent<GameObjectHandle, Local>>(Triangle::boundsData());
            cell.add<BoundsComponent<GameObjectHandle, World>>();
            cell.add<Rotation3DComponent<GameObjectHandle, Local>>();

            cell.add<Position3DComponent<GameObjectHandle, Local>>(static_cast<float>(x), static_cast<float>(y), 0.0f);
            cell.add<Position3DComponent<GameObjectHandle, World>>(0.0f, 0.0f, 0.0f);
            cell.add<TransformComponent<GameObjectHandle, World>>(1.0f);
            cell.add<RenderPrototypeComponent<GameObjectHandle, Instanced>>(
                CellShader.handle(), CellMaterial.handle(), CellMesh.handle()
            );

            CELLS[cellIdx]            = cell.handle();
            CELL_GEN_CURRENT[cellIdx] = CELL_GEN_NEXT[cellIdx]    = isAlive;
        }
    }



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
                .addSystem(callableSystemForLambda<GameObject>([&](UpdateContext& updateContext) {

                    for (int i = 0; i < GRID_WIDTH; i++) {
                        for (int j = 0; j < GRID_HEIGHT; j++) {

                            const auto refCellIdx = toGridIndex(i, j);
                            const bool isAlive = CELL_GEN_CURRENT[refCellIdx];

                            int aliveNeighbors = 0;

                            for (int a = i-1; a <= i+1 ; a++) {
                                for (int b = j-1; b <= j+1; b++) {

                                    if (a == i && b == j) {
                                        continue;
                                    }

                                    const auto neighborCellIdx = toGridIndex(a, b);

                                    if (CELL_GEN_CURRENT[neighborCellIdx]) {
                                        aliveNeighbors++;
                                    }
                                }
                            }

                            CELL_GEN_NEXT[refCellIdx] = aliveNeighbors == 3 || (isAlive && aliveNeighbors == 2);

                        }
                    }


                    CELL_GEN_CURRENT.swap(CELL_GEN_NEXT);

                    for (int refCellIdx = 0; refCellIdx < GRID_WIDTH * GRID_HEIGHT; refCellIdx++) {
                        auto cell = updateContext.find<GameObjectHandle>(CELLS[refCellIdx]);
                        if (CELL_GEN_CURRENT[refCellIdx]) {
                            cell->add<Active<GameObjectHandle>>();
                        } else {
                            cell->remove<Active<GameObjectHandle>>();
                        }
                    }

                }));

            gameLoop.phase(PhaseType::Post)
                 .addPass(EngineState::Running)

                .addSystem<YawPitchRollUpdateSystem<GameObjectHandle>>()
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

                .addSystem<
                    SceneRenderSystem<
                        ViewportHandle,
                        GameObjectHandle,
                        RenderCommandBuffer
                    >
                >(sceneMemberVisibilityRegistry)
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
                    DirtyComponentSpec<Direction3DComponent>,
                    DirtyComponentSpec<helios::physics::motion::components::Velocity3DComponent>
                >>()
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