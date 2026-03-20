# MinecraftConsoles

[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/xjc9JW4Bfp)

![Tutorial World](.github/TutorialWorld.png)

## Introduction

This project contains the source code of Minecraft Legacy Console Edition v1.6.0560.0 (TU19) from https://archive.org/details/minecraft-legacy-console-edition-source-code, with some fixes and improvements applied.

[Nightly Build](https://github.com/MLE-MP/MinecraftConsoles/releases/tag/nightly)

## Platform Support

- **Windows**: Supported for building and running the project
- **Linux**: The Windows nightly build may run through Wine or Proton Experimental, Tested by maintainers.
- **macOS / Linux**: The Windows nightly build may run through Wine or CrossOver based on community reports, but this is unofficial and not currently tested by the maintainers

## Features

- Fixed compilation and execution in both Debug and Release mode on Windows using Visual Studio 2022
- Added support for keyboard and mouse input
- Added fullscreen mode support (toggle using F11)
- (WIP) Disabled V-Sync for better performance
- Added a high-resolution timer path on Windows for smoother high-FPS gameplay timing
- Device's screen resolution will be used as the game resolution instead of using a fixed resolution (1920x1080)
- Multiplayer Discovery

## Multiplayer

Basic LAN multiplayer is available on the Windows build

- Hosting a multiplayer world automatically advertises it on the server browser.
- Other players can discover the session from the in-game Join Game menu.

### Launch Arguments

| Argument           | Description                                                                                         |
|--------------------|-----------------------------------------------------------------------------------------------------|
| `-server`          | Launches a headless server instead of the client.                                                   |

Headless server example:
```
Minecraft.Client.exe -server
```

The headless server also reads and writes `server.properties` in the working directory. If `-ip` / `-port` are omitted in `-server` mode, it falls back to `server-ip` / `server-port` from that file. Dedicated-server host options such as `trust-players`, `pvp`, `fire-spreads`, `tnt`, `difficulty`, `gamemode`, `spawn-animals`, and `spawn-npcs` are persisted there as well.

## Controls (Keyboard & Mouse)

- **Movement**: `W` `A` `S` `D`
- **Jump / Fly (Up)**: `Space`
- **Sneak / Fly (Down)**: `Shift` (Hold)
- **Sprint**: `Ctrl` (Hold) or Double-tap `W`
- **Inventory**: `E`
- **Drop Item**: `Q`
- **Crafting**: `C` Use `Q` and `E` to move through tabs (cycles Left/Right)
- **Toggle View (FPS/TPS)**: `F5`
- **Fullscreen**: `F11`
- **Pause Menu**: `Esc`
- **Attack / Destroy**: `Left Click`
- **Use / Place**: `Right Click`
- **Select Item**: `Mouse Wheel` or keys `1` to `9`
- **Accept or Decline Tutorial hints**: `Enter` to accept and `B` to decline
- **Game Info (Player list and Host Options)**: `TAB`
- **Toggle HUD**: `F1`
- **Toggle F3 Info**: `F3`

## Build & Run

1. Install [Visual Studio 2022](https://aka.ms/vs/17/release/vs_community.exe).
2. Clone the repository.
3. Open the project by double-clicking `MinecraftConsoles.sln`.
4. Make sure `Minecraft.Client` is set as the Startup Project.
5. Set the build configuration to **Release** (Release is also OK but has some bugs) and the target platform to **Windows64**, then build and run.

### CMake (Windows x64)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target MinecraftClient
```

For more information, see [COMPILE.md](COMPILE.md).

## Known Issues

- Native builds for platforms other than Windows have not been tested and are most likely non-functional. The Windows nightly build will run on macOS and Linux through Wine or CrossOver.

## Contributors
Would you like to contribute to this project? Please read our [Contributor's Guide](CONTRIBUTING.md) before doing so! This document includes our current goals, standards for inclusions, rules, and more.

## Star History

[![Star History Chart](https://api.star-history.com/image?repos=mclemp/MCLEClient&type=date&legend=top-left)](https://www.star-history.com/?repos=mclemp%2FMCLEClient&type=date&legend=top-left)
