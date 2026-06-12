#pragma once

#include "SubtitlesOverlay.h"
#include "VideoPanel.h"
#include "ofMain.h"
#include "ofxGui.h"

/// Application entry point for the media player.
///
/// Owns a full-window VideoPanel, optional SubtitlesOverlay, and an ofxGui panel.
class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	void windowResized(int w, int h) override;

private:
	void onPlayPressed();
	void onCyclePressed();
	void onStopPressed();
	void onSubtitlesToggled(bool& value);
	void refreshStatusLabel();
	void syncSubtitleText();

	VideoPanel videoPanel;
	SubtitlesOverlay subtitles;
	ofRectangle panelBounds;

	ofxPanel gui;
	ofxButton playButton;
	ofxButton cycleButton;
	ofxButton stopButton;
	ofxToggle subtitlesToggle;
	ofxLabel statusLabel;
};
