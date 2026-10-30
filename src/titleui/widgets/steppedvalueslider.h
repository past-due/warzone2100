// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** \file
 *  A slider that steps through a list of values, evenly spaced along the track
 */

#pragma once

#include "lib/widget/slider.h"

#include <functional>
#include <vector>

class WzSteppedValueSlider : public W_SLIDER
{
protected:
	WzSteppedValueSlider(W_SLDINIT const *init);
public:
	typedef std::function<void (WzSteppedValueSlider&)> OnReleaseFunc;

	// values must be in ascending order
	static std::shared_ptr<WzSteppedValueSlider> make(const std::vector<int32_t>& values);

	void display(int xOffset, int yOffset) override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;

	int32_t value() const;
	void setValue(int32_t value); // a value between two stops is drawn between them
	void setMarkerValue(optional<int32_t> value);
	void setMaxAllowedValue(optional<int32_t> value);
	void setOnRelease(const OnReleaseFunc& func);
	bool isDragging() const;

private:
	void handlePosChanged();
	uint32_t posForValue(int32_t value) const;
	uint32_t maxAllowedPos() const;
	float fractionalPosForValue(int32_t value) const;
	float centerXForFractionalPos(float fractionalPos) const;

private:
	std::vector<int32_t> values;
	int32_t currentValue = 0;
	optional<int32_t> markerValue;
	optional<int32_t> maxAllowedValue;
	bool dragging = false;
	OnReleaseFunc onReleaseFunc;
};
