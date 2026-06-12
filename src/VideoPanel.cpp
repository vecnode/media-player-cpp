/*
 * media-player-cpp — VideoPanel
 * Copyright (c) vecnode 2026
 */

#include "VideoPanel.h"
#include "PlatformVideo.h"

#include <algorithm>
#include <system_error>

namespace {

namespace fs = of::filesystem;

bool isVideoPath(const fs::path& path) {
	static const char* kExtensions[] = {".mp4", ".mov", ".avi", ".mkv", ".webm"};
	const std::string ext = ofGetExtensionLower(path);
	return std::any_of(std::begin(kExtensions), std::end(kExtensions),
		[&](const char* candidate) { return ext == candidate; });
}

bool pathsEquivalent(const fs::path& a, const fs::path& b) {
	try {
		return fs::equivalent(a, b);
	} catch (...) {
		return fs::absolute(a) == fs::absolute(b);
	}
}

fs::path normalizeDir(fs::path path) {
	if (path.empty()) {
		return path;
	}
	std::error_code ec;
	path = fs::absolute(path, ec);
	if (ec) {
		return path;
	}
	path.make_preferred();
	if (!path.string().empty()) {
		const char last = path.string().back();
		if (last != '\\' && last != '/') {
			path /= "";
		}
	}
	return path;
}

std::string formatPathForLog(const fs::path& path) {
	fs::path copy = path;
	copy.make_preferred();
	return copy.string();
}

std::vector<fs::path> buildSearchRoots() {
	const fs::path exeDir = normalizeDir(ofFilePath::getCurrentExeDirFS());
	std::vector<fs::path> roots;

	roots.push_back(exeDir / "data");
	roots.push_back(exeDir);

#ifndef NDEBUG
	roots.push_back(normalizeDir(fs::path("bin") / "data"));
	roots.push_back(normalizeDir("bin"));
#endif

	return roots;
}

std::vector<fs::path> existingUniqueRoots(const std::vector<fs::path>& candidates) {
	std::vector<fs::path> roots;

	for (auto root : candidates) {
		root = normalizeDir(root);
		std::error_code ec;
		if (root.empty() || !fs::exists(root, ec) || !fs::is_directory(root, ec)) {
			continue;
		}

		const bool alreadyListed = std::any_of(roots.begin(), roots.end(),
			[&](const fs::path& existing) { return pathsEquivalent(existing, root); });

		if (!alreadyListed) {
			roots.push_back(root);
		}
	}

	return roots;
}

} // namespace

void VideoPanel::scanDataFolder() {
	videoPaths.clear();
	searchLog.clear();

	const auto roots = existingUniqueRoots(buildSearchRoots());

	if (roots.empty()) {
		searchLog = "no folders found next to " + formatPathForLog(ofFilePath::getCurrentExeDirFS());
		ofLogError("VideoPanel") << searchLog;
		return;
	}

	for (const auto& root : roots) {
		std::size_t foundInRoot = 0;
		std::error_code ec;

		for (const auto& entry : fs::directory_iterator(root, ec)) {
			if (ec) {
				ofLogError("VideoPanel") << "directory_iterator failed for " << formatPathForLog(root)
					<< ": " << ec.message();
				break;
			}

			if (!entry.is_regular_file(ec)) {
				continue;
			}

			const fs::path filePath = entry.path();
			if (!isVideoPath(filePath)) {
				continue;
			}

			const std::string absPath = formatPathForLog(fs::absolute(filePath));
			const bool alreadyFound = std::any_of(videoPaths.begin(), videoPaths.end(),
				[&](const std::string& existing) {
					return pathsEquivalent(existing, absPath);
				});

			if (!alreadyFound) {
				videoPaths.push_back(absPath);
				++foundInRoot;
				ofLogNotice("VideoPanel") << "  found " << filePath.filename().string();
			}
		}

		searchLog += (searchLog.empty() ? "" : " | ")
			+ formatPathForLog(root) + " (" + ofToString(foundInRoot) + ")";
		ofLogNotice("VideoPanel") << "Searched " << formatPathForLog(root)
			<< " -> " << foundInRoot << " video(s)";
	}

	ofLogNotice("VideoPanel") << "Total: " << videoPaths.size() << " video(s)";
}

void VideoPanel::ensurePlayerConfigured(ofVideoPlayer& player, bool& configuredFlag) {
	if (!configuredFlag) {
		PlatformVideo::configurePlayer(player);
		configuredFlag = true;
	}
}

void VideoPanel::invalidatePreload() {
	preloadedIndex = std::numeric_limits<std::size_t>::max();
}

bool VideoPanel::loadIntoPlayer(ofVideoPlayer& target, std::size_t index) {
	if (index >= videoPaths.size()) {
		return false;
	}

	fs::path absPath = fs::path(videoPaths[index]);
	absPath.make_preferred();

	// Media Foundation load() closes the engine internally; avoid a redundant close() here.
	if (!target.load(PlatformVideo::loadPath(absPath))) {
		ofLogWarning("VideoPanel") << "Failed to load " << formatPathForLog(absPath);
		return false;
	}

	target.setLoopState(OF_LOOP_NORMAL);
	target.setVolume(1.0f);

	ofLogNotice("VideoPanel") << "Loaded " << absPath.filename().string()
		<< " (" << target.getWidth() << "x" << target.getHeight() << ")";

	return true;
}

void VideoPanel::setup() {
	ensurePlayerConfigured(activePlayer(), playersConfigured[activeSlot]);

	scanDataFolder();

	if (videoPaths.empty()) {
		ofLogError("VideoPanel") << "No playable video found. Searched: " << searchLog;
		return;
	}

	if (!loadAtIndex(0)) {
		ofLogError("VideoPanel") << "Failed to load first video";
		return;
	}

	PlatformVideo::primeFirstFrame(activePlayer());
}

bool VideoPanel::loadAtIndex(std::size_t index) {
	if (index >= videoPaths.size()) {
		return false;
	}

	ensurePlayerConfigured(activePlayer(), playersConfigured[activeSlot]);
	invalidatePreload();

	currentIndex = index;

	if (!loadIntoPlayer(activePlayer(), index)) {
		loadedPath.clear();
		return false;
	}

	fs::path absPath = fs::path(videoPaths[index]);
	loadedPath = absPath.filename().string();
	return true;
}

void VideoPanel::preloadNextClip() {
	if (videoPaths.size() < 2) {
		return;
	}

	const std::size_t nextIndex = (currentIndex + 1) % videoPaths.size();
	if (preloadedIndex == nextIndex && standbyPlayer().isLoaded()) {
		return;
	}

	ensurePlayerConfigured(standbyPlayer(), playersConfigured[1 - activeSlot]);

	if (!loadIntoPlayer(standbyPlayer(), nextIndex)) {
		invalidatePreload();
		return;
	}

	preloadedIndex = nextIndex;
	standbyPlayer().setVolume(0.0f);
	standbyPlayer().setPaused(true);
}

void VideoPanel::play() {
	if (!activePlayer().isLoaded()) {
		return;
	}

	activePlayer().setPaused(false);
	activePlayer().play();
}

void VideoPanel::stop() {
	PlatformVideo::stopPlayback(activePlayer());
}

void VideoPanel::cycleNext() {
	if (videoPaths.empty()) {
		return;
	}

	const bool wasPlaying = activePlayer().isPlaying();
	const std::size_t nextIndex = (currentIndex + 1) % videoPaths.size();

	const bool canSwap = preloadedIndex == nextIndex && standbyPlayer().isLoaded();

	if (canSwap) {
		activeSlot = 1 - activeSlot;
		currentIndex = nextIndex;
		loadedPath = fs::path(videoPaths[nextIndex]).filename().string();
		activePlayer().setVolume(1.0f);
		activePlayer().setLoopState(OF_LOOP_NORMAL);
		invalidatePreload();

		ofLogNotice("VideoPanel") << "Switched to " << loadedPath << " (prefetched)";
	} else if (!loadAtIndex(nextIndex)) {
		return;
	}

	if (wasPlaying) {
		activePlayer().setPaused(false);
		activePlayer().play();
	} else {
		PlatformVideo::primeFirstFrame(activePlayer());
	}
}

void VideoPanel::update() {
	if (activePlayer().isLoaded()) {
		activePlayer().update();
	}

	if (standbyPlayer().isLoaded()) {
		standbyPlayer().update();
	}

	preloadNextClip();
}

void VideoPanel::draw(const ofRectangle& bounds) const {
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
