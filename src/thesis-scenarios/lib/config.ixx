module;

#include <cstddef>
#include <thread>
#include <vector>

export module thesis.config;

import helios.math;

import helios.ecs.entity;


export namespace thesis::config {
    const     auto MAX_THREADS           = std::thread::hardware_concurrency() - 1U;
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
    std::vector<T> reservedVector(const std::size_t capacity = WARMUP_FRAMES + MEASURE_FRAMES + COOLDOWN_FRAMES ) {
        std::vector<T> vec;
        vec.reserve(capacity);
        return vec;
    }

    std::vector<float> FRAME_TIMINGS                       = reservedVector<float>();
    std::vector<float> SCHEDULE_TIMINGS                    = reservedVector<float>();
    std::vector<float> S_DELTA_DET_TIMINGS                 = reservedVector<float>();
    std::vector<float> S_DELTA_PRT_TIMINGS                 = reservedVector<float>();
    std::vector<float> M_COL_TIMINGS                       = reservedVector<float>();
    std::vector<float> M_LFT_TIMINGS                       = reservedVector<float>();
    std::vector<std::size_t> COLLISION_PAIRS_FOUND_GO      = reservedVector<std::size_t>();
    std::vector<std::size_t> COLLISION_PAIRS_FOUND_PRT     = reservedVector<std::size_t>();
    std::vector<std::size_t> COLLISION_PAIRS_PROCESSED_GO  = reservedVector<std::size_t>();
    std::vector<std::size_t> COLLISION_PAIRS_PROCESSED_PRT = reservedVector<std::size_t>();
    std::vector<std::size_t> COLLISION_COMMANDS_GO         = reservedVector<std::size_t>();
    std::vector<std::size_t> LIFETIME_COMMANDS_PRT         = reservedVector<std::size_t>();
    std::vector<std::size_t> PARTICLE_POPULATION           = reservedVector<std::size_t>();
}