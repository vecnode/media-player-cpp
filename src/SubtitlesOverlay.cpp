/*
 * media-player-cpp — SubtitlesOverlay
 * Copyright (c) vecnode 2026
 */

#include "SubtitlesOverlay.h"

namespace {

bool tryLoadFont(ofTrueTypeFont& font, const std::string& path, int size) {
	if (!ofFile::doesFileExist(path)) {
		return false;
	}
	return font.load(path, size);
}

} // namespace

bool SubtitlesOverlay::loadFont() {
	static const char* kCandidates[] = {
		"fonts/verdana.ttf",
		"verdana.ttf",
#ifdef TARGET_WIN32
		"C:/Windows/Fonts/segoeui.ttf",
		"C:/Windows/Fonts/arial.ttf",
#endif
	};

	for (const char* path : kCandidates) {
		if (tryLoadFont(font, path, kFontSize)) {
			ofLogNotice("SubtitlesOverlay") << "Font: " << path;
			return true;
		}
	}

	if (font.load(OF_TTF_SANS, kFontSize)) {
		ofLogNotice("SubtitlesOverlay") << "Font: " << OF_TTF_SANS;
		return true;
	}

	ofLogWarning("SubtitlesOverlay") << "No TTF found; using bitmap fallback";
	return false;
}

void SubtitlesOverlay::setup() {
	fontLoaded = loadFont();
}

void SubtitlesOverlay::setEnabled(bool value) {
	enabled = value;
}

void SubtitlesOverlay::toggle() {
	enabled = !enabled;
}

void SubtitlesOverlay::setText(const std::string& value) {
	text = value;
}

void SubtitlesOverlay::draw(const ofRectangle& bounds) const {
	if (!enabled || text.empty()) {
		return;
	}

	// Anchor line at 4/5 height (center of subtitle block sits on this line).
	const float anchorY = bounds.y + bounds.height * kVerticalAnchor;

	if (fontLoaded) {
		const ofRectangle textBounds = font.getStringBoundingBox(text, 0, 0);
		const float boxW = textBounds.width + kPadding * 2.f;
		const float boxH = textBounds.height + kPadding * 2.f;
		const float boxX = bounds.x + (bounds.width - boxW) * 0.5f;
		const float boxY = anchorY - boxH * 0.5f;

		ofSetColor(0, 220);
		ofDrawRectangle(boxX, boxY, boxW, boxH);

		ofSetColor(255);
		font.drawString(text, boxX + kPadding - textBounds.x, boxY + kPadding - textBounds.y);
		return;
	}

	// Bitmap fallback when no TTF is available.
	const float lineH = 16.f;
	const float textW = static_cast<float>(text.size()) * 8.f;
	const float boxW = textW + kPadding * 2.f;
	const float boxH = lineH + kPadding * 2.f;
	const float boxX = bounds.x + (bounds.width - boxW) * 0.5f;
	const float boxY = anchorY - boxH * 0.5f;

	ofSetColor(0, 220);
	ofDrawRectangle(boxX, boxY, boxW, boxH);

	ofSetColor(255);
	ofDrawBitmapString(text, boxX + kPadding, boxY + kPadding + lineH - 4.f);
}
