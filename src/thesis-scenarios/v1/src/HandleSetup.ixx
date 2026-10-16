module;

export module thesis.handles;

import helios.ecs;

export namespace thesis::handles {

struct UniHandleDomain {};
using UniHandle = helios::ecs::common::types::EntityHandle<UniHandleDomain>;

// All roles share the one unified handle domain.
using WindowHandle       = UniHandle;
using RenderTargetHandle = UniHandle;
using ViewportHandle     = UniHandle;
using TextureHandle      = UniHandle;
using ShaderHandle       = UniHandle;
using MaterialHandle     = UniHandle;
using MeshHandle         = UniHandle;
using SceneHandle        = UniHandle;
using CameraHandle       = UniHandle;

} // namespace thesis::handles
