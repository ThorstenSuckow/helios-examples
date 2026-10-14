module;

#include <thread>

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
    constexpr auto OBJECTS_PER_AXIS      = 5;
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
}