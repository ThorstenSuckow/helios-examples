
#include <thread>
#include <utility>
#include <numbers>


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

#include <algorithm>
#include <cmath>

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

    constexpr int AXIS_LENGTH   = 500;
    constexpr int OBJECTS_PER_AXIS = 333;

    const int CELL_SIZE = std::max(2, static_cast<int>(AXIS_LENGTH / OBJECTS_PER_AXIS)) ;

    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================

    // gameworld
    auto engineRuntime = helios::engine::bootstrap::bootstrapGameWorld();

    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;

    gameWorld.resourceRegistry().emplace<helios::core::thread::ThreadPool>(
        std::max(1U, std::thread::hardware_concurrency() - 1U)
    );

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
    MainRenderTarget.add<Size2DComponent>();
    MainRenderTarget.add<ClearComponent>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Black);

    auto CullingViewport = gameWorld.add<helios::engine::bootstrap::ViewportHandle>();
    CullingViewport.add<DebugNameComponent<ViewportHandle>>("CullingViewport");
    CullingViewport.add<ClearComponent>(ClearFlags::Color);
    CullingViewport.add<ColorComponent>(helios::engine::rendering::common::types::Colors::LightGray);
    // RenderTarget : Viewport (1:N)
    CullingViewport.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);
    CullingViewport.add<RectComponent<>>(helios::math::vec4f{0.0F, 0.0f, 1.0F, 1.0F});


    auto MainScene = gameWorld.add<SceneHandle>();
    MainWindow.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);

    // Viewport : Scene (N:1)
    CullingViewport.add<DefaultSceneBindingComponent>(MainScene);

    auto CullingCamera = gameWorld.add<CameraHandle>();
    CullingCamera.add<DebugNameComponent<CameraHandle>>("CullingCamera");
    CullingCamera.add<PerspectiveCameraComponent>(
        helios::math::radians(90.0F), WINDOW_ASPECT_RATIO_NUMER / (WINDOW_ASPECT_RATIO_DENOM)
    );
    CullingCamera.add<ProjectionMatrixComponent>();
    CullingCamera.add<ViewMatrixComponent>();
    CullingCamera.add<YawPitchRollComponent>();
    CullingCamera.add<Rotation3DComponent<Local>>();
    CullingCamera.add<TransformComponent<World>>(1.0F);
    CullingCamera.add<Position3DComponent<Local>>(0.0F, 0.0F, (-AXIS_LENGTH / 2.0F) - 50.0f);
    CullingCamera.add<DefaultSceneMemberComponent>(MainScene);
    CullingViewport.add<DefaultCameraBindingComponent>(CullingCamera);


    // ========================================
    // Rendering Management setup
    // ========================================

    auto BoundaryShader = gameWorld.add<ShaderHandle>();
    BoundaryShader.add<ShaderSourceComponent>("./resources/cube.vert", "./resources/cube.frag");
    BoundaryShader.add<UniformMappingsComponent<UniformScope::Pass>>(
        UniformMapping{.semantics = UniformSemantics::ProjectionMatrix, .name = "projectionMatrix"},
        UniformMapping{.semantics = UniformSemantics::ViewMatrix, .name = "viewMatrix"}
    );
    BoundaryShader.add<UniformMappingsComponent<UniformScope::Material>>(
        UniformMapping{.semantics = UniformSemantics::MaterialBaseColor, .name = "color"}
    );
    BoundaryShader.add<UniformMappingsComponent<UniformScope::Draw>>(
        UniformMapping{.semantics = UniformSemantics::ModelMatrix, .name = "modelMatrix"}
    );

    auto BoundaryMesh = gameWorld.add<MeshHandle>();
    BoundaryMesh.add<MeshDataComponent>(Rect::meshData());
    BoundaryMesh.add<MeshUploadRequestComponent>();
    BoundaryMesh.add<VertexAttributeLayoutComponent<PerVertex>>(VertexAttributeLayout{
        .attribute =
            VertexAttribute{.semantics = VertexAttributeSemantics::Position, .type = VertexAttributeType::Vec3f},
        .location = 0,
        .stride = sizeof(Vertex),
        .offset = offsetof(Vertex, position),
        .divisor = 0
    });
    BoundaryMesh.add<VertexAttributeLayoutComponent<PerInstance>>(VertexAttributeLayout{
        .attribute =
            VertexAttribute{
                .semantics = VertexAttributeSemantics::InstancedModelMatrix, .type = VertexAttributeType::Mat4f
            },
        .location = 4,
        .stride = sizeof(InstanceData),
        .offset = offsetof(InstanceData, modelMatrix),
        .divisor = 1
    });

    auto BoundaryMaterial = gameWorld.add<MaterialHandle>();
    BoundaryMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Red);



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
    CubeMesh.add<MeshDataComponent>(Rect::meshData());
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

    // ========================================
    // Entity Setup
    // ========================================
    // boundaries
    auto createBoundaries = [&]<typename THandle>() {
        for (int i = 0; i < 4; i++) {
            auto bndLeft =  gameWorld.add<THandle>();

            switch (i) {
                case 0: bndLeft.template add<Position3DComponent<Local>>(0.0F, -AXIS_LENGTH/2 , 0.0F);
                    break;
                case 1: bndLeft.template add<Position3DComponent<Local>>(AXIS_LENGTH/2 , 0.0F, 0.0F);
                    break;
                case 2: bndLeft.template add<Position3DComponent<Local>>(0.0F, AXIS_LENGTH/2 , 0.0F);
                    break;
                case 3: bndLeft.template add<Position3DComponent<Local>>(-AXIS_LENGTH/2, 0.0F, 0.0F);
                    break;
            }
            bndLeft.template add<Scale3DComponent<Local>>(AXIS_LENGTH , 1.0F, 1.0F);
            bndLeft.template add<DefaultSceneMemberComponent>(MainScene);
            bndLeft.template add<BoundsComponent<Local>>(WireframeCube::boundsData());
            bndLeft.template add<Rotation3DComponent<Local>>(
                helios::math::quatf::fromEulerAngles<Intrinsic>(
                    0.0f, 0.0f, i* (std::numbers::pi / 2)
                )
            );
            bndLeft.template add<helios::physics::collision::components::CollisionComponent>();
            bndLeft.template add<BoundsComponent<World>>();
            bndLeft.template add<TransformComponent<World>>(1.0F);
            bndLeft.template add<DefaultRenderPrototypeComponent<Instanced>>(
                CubeShader.handle(), BoundaryMaterial.handle(), BoundaryMesh.handle()
            );
        }
    };

    auto createObjects = [&]<typename THandle>(MaterialHandle materialHandle, const int randomizerOffset = 0){
        int NUM_OBJECTS = 0;
        float delx = static_cast<float>(AXIS_LENGTH) / OBJECTS_PER_AXIS;
        float dely = static_cast<float>(AXIS_LENGTH) / OBJECTS_PER_AXIS;
        float startLeft = -AXIS_LENGTH / 2.0F;
        float endRight = AXIS_LENGTH / 2.0F;
        float endTop = AXIS_LENGTH / 2.0F;
        float startBottom = -AXIS_LENGTH / 2.0F;
        float x = startLeft;
        float y = startBottom;

        for (int i = 0; i < OBJECTS_PER_AXIS; i++) {
            x += delx;
            y = startBottom;
            for (int j = 0; j < OBJECTS_PER_AXIS; j++) {
                auto cube = gameWorld.add<THandle>();
                cube.template add<helios::physics::collision::components::CollisionComponent>();
                cube.template add<DefaultSceneMemberComponent>(MainScene);
                cube.template add<BoundsComponent<Local>>(WireframeCube::boundsData());
                cube.template add<BoundsComponent<World>>();
                cube.template add<Rotation3DComponent<Local>>();
                cube.template add<Scale3DComponent<Local>>(1.0F, 1.0F, 1.0F);
                cube.template add<Position3DComponent<Local>>(
                    static_cast<float>(
                        x <= startLeft  ? x + 5 : (x >= endRight ? x - 5 : x)
                    ), static_cast<float>(
                        y <= startBottom ? y + 5 : (y >= endTop ? y - 5 : y)
                    ), 0.0F
                );
                cube.template add<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
                cube.template add<helios::physics::motion::components::Velocity3DComponent<Local>>(
                    randomVec3f(NUM_OBJECTS + randomizerOffset).withZ(0.0F).normalize()
                );

                cube.template add<TransformComponent<World>>(1.0F);
                cube.template add<DefaultRenderPrototypeComponent<Instanced>>(
                    CubeShader.handle(), materialHandle, CubeMesh.handle()
                );
                NUM_OBJECTS++;

                y += dely;
            }
        }

        logger.info("Created {} objects.", NUM_OBJECTS);
    };

    auto GameObjectMaterial = gameWorld.add<MaterialHandle>();
    GameObjectMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Blue);

    auto ParticleMaterial = gameWorld.add<MaterialHandle>();
    ParticleMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Green);

    createObjects.operator()<GameObjectHandle>(GameObjectMaterial.handle());
    createObjects.operator()<ParticleHandle>(ParticleMaterial.handle(), OBJECTS_PER_AXIS * OBJECTS_PER_AXIS);
    createBoundaries.operator()<GameObjectHandle>();
    createBoundaries.operator()<ParticleHandle>();

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
            .add<
                Sequential<
                    PollEventsSystem,
                    WindowCreateSystem<WindowHandle>
                >
            >()
        .endSchedule<DefaultGLFWPlatformManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Warmup))
            .add<
                Sequential<
                    MeshUploadSystem<MeshHandle>,
                    ShaderCompileSystem<ShaderHandle>,
                    DefaultWarmupDoneSystem
                >
            >()
        .endSchedule<
            DefaultMeshUploadManager,
            DefaultShaderCompileManager,
            EngineStateManager
        >()

        // MAIN
        .beginSchedule(GameWorld::sessionState(EngineState::Running))


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
                >,
                Sequential<
                    MotionIntegrationSystem<ParticleHandle>,
                    WorldTransformSystem<ParticleHandle>,
                    WorldBoundsUpdateSystem<ParticleHandle>
                >
            >()
            .add(
                Sequential(
                    helios::physics::collision::systems::GridCollisionDetectionSystem<GameObjectHandle>(
                        helios::math::aabbf{
                            -AXIS_LENGTH / 2.0F, -AXIS_LENGTH / 2.0F, 0.0f,
                            AXIS_LENGTH / 2.0F, AXIS_LENGTH / 2.0F, 0.0f
                        }, CELL_SIZE
                    ),
                    helios::physics::collision::systems::CollisionResponseSystem<GameObjectHandle>()
                ),
                Sequential(
                    helios::physics::collision::systems::GridCollisionDetectionSystem<ParticleHandle>(
                        helios::math::aabbf{
                            -AXIS_LENGTH / 2.0F, -AXIS_LENGTH / 2.0F, 0.0f,
                            AXIS_LENGTH / 2.0F, AXIS_LENGTH / 2.0F, 0.0f
                        }, CELL_SIZE
                    ),
                    helios::physics::collision::systems::CollisionResponseSystem<ParticleHandle>()
                )
            )

            // this will produce render commands after scenes have been culled according to
            // their active viewports
            .add(
                Sequential(
                    SceneMemberVisibilitySystem<
                        GameObjectHandle, Instanced, NullCullingStrategy<GameObjectHandle>, DefaultRenderHandles
                    >(NullCullingStrategy<GameObjectHandle>())
                ),
                Sequential(
                    SceneMemberVisibilitySystem<
                        ParticleHandle, Instanced, NullCullingStrategy<ParticleHandle>, DefaultRenderHandles
                    >(NullCullingStrategy<ParticleHandle>())
                )
            )
            // consume the scenemember-registry
            .add<
                DefaultSceneRenderSystem<GameObjectHandle, Instanced>,
                DefaultSceneRenderSystem<ParticleHandle, Instanced>
            >()
        .endSchedule<DefaultRenderManager>()

        // POST
        // Clear, bufferswapping
        .beginSchedule(GameWorld::sessionState(EngineState::Running))
            .add(
                Sequential(
                    GLFWWindowCloseSystem<WindowHandle>(),
                    WindowBasedShutdownSystem<WindowHandle>(),
                    ClearAllDirtySetsSystem(),
                    ImGuiOverlayRenderSystem(imguiOverlay),
                    SwapBuffersSystem<WindowHandle>())
            )
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

        // Game Logic Update
        const GamepadState gamepadState = GamepadState();
        const auto inputSnapshot = InputSnapshot(gamepadState);

        // Frame Synchronization is now done via GLFWSwapBuffersSystems
        gameLoop.update(frameTiming, inputSnapshot);

        frameTiming = framePacer.sync();
        fpsMetrics.addFrame(frameTiming);
    }

    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));

    return 0;
}