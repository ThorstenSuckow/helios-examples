#if !HEADLESS

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
        >()
        .beginSchedule(GameWorld::sessionState(EngineState::Running))
        .add<Sequential<
            YawPitchRollUpdateSystem<CameraHandle>,
            WorldTransformSystem<CameraHandle>,
            PerspectiveCameraUpdateSystem<CameraHandle>
            >
        >();

        auto& managerSchedule = scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running));
        auto& systemSchedule = scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running));

#else

        auto& managerSchedule = scheduler.beginSchedule();
        auto& systemSchedule = scheduler.beginSchedule();

#endif