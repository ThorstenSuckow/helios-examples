        scheduler.beginSchedule(GameWorld::sessionState(EngineState::Any))
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
        >();