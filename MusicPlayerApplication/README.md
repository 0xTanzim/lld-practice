# Music Player Application

LLD exercise: a music player — song library, playlists, interchangeable playback strategies, and pluggable audio output devices.

## Patterns Used

| Pattern | Where | Idea |
|---|---|---|
| **Strategy** | `strategies/` | Playback order swappable — Sequential, Random, Custom Queue |
| **Adapter** | `device/` | Uniform `IAudioOutputDevice` over differing external device APIs (`external/`) |
| **Factory** | `factories/DeviceFactory.hpp` | Creates the right adapter per `DeviceType` |
| **Facade** | `MusicPlayerFacade.hpp` | Single orchestration point for device + strategy + engine |
| **Singleton** | managers + facade + app | One manager instance per concern |

## Structure

```
MusicPlayerApplication/
├── main.cpp                    # Composition root + demo
├── MusicPlayerApplication.hpp  # Client API (library + playlists + playback)
├── MusicPlayerFacade.hpp       # Facade — orchestrates engine/strategy/device
│
├── core/
│   └── AudioEngine.hpp         # Play/pause/resume state machine
│
├── enums/
│   ├── DeviceType.hpp
│   └── PlayStrategyType.hpp
│
├── models/
│   ├── Song.hpp
│   └── Playlist.hpp
│
├── managers/
│   ├── PlaylistManager.hpp
│   ├── DeviceManager.hpp
│   └── StrategyManager.hpp
│
├── strategies/
│   ├── PlayStrategy.hpp                # Abstract strategy
│   ├── SequentialPlayStrategy.hpp
│   ├── RandomPlayStrategy.hpp
│   └── CustomQueueStrategy.hpp         # Queue jumps the line over sequential order
│
├── device/
│   ├── IAudioOutputDevice.hpp          # Target interface
│   ├── BluetoothSpeakerAdapter.hpp
│   ├── WiredSpeakerAdapter.hpp
│   └── HeadphonesAdapter.hpp
│
├── external/                           # Simulated third-party device APIs
│   ├── BluetoothSpeakerAPI.hpp
│   ├── WiredSpeakerAPI.hpp
│   └── HeadphonesAPI.hpp
│
└── factories/
    └── DeviceFactory.hpp
```

## Build & Run

```bash
g++ -std=c++26 -Wall -o main main.cpp && ./main
```

## Flow

```
createSongInLibrary → createPlaylist → addSongToPlaylist
    → connectAudioDevice(DeviceType)
    → selectPlayStrategy(PlayStrategyType) → loadPlaylist
    → playAllTracksInPlaylist / playSingleSong / queueSongNext / playPreviousTrack
```

## Ownership Rules

- `MusicPlayerApplication` owns library `Song`s (deleted in dtor)
- `PlaylistManager` owns `Playlist`s (deleted in dtor)
- `StrategyManager` owns the three strategies
- `DeviceManager` owns the connected device (old device deleted on reconnect)
- `MusicPlayerFacade` owns `AudioEngine`
- Playlists/strategies hold `Song*`/`Playlist*` non-owning references
- `main` deletes all singletons at exit — zero leaks

## Extending

- **New playback mode**: subclass `PlayStrategy`, register in `StrategyManager` + `PlayStrategyType`
- **New output device**: subclass `IAudioOutputDevice` wrapping its external API, add to `DeviceFactory` + `DeviceType`
