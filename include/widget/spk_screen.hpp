#pragma once

#include "widget/spk_widget.hpp"

namespace spk
{
	class Screen : public spk::Widget
	{
	private:
		static inline Screen *_activeMenu = nullptr;

	public:
		Screen(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);
	};
}