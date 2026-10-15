#pragma once

#include "engine/system.hpp"
#include "rendering/engine/rendering_object_trait.hpp"

namespace spk
{
	class RenderingSystem : public System,
							public RenderingObjectTrait
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

		[[nodiscard]] bool _canBuildRenderSnapshot() const override
		{
			return isActive();
		}

	public:
		using RenderingObjectTrait::updateState;
		using System::System;
		using System::updateState;
	};
}
