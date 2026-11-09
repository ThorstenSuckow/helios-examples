
#include <algorithm>
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

    // gameworld
    auto engineRuntime = helios::engine::bootstrap::bootstrapGameWorld();

    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;

    // ========================================
    // Window Setup
    // ========================================
    auto MainWindow = gameWorld.add<WindowHandle>();
    MainWindow.add<WindowCreateRequestComponent>(WindowConfig{
        "helios - Game of Life",
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        WINDOW_ASPECT_RATIO_NUMER,
        WINDOW_ASPECT_RATIO_DENOM,
        ENABLE_VSYNC
    });

    // ========================================
    // Scene and Viewport Setup
    // ========================================

    auto MainRenderTarget = gameWorld.add<RenderTargetHandle>();
    MainRenderTarget.add<OpenGLRenderTargetIdComponent>(0);
    MainRenderTarget.add<Size2DComponent>();
    MainRenderTarget.add<ClearComponent>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Black);

    auto MainViewport = gameWorld.add<ViewportHandle>();
    MainViewport.add<DebugNameComponent>("MainViewport");
    MainViewport.add<ClearComponent>(ClearFlags::Color);
    MainViewport.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Black);
    // RenderTarget : Viewport (1:N)
    MainViewport.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);
    MainViewport.add<RectComponent>(helios::math::vec4f{0.0f, 0.0f, 1.0f, 1.0f});

    auto MainScene = gameWorld.add<SceneHandle>();
    MainWindow.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);

    // Viewport : Scene (N:1)
    MainViewport.add<DefaultSceneBindingComponent>(MainScene);

    auto MainCamera = gameWorld.add<CameraHandle>();
    MainCamera.add<DebugNameComponent>("MainCamera");
    MainCamera.add<PerspectiveCameraComponent>(
        helios::math::radians(90.0f), WINDOW_ASPECT_RATIO_NUMER / WINDOW_ASPECT_RATIO_DENOM
    );
    MainCamera.add<ProjectionMatrixComponent>();
    MainCamera.add<ViewMatrixComponent>();
    MainCamera.add<YawPitchRollComponent>();
    MainCamera.add<Rotation3DComponent<Local>>();
    MainCamera.add<TransformComponent<World>>(1.0f);
    MainCamera.add<Position3DComponent<Local>>(0.0f, 0.0f, -5.0f);


    MainViewport.add<DefaultCameraBindingComponent>(MainCamera);

    // ========================================
    // Rendering Management setup
    // ========================================

    // Texture Setup
    auto TextureShader = gameWorld.add<ShaderHandle>();
    TextureShader.add<ShaderSourceComponent>(
        "./resources/shader/texture.vert", "./resources/shader/texture.frag"
    );
    TextureShader.add<UniformMappingsComponent<UniformScope::Pass>>(
        UniformMapping{UniformSemantics::ProjectionMatrix, "projectionMatrix"},
        UniformMapping{UniformSemantics::ViewMatrix, "viewMatrix"}
    );
    TextureShader.add<UniformMappingsComponent<UniformScope::Material>>(
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );
    TextureShader.add<UniformMappingsComponent<UniformScope::Draw>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"}
    );

    // define Texture Entity
    auto Texture = gameWorld.add<TextureHandle>();
    Texture.add<TextureSourceComponent>("./resources/textures/garagecraft_games.png");

    // TextureObject uses a Rect
    auto TextureMesh = gameWorld.add<MeshHandle>();
    TextureMesh.add<MeshDataComponent>(Rect::meshData());
    TextureMesh.add<MeshUploadRequestComponent>();
    TextureMesh.add<VertexAttributeLayoutComponent<PerVertex>>(
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::Position, VertexAttributeType::Vec3f},
            0,
            sizeof(Vertex),
            offsetof(Vertex, position),
            0
        },
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::TextureCoordinates, VertexAttributeType::Vec2f},
            2,
            sizeof(Vertex),
            offsetof(Vertex, texCoords),
            0
        }
    );
    auto TextureMaterial = gameWorld.add<MaterialHandle>();
    TextureMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Blue);

    // Plain Mesh
    // Texture Setup
    auto PlainShader = gameWorld.add<ShaderHandle>();
    PlainShader.add<ShaderSourceComponent>(
        "./resources/shader/mesh.vert", "./resources/shader/mesh.frag"
    );
    PlainShader.add<UniformMappingsComponent<UniformScope::Pass>>(
        UniformMapping{UniformSemantics::ProjectionMatrix, "projectionMatrix"},
        UniformMapping{UniformSemantics::ViewMatrix, "viewMatrix"}
    );
    PlainShader.add<UniformMappingsComponent<UniformScope::Material>>(
        UniformMapping{UniformSemantics::MaterialBaseColor, "color"}
    );
    PlainShader.add<UniformMappingsComponent<UniformScope::Draw>>(
        UniformMapping{UniformSemantics::ModelMatrix, "modelMatrix"}
    );

    auto PlainMesh = gameWorld.add<MeshHandle>();
    PlainMesh.add<MeshDataComponent>(WireframeSphere::meshData());
    PlainMesh.add<MeshUploadRequestComponent>();
    PlainMesh.add<VertexAttributeLayoutComponent<PerVertex>>(
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::Position, VertexAttributeType::Vec3f},
            0,
            sizeof(Vertex),
            offsetof(Vertex, position),
            0
        },
        VertexAttributeLayout{
            VertexAttribute{VertexAttributeSemantics::TextureCoordinates, VertexAttributeType::Vec2f},
            2,
            sizeof(Vertex),
            offsetof(Vertex, texCoords),
            0
        }
    );
    auto PlainMaterial = gameWorld.add<MaterialHandle>();
    PlainMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Red);

    // ========================================
    // Entity Setup
    // ========================================
    auto texturedObject = gameWorld.add<GameObjectHandle>();
    texturedObject.add<DefaultSceneMemberComponent>(MainScene);
    texturedObject.add<BoundsComponent<Local>>(Rect::boundsData());
    texturedObject.add<BoundsComponent<World>>();
    texturedObject.add<Rotation3DComponent<Local>>();
    texturedObject.add<Position3DComponent<Local>>(1.0f, 0.0f, 0.0f);
    texturedObject.add<Position3DComponent<World>>(0.0f, 0.0f, 0.0f);
    texturedObject.add<TransformComponent<World>>(1.0f);
    texturedObject.add<DefaultRenderPrototypeComponent<NonInstanced>>(
        TextureShader.handle(), TextureMaterial.handle(), TextureMesh.handle(), Texture.handle()
    );

    auto plainObject = gameWorld.add<GameObjectHandle>();
    plainObject.add<DefaultSceneMemberComponent>(MainScene);
    plainObject.add<BoundsComponent<Local>>(WireframeCube::boundsData());
    plainObject.add<BoundsComponent<World>>();
    plainObject.add<Rotation3DComponent<Local>>();
    plainObject.add<Position3DComponent<Local>>(-1.0f, 0.0f, 0.0f);
    plainObject.add<Position3DComponent<World>>(0.0f, 0.0f, 0.0f);
    plainObject.add<TransformComponent<World>>(1.0f);
    plainObject.add<DefaultRenderPrototypeComponent<NonInstanced>>(
        PlainShader.handle(), PlainMaterial.handle(), PlainMesh.handle()
    );

    // ----------------------------------------
    // ImGui and Debug Tooling
    // ----------------------------------------
    auto imguiBackend = ImGuiGlfwOpenGLBackend(MainWindow.handle(), gameWorld.ecsWorld());
    auto imguiOverlay = ImGuiOverlay::forBackend(&imguiBackend);
    auto fpsMetrics = FpsMetrics();
    auto framePacer = FramePacer();
    framePacer.setTargetFps(0.0F);
    FrameTiming frameTiming{};

    auto menu = new MainMenuWidget();
    auto fpsWidget = new FpsWidget(&fpsMetrics, &framePacer);
    auto logWidget = new LogWidget();

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

    // ----------------------------------------
    // GameLoop Config
    // ----------------------------------------
    gameLoop.scheduler()

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
                    WindowCreateSystem<WindowHandle>
                >
            >()
        .endWith<DefaultGLFWPlatformManager>()

        .beginSchedule(GameWorld::sessionState(EngineState::Warmup))
            .add<TextureUploadSystem<TextureHandle>>()
            .add<MeshUploadSystem<MeshHandle>>()
            .add<ShaderCompileSystem<ShaderHandle>>()
            .add<DefaultWarmupDoneSystem>()

        .endWith<
            Sequential<
                OpenGLTextureUploadManager<TextureHandle>,
                DefaultMeshUploadManager,
                DefaultShaderCompileManager,
                EngineStateManager
            >
        >()

        .beginSchedule(GameWorld::sessionState(EngineState::Running))

            // create parallel groups
            .add<
                Sequential<WorldTransformSystem<GameObjectHandle>, WorldBoundsUpdateSystem<GameObjectHandle>>,
                Sequential<
                    YawPitchRollUpdateSystem<CameraHandle>,
                    WorldTransformSystem<CameraHandle>,
                    PerspectiveCameraUpdateSystem<CameraHandle>
                >,
                Sequential<WorldTransformSystem<ParticleHandle>, WorldBoundsUpdateSystem<ParticleHandle>>
            >()

            // this will produce render commands after scenes have been culled according to
            // their active viewports
            .add(
                 DefaultSceneMemberVisibilitySystem<
                     GameObjectHandle, NonInstanced, AABBCullingStrategy<GameObjectHandle>
                 >(AABBCullingStrategy<GameObjectHandle>())
             )

            .add<DefaultSceneRenderSystem<GameObjectHandle, NonInstanced>>()
        .endWith<DefaultRenderManager>()


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

        const GamepadState gamepadState = GamepadState(); // = {};//inputManager->gamepadState(Gamepad::ONE);
        const auto inputSnapshot = InputSnapshot(gamepadState);

        gameLoop.update(frameTiming, inputSnapshot); // inputSnapshot, viewportSnapshots);;

        frameTiming = framePacer.sync();
        fpsMetrics.addFrame(frameTiming);
    }

    logger.info("Engine is now in State {0}", std::to_underlying(gameWorld.session().state<EngineState>()));

    return 0;
}