#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>

#include "model3d/object_headless.hpp"
#include "rendering/render_snapshot.hpp"

namespace spk::Model3D
{
	template <typename TGraphics>
	concept ObjectGraphics = requires(
		TGraphics &graphics,
		RenderSnapshot::Builder &builder,
		const Matrix4x4 &transform) {
		typename TGraphics::ElementType;
		requires std::derived_from<typename TGraphics::ElementType, Blueprint::Element>;
		requires std::constructible_from<TGraphics, const typename TGraphics::ElementType &>;
		{ graphics.buildRenderSnapshot(builder, transform) } -> std::same_as<void>;
	};

	template <ObjectGraphics TGraphics>
	class Object : public ObjectHeadless
	{
	public:
		using ElementType = typename TGraphics::ElementType;
		using ElementNode = ObjectHeadless::ElementNode;

	private:
		struct GraphicsEntry
		{
			std::unique_ptr<TGraphics> graphics;
			const Blueprint::Element *definition;
			const ObjectHeadless *instance;
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
			if (it != _graphics.end() &&
				(it->second.definition != &node.definition() || it->second.instance != &node.instance()))
			{
				_graphics.erase(it);
				it = _graphics.end();
			}
			if (it == _graphics.end())
			{
				auto graphics = std::make_unique<TGraphics>(*typed);
				it = _graphics.emplace(&node, GraphicsEntry{std::move(graphics), &node.definition(), &node.instance(), _generation}).first;
			}
			it->second.generation = _generation;
			return it->second.graphics.get();
		}

		void _collect(
			const Attachable &node,
			RenderSnapshot::Builder *builder,
			const Matrix4x4 &transform)
		{
			if (const auto *element = dynamic_cast<const ElementNode *>(&node))
			{
				if (TGraphics *graphics = _prepare(*element); graphics != nullptr && builder != nullptr)
				{
					graphics->buildRenderSnapshot(*builder, transform * element->globalTransform());
				}
			}
			for (const Attachable *child : node.children())
			{
				_collect(*child, builder, transform);
			}
		}

		template <typename TCapture>
		void _collect(
			const Attachable &node,
			RenderSnapshot::Builder &builder,
			const Matrix4x4 &transform,
			TCapture &capture)
		{
			if (const auto *element = dynamic_cast<const ElementNode *>(&node))
			{
				if (TGraphics *graphics = _prepare(*element); graphics != nullptr)
				{
					graphics->buildRenderSnapshot(builder, transform * element->globalTransform(), capture);
				}
			}
			for (const Attachable *child : node.children())
			{
				_collect(*child, builder, transform, capture);
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

		void _refresh(RenderSnapshot::Builder *builder, const Matrix4x4 &transform)
		{
			_nextGeneration();
			_collect(*this, builder, transform);
			_prune();
		}

		void _rebindMovedOwner(const ObjectHeadless *previous) noexcept
		{
			for (auto &entry : _graphics)
			{
				if (entry.second.instance == previous)
				{
					entry.second.instance = this;
				}
			}
		}

	public:
		explicit Object(Blueprint blueprint) :
			ObjectHeadless(std::move(blueprint))
		{
		}

		explicit Object(const ObjectHeadless &puppet) :
			ObjectHeadless(puppet)
		{
		}

		explicit Object(ObjectHeadless &&puppet) :
			ObjectHeadless(std::move(puppet))
		{
		}

		Object(const Object &other) :
			ObjectHeadless(other)
		{
		}

		Object &operator=(const Object &other)
		{
			if (this != &other)
			{
				ObjectHeadless::operator=(other);
				_graphics.clear();
				_generation = 0;
			}
			return *this;
		}

		Object(Object &&other) :
			ObjectHeadless(std::move(other)),
			_graphics(std::move(other._graphics)),
			_generation(std::exchange(other._generation, 0))
		{
			_rebindMovedOwner(&other);
		}

		Object &operator=(Object &&other)
		{
			if (this != &other)
			{
				ObjectHeadless::operator=(std::move(other));
				_graphics = std::move(other._graphics);
				_generation = std::exchange(other._generation, 0);
				other._graphics.clear();
				_rebindMovedOwner(&other);
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
			return const_cast<Object *>(this)->graphics(node);
		}

		void synchronize()
		{
			_refresh(nullptr, Matrix4x4::identity());
		}

		void buildRenderSnapshot(
			RenderSnapshot::Builder &builder,
			const Matrix4x4 &transform = Matrix4x4::identity())
		{
			_refresh(&builder, transform);
		}

		template <typename TCapture>
		void buildRenderSnapshot(
			RenderSnapshot::Builder &builder,
			const Matrix4x4 &transform,
			TCapture &capture)
		{
			_nextGeneration();
			_collect(*this, builder, transform, capture);
			_prune();
		}
	};
}
