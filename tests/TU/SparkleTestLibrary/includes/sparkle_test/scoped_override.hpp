#pragma once

#include <utility>

namespace sparkle_test
{
	template <typename TValue>
	class ScopedOverride
	{
	private:
		TValue *_target;
		TValue _previous;

	public:
		ScopedOverride(TValue &target, TValue replacement) :
			_target(&target),
			_previous(std::move(target))
		{
			target = std::move(replacement);
		}

		~ScopedOverride()
		{
			*_target = std::move(_previous);
		}

		ScopedOverride(const ScopedOverride &) = delete;
		ScopedOverride &operator=(const ScopedOverride &) = delete;
	};

	template <typename TValue, typename TReplacement>
	[[nodiscard]] ScopedOverride<TValue> scopedOverride(TValue &target, TReplacement &&replacement)
	{
		return ScopedOverride<TValue>(target, TValue(std::forward<TReplacement>(replacement)));
	}
}
