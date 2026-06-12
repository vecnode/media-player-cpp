/*
 * media-player-cpp — ofApp
 * Copyright (c) vecnode 2026
 */

#include "ofApp.h"
#include "PlatformVideo.h"

void ofApp::setup() {
	ofSetVerticalSync(false);
	ofSetFrameRate(0);
	ofBackground(0);

#ifdef TARGET_WIN32
	PlatformVideo::configureDataPathRoot();
#endif

	panelBounds.set(0, 0, ofGetWidth(), ofGetHeight());

	playButton.addListener(this, &ofApp::onPlayPressed);
	cycleButton.addListener(this, &ofApp::onCyclePressed);
	stopButton.addListener(this, &ofApp::onStopPressed);
	subtitlesToggle.addListener(this, &ofApp::onSubtitlesToggled);

	gui.setup("Controls");
	gui.add(playButton.setup("Play"));
	gui.add(cycleButton.setup("Next Video"));
	gui.add(stopButton.setup("Stop"));
	gui.add(subtitlesToggle.setup("Subtitles", false));
	gui.add(statusLabel.setup("clip", ""));

	subtitles.setup();
	syncSubtitleText();

	videoPanel.setup();
	refreshStatusLabel();
}

void ofApp::exit() {
	playButton.removeListener(this, &ofApp::onPlayPressed);
	cycleButton.removeListener(this, &ofApp::onCyclePressed);
	stopButton.removeListener(this, &ofApp::onStopPressed);
	subtitlesToggle.removeListener(this, &ofApp::onSubtitlesToggled);
}

void ofApp::onPlayPressed() {
	videoPanel.play();
	refreshStatusLabel();
}

void ofApp::onCyclePressed() {
	videoPanel.cycleNext();
	syncSubtitleText();
	refreshStatusLabel();
}

void ofApp::onStopPressed() {
	videoPanel.stop();
	refreshStatusLabel();
}

void ofApp::onSubtitlesToggled(bool& value) {
	subtitles.setEnabled(value);
}

void ofApp::syncSubtitleText() {
	if (videoPanel.isLoaded()) {
		subtitles.setText(videoPanel.getLoadedPath());
	} else {
		subtitles.setText("Sample subtitle");
	}
}

void ofApp::refreshStatusLabel() {
	if (!videoPanel.isLoaded()) {
		statusLabel = "no clip loaded";
		return;
	}

	const std::string state = videoPanel.isPlaying() ? "playing" : "stopped";
	statusLabel = ofToString(videoPanel.getCurrentIndex() + 1) + "/"
		+ ofToString(videoPanel.getClipCount()) + "  "
		+ videoPanel.getLoadedPath() + "  (" + state + ")";
}

void ofApp::update() {
	videoPanel.update();
}

void ofApp::draw() {
	ofSetColor(255);
	videoPanel.draw(panelBounds);
	subtitles.draw(panelBounds);

	if (!videoPanel.isLoaded()) {
		ofSetColor(255);
		const std::string msg = videoPanel.getClipCount() > 0
			? "Video(s) found but failed to load. Check console."
			: "No video found. Searched:";
		ofDrawBitmapStringHighlight(msg, 20, ofGetHeight() - 56);
		ofDrawBitmapStringHighlight(videoPanel.getSearchLog(), 20, ofGetHeight() - 40);
	}

	gui.draw();
}

void ofApp::windowResized(int w, int h) {
	panelBounds.set(0, 0, w, h);
}
