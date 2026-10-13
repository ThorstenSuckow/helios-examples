        scheduler.beginSchedule(GameWorld::sessionState(EngineState::Running))
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