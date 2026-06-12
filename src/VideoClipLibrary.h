#pragma once

#include "ofMain.h"
#include <cstddef>
#include <string>
#include <vector>

/// Metadata for one playable file in the playlist.
struct VideoClip {
	std::string absolutePath;
	std::string displayName;
};

/// Discovers and indexes video files from disk.
///
/// Search order: {exe}/data, {exe}/, then bin/ paths in debug builds.
/// Clips are sorted by path for stable playlist order across runs.
class VideoClipLibrary {
public:
	void scan();

	bool empty() const { return clips.empty(); }
	std::size_t size() const { return clips.size(); }

	const VideoClip& clipAt(std::size_t index) const;
	const std::vector<VideoClip>& allClips() const { return clips; }
	const std::string& getSearchLog() const { return searchLog; }

	/// Returns (currentIndex + 1) % size(), or 0 when empty.
	std::size_t nextIndex(std::size_t currentIndex) const;

private:
	std::vector<VideoClip> clips;
	std::string searchLog;
};
