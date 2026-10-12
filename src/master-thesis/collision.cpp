
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
#include <optional>

#include "../Namespaces.h"

helios::math::vec3f randomVec3f(const std::uint64_t seed) {

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

    constexpr int OBJECTS_PER_AXIS = 317;
    constexpr auto BORDER_SCALE = vec3f{1.0F, 1.0F, 1.0F};
    constexpr int DISTANCE_BETWEEN_OBJECTS = 2;
    constexpr int CREATE_PARTICLES = 2;
    constexpr float PARTICLE_SPEED = 32.0F;
    constexpr float GAME_OBJECT_SPEED = 1.0f;
    constexpr float PARTICLE_LIFETIME = 1.0f;
    constexpr int AXIS_LENGTH      = DISTANCE_BETWEEN_OBJECTS * OBJECTS_PER_AXIS + DISTANCE_BETWEEN_OBJECTS - 1;

    const int CELL_SIZE = static_cast<int>(AXIS_LENGTH / OBJECTS_PER_AXIS);

        logger.info("Actual axis length: {}; Cell size: {}", AXIS_LENGTH, CELL_SIZE);

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
    CullingViewport.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Black);
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
    },
    VertexAttributeLayout{
           .attribute =
               VertexAttribute{
                   .semantics = VertexAttributeSemantics::InstancedNormalizedAge, .type = VertexAttributeType::Float
               },
           .location = 8,
           .stride = sizeof(InstanceData),
           .offset = offsetof(InstanceData, normalizedAge),
           .divisor = 1
       });

    // ========================================
    // Entity Setup
    // ========================================
    // boundaries
    auto createBoundaries = [&]<typename THandle>() {

        float borderAccount  = BORDER_SCALE[0]/2.0f;
        for (int i = 0; i < 4; i++) {
            auto bndLeft =  gameWorld.add<THandle>();

            switch (i) {
                case 0: bndLeft.template add<Position3DComponent<Local>>(0.0F, -AXIS_LENGTH/2.0f - borderAccount , 0.0F);
                    break;
                case 1: bndLeft.template add<Position3DComponent<Local>>(AXIS_LENGTH/2.0f + borderAccount , 0.0F, 0.0F);
                    break;
                case 2: bndLeft.template add<Position3DComponent<Local>>(0.0F, AXIS_LENGTH/2.0f + borderAccount , 0.0F);
                    break;
                case 3: bndLeft.template add<Position3DComponent<Local>>(-AXIS_LENGTH/2.0f - borderAccount , 0.0F, 0.0F);
                    break;
            }
            bndLeft.template add<Scale3DComponent<Local>>(AXIS_LENGTH , BORDER_SCALE[1], BORDER_SCALE[2]);
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

    auto GameObjectMaterial = gameWorld.add<MaterialHandle>();
    GameObjectMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Blue);

    auto ParticleMaterial = gameWorld.add<MaterialHandle>();
    ParticleMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Green);

    auto createObjects = [&]<typename THandle>(MaterialHandle materialHandle, const int randomizerOffset = 0) {
        int NUM_OBJECTS = 0;

        float borderAccount  = BORDER_SCALE[0]/2.0f;
        float startLeft = -AXIS_LENGTH / 2.0F;
        float endRight = AXIS_LENGTH / 2.0F;
        float endTop = AXIS_LENGTH / 2.0F;
        float startBottom = -AXIS_LENGTH / 2.0F - borderAccount;
        float x = startLeft + DISTANCE_BETWEEN_OBJECTS - borderAccount;

        for (int i = 0; i < OBJECTS_PER_AXIS; i++) {

            float y = startBottom + DISTANCE_BETWEEN_OBJECTS;

            for (int j = 0; j < OBJECTS_PER_AXIS; j++) {
                auto cube = gameWorld.add<THandle>();
                cube.template add<helios::physics::collision::components::CollisionComponent>();
                cube.template add<DefaultSceneMemberComponent>(MainScene);
                cube.template add<BoundsComponent<Local>>(WireframeCube::boundsData());
                cube.template add<BoundsComponent<World>>();
                cube.template add<Rotation3DComponent<Local>>();
                cube.template add<Scale3DComponent<Local>>(1.0F, 1.0F, 1.0F);
                cube.template add<Position3DComponent<Local>>(
                    static_cast<float>(x), static_cast<float>(y), 0.0F
                );
                cube.template add<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
                cube.template add<helios::physics::motion::components::Velocity3DComponent<Local>>(
                    randomVec3f(NUM_OBJECTS + randomizerOffset).withZ(0.0F).normalize() * GAME_OBJECT_SPEED
                );

                cube.template add<TransformComponent<World>>(1.0F);
                cube.template add<DefaultRenderPrototypeComponent<Instanced>>(
                    CubeShader.handle(), materialHandle, CubeMesh.handle()
                );
                NUM_OBJECTS++;

                y += DISTANCE_BETWEEN_OBJECTS;
            }
            x += DISTANCE_BETWEEN_OBJECTS;

        }

        logger.info("Created {} objects.", NUM_OBJECTS);
    };

    auto createParticles = [&]<typename THandle>(
        EntityManager<THandle>& entityManager,
        std::uint64_t key, int count, helios::math::vec3f location) {

        for (int i = 0; i < count; i++) {
            auto handle = entityManager.create();
            auto cube = entity::Entity{handle, &entityManager};
            cube.setActive(true);
            cube.template add<helios::physics::collision::components::CollisionComponent>();
            cube.template add<DefaultSceneMemberComponent>(MainScene);
            cube.template add<BoundsComponent<Local>>(WireframeCube::boundsData());
            cube.template add<BoundsComponent<World>>();
            cube.template add<LifetimeComponent>(PARTICLE_LIFETIME);
            cube.template add<Rotation3DComponent<Local>>();
            cube.template add<Scale3DComponent<Local>>(1.0F, 1.0F, 1.0F);
            cube.template add<Position3DComponent<Local>>(location);
            cube.template add<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
             cube.template add<helios::physics::motion::components::Velocity3DComponent<Local>>(
                randomVec3f(key + i).withZ(0.0F).normalize()*PARTICLE_SPEED
            );

            cube.template add<TransformComponent<World>>(1.0F);
            cube.template add<DefaultRenderPrototypeComponent<Instanced>>(
                CubeShader.handle(), ParticleMaterial.handle(), CubeMesh.handle()
            );
        }
    };


    createObjects.operator()<GameObjectHandle>(GameObjectMaterial.handle());
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

    gameWorld.managerRegistry().add<helios::physics::collision::CollisionManager<GameObjectHandle, ParticleHandle>>(
        [&](EntityManager<GameObjectHandle>& entityManager, EntityManager<ParticleHandle>& targetEntityManager,
            std::span<const helios::physics::collision::commands::CollisionCommand<GameObjectHandle>> collisionCommands) {
           // logger.info("CollisionManager: {} collisions detected.", collisionCommands.size());
            for (const auto& collision : collisionCommands) {
                createParticles.operator()<ParticleHandle>(targetEntityManager,
                    ((collision.leftHandle.entityId() << 32) |  collision.rightHandle.entityId()),
                CREATE_PARTICLES,
                collision.location);
            }

        }
    );

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
            Sequential<
                DefaultMeshUploadManager,
                DefaultShaderCompileManager,
                EngineStateManager
            >
        >()

        // MAIN
        .beginSchedule(GameWorld::sessionState(EngineState::Running))
        .endSchedule<
            Sequential<
            helios::physics::collision::CollisionManager<GameObjectHandle, ParticleHandle>,
            helios::gameplay::lifecycle::LifetimeManager<ParticleHandle>
             >
        >()


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
                    helios::physics::collision::systems::CollisionDetectionSystem<GameObjectHandle,
                    helios::physics::collision::strategies::GridCollisionDetection<GameObjectHandle>>(
                        helios::physics::collision::strategies::GridCollisionDetection<GameObjectHandle>(
                        helios::math::aabbf{
                            -AXIS_LENGTH / 2.0F, -AXIS_LENGTH / 2.0F, 0.0f,
                            AXIS_LENGTH / 2.0F, AXIS_LENGTH / 2.0F, 0.0f
                        }, CELL_SIZE
                    )),
                    helios::physics::collision::systems::CollisionResponseSystem<
                        GameObjectHandle,
                        helios::physics::collision::strategies::CollisionImpulseResponse
                    >(
                        helios::physics::collision::strategies::CollisionImpulseResponse{}
                    )
                ),
                Sequential(
                    helios::physics::collision::systems::CollisionDetectionSystem<ParticleHandle,
                    helios::physics::collision::strategies::GridCollisionDetection<ParticleHandle>>(
                    helios::physics::collision::strategies::GridCollisionDetection<ParticleHandle>(
                        helios::math::aabbf{
                            -AXIS_LENGTH / 2.0F, -AXIS_LENGTH / 2.0F, 0.0f,
                            AXIS_LENGTH / 2.0F, AXIS_LENGTH / 2.0F, 0.0f
                        }, CELL_SIZE
                    )),
                    helios::physics::collision::systems::CollisionResponseSystem<
                        ParticleHandle,
                        helios::physics::collision::strategies::CollisionImpulseResponse
                    >(
                        helios::physics::collision::strategies::CollisionImpulseResponse{}
                    ),
                    [](TypedCommandBuffer<LifetimeCommand<ParticleHandle>>& commandBuffer,
                        Query<ParticleHandle, ReadSet<LifetimeComponent>, WriteSet<LifetimeComponent>> query, const UpdateContext& updateContext) {

                        for (auto [entity, lifetime] : query) {
                            lifetime->tick(updateContext.deltaTime());

                            if (lifetime->isExpired())
                                commandBuffer.add<LifetimeCommand<ParticleHandle>>(
                                LifetimeCommand<ParticleHandle>(
                                    entity.handle(),
                                    LifetimeCommandType::LifetimeExpired
                                ));
                        }
                    }
                )
            )

            // this will produce render commands after scenes have been culled according to
            // their active viewports
            .add(
                Sequential(
                    SceneMemberVisibilitySystem<
                        GameObjectHandle, Instanced, NullCullingStrategy<GameObjectHandle>, DefaultRenderHandles
                    >(NullCullingStrategy<GameObjectHandle>()),
                    DefaultSceneRenderSystem<GameObjectHandle, Instanced>()
                ),
                Sequential(
                    SceneMemberVisibilitySystem<
                        ParticleHandle, Instanced, NullCullingStrategy<ParticleHandle>, DefaultRenderHandles
                    >(NullCullingStrategy<ParticleHandle>()),
                    [&](EntityManager<ParticleHandle>& entityManager,
                       DefaultSceneMemberVisibilityRegistry<ParticleHandle, Instanced>& visibilityRegistry) {
                        auto* lifetimeSparseSet = entityManager.sparseSet<LifetimeComponent>();
                          //  logger.info("ParticleManager: {} particles visible.", lifetimeSparseSet->componentCount());
                        visibilityRegistry.forEachVisibleMember([&lifetimeSparseSet = *lifetimeSparseSet](auto& visibleContext) {
                            if (auto* cmp = lifetimeSparseSet.get(visibleContext.memberHandle.entityId())) {
                                visibleContext.normalizedAge = cmp->value() / cmp->lifetime();
                            }
                        });
                    },
                    DefaultSceneRenderSystem<ParticleHandle, Instanced>())

                )

        .endSchedule<Sequential<DefaultRenderManager>>()

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