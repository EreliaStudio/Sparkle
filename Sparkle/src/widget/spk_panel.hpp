#pragma once

#include "widget/spk_widget.hpp"

namespace spk
{
	class Panel : public spk::Widget
	{
	private:
		static inline Panel* _activePanel = nullptr;

	public:
		Panel(spk::Widget* p_parent = nullptr);
	};
}