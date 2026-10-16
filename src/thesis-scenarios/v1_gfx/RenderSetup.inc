    constexpr unsigned int SCREEN_WIDTH = 1280;
    constexpr unsigned int SCREEN_HEIGHT = 720;

    constexpr bool ENABLE_VSYNC = false;

    constexpr float WINDOW_ASPECT_RATIO_NUMER = 16.0F;
    constexpr float WINDOW_ASPECT_RATIO_DENOM = 9.0F;

    auto& gameWorld = engineRuntime->gameWorld;
    gameWorld.session().trackState<helios::engine::runtime::enginestate::types::EngineState>();

    gameWorld.session().template setStateFrom<EngineState>(StateTransitionContext<EngineState>(
        EngineState::Undefined, EngineState::Booting, EngineStateTransitionId::BootRequest
    ));

    auto MainWindow = gameWorld.add<WindowHandle>();
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

    auto MainRenderTarget = gameWorld.add<RenderTargetHandle>();
    MainRenderTarget.add<OpenGLRenderTargetIdComponent>(0);
    MainRenderTarget.add<Size2DComponent>();
    MainRenderTarget.add<ClearComponent>(ClearFlags::Color);
    MainRenderTarget.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Black);

    auto MainViewport = gameWorld.add<ViewportHandle>();
    MainViewport.add<DebugNameComponent<ViewportHandle>>("MainViewport");
    MainViewport.add<ClearComponent>(ClearFlags::Color);
    MainViewport.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Black);
    // RenderTarget : Viewport (1:N)
    MainViewport.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);
    MainViewport.add<RectComponent<>>(helios::math::vec4f{0.0F, 0.0f, 1.0F, 1.0F});


    auto MainScene = gameWorld.add<SceneHandle>();
    MainWindow.add<DefaultRenderTargetBindingComponent>(MainRenderTarget);

    // Viewport : Scene (N:1)
    MainViewport.add<DefaultSceneBindingComponent>(MainScene);

    auto MainCamera = gameWorld.add<CameraHandle>();
    MainCamera.add<DebugNameComponent<CameraHandle>>("MainCamera");
    MainCamera.add<PerspectiveCameraComponent>(
        helios::math::radians(90.0F), WINDOW_ASPECT_RATIO_NUMER / (WINDOW_ASPECT_RATIO_DENOM)
    );
    MainCamera.add<ProjectionMatrixComponent>();
    MainCamera.add<ViewMatrixComponent>();
    MainCamera.add<YawPitchRollComponent>();
    MainCamera.add<Rotation3DComponent<Local>>();
    MainCamera.add<TransformComponent<World>>(1.0F);
    MainCamera.add<Position3DComponent<Local>>(0.0F, 0.0F, (-AXIS_LENGTH / 2.0F) - 50.0f);
    MainCamera.add<DefaultSceneMemberComponent>(MainScene);
    MainViewport.add<DefaultCameraBindingComponent>(MainCamera);


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
    auto InstanceShader = gameWorld.add<ShaderHandle>();
    InstanceShader.add<ShaderSourceComponent>("./resources/cube.vert", "./resources/cube.frag");
    InstanceShader.add<UniformMappingsComponent<UniformScope::Pass>>(
        UniformMapping{.semantics = UniformSemantics::ProjectionMatrix, .name = "projectionMatrix"},
        UniformMapping{.semantics = UniformSemantics::ViewMatrix, .name = "viewMatrix"}
    );
    InstanceShader.add<UniformMappingsComponent<UniformScope::Material>>(
        UniformMapping{.semantics = UniformSemantics::MaterialBaseColor, .name = "color"}
    );
    InstanceShader.add<UniformMappingsComponent<UniformScope::Draw>>(
        UniformMapping{.semantics = UniformSemantics::ModelMatrix, .name = "modelMatrix"}
    );

    auto RectMesh = gameWorld.add<MeshHandle>();
    RectMesh.add<MeshDataComponent>(Rect::meshData());
    RectMesh.add<MeshUploadRequestComponent>();
    RectMesh.add<VertexAttributeLayoutComponent<PerVertex>>(VertexAttributeLayout{
        .attribute =
            VertexAttribute{.semantics = VertexAttributeSemantics::Position, .type = VertexAttributeType::Vec3f},
        .location = 0,
        .stride = sizeof(Vertex),
        .offset = offsetof(Vertex, position),
        .divisor = 0
    });
    RectMesh.add<VertexAttributeLayoutComponent<PerInstance>>(VertexAttributeLayout{
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


    auto GameObjectMaterial = gameWorld.add<MaterialHandle>();
    GameObjectMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Blue);

    auto ParticleMaterial = gameWorld.add<MaterialHandle>();
    ParticleMaterial.add<ColorComponent>(helios::engine::rendering::common::types::Colors::Green);

    auto gameObjectRenderCallback = [&](auto& gameObject) {
        gameObject.setActive(true);
        gameObject.template add<thesis::components::GameObjectTagComponent>();
        gameObject.template add<DefaultSceneMemberComponent>(MainScene);
        gameObject.template add<DefaultRenderPrototypeComponent<Instanced>>(
            InstanceShader.handle(), GameObjectMaterial.handle(), RectMesh.handle()
        );
    };

    auto borderRenderCallbackGameObject = [&](auto& border) {
        border.setActive(true);
        border.template add<thesis::components::GameObjectTagComponent>();
        border.template add<DefaultSceneMemberComponent>(MainScene);
        border.template add<DefaultRenderPrototypeComponent<Instanced>>(
            InstanceShader.handle(), BoundaryMaterial.handle(), RectMesh.handle()
        );
    };

    auto borderRenderCallbackParticle = [&](auto& border) {
        border.setActive(true);
        border.template add<thesis::components::ParticleTagComponent>();
        border.template add<DefaultSceneMemberComponent>(MainScene);
        border.template add<DefaultRenderPrototypeComponent<Instanced>>(
            InstanceShader.handle(), BoundaryMaterial.handle(), RectMesh.handle()
        );
    };

    auto particleRenderCallback = [&](auto& particle) {
        particle.setActive(true);
        particle.template add<thesis::components::ParticleTagComponent>();
        particle.template add<DefaultSceneMemberComponent>(MainScene);
        particle.template add<DefaultRenderPrototypeComponent<Instanced>>(
            InstanceShader.handle(), ParticleMaterial.handle(), RectMesh.handle()
        );
    };