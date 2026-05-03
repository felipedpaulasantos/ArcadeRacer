# AGENTS.md — ArcadeRacer Codebase Guide

Arcade racing game for Unreal Engine 5.7, inspired by Burnout 3 / NFS Hot Pursuit. Mixed Blueprint + C++ project.

---

## Architecture Overview

| Layer | Location | Role |
|---|---|---|
| Vehicle physics (plugin) | `Plugins/ArcadeVehicleSystem/Source/` | Arcade physics simulation, networking, suspension |
| Game logic (Blueprints) | `Content/Core/` | Controllers, GameModes, GameplayEffects, GlobalEvents, Interfaces, Models, Pawns |
| C++ utilities | `Source/ArcadeRacer/Public/` & `Private/` | Thin helper libraries exposed to Blueprints |

**Rule:** C++ in `Source/ArcadeRacer/` is intentionally minimal (only two Blueprint function libraries). New gameplay logic lives in `Content/Core/` Blueprints. Only add C++ there for functionality not expressible in Blueprints.

---

## Vehicle System (ArcadeVehicleSystem Plugin)

Third-party plugin by Furious Production LTD. **Do not modify plugin source** unless absolutely necessary.

- Base class: `UArcadeVehicleMovementComponentBase` (`Plugins/.../Public/Movement/`)
  - Skeletal mesh vehicles → `UArcadeVehicleMovementComponent`
  - Static mesh vehicles → `UStaticArcadeVehicleMovementComponent`
- All vehicle tuning lives in `FVehicleSettings` (nested structs: `Physics`, `Engine`, `Steering`, `Suspension`, `Advanced`)
- To change settings at runtime: call `SetVehicleSettings()` / `SetPhysicsSettings()` etc., then **must call `ApplyVehicleSettings()`** to reinitialize
- Input flow: call `SetAccelerationInput(float)`, `SetTurningInput(float)`, `SetDriftInput(bool)`, `SetStabilizationInput(bool)` on the movement component
- Speed unit conversion: `velocity * KMH_MULTIPLIER` (= `0.036f`) → km/h
- Custom movement extensions: bind to the `CalculateCustomVehicleMovement` delegate instead of subclassing

### Networking
- Client → Server: unreliable state RPC (`OnReceiveState_Server`)
- Server → all clients: replicated `FVehiclePhysicsState` (`OnRep_ServerState`)
- Teleport: reliable multicast (`OnReceiveTeleport_Client`)
- Physics correction uses exponential interpolation; snap thresholds are in `FVehiclePhysicsSettings`

---

## C++ Blueprint Libraries (`Source/ArcadeRacer/`)

### `UDetailedPrintLibrary`
Drop-in replacement for `Print String` that prefixes the message with the calling class name: `[VehiclePawn_C] message`. Use `DetailedPrintString` in all debug Blueprint nodes.

### `ULandscapeSplineBlueprintLibrary`
Exposes Landscape Splines with a `USplineComponent`-like API (not natively available in Blueprints):
- `GetLandscapeSplineLocationAtDistance` / `...OnLandscape` — distance-along query
- `FindLandscapeSplineInputKeyClosestToWorldLocation` — nearest-point query
- InputKey format: `SegmentIndex + AlphaWithinSegment` (e.g., `3.25` = segment 3 at 25%)
- Results are **approximations** via sampled interpolation points (default 64 samples/segment)

---

## Key Plugins (uproject)

| Plugin | Purpose |
|---|---|
| `ArcadeVehicleSystem` | Arcade vehicle physics (primary vehicle system) |
| `GameplayAbilities` | GAS for gameplay effects (`Content/Core/GameplayEffects/`) |
| `GlobalEvents` | Cross-Blueprint event bus (`Content/Core/GlobalEvents/`) |
| `PCG` | Procedural content generation (`Content/PCG/`) |
| `RawInput` | Raw controller/wheel input support |
| `ChaosVehiclesPlugin` | Enabled but **not** used for vehicle movement; ArcadeVehicleSystem replaces it |
| `FathomUELink` | Explicitly **disabled** — engine plugin with missing/incompatible binaries that caused startup warnings |

---

## Developer Workflows

**Clean build artifacts** (run from project root in PowerShell):
```powershell
.\clean-unreal.ps1
```
Deletes `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache` recursively across project and plugins.

**Rebuild:** Open `ArcadeRacer.sln` in Visual Studio and build `Development Editor | Win64`, or use Unreal Editor → Tools → Compile.

**Adding C++ to the game module:** Add dependencies in `Source/ArcadeRacer/ArcadeRacer.Build.cs`. Current public deps: `Core`, `CoreUObject`, `Engine`, `InputCore`, `Landscape`.

---

## Conventions

- Code comments may appear in **Portuguese** (project author's native language) — this is normal, not an error.
- Blueprint assets for variants live in `Content/Variant_Offroad/` and `Content/Variant_Timetrial/`.
- Vehicle sound assets are under `Content/CarsSounds/Model1/`–`Model7/`.
- Environment assets use `UltraDynamicSky` and `EasyAtmos` plugins (content-only, under `Content/`).

