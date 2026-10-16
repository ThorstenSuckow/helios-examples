auto METRICS = thesis::config::METRICS{};

auto engineRuntime = thesis::bootstrap::bootstrapGameWorld();
auto& ecsDataContainer = engineRuntime->gameWorld.resourceRegistry();
auto& rrr = ecsDataContainer.get<helios::ecs::common::RuntimeResultRegistry>();

auto scheduler = helios::ecs::scheduling::Scheduler();
ecsDataContainer.emplace<UpdateContext>(0.0f, 0.0f, 0, helios::engine::input::InputSnapshot{});
scheduler.init(ecsDataContainer);

FrameTiming frameTiming{};
std::size_t frameCount = 0;
float totalTime = 0.0f;

thesis::util::prepare(ecsDataContainer);