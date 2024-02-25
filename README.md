# Floorb's Infra Mod
A utility mod for [INFRA](https://store.steampowered.com/app/251110/INFRA/), implemented as a Source engine plugin.

## Features
### Success Counters
Based on [INFRA Success Counters](https://www.moddb.com/mods/infra-success-counters) by fehc.

For those who want to discover everything that every map has to offer. 

A small overlay is added which shows per-map counters of successful actions: defect and corruption photos taken, repairs made, geocaches found, and water flow meters repaired.

### Functional Camera
Images taken with the in-game camera are saved to the `DCIM` folder under your INFRA installation folder (the same folder as `infra.exe`.)

## Installation
Locate your INFRA installation folder (the folder with `infra.exe`) and go one level deeper into the `infra/` folder under that. You should find `gameinfo.txt` in this folder. Extract the mod release archive to this folder,
which should leave you with a `floorb_infra_mod.dll` and a `addons/floorb_infra_mod.vdf`. Launch the game, and the overlay should display once you're in-game.

## Configuration
After the first run, a file called `floorb_infra_mod.ini` is created in your INFRA installation folder (the same folder as `infra.exe`.) You can also create it there yourself before the first run.

The format is as follows:

```
[features]
success_counters = true # whether to enable the success counters overlay
functional_camera = true # whether to enable saving camera pictures to disk
```

## Thanks To
[fehc](https://www.moddb.com/members/fehc) for their [INFRA Success Counters](https://www.moddb.com/mods/infra-success-counters) mod, which this code is based on. Their code is included in this project.

[Photon](https://github.com/hero622/photon/), from which I borrowed a few snippets for implementing a Source engine plugin.

[Dear ImGui](https://github.com/ocornut/imgui), which is included in this project for the Success Counters UI.

[MinHook](https://github.com/TsudaKageyu/minhook), which is included in this project for hooking functions in INFRA.

[simpleini](https://github.com/brofield/simpleini), which is included in this project for loading/saving the config.

## Contact
If you're trying to reverse engineer INFRA and want my notes or help, let me know - I can be located in the [Loiste Community Discord group](https://discord.com/invite/qY94Xsg).
