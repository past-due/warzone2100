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

#include "steppedvalueslider.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"

#include <algorithm>

WzSteppedValueSlider::WzSteppedValueSlider(W_SLDINIT const *init)
	: W_SLIDER(init)
{ }

std::shared_ptr<WzSteppedValueSlider> WzSteppedValueSlider::make(const std::vector<int32_t>& values)
{
	ASSERT_OR_RETURN(nullptr, values.size() >= 2, "Needs at least two values");
	ASSERT_OR_RETURN(nullptr, std::is_sorted(values.begin(), values.end()), "Values must be in ascending order");

	W_SLDINIT sSldInit;
	sSldInit.numStops = static_cast<UWORD>(values.size() - 1);
	sSldInit.barSize = 12;
	sSldInit.width = sSldInit.numStops + sSldInit.barSize;
	sSldInit.height = sSldInit.barSize;

	class make_shared_enabler : public WzSteppedValueSlider
	{
	public:
		make_shared_enabler(W_SLDINIT const *init) : WzSteppedValueSlider(init) { }
	};
	auto result = std::make_shared<make_shared_enabler>(&sSldInit);
	result->values = values;
	result->currentValue = values.front();

	auto weakSelf = std::weak_ptr<WzSteppedValueSlider>(result);
	result->addOnChange([weakSelf](W_SLIDER&) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No longer valid");
		strongSelf->handlePosChanged();
	});
	return result;
}

int32_t WzSteppedValueSlider::value() const
{
	return currentValue;
}

void WzSteppedValueSlider::setValue(int32_t value)
{
	currentValue = value;
	pos = posForValue(value);
}

void WzSteppedValueSlider::setMarkerValue(optional<int32_t> value)
{
	markerValue = value;
}

void WzSteppedValueSlider::setMaxAllowedValue(optional<int32_t> value)
{
	maxAllowedValue = value;
}

void WzSteppedValueSlider::setOnRelease(const OnReleaseFunc& func)
{
	onReleaseFunc = func;
}

bool WzSteppedValueSlider::isDragging() const
{
	return dragging;
}

void WzSteppedValueSlider::handlePosChanged()
{
	pos = std::min(pos, maxAllowedPos());
	currentValue = values[pos];
}

// The last stop at or below value (or the first stop)
uint32_t WzSteppedValueSlider::posForValue(int32_t value) const
{
	auto it = std::upper_bound(values.begin(), values.end(), value);
	if (it == values.begin())
	{
		return 0;
	}
	return static_cast<uint32_t>(std::distance(values.begin(), it) - 1);
}

uint32_t WzSteppedValueSlider::maxAllowedPos() const
{
	if (!maxAllowedValue.has_value())
	{
		return numStops;
	}
	return posForValue(maxAllowedValue.value());
}

float WzSteppedValueSlider::fractionalPosForValue(int32_t value) const
{
	uint32_t lowerPos = posForValue(value);
	if (lowerPos >= numStops || value <= values[lowerPos])
	{
		return static_cast<float>(lowerPos);
	}
	const int32_t lowerValue = values[lowerPos];
	const int32_t upperValue = values[lowerPos + 1];
	return static_cast<float>(lowerPos) + static_cast<float>(value - lowerValue) / static_cast<float>(upperValue - lowerValue);
}

// Matches the position mapping in W_SLIDER::updateSliderFromMousePosition
float WzSteppedValueSlider::centerXForFractionalPos(float fractionalPos) const
{
	return static_cast<float>(barSize) / 2.f + static_cast<float>(width() - barSize) * fractionalPos / static_cast<float>(std::max<uint32_t>(numStops, 1));
}

void WzSteppedValueSlider::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	W_SLIDER::clicked(psContext, key);
	dragging = isEnabled();
}

void WzSteppedValueSlider::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	W_SLIDER::released(psContext, key);
	if (!dragging)
	{
		return;
	}
	dragging = false;
	if (onReleaseFunc)
	{
		onReleaseFunc(*this);
	}
}

void WzSteppedValueSlider::display(int xOffset, int yOffset)
{
	const float x0 = static_cast<float>(xOffset + x());
	const float yMid = static_cast<float>(yOffset + y()) + static_cast<float>(height()) / 2.f;
	const bool enabled = isEnabled();
	const bool active = enabled && (dragging || isHighlighted());
	auto fade = [enabled](PIELIGHT colour) {
		if (!enabled)
		{
			colour.byte.a = colour.byte.a / 2;
		}
		return colour;
	};

	const float trackHalfHeight = 2.f;
	const float trackX0 = x0 + centerXForFractionalPos(0.f);
	const float trackX1 = x0 + centerXForFractionalPos(static_cast<float>(numStops));
	const float limitX = (maxAllowedValue.has_value()) ? x0 + centerXForFractionalPos(fractionalPosForValue(maxAllowedValue.value())) : trackX1;
	const float thumbX = x0 + centerXForFractionalPos(fractionalPosForValue(currentValue));

	pie_DrawRoundedBox(trackX0, yMid - trackHalfHeight, limitX, yMid + trackHalfHeight, fade(pal_RGBA(255, 255, 255, 50)), trackHalfHeight);
	if (limitX < trackX1)
	{
		pie_DrawRoundedBox(limitX, yMid - trackHalfHeight, trackX1, yMid + trackHalfHeight, fade(pal_RGBA(255, 255, 255, 15)), trackHalfHeight);
		pie_UniTransBoxFill(limitX, yMid - 5.f, limitX + 1.f, yMid + 5.f, fade(pal_RGBA(255, 255, 255, 110)));
	}
	if (thumbX > trackX0)
	{
		pie_DrawRoundedBox(trackX0, yMid - trackHalfHeight, thumbX, yMid + trackHalfHeight, fade(WZCOL_TEXT_MEDIUM), trackHalfHeight);
	}
	if (markerValue.has_value())
	{
		const float markerX = x0 + centerXForFractionalPos(fractionalPosForValue(markerValue.value()));
		pie_UniTransBoxFill(markerX, yMid - 4.f, markerX + 1.f, yMid + 4.f, fade(pal_RGBA(255, 255, 255, 90)));
	}

	const float thumbRadius = static_cast<float>(barSize) / 2.f;
	pie_DrawRoundedBox(thumbX - thumbRadius, yMid - thumbRadius, thumbX + thumbRadius, yMid + thumbRadius,
		fade(WZCOL_TEXT_BRIGHT), thumbRadius,
		(active) ? WZCOL_TEXT_MEDIUM : fade(pal_RGBA(0, 0, 0, 200)), (active) ? 2.f : 1.f);
}
