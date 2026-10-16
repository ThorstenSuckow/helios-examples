module;

#include <cstddef>
#include <thread>
#include <vector>
#include <chrono>

export module thesis.config;

import helios.math;

import helios.ecs.entity;


export namespace thesis::config {

    constexpr auto LOGGING_ENABLED       = false;
    constexpr auto TARGET_FPS            = 0.0F;
    constexpr auto FIXED_DELTA_TIME      = 1.0F / 60.0F;
    constexpr auto WARMUP_FRAMES         = 1000;
    constexpr auto MEASURE_FRAMES        = 5000;
    constexpr auto COOLDOWN_FRAMES       = 100;
    constexpr auto OBJECTS_PER_AXIS      = 300;
    constexpr auto BORDER_SCALE          = helios::math::vec3f{1.0F, 1.0F, 1.0F};
    constexpr auto TILE_SIZE             = 2;
    constexpr auto PARTICLE_SPAWN_COUNT  = 2;
    constexpr auto PARTICLE_SPEED        = 32.0F;
    constexpr auto GAME_OBJECT_SPEED     = 1.0f;
    constexpr auto PARTICLE_LIFETIME     = 1.0f;
    constexpr auto AXIS_LENGTH           = TILE_SIZE * OBJECTS_PER_AXIS + TILE_SIZE - 1;
    constexpr auto CELL_SIZE             = AXIS_LENGTH / OBJECTS_PER_AXIS;

        constexpr auto SPATIAL_GRID = helios::math::aabbf{
            -AXIS_LENGTH / 2.0F, -AXIS_LENGTH / 2.0F, 0.0f,
            AXIS_LENGTH / 2.0F, AXIS_LENGTH / 2.0F, 0.0f
        };


        template<typename T>
        std::vector<T> resizedVector(const std::size_t capacity = WARMUP_FRAMES + MEASURE_FRAMES + COOLDOWN_FRAMES ) {
            std::vector<T> vec;
            vec.resize(capacity);
            return vec;
        }


        struct METRICS {
            std::vector<std::chrono::nanoseconds::rep> M_COL_TIMINGS       = resizedVector<std::chrono::nanoseconds::rep>();
            std::vector<std::chrono::nanoseconds::rep> M_LFT_TIMINGS       = resizedVector<std::chrono::nanoseconds::rep>();

            std::vector<std::chrono::nanoseconds::rep> S_BETA_GO_TIMINGS = resizedVector<std::chrono::nanoseconds::rep>();
            std::vector<std::chrono::nanoseconds::rep> S_BETA_PRT_TIMINGS = resizedVector<std::chrono::nanoseconds::rep>();

            std::vector<std::chrono::nanoseconds::rep> S_DELTA_DET_GO_TIMINGS = resizedVector<std::chrono::nanoseconds::rep>();
            std::vector<std::chrono::nanoseconds::rep> S_DELTA_DET_PRT_TIMINGS = resizedVector<std::chrono::nanoseconds::rep>();
            std::vector<std::chrono::nanoseconds::rep> S_DELTA_RES_GO_TIMINGS = resizedVector<std::chrono::nanoseconds::rep>();
            std::vector<std::chrono::nanoseconds::rep> S_DELTA_RES_PRT_TIMINGS = resizedVector<std::chrono::nanoseconds::rep>();
            std::vector<std::chrono::nanoseconds::rep> S_LAMBDA_TIMINGS = resizedVector<std::chrono::nanoseconds::rep>();

            std::vector<std::chrono::nanoseconds::rep> SCHEDULE_TIMINGS    = resizedVector<std::chrono::nanoseconds::rep>();

            std::vector<std::size_t> COLLISION_PAIRS_FOUND_GO      = resizedVector<std::size_t>();
            std::vector<std::size_t> COLLISION_PAIRS_FOUND_PRT     = resizedVector<std::size_t>();
            std::vector<std::size_t> COLLISION_PAIRS_PROCESSED_GO  = resizedVector<std::size_t>();
            std::vector<std::size_t> COLLISION_PAIRS_PROCESSED_PRT = resizedVector<std::size_t>();
            std::vector<std::size_t> COLLISION_COMMANDS_GO         = resizedVector<std::size_t>();
            std::vector<std::size_t> LIFETIME_COMMANDS_PRT         = resizedVector<std::size_t>();
            std::vector<std::size_t> PARTICLE_POPULATION           = resizedVector<std::size_t>();
        };;
}