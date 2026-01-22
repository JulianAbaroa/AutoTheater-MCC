# AutoTheater-MCC 

AutoTheater-MCC is an automated cinematography and replay orchestration framework for Halo: The Master Chief Collection. Built upon the Librarian internal core, it utilizes a non-invasive proxy injection method to interface with the game engine, enabling automated camera management and event-driven replay playback.

The core vision behind this project was to create a practical tool for content creation. By automating camera management and playback pacing, it aims to simplify the process of capturing cinematic footage, moving away from manual theater controls to a more reliable, event-driven system.

## Workflow Overview

The tool operates in two distinct phases to ensure data accuracy and reliable orchestration:

1. **BuildTimeline Phase:** During the first pass, the user plays the desired replay. The system hooks into the engine's message and telemetry systems to capture and record GameEvents. Users may accelerate playback (e.g., 16x) to speed up this data acquisition process.

2. **ExecuteDirector Phase:** Upon returning to the theater menu and restarting the replay, the system transitions to execution mode. The Director system utilizes the previously generated timeline to automatically manage camera cuts and playback speed in real-time.

## Architecture

The project is organized into several modular layers, each responsible for a specific aspect of the engine interfacing and logic execution.

### Core

- **Scanner:** Implements a memory orchestration system using AOB (Array of Bytes) signature scanning. This ensures that the tool can dynamically locate essential engine function addresses across different game builds without relying on hardcoded offsets.

- **Systems:**

    - **Timeline:** Manages the collection and organization of game events into a structured chronological format.

    - **Theater:** Provides an interface for game engine interaction, such as modifying playback speeds and retrieving player metadata.

    - **Director:** The primary logic controller. It interprets the timeline data to execute camera transitions and pacing adjustments.

- **Threads:**

    - **MainThread:** Controls the global state machine (Start, BuildTimeline, ExecuteDirector, End).

    - **DirectorThread:** Dedicated thread for the continuous update loop of the Director logic.

    - **InputThread:** Manages user-defined hotkeys and provides the Director with the necessary methods to simulate theater inputs (e.g., player switching).

    - **LogThread:** Handles logging and telemetry monitoring during the event registration phase.

### Hooks

Hooks are categorized by their functional purpose within the engine:

- **Data:** Intercepts real-time information, including the current replay module state, player focus IDs, and dynamic UI messages used for event construction.

- **Lifecycle:** Orchestrates the installation and removal of sub-system hooks during the engine's initialization and destruction phases to ensure stability.

- **MovReader:** Analyzes the .mov replay files during the loading process to extract initial player tables, header sizes, and film metadata.

- **Telemetry:** Facilitates the discovery and tracking of the engine's internal PlayerTable and ObjectTable.

### Proxy and Utilities

- **Proxy:** Implements wtsapi32.dll export definitions to allow for automatic loading by the game executable.

- **Utils:** Contains foundational utilities for string formatting and thread-safe logging.

- **External:** Integration of the MinHook library for low-level function redirection.

## Compatibility

AutoTheater-MCC is only compatible with Halo Reach: Custom Game Browser and Custom Games replays.

## Technical Requirements

- **Language:** C++23.

- **Compiler:** Visual Studio 2022.

- **Dependencies:** [MinHook](https://github.com/TsudaKageyu/minhook).

- **Platform:** Windows 10/11 (x64).

## Credits and Acknowledgments

- **[AlphaRing](https://github.com/WinterSquire/AlphaRing):** For the foundational DLL proxy architecture and export definitions.

- **[Mjolnir-Forge-Editor](https://github.com/Waffle1434/Mjolnir-Forge-Editor):** For the initial inspiration and proof-of-concept regarding Halo engine memory manipulation, which served as the primary motivation for this project.

- **MinHook:** Used for reliable function hooking.

- **Halo Modding Community:** Contributions to the understanding of the Blam! Engine.

## Disclaimer

**This project is intended for educational and content creation purposes.** AutoTheater-MCC is designed to work exclusively with **Easy Anti-Cheat (EAC) disabled**. The developer does not condone or support the use of this tool in a manner that violates software terms of service. 

*Note: AutoTheater is in its early stages of development and may cause unexpected game crashes.*

**Legal:** Halo: The Master Chief Collection © Microsoft Corporation. AutoTheater-MCC was created under Microsoft's ["Game Content Usage Rules"](https://www.xbox.com/en-US/developers/rules) using game data and code structures from Halo: Reach, and it is not endorsed by or affiliated with Microsoft.

## Installation

1. Download the latest `wtsapi32.dll` from the [Releases](https://github.com/JulianAbaroa/AutoTheater-MCC/releases) section.
2. Navigate to your Halo: MCC installation folder (e.g., `C:\SteamLibrary\steamapps\common\Halo The Master Chief Collection\MCC\Binaries\Win64\`).
3. Place the `wtsapi32.dll` on the `Win64` directory, with the other DLLs.
4. **Important:** Launch the game with **Easy Anti-Cheat (EAC) Disabled**.
