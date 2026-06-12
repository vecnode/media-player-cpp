#pragma once

#include "VideoClipLibrary.h"
#include "ofMain.h"
#include "ofVideoPlayer.h"

#include <cstddef>
#include <limits>

/// High-performance dual-buffer video playback engine.
///
/// Ping-pong architecture:
///   - Two ofVideoPlayer slots (active + standby).
///   - Standby silently preloads the next playlist item.
///   - skipToNext() swaps instantly when prefetch is ready; otherwise sync-loads.
///
/// Platform decode/render policy lives in PlatformVideo; this class owns transport
/// state, slot management, and prefetch scheduling.
class VideoPlaybackEngine {
public:
	/// Result of a clip change — useful for UI/logging extensions.
	enum class SwitchMethod {
		None,
		PrefetchSwap,
		SyncLoad
	};

	struct SwitchResult {
		bool success = false;
		SwitchMethod method = SwitchMethod::None;
		std::size_t index = 0;
	};

	void setup();
	void attachLibrary(const VideoClipLibrary* library);

	void update();
	void draw(const ofRectangle& bounds) const;

	bool openIndex(std::size_t index, bool primePreviewFrame = true);
	SwitchResult skipToNext();

	void play();
	void stop();

	bool isLoaded() const;
	bool isPlaying() const;
	std::size_t currentIndex() const { return currentClipIndex; }
	const VideoClip& currentClip() const;

private:
	static constexpr std::size_t kSlotCount = 2;
	static constexpr std::size_t kInvalidPreloadIndex = std::numeric_limits<std::size_t>::max();
	static constexpr int kPreloadRetryCooldownFrames = 120;

	void ensureSlotConfigured(int slotIndex);
	bool loadClipIntoSlot(int slotIndex, std::size_t clipIndex, bool logLoad);
	void preloadNextClip();
	void invalidatePreload();
	bool isPreloadReady(std::size_t expectedIndex) const;
	void silenceStandby();
	void swapToPrefetchedClip(std::size_t nextIndex);
	void applyTransportAfterSwitch(bool wasPlaying);

	int standbySlotIndex() const { return 1 - activeSlotIndex; }
	ofVideoPlayer& activePlayer() { return slots[activeSlotIndex]; }
	const ofVideoPlayer& activePlayer() const { return slots[activeSlotIndex]; }
	ofVideoPlayer& standbyPlayer() { return slots[standbySlotIndex()]; }
	const ofVideoPlayer& standbyPlayer() const { return slots[standbySlotIndex()]; }

	const VideoClipLibrary* clipLibrary = nullptr;

	ofVideoPlayer slots[kSlotCount];
	bool slotsConfigured[kSlotCount] = {false, false};
	int activeSlotIndex = 0;

	std::size_t currentClipIndex = 0;
	std::size_t preloadedIndex = kInvalidPreloadIndex;
	int preloadRetryCooldown = 0;
};
