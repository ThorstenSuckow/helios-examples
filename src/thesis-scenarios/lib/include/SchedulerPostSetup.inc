#if !HEADLESS
        scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running))
            .add(
                Sequential(
                    SceneMemberVisibilitySystem<
                        UniHandle, Instanced, NullCullingStrategy<UniHandle>, DefaultRenderHandles
                    >(NullCullingStrategy<UniHandle>()),
                    [&](EntityManager<UniHandle>& entityManager,
                       DefaultSceneMemberVisibilityRegistry<UniHandle, Instanced>& visibilityRegistry) {
                        // only entities with a LifetimeComponent (particles) fade with their age
                        auto* lifetimeSparseSet = entityManager.sparseSet<LifetimeComponent>();
                        visibilityRegistry.forEachVisibleMember([&lifetimeSparseSet = *lifetimeSparseSet](auto& visibleContext) {
                            if (auto* cmp = lifetimeSparseSet.get(visibleContext.memberHandle.entityId())) {
                                visibleContext.normalizedAge = cmp->value() / cmp->lifetime();
                            }
                        });
                    },
                    DefaultSceneRenderSystem<UniHandle, Instanced>()
                )
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
#endif
