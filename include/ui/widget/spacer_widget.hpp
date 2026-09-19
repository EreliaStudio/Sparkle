#pragma once

#include <string>

#include "ui/widget.hpp"

namespace spk
{
	class SpacerWidget : public Widget
	{
	public:
		explicit SpacerWidget(std::string name, Widget *parent = nullptr);
	};
}
