#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <vector>

#include "exception.hpp"
#include "light/prioritizer.hpp"

namespace
{
	using Entry = spk::Light::Selection::Entry;
	Entry local(std::uint64_t ordinal, float score, spk::Light::Selection::Type type = spk::Light::Selection::Type::Point)
	{
		Entry entry;
		entry.registrationOrder = ordinal;
		entry.score = score;
		entry.type = type;
		return entry;
	}

	std::vector<std::uint64_t> ordinals(const std::vector<Entry> &entries)
	{
		std::vector<std::uint64_t> result;
		for (const auto &entry : entries)
		{
			result.push_back(entry.registrationOrder);
		}
		return result;
	}
}

TEST(CoreLightPrioritizer, InitialTopNIsGlobalAcrossPointAndSpotAndDeterministic)
{
	std::vector<Entry> candidates;
	for (std::uint64_t i = 12; i >= 1; --i)
	{
		candidates.push_back(local(i, 1.0f, i % 2 ? spk::Light::Selection::Type::Spot : spk::Light::Selection::Type::Point));
	}
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select(candidates, {}, 8, 0.1f, false)),
			  (std::vector<std::uint64_t>{1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST(CoreLightPrioritizer, StrongerCandidateBelowStrictThresholdDoesNotReplaceIncumbent)
{
	const std::vector<Entry> candidates{local(1, 10.0f), local(2, 10.9f)};
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select(candidates, {1}, 1, 0.1f, false)),
			  (std::vector<std::uint64_t>{1}));
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select({local(1, 10.0f), local(2, 11.0f)}, {1}, 1, 0.1f, false)),
			  (std::vector<std::uint64_t>{1}));
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select({local(1, 10.0f), local(2, 11.1f)}, {1}, 1, 0.1f, false)),
			  (std::vector<std::uint64_t>{2}));
}

TEST(CoreLightPrioritizer, VacanciesFillWithoutHysteresisAndDoNotResetOtherIncumbents)
{
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select({local(2, 10), local(3, 9), local(4, 8)}, {1, 2}, 2, 0.5f, false)),
			  (std::vector<std::uint64_t>{2, 3}));
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select({local(1, 10), local(2, 10.1f), local(3, 11.0f)}, {1}, 2, 0.5f, false)),
			  (std::vector<std::uint64_t>{3, 1}));
}

TEST(CoreLightPrioritizer, ReconfigurationRebuildsRawTopN)
{
	const std::vector<Entry> candidates{local(1, 10), local(2, 11)};
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select(candidates, {1}, 1, 0.5f, false)),
			  (std::vector<std::uint64_t>{1}));
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select(candidates, {1}, 1, 0.5f, true)),
			  (std::vector<std::uint64_t>{2}));
}

TEST(CoreLightPrioritizer, RecomputesWeakestAfterEveryReplacement)
{
	const std::vector<Entry> candidates{local(1, 5), local(2, 10), local(3, 8), local(4, 7)};
	EXPECT_EQ(ordinals(spk::Light::Prioritizer::select(candidates, {1, 2}, 2, 0.1f, false)),
			  (std::vector<std::uint64_t>{2, 3}));
}

TEST(CoreLightPrioritizer, ZeroLimitAndDirectionalNotConsideredLocal)
{
	EXPECT_TRUE(spk::Light::Prioritizer::select({local(1, 100), local(2, 100, spk::Light::Selection::Type::Directional)}, {}, 0, 0.1f, false).empty());
}

TEST(CoreLightPrioritizer, BadScoresAndHysteresisAreRejected)
{
	EXPECT_THROW(spk::Light::Prioritizer::select({local(1, -1)}, {}, 1, 0.1f, false), spk::Exception);
	EXPECT_THROW(spk::Light::Prioritizer::select({local(1, 1)}, {}, 1, std::numeric_limits<float>::quiet_NaN(), false), spk::Exception);
}
