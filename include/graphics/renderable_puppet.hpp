#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>

#include "rendering/render_snapshot.hpp"
#include "structure/puppet.hpp"

namespace spk::Assembly
{
	template <typename TGraphics>
	concept PuppetGraphics = requires(
		TGraphics &graphics,
		RenderSnapshot::Builder &builder,
		const Matrix4x4 &transform)
	{
		typename TGraphics::ElementType;
		requires std::derived_from<typename TGraphics::ElementType, Blueprint::Element>;
		requires std::constructible_from<TGraphics, const typename TGraphics::ElementType &>;
		{ graphics.buildRenderSnapshot(builder, transform) } -> std::same_as<void>;
	};

	template <PuppetGraphics TGraphics>
	class RenderablePuppet : public Puppet
	{
	public:
		using ElementType = typename TGraphics::ElementType;
		using ElementNode = Puppet::ElementNode;

	private:
		struct GraphicsEntry
		{
			std::unique_ptr<TGraphics> graphics;
			const Blueprint::Element *definition;
			std::uint64_t generation;
		};

		std::unordered_map<const ElementNode *, GraphicsEntry> _graphics;
		std::uint64_t _generation = 0;

		[[nodiscard]] TGraphics *_prepare(const ElementNode &node)
		{
			const auto *typed = dynamic_cast<const ElementType *>(&node.definition());
			if (typed == nullptr)
			{
				return nullptr;
			}
			auto it = _graphics.find(&node);
			if (it != _graphics.end() && it->second.definition != &node.definition())
			{
				it = _graphics.erase(it);
			}
			if (it == _graphics.end())
			{
				auto graphics = std::make_unique<TGraphics>(*typed);
				it = _graphics.emplace(&node, GraphicsEntry{std::move(graphics), &node.definition(), _generation}).first;
			}
			it->second.generation = _generation;
			return it->second.graphics.get();
		}

		template <typename TVisitor>
		void _visit(const Attachable &node, TVisitor &visitor)
		{
			if (const auto *element = dynamic_cast<const ElementNode *>(&node))
			{
				if (TGraphics *graphics = _prepare(*element))
				{
					visitor(*element, *graphics);
				}
			}
			for (const Attachable *child : node.children())
			{
				_visit(*child, visitor);
			}
		}

		void _nextGeneration()
		{
			if (_generation == std::numeric_limits<std::uint64_t>::max())
			{
				_graphics.clear();
				_generation = 0;
			}
			++_generation;
		}

		void _prune()
		{
			std::erase_if(_graphics, [this](const auto &entry) {
				return entry.second.generation != _generation;
			});
		}

	public:
		explicit RenderablePuppet(Blueprint blueprint) :
			Puppet(std::move(blueprint))
		{
		}

		explicit RenderablePuppet(const Puppet &puppet) :
			Puppet(puppet)
		{
		}

		explicit RenderablePuppet(Puppet &&puppet) :
			Puppet(std::move(puppet))
		{
		}

		RenderablePuppet(const RenderablePuppet &other) :
			Puppet(other)
		{
		}

		RenderablePuppet &operator=(const RenderablePuppet &other)
		{
			if (this != &other)
			{
				Puppet::operator=(other);
				_graphics.clear();
				_generation = 0;
			}
			return *this;
		}

		RenderablePuppet(RenderablePuppet &&other) :
			Puppet(std::move(other)),
			_graphics(std::move(other._graphics)),
			_generation(std::exchange(other._generation, 0))
		{
		}

		RenderablePuppet &operator=(RenderablePuppet &&other)
		{
			if (this != &other)
			{
				Puppet::operator=(std::move(other));
				_graphics = std::move(other._graphics);
				_generation = std::exchange(other._generation, 0);
				other._graphics.clear();
			}
			return *this;
		}

		[[nodiscard]] std::size_t graphicsCount() const noexcept
		{
			return _graphics.size();
		}

		[[nodiscard]] TGraphics *graphics(const ElementNode &node) noexcept
		{
			const auto it = _graphics.find(&node);
			return it == _graphics.end() ? nullptr : it->second.graphics.get();
		}

		[[nodiscard]] const TGraphics *graphics(const ElementNode &node) const noexcept
		{
			return const_cast<RenderablePuppet *>(this)->graphics(node);
		}

		template <typename TVisitor>
		void visitGraphics(TVisitor &&visitor)
		{
			_nextGeneration();
			_visit(*this, visitor);
			_prune();
		}

		void synchronize()
		{
			visitGraphics([](const ElementNode &, TGraphics &) {
			});
		}

		void buildRenderSnapshot(
			RenderSnapshot::Builder &builder,
			const Matrix4x4 &transform = Matrix4x4::identity())
		{
			visitGraphics([&](const ElementNode &node, TGraphics &graphics) {
				graphics.buildRenderSnapshot(builder, transform * node.globalTransform());
			});
		}
	};
}
