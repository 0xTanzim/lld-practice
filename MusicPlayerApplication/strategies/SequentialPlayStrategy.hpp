#pragma once
#include<iostream>
#include "../models/Playlist.hpp"
#include "PlayStrategy.hpp"

using namespace std;

class SequentialPlayStrategy : public PlayStrategy {
private:
    Playlist* currentPlaylist;
    int currentIndex;
public:
    SequentialPlayStrategy() {
        currentPlaylist = nullptr;
        currentIndex = -1;
    }

    void setPlaylist(Playlist* playlist) override {
        currentPlaylist = playlist;
        currentIndex = -1;
    }

    void addToNext(Song* song) override {
        (void)song;
        throw logic_error("Queuing next is not supported by sequential playback.");
    }

    bool hasNext() override {
        return (currentPlaylist && (currentIndex + 1) < currentPlaylist->getSize());
    }

    Song* next() override {
        if (!currentPlaylist || currentPlaylist->getSize() == 0) {
            throw runtime_error("No playlist loaded or playlist is empty.");
        }
        currentIndex = currentIndex + 1;
        return currentPlaylist->getSongs()[currentIndex];
    }

    bool hasPrevious() override {
        return currentIndex > 0;
    }

    Song* previous() override {
        if (!currentPlaylist || currentPlaylist->getSize() == 0) {
            throw runtime_error("No playlist loaded or playlist is empty.");
        }
        if (currentIndex <= 0) {
            throw runtime_error("No previous song available.");
        }
        currentIndex = currentIndex - 1;
        return currentPlaylist->getSongs()[currentIndex];
    }
};