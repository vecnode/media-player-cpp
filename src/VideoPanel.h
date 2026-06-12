#pragma once

#include "ofMain.h"
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

/// Full-screen video panel with dual-player prefetch for fast clip changes.
///
/// Discovers clips next to the executable ({exe}/data, then {exe}/),
/// loads the first match on setup, and renders via the GPU texture path.
class VideoPanel {
public:
	void setup();

	void update();
	void draw(const ofRectangle& bounds) const;

	void play();
	void stop();
	void cycleNext();

	bool isLoaded() const { return activePlayer().isLoaded(); }
	bool isPlaying() const { return activePlayer().isPlaying(); }
	const std::string& getLoadedPath() const { return loadedPath; }
	const std::string& getSearchLog() const { return searchLog; }
	std::size_t getClipCount() const { return videoPaths.size(); }
	std::size_t getCurrentIndex() const { return currentIndex; }

private:
	void scanDataFolder();
	void ensurePlayerConfigured(ofVideoPlayer& player, bool& configuredFlag);
	bool loadIntoPlayer(ofVideoPlayer& player, std::size_t index);
	bool loadAtIndex(std::size_t index);
	void preloadNextClip();
	void invalidatePreload();

	ofVideoPlayer& activePlayer() { return players[activeSlot]; }
	const ofVideoPlayer& activePlayer() const { return players[activeSlot]; }
	ofVideoPlayer& standbyPlayer() { return players[1 - activeSlot]; }
	const ofVideoPlayer& standbyPlayer() const { return players[1 - activeSlot]; }

	ofVideoPlayer players[2];
	int activeSlot = 0;
	bool playersConfigured[2] = {false, false};

	std::vector<std::string> videoPaths;
	std::size_t currentIndex = 0;
	std::size_t preloadedIndex = std::numeric_limits<std::size_t>::max();
	std::string loadedPath;
	std::string searchLog;
};
