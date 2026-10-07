
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

    constexpr int OBJECT_COUNT = 143; // per axis
    constexpr std::size_t OBJECT_DISTANCE = 3;

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
    CullingCamera.add<Position3DComponent<Local>>(0.0F, 0.0F, -75.0F);
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

    auto CubeMaterial = gameWorld.add<MaterialHandle>();
    CubeMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Blue);

    // ========================================
    // Entity Setup
    // ========================================
    // boundaries
    for (int i = 0; i < 4; i++) {
        auto bndLeft =  gameWorld.add<GameObjectHandle>();

        switch (i) {
            case 0: bndLeft.add<Position3DComponent<Local>>(0.0F, -OBJECT_COUNT/2 , 0.0F);
                break;
            case 1: bndLeft.add<Position3DComponent<Local>>(OBJECT_COUNT/2 , 0.0F, 0.0F);
                break;
            case 2: bndLeft.add<Position3DComponent<Local>>(0.0F, OBJECT_COUNT/2 , 0.0F);
                break;
            case 3: bndLeft.add<Position3DComponent<Local>>(-OBJECT_COUNT/2, 0.0F, 0.0F);
                break;
        }
        bndLeft.add<Scale3DComponent<Local>>(OBJECT_COUNT , 1.0F, 1.0F);
        bndLeft.add<DefaultSceneMemberComponent>(MainScene);
        bndLeft.add<BoundsComponent<Local>>(WireframeCube::boundsData());
        bndLeft.add<Rotation3DComponent<Local>>(
            helios::math::quatf::fromEulerAngles<Intrinsic>(
                0.0f, 0.0f, i* (std::numbers::pi / 2)
            )
        );
        bndLeft.add<helios::physics::collision::components::CollisionComponent>();
        bndLeft.add<BoundsComponent<World>>();
        bndLeft.add<TransformComponent<World>>(1.0F);
        bndLeft.add<DefaultRenderPrototypeComponent<Instanced>>(
            CubeShader.handle(), BoundaryMaterial.handle(), BoundaryMesh.handle()
        );
    }

    // cubes
    for (int x = -OBJECT_COUNT / 2; x < OBJECT_COUNT / 2; x += OBJECT_DISTANCE) {
        for (int y = -OBJECT_COUNT / 2; y < OBJECT_COUNT / 2; y += OBJECT_DISTANCE) {
            auto cube = gameWorld.add<GameObjectHandle>();
            cube.add<helios::physics::collision::components::CollisionComponent>();
            cube.add<DefaultSceneMemberComponent>(MainScene);
            cube.add<BoundsComponent<Local>>(WireframeCube::boundsData());
            cube.add<BoundsComponent<World>>();
            cube.add<Rotation3DComponent<Local>>();
            cube.add<Scale3DComponent<Local>>(1.0F, 1.0F, 1.0F);
            cube.add<Position3DComponent<Local>>(
                static_cast<float>(
                    x <= -OBJECT_COUNT/2  ? x + 5 : (x >= OBJECT_COUNT/2 ? x - 5 : x)
                ), static_cast<float>(
                    y <= -OBJECT_COUNT/2 ? y + 5 : (y >= OBJECT_COUNT/2 ? y - 5 : y)
                ), 0.0F
            );
            cube.add<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
            cube.add<helios::physics::motion::components::Velocity3DComponent<Intent>>(
                randomVec3f(x * y).withZ(0.0F).normalize()
            );
            cube.add<helios::physics::motion::components::Velocity3DComponent<Local>>();

            cube.add<TransformComponent<World>>(1.0F);
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

            .add(
                // replacement for systems that compute the local velocity from intended velocity,
                // such as component systems
                [&](Query<
                    GameObjectHandle,
                    ReadSet<Velocity3DComponent<Intent>, Velocity3DComponent<Local>>,
                    WriteSet<Velocity3DComponent<Local>
                    >,
                    Filter<
                        IsActive,
                        AnyDirty<Active, Velocity3DComponent<Intent>>
                    >
                > query) {
                    for (auto [entity, intendedVelocity, localVelocity ] : query) {
                         localVelocity->setValue(intendedVelocity->value());
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
            .add(
                Sequential(
                    helios::physics::collision::systems::GridCollisionDetectionSystem<GameObjectHandle>(
                        helios::math::aabbf{
                            -OBJECT_COUNT / 2.0F, -OBJECT_COUNT / 2.0F, 0.0f,
                            OBJECT_COUNT / 2.0F, OBJECT_COUNT / 2.0F, 0.0f
                        }, 10.0f
                    ),
                    [&logger](
                    const helios::physics::collision::CollisionDetectionResult<GameObjectHandle>& collisionResult,
                    UpdateContext& ctx,
                    Query<
                        GameObjectHandle,
                        ReadSet<Velocity3DComponent<Local>>,
                        WriteSet<Velocity3DComponent<Local>>
                    > query
                ) {

                    const auto& collisions = collisionResult.collisionPairs();

                    constexpr float restitution = 1.0f;

                    for (const auto& collisionPair : collisions) {

                        auto lftResult = query.get(collisionPair.leftHandle);
                        auto rgtResult = query.get(collisionPair.rightHandle);

                        // staionary collision (most unlikely)
                        if (!lftResult && !rgtResult) [[unlikely]] {
                            continue;
                        }

                        helios::math::vec3f leftVelocity{0.0f, 0.0f, 0.0f};
                        helios::math::vec3f rightVelocity{0.0f, 0.0f, 0.0f};

                        if (lftResult) {
                            auto [entity, velocity] = *lftResult;
                            leftVelocity = velocity->value();
                        }

                        if (rgtResult) {
                            auto [entity, velocity] = *rgtResult;
                            rightVelocity = velocity->value();
                        }

                        // at least one stationary
                        const float leftInverseMass  = lftResult ? 1.0f : 0.0f;
                        const float rightInverseMass = rgtResult ? 1.0f : 0.0f;

                        const auto relativeVelocity =
                            rightVelocity - leftVelocity;

                        // overlapNormal: left -> right.
                        const float velocityAlongNormal =
                            helios::math::dot(
                                relativeVelocity,
                                collisionPair.overlapNormal
                            );

                        // moving away from each other - ignore
                        if (velocityAlongNormal >= 0.0f) {
                            continue;
                        }

                        const float impulseMagnitude =
                            -(1.0f + restitution) *
                            velocityAlongNormal /
                            (leftInverseMass + rightInverseMass);

                        const auto impulse = collisionPair.overlapNormal * impulseMagnitude;

                        if (lftResult) {
                            auto [entity, velocity] = *lftResult;

                            entity
                                .track<Velocity3DComponent<Local>>()
                                ->setValue(
                                    leftVelocity -
                                    impulse * leftInverseMass
                                );
                        }

                        if (rgtResult) {
                            auto [entity, velocity] = *rgtResult;

                            entity
                                .track<Velocity3DComponent<Local>>()
                                ->setValue(
                                    rightVelocity +
                                    impulse * rightInverseMass
                                );
                        }
                    }
                }
            ))

            // this will produce render commands after scenes have been culled according to
            // their active viewports
            .add(
                SceneMemberVisibilitySystem<
                    GameObjectHandle, Instanced, NullCullingStrategy<GameObjectHandle>, DefaultRenderHandles
                >(NullCullingStrategy<GameObjectHandle>())
            )
            // consume the scenemember-registry
            .add<DefaultSceneRenderSystem<GameObjectHandle, Instanced>>()
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