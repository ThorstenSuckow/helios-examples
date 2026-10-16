module;

#include <tuple>

export module thesis.handles;

import helios.ecs;

export namespace thesis::handles {

struct GameObjectHandleDomain {};
using GameObjectHandle = helios::ecs::common::types::EntityHandle<GameObjectHandleDomain>;

struct ParticleHandleDomain {};
using ParticleHandle = helios::ecs::common::types::EntityHandle<ParticleHandleDomain>;

struct PlatformHandleDomain {};
using PlatformHandle = helios::ecs::common::types::EntityHandle<PlatformHandleDomain>;

struct WindowHandleDomain {};
using WindowHandle = helios::ecs::common::types::EntityHandle<WindowHandleDomain>;

struct RenderTargetHandleDomain {};
using RenderTargetHandle = helios::ecs::common::types::EntityHandle<RenderTargetHandleDomain>;

struct ViewportHandleDomain {};
using ViewportHandle = helios::ecs::common::types::EntityHandle<ViewportHandleDomain>;

struct TextureHandleDomain {};
using TextureHandle = helios::ecs::common::types::EntityHandle<TextureHandleDomain>;

struct ShaderHandleDomain {};
using ShaderHandle = helios::ecs::common::types::EntityHandle<ShaderHandleDomain>;

struct MaterialHandleDomain {};
using MaterialHandle = helios::ecs::common::types::EntityHandle<MaterialHandleDomain>;

struct MeshHandleDomain {};
using MeshHandle = helios::ecs::common::types::EntityHandle<MeshHandleDomain>;

struct SceneHandleDomain {};
using SceneHandle = helios::ecs::common::types::EntityHandle<SceneHandleDomain>;

struct CameraHandleDomain {};
using CameraHandle = helios::ecs::common::types::EntityHandle<CameraHandleDomain>;

// Handle types registered with the EntityWorld.
using WorldHandles = std::tuple<
    GameObjectHandle,
    ParticleHandle,
    WindowHandle,
    PlatformHandle,
    SceneHandle,
    RenderTargetHandle,
    CameraHandle,
    ViewportHandle,
    TextureHandle,
    ShaderHandle,
    MaterialHandle,
    MeshHandle
>;

// Handle types whose entities are rendered by the RenderManager.
using RenderMemberHandles = std::tuple<ParticleHandle, GameObjectHandle>;

} // namespace thesis::handles
