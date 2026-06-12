/*
 * media-player-cpp — VideoPlaybackEngine
 * Copyright (c) vecnode 2026
 *
 * Dual-player prefetch keeps the next clip decoded in the standby slot so
 * skipToNext() can swap instantly instead of blocking on Media Foundation load().
 */

#include "VideoPlaybackEngine.h"
#include "PlatformVideo.h"

void VideoPlaybackEngine::setup() {
	ensureSlotConfigured(activeSlotIndex);
}

void VideoPlaybackEngine::attachLibrary(const VideoClipLibrary* library) {
	clipLibrary = library;
}

void VideoPlaybackEngine::ensureSlotConfigured(int slotIndex) {
	if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= kSlotCount) {
		return;
	}
	if (!slotsConfigured[slotIndex]) {
		PlatformVideo::configurePlayer(slots[slotIndex]);
		slotsConfigured[slotIndex] = true;
	}
}

void VideoPlaybackEngine::invalidatePreload() {
	preloadedIndex = kInvalidPreloadIndex;
}

bool VideoPlaybackEngine::isPreloadReady(std::size_t expectedIndex) const {
	if (preloadedIndex != expectedIndex) {
		return false;
	}

	const auto& standby = standbyPlayer();
	return standby.isLoaded()
		&& standby.getWidth() > 0
		&& standby.getHeight() > 0;
}

void VideoPlaybackEngine::silenceStandby() {
	if (!standbyPlayer().isLoaded()) {
		return;
	}

	standbyPlayer().setVolume(0.0f);
	standbyPlayer().setPaused(true);
}

bool VideoPlaybackEngine::loadClipIntoSlot(int slotIndex, std::size_t clipIndex, bool logLoad) {
	if (!clipLibrary || clipIndex >= clipLibrary->size()) {
		return false;
	}

	ensureSlotConfigured(slotIndex);

	const VideoClip& clip = clipLibrary->clipAt(clipIndex);
	of::filesystem::path absPath = clip.absolutePath;
	absPath.make_preferred();

	if (!slots[slotIndex].load(PlatformVideo::loadPath(absPath))) {
		ofLogWarning("VideoPlaybackEngine") << "Failed to load " << clip.absolutePath;
		return false;
	}

	slots[slotIndex].setLoopState(OF_LOOP_NORMAL);
	slots[slotIndex].setVolume(1.0f);

	if (logLoad) {
		ofLogNotice("VideoPlaybackEngine") << "Loaded " << clip.displayName
			<< " (" << slots[slotIndex].getWidth() << "x" << slots[slotIndex].getHeight() << ")";
	}

	return true;
}

bool VideoPlaybackEngine::openIndex(std::size_t index, bool primePreviewFrame) {
	if (!clipLibrary || index >= clipLibrary->size()) {
		return false;
	}

	ensureSlotConfigured(activeSlotIndex);
	invalidatePreload();

	if (!loadClipIntoSlot(activeSlotIndex, index, true)) {
		return false;
	}

	currentClipIndex = index;

	if (primePreviewFrame) {
		PlatformVideo::primeFirstFrame(activePlayer());
	}

	return true;
}

void VideoPlaybackEngine::preloadNextClip() {
	if (!clipLibrary || clipLibrary->size() < 2 || !activePlayer().isLoaded()) {
		return;
	}

	if (preloadRetryCooldown > 0) {
		--preloadRetryCooldown;
		return;
	}

	const std::size_t nextIndex = clipLibrary->nextIndex(currentClipIndex);
	if (isPreloadReady(nextIndex)) {
		return;
	}

	if (!loadClipIntoSlot(standbySlotIndex(), nextIndex, false)) {
		invalidatePreload();
		preloadRetryCooldown = kPreloadRetryCooldownFrames;
		return;
	}

	preloadedIndex = nextIndex;
	silenceStandby();

	ofLogVerbose("VideoPlaybackEngine") << "Prefetched " << clipLibrary->clipAt(nextIndex).displayName;
}

void VideoPlaybackEngine::swapToPrefetchedClip(std::size_t nextIndex) {
	activeSlotIndex = standbySlotIndex();
	currentClipIndex = nextIndex;

	activePlayer().setVolume(1.0f);
	activePlayer().setLoopState(OF_LOOP_NORMAL);
	invalidatePreload();

	ofLogNotice("VideoPlaybackEngine") << "Switched to "
		<< clipLibrary->clipAt(nextIndex).displayName << " (prefetched)";
}

void VideoPlaybackEngine::applyTransportAfterSwitch(bool wasPlaying) {
	if (wasPlaying) {
		activePlayer().setPaused(false);
		activePlayer().play();
	} else {
		PlatformVideo::primeFirstFrame(activePlayer());
	}
}

VideoPlaybackEngine::SwitchResult VideoPlaybackEngine::skipToNext() {
	SwitchResult result;

	if (!clipLibrary || clipLibrary->empty()) {
		return result;
	}

	const bool wasPlaying = activePlayer().isPlaying();
	const std::size_t nextIndex = clipLibrary->nextIndex(currentClipIndex);

	if (isPreloadReady(nextIndex)) {
		swapToPrefetchedClip(nextIndex);
		result.method = SwitchMethod::PrefetchSwap;
		result.success = true;
	} else if (!openIndex(nextIndex, false)) {
		return result;
	} else {
		result.method = SwitchMethod::SyncLoad;
		result.success = true;
	}

	result.index = nextIndex;
	applyTransportAfterSwitch(wasPlaying);
	return result;
}

void VideoPlaybackEngine::play() {
	if (!activePlayer().isLoaded()) {
		return;
	}

	activePlayer().setPaused(false);
	activePlayer().play();
}

void VideoPlaybackEngine::stop() {
	PlatformVideo::stopPlayback(activePlayer());
}

bool VideoPlaybackEngine::isLoaded() const {
	return activePlayer().isLoaded();
}

bool VideoPlaybackEngine::isPlaying() const {
	return activePlayer().isPlaying();
}

const VideoClip& VideoPlaybackEngine::currentClip() const {
	static const VideoClip kEmpty;
	if (!clipLibrary) {
		return kEmpty;
	}
	return clipLibrary->clipAt(currentClipIndex);
}

void VideoPlaybackEngine::update() {
	if (activePlayer().isLoaded()) {
		activePlayer().update();
	}

	if (standbyPlayer().isLoaded()) {
		standbyPlayer().update();
	}

	preloadNextClip();
}

void VideoPlaybackEngine::draw(const ofRectangle& bounds) const {
	if (!activePlayer().isLoaded()) {
		return;
	}

	const ofTexture& tex = activePlayer().getTexture();
	if (tex.isAllocated()) {
		ofSetColor(255);
		tex.draw(bounds);
		return;
	}

	activePlayer().draw(bounds.x, bounds.y, bounds.width, bounds.height);
}
