#pragma once

#include "structure/math/spk_vector2.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class SpacerWidget : public Widget
	{
	public:
		SpacerWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
			Widget(p_name, p_parent)
		{
		}

		~SpacerWidget() override
		{
		}
	};
}