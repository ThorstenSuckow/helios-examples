/**
 * @file Namespaces.h
 * @brief Common using-namespace declarations for the scoring demo.
 *
 * Include this header after all module imports to bring frequently
 * used namespaces into scope.
 */

#pragma once

using namespace helios::engine::bootstrap;

// External backends
using namespace helios::opengl;

// Rendering
using namespace helios::engine::rendering;
using namespace helios::engine::rendering::mesh;
using namespace helios::engine::rendering::material;
using namespace helios::engine::rendering::shader;
using namespace helios::engine::rendering::mesh::types;
using namespace helios::engine::rendering::material::types;
using namespace helios::engine::rendering::shader::types;
using namespace helios::engine::rendering::shader::components;
using namespace helios::engine::rendering::shader::systems;
using namespace helios::engine::rendering::viewport;
using namespace helios::engine::rendering::viewport::types;
using namespace helios::engine::rendering::renderTarget;
using namespace helios::engine::rendering::renderTarget::types;
using namespace helios::engine::rendering::renderTarget::components;
using namespace helios::engine::rendering::mesh::assets;
using namespace helios::engine::rendering::mesh::components;
using namespace helios::engine::rendering::mesh::systems;

// Input
using namespace helios::engine::input;
using namespace helios::engine::input::gamepad;
using namespace helios::engine::input::types;

// Core libraries
using namespace helios::math;
using namespace helios::engine::scene;
using namespace helios::engine::scene::types;
using namespace helios::engine::core::units;
using namespace helios::engine::core::types;
using namespace helios::engine::core;
using namespace helios::engine::core::thread;
using namespace helios::engine::core::container;
using namespace helios::engine::core::components;
using namespace helios::engine::core::systems;
using namespace helios::engine::util::io;
using namespace helios::engine::util::time;

// ECS library
using namespace helios::ecs;
using namespace helios::ecs::commands;
using namespace helios::ecs::components;
using namespace helios::ecs::concepts;
using namespace helios::ecs::strategies;
using namespace helios::ecs::types;


// Engine core
using namespace helios::engine::state;
using namespace helios::engine::state::types;
using namespace helios::engine::tooling;


// Runtime
using namespace helios::engine::runtime::enginestate;
using namespace helios::engine::runtime::enginestate::systems;
using namespace helios::engine::runtime::enginestate::types;
using namespace helios::engine::runtime::messaging::command;
using namespace helios::engine::runtime::world;
using namespace helios::engine::runtime::world::types;
using namespace helios::engine::runtime::gameloop;
using namespace helios::engine::runtime::pooling;

// Platform
using namespace helios::engine::platform::lifecycle;
using namespace helios::engine::platform::lifecycle::commands;
using namespace helios::engine::platform::lifecycle::systems;
using namespace helios::engine::platform::environment;
using namespace helios::engine::platform::environment::components;
using namespace helios::engine::platform::environment::systems;
using namespace helios::engine::platform::environment::types;
using namespace helios::engine::platform::window;
using namespace helios::engine::platform::window::components;
using namespace helios::engine::platform::window::systems;
using namespace helios::engine::platform::window::types;
using namespace helios::glfw;
using namespace helios::glfw::components;
using namespace helios::glfw::systems;
using namespace helios::opengl::components;

// imgui
using namespace helios::imgui;
using namespace helios::imgui::widgets;
using namespace helios::imgui::systems;

using namespace helios::engine::util::log;

// Mechanics: scoring, timing, combat

using namespace helios::engine::runtime::timing;
using namespace helios::engine::runtime::timing::systems;


// Modules: spatial, scene, rendering, UI, AI
using namespace helios::engine::spatial::components;
using namespace helios::engine::scene::components;
using namespace helios::engine::scene::systems;
using namespace helios::engine::scene;
using namespace helios::engine::rendering::viewport::systems;
using namespace helios::engine::rendering;
using namespace helios::engine::rendering::common::components;
using namespace helios::engine::rendering::common::types;
using namespace helios::engine::rendering::common::commands;


using namespace helios::physics::motion::components;
using namespace helios::physics::motion::systems;