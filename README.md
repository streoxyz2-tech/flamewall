# Predictive Input Buffer — Ayunaka

A separate Geode mod project for Geometry Dash 2.2081.

## Current implementation

- Configurable buffer window: **2–16 physics ticks**.
- F6 toggles the buffer during a level.
- Buffers jump-button press and release events in order, preserving the hold duration.
- Other buttons pass through unchanged.
- Clears pending events on death/reset and fails open if the queue overflows.

## Important accuracy note

This first implementation is an **input-delay buffer**, not a full ghost-player physics predictor. It queues the jump event for the configured number of physics ticks; it does not yet simulate future collisions or decide whether a later tick is safer. The name is retained as the project name, but the predictive simulation is a separate, more advanced stage.

A 240 Hz physics tick is approximately 4.17 ms. For example, 2 ticks ≈ 8.3 ms and 16 ticks ≈ 66.7 ms.

## Build

Requires Geode SDK for GD 2.2081, CMake, and a C++ toolchain.

```bash
geode sdk install
geode sdk install-binaries
geode build
```

The built `.geode` file should be in the `build` directory.

## Compatibility

Target: Geometry Dash 2.2081 / Geode 5.9.0. This source has not been compiled or tested in-game in this environment. Hook signatures and tick timing must be confirmed against the exact SDK/bindings installed on the build machine.
