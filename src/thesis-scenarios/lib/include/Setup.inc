// Scenarios read schedule and runtime system timings (scheduler.timings(...)).
// Requires helios-ecs-config.h to be included before this file.
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_ALL) != HELIOS_PERFORMANCE_ALL
#error "This scenario requires HELIOS_PERFORMANCE=3 (HELIOS_PERFORMANCE_SCHEDULES | HELIOS_PERFORMANCE_RUNTIME_SYSTEMS)."
#endif

if (!thesis::config::init(argc, argv)) {
    return 2;
}


auto METRICS = thesis::config::METRICS{};

#ifdef THREAD_COUNT
    const std::size_t MAX_THREADS = static_cast<std::size_t>(THREAD_COUNT);
#else
    const std::size_t MAX_THREADS = std::thread::hardware_concurrency() - 1U;
#endif

auto engineRuntime = thesis::bootstrap::bootstrapGameWorld();
auto& ecsDataContainer = engineRuntime->gameWorld.resourceRegistry();
auto& rrr = ecsDataContainer.get<helios::ecs::common::RuntimeResultRegistry>();

auto scheduler = helios::ecs::scheduling::Scheduler();
ecsDataContainer.emplace<UpdateContext>(0.0F, 0.0F, 0, helios::engine::input::InputSnapshot{});
scheduler.init(ecsDataContainer);

FrameTiming frameTiming{};
#if HEADLESS
// Without FramePacer, frameTiming is never updated; enforce the fixed time step explicitly.
frameTiming.deltaTime = thesis::config::FIXED_DELTA_TIME;
#endif
std::size_t frameCount = 0;
float totalTime = 0.0f;

thesis::util::prepare(ecsDataContainer, MAX_THREADS);