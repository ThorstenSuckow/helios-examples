
#include <algorithm>
#include <cassert>
#include <thread>
#include <utility>
#include <cmath>

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

template <typename TEmitterHandle, typename TSpawnHandle = TEmitterHandle>
struct RadialSpawnPolicy {

    using EmitterHandleType = TEmitterHandle;
    using SpawnHandleType = TSpawnHandle;

    using SpawnContext = helios::gameplay::spawning::types::TypedSpawnContext<TEmitterHandle, TSpawnHandle>;
    using SpawnEntityType = Entity<helios::ecs::EntityManager<TSpawnHandle>>;
    using EntityPool = helios::engine::runtime::pooling::EntityPool;

    std::size_t spawnCount(SpawnContext& updateContext, const SpawnContext& spawnContext) noexcept {

        if (spawnContext.poolSnapshot.inactiveCount >= spawnContext.requiredAmount) {
            return spawnContext.requiredAmount;
        }

        return 0;
    }

    bool onBeforeSpawn(EntityPool& pool, SpawnEntityType& entity) noexcept {
        entity.resetTo(pool.prefab<SpawnHandleType>());
        return true;
    }

    std::size_t spawn(
        SpawnContext& updateContext, const SpawnContext& spawnContext, std::span<SpawnEntityType> spawnEntities
    ) noexcept {

        auto frac = helios::math::radians(360.0F / spawnEntities.size());

        float i = 0.0F;
        float spread = 4.0F;
        for (auto& entity : spawnEntities) {
            auto* cmp = entity.template get<Velocity3DComponent<TSpawnHandle, Local>>();
            auto veloc =
                helios::math::vec3f{std::cos(frac * i) * spread, std::sin(frac * i) * spread, 0.0F}.normalize();
            cmp->setValue(veloc * 12.0F);
            entity.setActive(true);
            ++i;
        }

        return spawnEntities.size();
    }

    bool update(SpawnContext& updateContext, SpawnContext& spawnContext) noexcept {
        return true;
    }
};

int main() {
    const auto& logger = helios::core::log::LogManager::loggerForScope("main");

    // ========================================
    // Constants
    // ========================================
    constexpr unsigned int SCREEN_WIDTH = 1280;
    constexpr unsigned int SCREEN_HEIGHT = 720;

    constexpr float WINDOW_ASPECT_RATIO_NUMER = 16.0F;
    constexpr float WINDOW_ASPECT_RATIO_DENOM = 9.0F;

    constexpr bool ENABLE_VSYNC = false;

    // ==========================================================
    // Infrastructure init / GameWorld / GameLoop / InputManager
    // ==========================================================

    auto maxWorker = std::max(1U, std::thread::hardware_concurrency() - 1);
    helios::core::thread::JobSystem jobSystem(maxWorker);

    // gameworld
    auto engineRuntime = helios::engine::bootstrap::bootstrapGameWorld(jobSystem);

    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;

    // ========================================
    // Window Setup
    // ========================================
    auto MainWindow = gameWorld.add<helios::engine::bootstrap::WindowHandle>();
    MainWindow.add<WindowCreateRequestComponent<WindowHandle>>(WindowConfig{
        .title = "helios - Game of Life",
        .size = {SCREEN_WIDTH, SCREEN_HEIGHT},
        .aspectRatioNumer = WINDOW_ASPECT_RATIO_NUMER,
        .aspectRatioDenom = WINDOW_ASPECT_RATIO_DENOM,
        .vsyncEnabled = ENABLE_VSYNC
    });

    // ========================================
    // Scene and Viewport Setup
    // ========================================

    auto MainRenderTarget = gameWorld.add<helios::engine::bootstrap::RenderTargetHandle>();
    MainRenderTarget.add<OpenGLRenderTargetIdComponent<RenderTargetHandle>>(0);
    MainRenderTarget.trackDirty<Size2DComponent<RenderTargetHandle>>();
    MainRenderTarget.add<ClearComponent<RenderTargetHandle>>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent<RenderTargetHandle>>(helios::engine::rendering::common::types::Colors::Black);

    auto MainViewport = gameWorld.add<ViewportHandle>();
    MainViewport.add<DebugNameComponent<ViewportHandle>>("MainViewport");
    MainViewport.add<ClearComponent<ViewportHandle>>(ClearFlags::Color);
    MainViewport.add<ColorComponent<ViewportHandle>>(helios::engine::rendering::common::types::Colors::Black);
    // RenderTarget : Viewport (1:N)
    MainViewport.add<DefaultRenderTargetBindingComponent<ViewportHandle>>(MainRenderTarget);
    MainViewport.add<RectComponent<ViewportHandle>>(helios::math::vec4f{0.0F, 0.0F, 1.0F, 1.0F});

    auto MainScene = gameWorld.add<SceneHandle>();
    MainWindow.add<DefaultRenderTargetBindingComponent<WindowHandle>>(MainRenderTarget);

    // Viewport : Scene (N:1)
    MainViewport.add<DefaultSceneBindingComponent<ViewportHandle>>(MainScene);

    auto MainCamera = gameWorld.add<CameraHandle>();
    MainCamera.add<DebugNameComponent<CameraHandle>>("MainCamera");
    MainCamera.trackDirty<PerspectiveCameraComponent<CameraHandle>>(
        helios::math::radians(90.0F), WINDOW_ASPECT_RATIO_NUMER / WINDOW_ASPECT_RATIO_DENOM
    );
    MainCamera.trackDirty<ProjectionMatrixComponent<CameraHandle>>();
    MainCamera.trackDirty<ViewMatrixComponent<CameraHandle>>();
    MainCamera.trackDirty<YawPitchRollComponent<CameraHandle>>();
    MainCamera.trackDirty<Rotation3DComponent<CameraHandle, Local>>();
    MainCamera.trackDirty<TransformComponent<CameraHandle, World>>(1.0F);
    MainCamera.trackDirty<Position3DComponent<CameraHandle, Local>>(0.0F, 0.0F, -50.0F);

    // camera does not need a scene binding, since there is no camera selection
    // using scenes. The SceneMemberVisibilitySystem will directly use the Viewport's mapping
    // MainCamera.add<SceneBindingComponent<CameraHandle>>(MainScene);
    MainViewport.add<DefaultCameraBindingComponent<ViewportHandle>>(MainCamera);

    // ========================================
    // Rendering Management setup
    // ========================================

    // Texture Setup
    auto ParticleShader = gameWorld.add<ShaderHandle>();
    ParticleShader.add<ShaderSourceComponent<ShaderHandle>>(
        "./resources/shader/particle.vert", "./resources/shader/particle.frag"
    );
    ParticleShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Pass>>(
        UniformMapping{.semantics = UniformSemantics::ProjectionMatrix, .name = "projectionMatrix"},
        UniformMapping{.semantics = UniformSemantics::ViewMatrix, .name = "viewMatrix"}
    );
    ParticleShader.add<UniformMappingsComponent<ShaderHandle, UniformScope::Material>>(
        UniformMapping{.semantics = UniformSemantics::MaterialBaseColor, .name = "color"}
    );

    // define Texture Entity
    auto ParticleTexture = gameWorld.add<TextureHandle>();
    ParticleTexture.add<TextureSourceComponent<TextureHandle>>("./resources/textures/particle.png");

    // TextureObject uses a Rect
    auto ParticleMesh = gameWorld.add<MeshHandle>();
    ParticleMesh.add<MeshDataComponent<MeshHandle>>(Rect::meshData());
    ParticleMesh.add<MeshUploadRequestComponent<MeshHandle>>();
    ParticleMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerVertex>>(
        VertexAttributeLayout{
            .attribute =
                VertexAttribute{.semantics = VertexAttributeSemantics::Position, .type = VertexAttributeType::Vec3f},
            .location = 0,
            .stride = sizeof(Vertex),
            .offset = offsetof(Vertex, position),
            .divisor = 0
        },
        VertexAttributeLayout{
            .attribute =
                VertexAttribute{
                    .semantics = VertexAttributeSemantics::TextureCoordinates, .type = VertexAttributeType::Vec2f
                },
            .location = 2,
            .stride = sizeof(Vertex),
            .offset = offsetof(Vertex, texCoords),
            .divisor = 0
        }
    );

    ParticleMesh.add<VertexAttributeLayoutComponent<MeshHandle, PerInstance>>(
        VertexAttributeLayout{
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
        }
    );

    auto ParticleMaterial = gameWorld.add<MaterialHandle>();
    ParticleMaterial.add<ColorComponent<MaterialHandle>>(helios::engine::rendering::common::types::Colors::Blue);

    // ========================================
    // Entity Setup
    // ========================================
    auto demoPoolKey = gameWorld.resource<EntityPoolRegistry>().add(EntityPool::make<ParticleHandle>());
    auto& demoPool = *gameWorld.resource<EntityPoolRegistry>().item(demoPoolKey);

    auto ParticlePrefab = demoPool.prefabEditor<ParticleHandle>();
    ParticlePrefab.add<DefaultSceneMemberComponent<ParticleHandle>>(MainScene);
    ParticlePrefab.add<EntityPoolKeyComponent<ParticleHandle>>(demoPoolKey);
    ParticlePrefab.trackDirty<BoundsComponent<ParticleHandle, Local>>(Rect::boundsData());
    ParticlePrefab.trackDirty<BoundsComponent<ParticleHandle, World>>();
    ParticlePrefab.trackDirty<Velocity3DComponent<ParticleHandle, Local>>();
    ParticlePrefab.trackDirty<LifetimeComponent<ParticleHandle>>(4.0F);
    ParticlePrefab.trackDirty<Rotation3DComponent<ParticleHandle, Local>>();
    ParticlePrefab.trackDirty<Position3DComponent<ParticleHandle, Local>>(1.0F, 0.0F, 0.0F);
    ParticlePrefab.trackDirty<Position3DComponent<ParticleHandle, World>>(0.0F, 0.0F, 0.0F);
    ParticlePrefab.trackDirty<TransformComponent<ParticleHandle, World>>(1.0F);
    ParticlePrefab.add<DefaultRenderPrototypeComponent<ParticleHandle, Instanced>>(
        ParticleShader.handle(), ParticleMaterial.handle(), ParticleMesh.handle(), ParticleTexture.handle()
    );
    auto prefabRequestCmp = gameWorld.add<ParticleHandle>();
    prefabRequestCmp.add<PrefabEntityPoolRequestComponent<ParticleHandle>>(10);
    prefabRequestCmp.add<EntityPoolKeyComponent<ParticleHandle>>(demoPoolKey);

    auto DEMO_SPAWN_POLICY_KEY = gameWorld.resource<SpawnPolicyRegistry>().add<RadialSpawnPolicy<ParticleHandle>>();

    auto ParticleEmitter = gameWorld.add<ParticleHandle>(true);
    ParticleEmitter.add<Position3DComponent<ParticleHandle, Local>>(0.0F, 0.0F, 0.0F);
    ParticleEmitter.add<SpawnRequestComponent<ParticleHandle>>(demoPoolKey, DEMO_SPAWN_POLICY_KEY, 10);
    ParticleEmitter.add<DefaultSceneMemberComponent<ParticleHandle>>(MainScene);

    // ----------------------------------------
    // ImGui and Debug Tooling
    // ----------------------------------------
    auto imguiBackend = ImGuiGlfwOpenGLBackend(MainWindow.handle(), gameWorld.ecsWorld());
    auto imguiOverlay = ImGuiOverlay::forBackend(&imguiBackend);
    auto fpsMetrics = FpsMetrics();
    auto framePacer = FramePacer();
    framePacer.setTargetFps(0.0F);
    FrameStats frameStats{};

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
    // LogManager::getInstance().enableSink<ImGuiLogSink>(logWidget);

    // ========================================
    // Initialization of GameWorld and Game Loop
    // ========================================
    float DELTA_TIME = 0.0F;

    // ----------------------------------------
    // GameLoop Config
    // ----------------------------------------
    gameLoop
        .phase(PhaseType::Pre)

        .beginPass(EngineState::Any)
        .addSystem<EngineFlowSystem>()
        .executeCommands<EngineStateManager>()
        .endPass()

        .beginPass(EngineState::Booting)
        .addSystem<PlatformInitSystem>()
        .executeCommands<DefaultGLFWPlatformManager>()
        .endPass()

        .beginPass(EngineState::Booted | EngineState::Running)
        .addSystem<PollEventsSystem>()
        .addSystem<WindowCreateSystem<WindowHandle>>()
        .executeCommands<DefaultGLFWPlatformManager>()
        .endPass()

        .beginPass(EngineState::Warmup)
        .addSystem<TextureUploadSystem<TextureHandle>>()
        .addSystem<MeshUploadSystem<MeshHandle>>()
        .addSystem<ShaderCompileSystem<ShaderHandle>>()
        .addSystem<EntityPoolWarmupSystem<ParticleHandle>>()
        .addSystem<DefaultWarmupDoneSystem>()
        .executeCommands<
            DefaultTextureUploadManager,
            DefaultMeshUploadManager,
            DefaultShaderCompileManager,
            EngineStateManager
        >()
        .executeCommands<EntityPoolManager<ParticleHandle>>()
        .endPass();

    // intentionally left empty
    gameLoop.phase(PhaseType::Main)
        .beginPass(EngineState::Running)

        .addSystem([&](EcsWorld& ecsWorld,
                       helios::ecs::command::TypedCommandBuffer<SpawnCommand<ParticleHandle>>& cmdBuffer) {
            for (auto [entity, requestComponent] :
                 ecsWorld.view<ParticleHandle, SpawnRequestComponent<ParticleHandle>>().withActive()) {
                cmdBuffer.add<SpawnCommand<ParticleHandle>>(
                    entity.handle(),
                    requestComponent->entityPoolKey,
                    requestComponent->spawnPolicyKey,
                    requestComponent->amount
                );

                entity.remove<SpawnRequestComponent<ParticleHandle>>();
            }
        })
        .executeCommands<SpawnManager<ParticleHandle>>()
        .endPass()

        .beginPass(EngineState::Running)
        .addSystem<MotionIntegrationSystem<ParticleHandle>>()
        .endPass();

    gameLoop.phase(PhaseType::Post)
        .beginPass(EngineState::Running)

        // create parallel groups
        .addParallelSystems<
            Serial<WorldTransformSystem<GameObjectHandle>, WorldBoundsUpdateSystem<GameObjectHandle>>,
            Serial<
                YawPitchRollUpdateSystem<CameraHandle>,
                WorldTransformSystem<CameraHandle>,
                PerspectiveCameraUpdateSystem<CameraHandle>
            >,
            Serial<WorldTransformSystem<ParticleHandle>, WorldBoundsUpdateSystem<ParticleHandle>>
        >()

        // this will produce render commands after scenes have been culled according to
        // their active viewports
        .addSystem<DefaultSceneMemberVisibilitySystem<ParticleHandle, Instanced, AABBCullingStrategy<ParticleHandle>>>(
            AABBCullingStrategy<ParticleHandle>()
        )
        .addSystem([&](EntityManager<ParticleHandle>& entityManager,
                       DefaultSceneMemberVisibilityRegistry<ParticleHandle, Instanced>& visibilityRegistry) {
            auto* lifetimeSparseSet = entityManager.sparseSet<LifetimeComponent<ParticleHandle>>();
            visibilityRegistry.forEachVisibleMember([&lifetimeSparseSet = *lifetimeSparseSet](auto& visibleContext) {
                if (auto* cmp = lifetimeSparseSet.get(visibleContext.memberHandle.entityId())) {
                    visibleContext.normalizedAge = cmp->value() / cmp->lifetime();
                }
            });
        })
        .addSystem<DefaultSceneRenderSystem<ParticleHandle, Instanced>>()

        .executeCommands<DefaultRenderManager>()
        .endPass()

        // Clear, bufferswapping, lifecycle
        .beginPass(EngineState::Running)
        .addSystem<GLFWWindowCloseSystem<WindowHandle>>()
        .addSystem<WindowBasedShutdownSystem<WindowHandle>>()
        .addSystem<ClearAllDirtySetsSystem>()
        .addSystem([&](EcsWorld& ecsWorld,
                       UpdateContext& updateContext,
                       helios::ecs::command::TypedCommandBuffer<ReleaseEntityCommand<ParticleHandle>>& cmdBuffer) {
            for (auto [entity, lcc, keyCmp] :
                 ecsWorld
                     .view<ParticleHandle, LifetimeComponent<ParticleHandle>, EntityPoolKeyComponent<ParticleHandle>>()
                     .withActive()) {

                lcc->tick(updateContext.deltaTime());

                if (lcc->isExpired()) {
                    cmdBuffer.template add<ReleaseEntityCommand<ParticleHandle>>(
                        keyCmp->entityPoolKey, entity.handle()
                    );
                }
            }
        })
        .addSystem<ImGuiOverlayRenderSystem>(imguiOverlay)
        .addSystem<SwapBuffersSystem<WindowHandle>>()
        .executeCommands<DefaultGLFWPlatformManager>()
        .executeCommands<EntityPoolManager<ParticleHandle>>()
        .endPass()

        .beginPass(EngineState::Shutdown)
        .addSystem<DestroySessionSystem>()
        .endPass();

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
        gameLoop.update(DELTA_TIME, inputSnapshot); // inputSnapshot, viewportSnapshots);;

        frameStats = framePacer.sync();
        fpsMetrics.addFrame(frameStats);
        DELTA_TIME = frameStats.totalFrameTime;
    }

    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));

    return 0;
}