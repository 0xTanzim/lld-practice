#include "MusicPlayerApplication.hpp"

using namespace std;

int main() {
    try {
        auto application = MusicPlayerApplication::getInstance();

        // Populate library — Bollywood
        application->createSongInLibrary("Kesariya",  "Arijit Singh",  "/music/kesariya.mp3");
        application->createSongInLibrary("Chaiyya Chaiyya",   "Sukhwinder Singh",  "/music/chaiyya_chaiyya.mp3");
        application->createSongInLibrary("Tum Hi Ho",  "Arijit Singh",  "/music/tum_hi_ho.mp3");
        application->createSongInLibrary("Jai Ho",  "A. R. Rahman",  "/music/jai_ho.mp3");
        application->createSongInLibrary("Zinda",  "Siddharth Mahadevan",  "/music/zinda.mp3");
        application->createSongInLibrary("Agar Tum Saath Ho",  "Arijit Singh",  "/music/agar_tum_saath_ho.mp3");
        application->createSongInLibrary("Raataan Lambiyan",  "Arijit Singh",  "/music/raataan_lambiyan.mp3");
        application->createSongInLibrary("Bolna",  "Arijit Singh",  "/music/bolna.mp3");
        application->createSongInLibrary("Controls",  "Armaan Malik",  "/music/controls.mp3");
        application->createSongInLibrary("Ghar",  "Armaan Malik",  "/music/ghar.mp3");
        application->createSongInLibrary("Sab Tera",  "Armaan Malik",  "/music/sab_tera.mp3");
        application->createSongInLibrary("Zara Zara",  "Armaan Malik",  "/music/zara_zara.mp3");

        // Populate library — International
        application->createSongInLibrary("Faded",  "Alan Walker",  "/music/faded.mp3");
        application->createSongInLibrary("On My Way",  "Alan Walker",  "/music/on_my_way.mp3");
        application->createSongInLibrary("Alone",  "Alan Walker",  "/music/alone.mp3");
        application->createSongInLibrary("Sing Me to Sleep",  "Alan Walker",  "/music/sing_me_to_sleep.mp3");
        application->createSongInLibrary("Shape of You",  "Ed Sheeran",  "/music/shape_of_you.mp3");
        application->createSongInLibrary("Blinding Lights",  "The Weeknd",  "/music/blinding_lights.mp3");
        application->createSongInLibrary("Believer",  "Imagine Dragons",  "/music/believer.mp3");
        application->createSongInLibrary("Despacito",  "Luis Fonsi",  "/music/despacito.mp3");

        // Create playlists and add songs
        application->createPlaylist("Bollywood Vibes");
        application->addSongToPlaylist("Bollywood Vibes", "Kesariya");
        application->addSongToPlaylist("Bollywood Vibes", "Chaiyya Chaiyya");
        application->addSongToPlaylist("Bollywood Vibes", "Tum Hi Ho");
        application->addSongToPlaylist("Bollywood Vibes", "Jai Ho");
        application->addSongToPlaylist("Bollywood Vibes", "Agar Tum Saath Ho");
        application->addSongToPlaylist("Bollywood Vibes", "Raataan Lambiyan");
        application->addSongToPlaylist("Bollywood Vibes", "Controls");
        application->addSongToPlaylist("Bollywood Vibes", "Ghar");

        application->createPlaylist("Global Hits");
        application->addSongToPlaylist("Global Hits", "Faded");
        application->addSongToPlaylist("Global Hits", "On My Way");
        application->addSongToPlaylist("Global Hits", "Alone");
        application->addSongToPlaylist("Global Hits", "Shape of You");
        application->addSongToPlaylist("Global Hits", "Blinding Lights");
        application->addSongToPlaylist("Global Hits", "Believer");
        application->addSongToPlaylist("Global Hits", "Despacito");

        // Connect device
        application->connectAudioDevice(DeviceType::BLUETOOTH);

        //Play/pause a single song
        application->playSingleSong("Zinda");
        application->pauseCurrentSong("Zinda");
        application->playSingleSong("Zinda");  // resume

        cout << "\n-- Sequential Playback --\n";
        application->selectPlayStrategy(PlayStrategyType::SEQUENTIAL);
        application->loadPlaylist("Bollywood Vibes");
        application->playAllTracksInPlaylist();

        cout << "\n-- Random Playback --\n";
        application->selectPlayStrategy(PlayStrategyType::RANDOM);
        application->loadPlaylist("Bollywood Vibes");
        application->playAllTracksInPlaylist();

        cout << "\n-- Custom Queue Playback --\n";
        application->selectPlayStrategy(PlayStrategyType::CUSTOM_QUEUE);
        application->loadPlaylist("Bollywood Vibes");
        application->queueSongNext("Kesariya");
        application->queueSongNext("Tum Hi Ho");
        application->playAllTracksInPlaylist();

        cout << "\n-- Global Hits (Sequential) --\n";
        application->selectPlayStrategy(PlayStrategyType::SEQUENTIAL);
        application->loadPlaylist("Global Hits");
        application->playAllTracksInPlaylist();

        cout << "\n-- Play Previous in Sequential --\n";
        application->selectPlayStrategy(PlayStrategyType::SEQUENTIAL);
        application->loadPlaylist("Bollywood Vibes");
        application->playAllTracksInPlaylist();

        application->playPreviousTrackInPlaylist();
        application->playPreviousTrackInPlaylist();

    } catch (const exception& error) {
        cerr << "Error: " << error.what() << endl;
    }
    return 0;
}