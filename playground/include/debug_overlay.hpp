#pragma once

#include <sparkle.hpp>

namespace taag
{
	class DebugOverlay : public spk::Widget
	{
	private:
		spk::DebugOverlay _debugOverlay;

		void _updateOutlineFromLayout();
		void _onGeometryChange() override;
		void _onUpdateEvent(spk::UpdateEvent &p_event) override;

	public:
		DebugOverlay(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);
	};
}