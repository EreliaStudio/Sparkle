#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "voxel/shape.hpp"

namespace spk::Voxel
{
	class Definition
	{
	public:
		class Builder;
		class Registry;

		using SlotMapping = std::unordered_map<std::string, std::string>;

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

	public:
		Definition();
		Definition(Shape shape, SlotMapping mapping);

		[[nodiscard]] static const Definition &air();
		[[nodiscard]] const Shape &shape() const noexcept;
		[[nodiscard]] const SlotMapping &slots() const noexcept;
		[[nodiscard]] const std::string &semanticSlot(std::string_view geometricKey) const;
		[[nodiscard]] bool operator==(const Definition &other) const noexcept;
		[[nodiscard]] bool isAir() const noexcept;
	};

	class Definition::Builder
	{
	private:
		std::shared_ptr<const Shape::Registry> _shapes;
		std::optional<Shape> _shape;
		SlotMapping _slots;

	public:
		explicit Builder(const Shape::Registry &shapes);
		Builder(const Shape::Registry &shapes, const spk::JSON::Value &json);

		void setShape(std::string_view id);
		void setSlots(SlotMapping slots);
		void addFromJSON(const spk::JSON::Value &value);
		[[nodiscard]] Definition build() const;
	};

	class Definition::Registry final : public spk::JSON::Catalog<Definition, std::uint32_t>
	{
	private:
		std::shared_ptr<const Shape::Registry> _shapes;

		[[nodiscard]] ID _parseKey(const spk::JSON::Reader &reader) const override;
		[[nodiscard]] Definition _parseElement(const spk::JSON::Reader &reader) const override;

	public:
		Registry() = default;
		explicit Registry(const Shape::Registry &shapes);

		void bind(const Shape::Registry &shapes);
		void add(ID id, Definition definition);

		[[nodiscard]] const Definition &at(const ID &id) const;
		[[nodiscard]] const Definition &operator[](const ID &id) const;
		[[nodiscard]] bool contains(const ID &id) const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
	};
}
