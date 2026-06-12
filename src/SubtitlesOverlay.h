#pragma once

#include "ofMain.h"
#include <string>

/// Renders a single subtitle line with a black backing plane.
///
/// The plane is horizontally centered and vertically anchored at 4/5 of the
/// draw bounds height (lower fifth of the frame, typical subtitle placement).
class SubtitlesOverlay {
public:
	void setup();

	void draw(const ofRectangle& bounds) const;

	void setEnabled(bool enabled);
	bool isEnabled() const { return enabled; }
	void toggle();

	void setText(const std::string& text);
	const std::string& getText() const { return text; }

private:
	bool loadFont();

	ofTrueTypeFont font;
	bool fontLoaded = false;
	bool enabled = false;
	std::string text = "Sample subtitle";

	static constexpr float kVerticalAnchor = 4.f / 5.f;
	static constexpr float kPadding = 14.f;
	static constexpr int kFontSize = 36;
};
