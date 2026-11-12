#include "light/prioritizer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <unordered_set>

#include "exception.hpp"

namespace spk::Light
{
	std::vector<Selection::Entry> Prioritizer::select(
		std::vector<Selection::Entry> candidates,
		const std::vector<std::uint64_t> &previouslyRetained,
		std::size_t limit,
		float hysteresis,
		bool resetHistory)
	{
		if (!std::isfinite(hysteresis) || hysteresis < 0.0f)
		{
			throw spk::Exception("Light::Prioritizer hysteresis must be finite and nonnegative");
		}

		for (const auto &candidate : candidates)
		{
			if (!std::isfinite(candidate.score) || candidate.score < 0.0f)
			{
				throw spk::Exception("Light::Prioritizer requires finite, nonnegative component scores");
			}
		}
		std::erase_if(candidates, [](const Selection::Entry &entry) {
			return entry.type == Selection::Type::Directional || entry.score == 0.0f;
		});

		const auto stronger = [](const Selection::Entry &a, const Selection::Entry &b) {
			if (a.score != b.score)
			{
				return a.score > b.score;
			}
			return a.registrationOrder < b.registrationOrder;
		};
		std::ranges::sort(candidates, stronger);
		for (std::size_t index = 1; index < candidates.size(); ++index)
		{
			if (candidates[index - 1].registrationOrder == candidates[index].registrationOrder)
			{
				throw spk::Exception("Light::Prioritizer received duplicate registration ordinals");
			}
		}

		std::vector<Selection::Entry> retained;
		retained.reserve(std::min(limit, candidates.size()));
		std::unordered_set<std::uint64_t> occupied;
		if (!resetHistory)
		{
			for (std::uint64_t ordinal : previouslyRetained)
			{
				const auto it = std::ranges::find_if(candidates, [ordinal](const Selection::Entry &candidate) {
					return candidate.registrationOrder == ordinal;
				});
				if (it != candidates.end() && occupied.insert(ordinal).second && retained.size() < limit)
				{
					retained.push_back(*it);
				}
			}
		}

		for (const Selection::Entry &candidate : candidates)
		{
			if (occupied.contains(candidate.registrationOrder))
			{
				continue;
			}
			if (retained.size() < limit)
			{
				retained.push_back(candidate);
				occupied.insert(candidate.registrationOrder);
				continue;
			}
			if (retained.empty())
			{
				continue;
			}

			// The weakest is the lowest score, breaking ties against the newest light.
			const auto weakest = std::ranges::min_element(retained, stronger);
			if (static_cast<double>(candidate.score) >
				static_cast<double>(weakest->score) * (1.0 + static_cast<double>(hysteresis)))
			{
				occupied.erase(weakest->registrationOrder);
				*weakest = candidate;
				occupied.insert(candidate.registrationOrder);
			}
		}
		std::ranges::sort(retained, stronger);
		return retained;
	}
}
