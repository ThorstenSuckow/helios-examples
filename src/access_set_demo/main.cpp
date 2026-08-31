
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


    gameLoop.phase(PhaseType::Main)
        .beginPass(EngineState::Any)

        .addSystem([&](Query<
            Read<Position3DComponent<GameObjectHandle>, Velocity3DComponent<GameObjectHandle>>,
            Write<Velocity3DComponent<GameObjectHandle>>
        > query) {
            for (auto [handle, position, velocity] : query) {
                velocity->setValue({0.0F, 0.0F, 0.0F});
            }
        })


        .addSystem([&](Query<
            Read<Position3DComponent<GameObjectHandle>, Velocity3DComponent<GameObjectHandle>>,
            Write<Velocity3DComponent<GameObjectHandle>>
        > query) {
            for (auto [handle, position, velocity] : query) {
                position->setValue({0.0F, 0.0F, 0.0F});
            }
        })

        .addSystem([&](Query<
            GameObjectHandle,
            types::Read<Position3DComponent, Velocity3DComponent>,
            types::Write<Velocity3DComponent>
        > query) {
            for (auto [handle, position, velocity] : query) {
                position->setValue({0.0F, 0.0F, 0.0F});
            }
        })
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