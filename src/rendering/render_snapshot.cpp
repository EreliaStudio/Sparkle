#include "rendering/render_snapshot.hpp"

#include <algorithm>
#include <utility>

#include "exception.hpp"

namespace spk
{
	RenderPass &RenderSnapshot::Builder::renderPass(const RenderPass::Key &key)
	{
		if (auto *entry = findPass(key.name))
		{
			if (entry->key.order != key.order)
			{
				throw InvalidRenderPassKeyError(
					"Render pass [" + key.name +
					"] already existed with a different order (Requested [" +
					std::to_string(key.order) +
					"] vs currently saved [" +
					std::to_string(entry->key.order) + "]).");
			}

			return *entry->pass;
		}

		auto pass = std::make_unique<RenderPass>();
		RenderPass &result = *pass;

		_passes.push_back({.key = key, .pass = std::move(pass)});

		return result;
	}

	RenderSnapshot RenderSnapshot::Builder::build()
	{
		std::ranges::stable_sort(
			_passes,
			[](const PendingPass &lhs, const PendingPass &rhs) {
				return lhs.key.order < rhs.key.order;
			});

		std::vector<RenderSnapshot::Pass> passes;
		passes.reserve(_passes.size());

		for (auto &entry : _passes)
		{
			passes.push_back({.name = std::move(entry.key.name), .commands = std::move(entry.pass)});
		}

		_passes.clear();

		return RenderSnapshot(std::move(passes));
	}

	RenderSnapshot::Builder::PendingPass *
	RenderSnapshot::Builder::findPass(const std::string &name)
	{
		auto it = std::ranges::find(
			_passes,
			name,
			[](const PendingPass &entry) {
				return entry.key.name;
			});

		return it != _passes.end()
				   ? &*it
				   : nullptr;
	}

	void RenderSnapshot::execute(RenderContext &renderContext) const
	{
		for (const auto &pass : _renderPasses)
		{
			try
			{
				pass.commands->execute(renderContext);
			}
			catch (spk::Exception &exception)
			{
				exception.addContext("Exception while executing render pass [" + pass.name + "]");
				throw;
			}
			catch (...)
			{
				throw spk::Exception(
					"Exception while executing render pass [" + pass.name + "]",
					std::current_exception());
			}
		}
	}

	RenderSnapshot::RenderSnapshot(
		std::vector<Pass> passes) :
		_renderPasses(std::move(passes))
	{
	}
}
