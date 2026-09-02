
#include <cassert>
#include <thread>

import helios.core;
import helios.ecs;
import helios.engine;
import helios.math;
import helios.physics;
import helios.opengl;
import helios.glfw;
import helios.imgui;
import helios.gameplay;

import helios.engine.bootstrap;

#include "../Namespaces.h"


int main() {

    auto maxWorker = std::max(2U, std::thread::hardware_concurrency() - 1);
    auto jobSystem = JobSystem(maxWorker);
    auto engineRuntime = bootstrapGameWorld(jobSystem);
    auto& gameWorld = engineRuntime->gameWorld;
    auto& gameLoop = engineRuntime->gameLoop;

    auto gameObject = gameWorld.add<GameObjectHandle>();
    gameObject.add<Position3DComponent>(0.0F, 0.0F, 0.0F);
    gameObject.add<Velocity3DComponent>(0.0F, 0.0F, 0.0F);
    gameObject.add<ColorComponent>(0.0F, 0.0F, 0.0F, 0.0f);

    using Q1 = Query<
        GameObjectHandle,
            Read<Position3DComponent, Velocity3DComponent>,
            Write<Velocity3DComponent>
    >;

    using Q2 = Query<
        GameObjectHandle,
            Read<ColorComponent>,
            Write<ColorComponent>
    >;

    gameLoop.phase(PhaseType::Main)
        .beginPass(EngineState::Any)

        .addSystem([&](Q1 query1, Q2 query2) {
            for (auto [entity, position, velocity] : query1) {
                velocity->setValue({0.0F, 0.0F, 0.0F});
            }
            for (auto [entity, color] : query2) {
                entity.setTrackedValue(color, vec4f{0.0F, 0.0F, 0.0F, 0.0f});
            }
        })
        .executeCommands<EntityMutationManager<GameObjectHandle>>()
        .endPass();

    gameWorld.init();
    gameLoop.init();

    gameWorld.session().template setStateFrom<EngineState>(StateTransitionContext<EngineState>(
        EngineState::Undefined, EngineState::Booting, EngineStateTransitionId::BootRequest
    ));

    FrameTiming frameTiming{};
    FramePacer framePacer{};
    float DELTA_TIME = 0.0f;


    InputSnapshot inputSnapshot{};

    while (gameLoop.isRunning()) {
        framePacer.beginFrame();

        gameLoop.update(frameTiming, inputSnapshot);

        frameTiming = framePacer.sync();
    }



}