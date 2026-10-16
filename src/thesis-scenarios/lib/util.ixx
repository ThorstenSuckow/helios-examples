module;

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <numbers>
#include <numeric>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <exception>

export module thesis.util;

import helios.core.common;
import helios.core.io;
import helios.core.thread;
import helios.ecs.entity;
import helios.ecs.common;
import helios.ecs.manager;
import helios.engine.core;
import helios.math;
import helios.physics;
import helios.engine.spatial;
import helios.engine.runtime;
import thesis.config;
import helios.engine.rendering.mesh;
import helios.gameplay.lifecycle;
import thesis.strategies;

export namespace thesis::util {

    helios::math::vec3f randomVec3f(const std::uint64_t seed) {
        auto rand = helios::core::common::Random(seed);
        return {
            rand.randomFloat(-1.0F, 1.0F), 
            rand.randomFloat(-1.0F, 1.0F), 
            rand.randomFloat(-1.0F, 1.0F)
        };
    }

    template<typename THandle>
    void createObjects(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        const int randomizerOffset = 0
    ) {
        createObjects<THandle>(entityManager, [](auto&){}, randomizerOffset);
    }

    template<typename THandle, typename TFunc>
    void createGameObjects(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        TFunc&& func,
        const std::uint32_t randomizerOffset = 0
    ) {
        std::uint32_t NUM_OBJECTS = 0;

        float borderAccount  = thesis::config::BORDER_SCALE[0]/2.0f;
        float startLeft = -thesis::config::AXIS_LENGTH / 2.0F;
        float startBottom = -thesis::config::AXIS_LENGTH / 2.0F - borderAccount;
        float x = startLeft + thesis::config::TILE_SIZE - borderAccount;

        for (int i = 0; i < thesis::config::OBJECTS_PER_AXIS; i++) {

            float y = startBottom + thesis::config::TILE_SIZE;

            for (int j = 0; j < thesis::config::OBJECTS_PER_AXIS; j++) {
                auto handle = entityManager.create();
                auto gameObject = helios::ecs::entity::Entity{handle, &entityManager};

                gameObject.template add<helios::physics::collision::components::CollisionComponent>();
                gameObject.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>>(
                    helios::engine::rendering::mesh::assets::WireframeCube::boundsData()
                );
                gameObject.template add<helios::engine::spatial::components::BoundsComponent<
                    helios::engine::core::types::World>>();
                gameObject.template add<helios::engine::spatial::components::Rotation3DComponent<
                    helios::engine::core::types::Local>>();
                gameObject.template add<helios::engine::spatial::components::Scale3DComponent<
                    helios::engine::core::types::Local>>(1.0F, 1.0F, 1.0F);
                gameObject.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(
                    static_cast<float>(x), static_cast<float>(y), 0.0F
                );
                gameObject.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::World>>(0.0F, 0.0F, 0.0F);
                gameObject.template add<helios::physics::motion::components::Velocity3DComponent<helios::engine::core::types::Local>>(
                    randomVec3f(
                             ((std::uint64_t)randomizerOffset << 32) | NUM_OBJECTS
                    ).withZ(0.0F).normalize() * thesis::config::GAME_OBJECT_SPEED
                );

                gameObject.template add<helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>>(1.0F);

                std::invoke(func, gameObject);

                NUM_OBJECTS++;

                y += thesis::config::TILE_SIZE;
            }
            x += thesis::config::TILE_SIZE;

        }
    };

    template<typename THandle>
    void createParticles(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        const int count,
        const helios::math::vec3f location,
        const int randomizerOffset = 0,
        const std::uint32_t index = 0
    ) {
        createParticles(entityManager, count, location, [](auto&){}, randomizerOffset, index);
    }
    
    template<typename THandle, typename TFunc>
    void createParticles(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        const std::uint32_t count,
        const helios::math::vec3f location,
        TFunc&& func,
        const std::uint32_t randomizerOffset = 0
    ) {

        for (std::uint32_t i = 0; i < count; i++) {
            auto handle = entityManager.create();
            auto particle = helios::ecs::entity::Entity{handle, &entityManager};

            particle.template add<helios::physics::collision::components::CollisionComponent>();

            particle.template add<helios::gameplay::lifecycle::components::LifetimeComponent>(thesis::config::PARTICLE_LIFETIME);

            particle.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>>(
                helios::engine::rendering::mesh::assets::WireframeCube::boundsData()
            );
            particle.template add<helios::engine::spatial::components::BoundsComponent<
                helios::engine::core::types::World>>();
            particle.template add<helios::engine::spatial::components::Rotation3DComponent<
                helios::engine::core::types::Local>>();
            particle.template add<helios::engine::spatial::components::Scale3DComponent<
                helios::engine::core::types::Local>>(1.0F, 1.0F, 1.0F);
            particle.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(location);
            particle.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::World>>(0.0F, 0.0F, 0.0F);
            particle.template add<helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>>(1.0F);

            particle.template add<helios::physics::motion::components::Velocity3DComponent<helios::engine::core::types::Local>>(
               randomVec3f(
                 (static_cast<std::uint64_t>(randomizerOffset) << 32) | i
               ).withZ(0.0F).normalize()*thesis::config::PARTICLE_SPEED
            );

            std::invoke(func, particle);
        }
    };

    template<typename THandle>
    void createBoundaries(helios::ecs::entity::EntityManager<THandle>& entityManager) {
        createBoundaries<THandle>(entityManager, [](auto&){});
    }   
    
    template<typename THandle, typename TFunc>
    void createBoundaries(
        helios::ecs::entity::EntityManager<THandle>& entityManager,
        TFunc&& func
    ) {

        constexpr auto borderAccount = config::BORDER_SCALE[0]/2.0f;

        for (int i = 0; i < 4; i++) {
            auto handle = entityManager.create();
            auto border = helios::ecs::entity::Entity{handle, &entityManager};

            switch (i) {
                case 0: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(0.0F, -config::AXIS_LENGTH/2.0f - borderAccount , 0.0F);
                    break;
                case 1: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(config::AXIS_LENGTH/2.0f + borderAccount , 0.0F, 0.0F);
                    break;
                case 2: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(0.0F, config::AXIS_LENGTH/2.0f + borderAccount , 0.0F);
                    break;
                case 3: border.template add<helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>>(-config::AXIS_LENGTH/2.0f - borderAccount , 0.0F, 0.0F);
                    break;
            }
            border.template add<helios::engine::spatial::components::Scale3DComponent<helios::engine::core::types::Local>>(config::AXIS_LENGTH , config::BORDER_SCALE[1], config::BORDER_SCALE[2]);
            border.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>>(helios::engine::rendering::mesh::assets::WireframeCube::boundsData());
            border.template add<helios::engine::spatial::components::Rotation3DComponent<helios::engine::core::types::Local>>(
                helios::math::quatf::fromEulerAngles<helios::math::Intrinsic>(
                    0.0f, 0.0f, i* (std::numbers::pi / 2)
                )
            );
            border.template add<helios::physics::collision::components::CollisionComponent>();
            border.template add<helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::World>>();
            border.template add<helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>>(1.0F);

            // update fixed component values so we do not require systems
            const auto boundsLocal = border.template get<
                helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::Local>
                >();

            const auto localRotation = border.template get<
                helios::engine::spatial::components::Rotation3DComponent<helios::engine::core::types::Local>
            >();
            const auto localPosition = border.template get<
                helios::engine::spatial::components::Position3DComponent<helios::engine::core::types::Local>
            >();
            const auto worldTransform =  border.template get<
                helios::engine::spatial::components::TransformComponent<helios::engine::core::types::World>
            >();

            const auto worldBounds = border.template get<
                helios::engine::spatial::components::BoundsComponent<helios::engine::core::types::World>
            >();

            worldTransform->setValue(localRotation->value().rotationMatrix().withTranslation(
                            localPosition->value())
            );

            worldTransform->setValue(localRotation->value()
                .rotationMatrix()
                .withScaling(helios::math::vec3f{config::AXIS_LENGTH , config::BORDER_SCALE[1], config::BORDER_SCALE[2]})
                .withTranslation(localPosition->value()));

            worldBounds->setValue(boundsLocal->value().applyTransform(worldTransform->value()));


            std::invoke(func, border);

        }
    };

    template<typename THandle>
    auto collCb() {
        return collCb<THandle>([](auto&){});
    }

    template<typename THandle, typename TFunc>
    auto collCb(TFunc&& func) {

        return [&, counter = std::uint32_t{0}](
          auto& targetEntityManager,
          auto collisionCommands,
          const helios::engine::runtime::gameloop::types::UpdateContext&
        ) mutable {
            for (const auto& collision : collisionCommands) {
                thesis::util::createParticles<THandle>(
                    targetEntityManager,
                    thesis::config::PARTICLE_SPAWN_COUNT,
                    collision.location,
                    std::forward<TFunc>(func),
                    counter++
                );
            }
        };
    }

    template<typename Thandle>
    auto grid() {
        return thesis::strategies::GridCollisionDetection<Thandle>(
            thesis::config::SPATIAL_GRID, thesis::config::CELL_SIZE
        );
    }

    void updateFrameMetrics(auto& ecsDataContainer, auto& frameTiming,  auto&frameCount, auto &totalTime) {
        totalTime += frameTiming.totalFrameTime;
        frameCount++;
        auto updateContext = helios::engine::runtime::gameloop::types::UpdateContext(
              frameTiming.deltaTime,
              totalTime,
              frameCount,
              {}
        );
        ecsDataContainer.template replace<helios::engine::runtime::gameloop::types::UpdateContext>(updateContext);
    };

    bool shouldbeRunning(std::size_t frameCount) {
        return frameCount < thesis::config::WARMUP_FRAMES +
                          thesis::config::MEASURE_FRAMES +
                          thesis::config::COOLDOWN_FRAMES;
    }

    void prepare(helios::ecs::common::container::EcsDataContainer& ecsDataContainer) {
        ecsDataContainer.emplace<helios::core::thread::ThreadPool>(thesis::config::MAX_THREADS);
    }

    void init(helios::ecs::common::container::EcsDataContainer& ecsDataContainer) {
        auto& managerRegistry = ecsDataContainer.get<helios::ecs::manager::ManagerRegistry>();

        for (auto& manager : managerRegistry.items()) {
            manager->init(ecsDataContainer);
        }
    }


    template<typename T>
    bool writeVectorToCSV(
        const std::string& filename,
        const std::string& column,
        const std::vector<T>& values) {
        const helios::core::io::BasicStringFileWriter writer;

        if (values.empty() || std::all_of(
            values.begin(), values.end(),
            [&](const auto& value) {
               return value == values.front();
            })
        ) {
            return true;
        }

        // open the file once and keep it open while writing all rows
        std::ofstream outfile{filename, std::ios::out | std::ios::trunc};
        if (!outfile) {
            return false;
        }

        bool ok = writer.tryWriteLine(outfile, "index," + column);

        if (!ok) {
            assert(ok);
            std::cerr << "Couldnt write to file." << std::endl;
            std::terminate();
        }
        if (values.size() < thesis::config::MEASURE_FRAMES + thesis::config::WARMUP_FRAMES) {
            std::cerr << values.size() << "  " << thesis::config::MEASURE_FRAMES + thesis::config::WARMUP_FRAMES << " " << " " << std::endl;
            std::terminate();
        }
        std::size_t index = 0;
        for (auto& value : values) {
            ok &= writer.tryWriteLine(outfile, std::format("{},{}", index++, value));
        }

        return ok;
    }


    std::string measurementsPath() {
        const std::time_t now = std::time(nullptr);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", &localTime);

        return (std::filesystem::current_path() / "measurements" / timestamp).string();
    }


    bool writeMetricsToCSV(thesis::config::METRICS& metrics) {
        namespace cfg = thesis::config;

        std::string directory = measurementsPath();

        std::error_code errorCode;
        std::filesystem::create_directories(directory, errorCode);
        if (errorCode) {
            return false;
        }

        const auto file = [&directory](const char* name) {
            return (std::filesystem::path{directory} / name).string();
        };

        bool ok = true;
        ok &= writeVectorToCSV(file("schedule_timings.csv"), "schedule_time", metrics.SCHEDULE_TIMINGS);

        ok &= writeVectorToCSV(file("s_beta_go_timings.csv"), "s_beta_go_time", metrics.S_BETA_GO_TIMINGS);
        ok &= writeVectorToCSV(file("s_beta_prt_timings.csv"), "s_beta_prt_time", metrics.S_BETA_PRT_TIMINGS);

        ok &= writeVectorToCSV(file("s_delta_det_go_timings.csv"), "s_delta_det_go_time", metrics.S_DELTA_DET_GO_TIMINGS);
        ok &= writeVectorToCSV(file("s_delta_det_prt_timings.csv"), "s_delta_det_prt_time", metrics.S_DELTA_DET_PRT_TIMINGS);
        ok &= writeVectorToCSV(file("s_delta_res_go_timings.csv"), "s_delta_res_go_time", metrics.S_DELTA_RES_GO_TIMINGS);
        ok &= writeVectorToCSV(file("s_delta_res_prt_timings.csv"), "s_delta_res_prt_time", metrics.S_DELTA_RES_PRT_TIMINGS);
        ok &= writeVectorToCSV(file("s_lambda_prt_timings.csv"), "s_lambda_prt_time", metrics.S_LAMBDA_TIMINGS);

        ok &= writeVectorToCSV(file("m_col_timings.csv"), "m_col_time", metrics.M_COL_TIMINGS);
        ok &= writeVectorToCSV(file("m_lft_timings.csv"), "m_lft_time", metrics.M_LFT_TIMINGS);
        ok &= writeVectorToCSV(file("collision_pairs_found_go.csv"), "pairs_go", metrics.COLLISION_PAIRS_FOUND_GO);
        ok &= writeVectorToCSV(file("collision_pairs_found_prt.csv"), "pairs_prt", metrics.COLLISION_PAIRS_FOUND_PRT);
        ok &= writeVectorToCSV(file("collision_pairs_processed_go.csv"), "pairs_go", metrics.COLLISION_PAIRS_PROCESSED_GO);
        ok &= writeVectorToCSV(file("collision_pairs_processed_prt.csv"), "pairs_prt", metrics.COLLISION_PAIRS_PROCESSED_PRT);
        ok &= writeVectorToCSV(file("collision_commands_go.csv"), "commands_go", metrics.COLLISION_COMMANDS_GO);
        ok &= writeVectorToCSV(file("lifetime_commands_prt.csv"), "commands_prt", metrics.LIFETIME_COMMANDS_PRT);
        ok &= writeVectorToCSV(file("particle_population.csv"), "particles", metrics.PARTICLE_POPULATION);

        return ok;
    }

}
