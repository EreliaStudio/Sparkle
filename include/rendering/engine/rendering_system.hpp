#pragma once

#include "engine/system.hpp"
#include "rendering/engine/engine_view.hpp"

namespace spk
{
	class RenderingSystem : public System,
							public EngineView::Contributor
	{
	protected:
		[[nodiscard]] bool _isAcceptingEvent() const override
		{
			return isActive();
		}

		[[nodiscard]] bool _canUpdateByDevice() const override
		{
			return isActive();
		}

		[[nodiscard]] bool _canContribute() const override
		{
			return isActive();
		}

	public:
		using EngineView::Contributor::updateState;
		using System::System;
		using System::updateState;
	};
}
