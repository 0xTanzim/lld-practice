#pragma once
#include<iostream>
#include<queue>
#include<stack>
#include "../models/Playlist.hpp"
#include "PlayStrategy.hpp"

using namespace std;

class CustomQueueStrategy : public PlayStrategy {
private:
    Playlist* currentPlaylist;
    int currentIndex;
    queue<Song*> nextQueue;
    stack<Song*> prevStack;

    Song* nextSequential() {
        if (currentPlaylist->getSize() == 0) {
            throw runtime_error("Playlist is empty.");
        }
        currentIndex = currentIndex + 1;
        return currentPlaylist->getSongs()[currentIndex];
    }

    Song* previousSequential() {
        if (currentPlaylist->getSize() == 0) {
            throw runtime_error("Playlist is empty.");
        }
        if (currentIndex <= 0) {
            throw runtime_error("No previous song available.");
        }
        currentIndex = currentIndex - 1;
        return currentPlaylist->getSongs()[currentIndex];
    }

public:
    CustomQueueStrategy() {
        currentPlaylist = nullptr;
        currentIndex = -1;
    }

    void setPlaylist(Playlist* playlist) override {
        currentPlaylist = playlist;
        currentIndex = -1;
        while (!nextQueue.empty()) {
            nextQueue.pop();
        }
        while(!prevStack.empty()) {
            prevStack.pop();
        }
    }

    bool hasNext() override {
        return (currentPlaylist && (currentIndex + 1) < currentPlaylist->getSize());
    }

    Song* next() override {
        if (!currentPlaylist || currentPlaylist->getSize() == 0) {
            throw runtime_error("No playlist loaded or playlist is empty.");
        }

        if (nextQueue.empty()) {
            // Queued songs jump the line; fall through to sequential order.
            return nextSequential();
        }

        Song* s = nextQueue.front();
        nextQueue.pop();
        prevStack.push(s);

        auto& list = currentPlaylist->getSongs();
        for (int i = 0; i < (int)list.size(); ++i) {
            if (list[i] == s) {
                currentIndex = i;
                break;
            }
        }
        return s;
    }

    bool hasPrevious() override {
        return currentIndex > 0;
    }

    Song* previous() override {
        if (!currentPlaylist || currentPlaylist->getSize() == 0) {
            throw runtime_error("No playlist loaded or playlist is empty.");
        }

        if (!prevStack.empty()) {
            Song* s = prevStack.top();
            prevStack.pop();

            auto& list = currentPlaylist->getSongs();
            for (int i = 0; i < (int)list.size(); ++i) {
                if (list[i] == s) {
                    currentIndex = i;
                    break;
                }
            }
            return s;
        }

        return previousSequential();
    }

    void addToNext(Song* song) override {
        if (!song) {
            throw runtime_error("Cannot enqueue null song.");
        }
        nextQueue.push(song);
    }
};