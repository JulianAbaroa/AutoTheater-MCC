# AutoTheater-MCC 

AutoTheater-MCC is an automated cinematography and replay orchestration framework for Halo: The Master Chief Collection. Built upon the Librarian internal core, it utilizes a non-invasive proxy injection method to interface with the game engine, enabling automated camera management and event-driven replay playback.

The core vision behind this project was to create a practical tool for content creation. By automating camera management and playback pacing, it aims to simplify the process of capturing cinematic footage, moving away from manual theater controls to a more reliable, event-driven system.

## Workflow Overview

The tool operates in two distinct phases to ensure data accuracy and reliable orchestration:

1. **BuildTimeline Phase:** During the first pass, the user plays the desired replay. The system hooks into the engine's message and telemetry systems to capture and record GameEvents. Users may accelerate playback (e.g., 16x) to speed up this data acquisition process.

2. **ExecuteDirector Phase:** Upon returning to the theater menu and restarting the replay, the system transitions to execution mode. The Director system utilizes the previously generated timeline to automatically manage camera cuts and playback speed in real-time.

Additionally, the framework provides a Default Phase for manual control, allowing users to modify playback speed and monitor engine state independently of the automated timeline or director logic.

## Installation

1. Download the latest `wtsapi32.dll` from the [Releases](https://github.com/JulianAbaroa/AutoTheater-MCC/releases) section.
2. Navigate to your Halo: MCC installation folder (e.g., `C:\SteamLibrary\steamapps\common\Halo The Master Chief Collection\MCC\Binaries\Win64\`).
3. Place the `wtsapi32.dll` on the `Win64` directory, with the other DLLs.
4. **Important:** Launch the game with **Easy Anti-Cheat (EAC) Disabled**.

## Compatibility

AutoTheater-MCC is only compatible with Halo Reach: Custom Game Browser and Custom Games replays.

## Technical Requirements

- **Language:** C++23.

- **Compiler:** Visual Studio 2022.

- **Dependencies:**
    - [MinHook](https://github.com/TsudaKageyu/minhook).
    - [Dear ImGui](https://github.com/ocornut/imgui).

- **Platform:** Windows 10/11 (x64).

## Credits and Acknowledgments

- **[AlphaRing](https://github.com/WinterSquire/AlphaRing):** For the foundational DLL proxy architecture and export definitions.

- **[Mjolnir-Forge-Editor](https://github.com/Waffle1434/Mjolnir-Forge-Editor):** For the initial inspiration and proof-of-concept regarding Halo engine memory manipulation, which served as the primary motivation for this project.

- **MinHook:** Used for reliable function hooking.

- **ImGui:** For the bloat-free immediate mode graphical user interface library.

- **Halo Modding Community:** Contributions to the understanding of the Blam! Engine.

## Disclaimer

**This project is intended for educational and content creation purposes.** AutoTheater-MCC is designed to work exclusively with **Easy Anti-Cheat (EAC) disabled**. The developer does not condone or support the use of this tool in a manner that violates software terms of service. 

*Note: AutoTheater is in its early stages of development and may cause unexpected game crashes.*

**Legal:** Halo: The Master Chief Collection © Microsoft Corporation. AutoTheater-MCC was created under Microsoft's ["Game Content Usage Rules"](https://www.xbox.com/en-US/developers/rules) using game data and code structures from Halo: Reach, and it is not endorsed by or affiliated with Microsoft.
