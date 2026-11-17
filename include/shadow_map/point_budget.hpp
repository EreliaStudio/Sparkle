#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace spk
{
	struct RenderContext;
}

namespace spk::ShadowMap
{
	class PointBudget final
	{
	public:
		struct Request
		{
			const void *system = nullptr;
			std::uint64_t viewIdentity = 0;
			std::uint64_t registrationOrder = 0;
			std::uint64_t admission = 0;
			float score = 0.0f;
			std::function<std::size_t()> limit;
			std::function<bool(RenderContext &)> needsRebuild;
		};

	private:
		std::uint64_t _identity;
		std::vector<Request> _requests;

	public:
		PointBudget();
		void add(Request request);
		void prepare(RenderContext &context) const;
		[[nodiscard]] bool consume(
			RenderContext &context,
			const void *system,
			std::uint64_t viewIdentity,
			std::uint64_t registrationOrder,
			std::uint64_t admission) const;
	};
}
