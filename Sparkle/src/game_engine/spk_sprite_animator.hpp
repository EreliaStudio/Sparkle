#pragma once

#include <chrono>
#include <vector>
#include <map>
#include "design_pattern/spk_notifier.hpp"
#include "math/spk_vector2.hpp"

namespace spk
{
	struct SpriteAnimation
	{
		int duration;
		spk::Vector2Int firstFrame;
		spk::Vector2Int frameOffset;
		int lenght;
	};

	class SpriteAnimator
	{
	public:
		using Callback = spk::Notifier::Callback;
		using Contract = spk::Notifier::Contract;

	private:
		spk::Notifier _animationChangementNotifier;

	public:
		SpriteAnimator()
		{

		}

		void activate(const std::string& p_animationName)
		{

		}

		std::unique_ptr<Contract> subscribe(const Callback& p_callback)
		{
			return (_animationChangementNotifier.subscribe(p_callback));
		}
	};
}