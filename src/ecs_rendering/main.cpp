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
    auto engineRuntime = helios::engine::bootstrap::bootstrapGameWorld();
    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;
    auto factory = helios::engine::factory::Factory<DefaultRenderHandles>(gameWorld.ecsWorld());
    //factory.setRenderBackendFactory(helios::opengl::OpenGLBackendFactory{});

    // ========================================
    // Window Setup
    // ========================================

    auto mainWindow = factory.createWindow<WindowHandle>(
        "helios - ECS Rendering Demo", SCREEN_WIDTH, SCREEN_HEIGHT
    );
    mainWindow.enforceAspectRatio(WINDOW_ASPECT_RATIO_NUMER, WINDOW_ASPECT_RATIO_DENOM)
              .enableVSync(ENABLE_VSYNC);

    // ========================================
    // Scene, camera and Viewport Setup
    // ========================================
    auto mainScene = factory.createScene();
    auto cameraTop = mainScene.addPerspectiveCamera(
        radians(90.0F), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM)
    );
    cameraTop.setPosition<Local>(helios::math::vec3f{0.0F, 0.0F, -50.0F});

    auto cameraBottom = mainScene.addPerspectiveCamera(
        radians(90.0F), WINDOW_ASPECT_RATIO_NUMER / (.5f * WINDOW_ASPECT_RATIO_DENOM)
    );
    cameraBottom.setPosition<Local>(helios::math::vec3f{0.0F, 0.0F, -110.0F});

    auto viewportTop = mainWindow.addViewport(helios::math::vec4f{0.0F, .5f, 1.0F, 0.5F});
    viewportTop.setBackgroundColor(Colors::LightGray);

    auto viewportBottom = mainWindow.addViewport(helios::math::vec4f{0.0F, .0f, 1.0F, 0.5F});
    viewportBottom.setBackgroundColor(Colors::Gray);

    viewportTop.render(mainScene, cameraTop);
    viewportBottom.render(mainScene, cameraBottom);

    // 3d Object

   /* auto cube = mainScene.addActor<GameObjectHandle>(cubeMesh, cubeMaterial);
    cube.renderWith(cubeShader);

    auto cube = gameWorld.add<GameObjectHandle>();
    cube.add<DefaultSceneMemberComponent>(mainScene);
    cube.add<BoundsComponent<Local>>(Triangle::boundsData());
    cube.add<BoundsComponent<World>>();
    cube.add<Rotation3DComponent<Local>>();

    cube.add<Position3DComponent<Local>>(
        static_cast<float>(x), static_cast<float>(y), 0.0F
    );
    cube.add<Position3DComponent<World>>(0.0F, 0.0F, 0.0F);
    cube.add<helios::physics::motion::components::Velocity3DComponent<Intent>>(
        randomVec3f(x * y).withZ(0.0F).normalize()
    );
    cube.add<helios::physics::motion::components::Velocity3DComponent<Local>>();

    cube.add<TransformComponent<World>>(1.0F);
    cube.add<DefaultRenderPrototypeComponent<Instanced>>(
        CubeShader.handle(), CubeMaterial.handle(), CubeMesh.handle()
    );*/

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

     CubeShader.add<VertexAttributeMappingsComponent>(
        VertexAttributeMappings(VertexAttributeMapping{
            .semantics = VertexAttributeSemantics::Position,
            .name = "position",
            .location = 0
        }, VertexAttributeMapping{
            .semantics = VertexAttributeSemantics::InstancedModelMatrix,
            .name = "instanceMatrix",
            .location = 4
        })
    );


    auto CubeMesh = gameWorld.add<MeshHandle>();
    CubeMesh.add<MeshDataComponent>(Triangle::meshData());
    CubeMesh.add<MeshUploadRequestComponent>();
    CubeMesh.add<VertexAttributeLayoutsComponent>(
        VertexAttributeLayouts(VertexAttributeLayout{
            .attribute = VertexAttribute{
                .semantics = VertexAttributeSemantics::Position,
                .type = VertexAttributeType::Vec3f
            },
            .inputRate = VertexInputRate::PerVertex,
            .stride = sizeof(Vertex),
            .offset = offsetof(Vertex, position)
        }, VertexAttributeLayout{
            .attribute = VertexAttribute{
                .semantics = VertexAttributeSemantics::InstancedModelMatrix,
                .type = VertexAttributeType::Mat4f
            },
            .inputRate = VertexInputRate::PerInstance,
            .stride = sizeof(InstanceData),
            .offset = offsetof(InstanceData, modelMatrix),
            .divisor = 1

        })
    );

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
            cube.add<DefaultSceneMemberComponent>(mainScene);
            cube.add<BoundsComponent<Local>>(Triangle::boundsData());
            cube.add<BoundsComponent<World>>();
            cube.add<Rotation3DComponent<Local>>();

            cube.add<Position3DComponent<Local>>(
                static_cast<float>(x), static_cast<float>(y), 0.0F
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
    auto imguiBackend = ImGuiGlfwOpenGLBackend(mainWindow.handle(), gameWorld.ecsWorld());
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
        .endWith<EngineStateManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Booting))
            .add<PlatformInitSystem>()
        .endWith<DefaultGLFWPlatformManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Booted | EngineState::Running))
            .add<
                Sequential<
                    PollEventsSystem,
                    DefaultAssignRenderTargetIdSystem,
                    WindowCreateSystem<WindowHandle>
                >
            >()
        .endWith<Sequential<
            DefaultGLFWPlatformManager,
            DefaultRenderTargetManager
        >>()

        .beginSchedule(GameWorld::sessionState(EngineState::Warmup))
            .add<
                Sequential<
                    MeshUploadSystem<MeshHandle>,
                    ShaderCompileSystem<ShaderHandle>,
                    DefaultWarmupDoneSystem
                >
            >()
        .endWith<
            Sequential<
                DefaultMeshUploadManager,
                DefaultShaderCompileManager,
                EngineStateManager
            >
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
            .add(
                VertexAttributeConfigurationSystem<GameObjectHandle, Instanced, DefaultRenderHandles>(),
                DefaultSceneMemberVisibilitySystem<
                    GameObjectHandle, Instanced, AABBCullingStrategy<GameObjectHandle>
                >(AABBCullingStrategy<GameObjectHandle>())
            )
            .add(
                [&](EntityManager<GameObjectHandle>& entityManager,
                    DefaultSceneMemberVisibilityRegistry<GameObjectHandle, Instanced>& visibilityRegistry,
                    ThreadPool& threadPool) {
                    const auto viewport = viewportTop.handle();

                    threadPool.runAndWait(2, [&](const std::size_t threadIndex) {
                        if (threadIndex == 0) {
                            enableMemberMaterialOverride(
                                entityManager, visibilityRegistry.culledMembers(viewport), true
                            );
                        } else if (threadIndex == 1) {
                            enableMemberMaterialOverride(
                                entityManager, visibilityRegistry.visibleMembers(viewport), false
                            );
                        }
                    });
                }
            )
            // consume the scenemember-registry
            .add<DefaultSceneRenderSystem<GameObjectHandle, Instanced>>()
        .endWith<DefaultRenderManager>()

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
        .endWith<DefaultGLFWPlatformManager>()

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

        gameLoop.update(frameTiming, inputSnapshot);

        frameTiming = framePacer.sync();
        fpsMetrics.addFrame(frameTiming);
    }

    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));

    return 0;
}

