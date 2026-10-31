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

		struct Slot
		{
			std::string name;
			float opacity = 1.0f;
		};

		using SlotMapping = std::unordered_map<std::string, Slot>;

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

	public:
		Definition();
		Definition(Shape shape, SlotMapping mapping);

		[[nodiscard]] static const Definition &air();
		[[nodiscard]] const Shape &shape() const noexcept;
		[[nodiscard]] const SlotMapping &slots() const noexcept;
		[[nodiscard]] const Slot &slot(std::string_view geometricKey) const;
		[[nodiscard]] float opacity(std::string_view geometricKey) const;
		[[nodiscard]] const std::string &semanticSlot(std::string_view geometricKey) const;
		[[nodiscard]] bool operator==(const Definition &other) const noexcept;
		[[nodiscard]] bool isAir() const noexcept;
	};

	class Definition::Builder
	{
	private:
		Shape::Registry _shapes;
		std::optional<Shape> _shape;
		SlotMapping _slots;

	public:
		Builder() = default;
		void bind(const Shape::Registry &shapes);

		void setShape(std::string_view id);
		void setSlots(SlotMapping slots);
		void addFromJSON(const spk::JSON::Value &value);
		[[nodiscard]] Definition build() const;
	};

	class Definition::Registry
	{
	public:
		using ID = std::uint32_t;
		class Builder;

	private:
		struct Data
		{
			std::unordered_map<ID, Definition> definitions;
		};

		std::shared_ptr<const Data> _data;

		explicit Registry(std::shared_ptr<const Data> data) noexcept;

	public:
		Registry();

		[[nodiscard]] const Definition &at(ID id) const;
		[[nodiscard]] const Definition &operator[](ID id) const;
		[[nodiscard]] const Definition *tryGet(ID id) const noexcept;
		[[nodiscard]] bool contains(ID id) const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
	};

	class Definition::Registry::Builder final : public spk::JSON::Catalog<Definition, std::uint32_t>
	{
	private:
		Shape::Registry _shapes;

		[[nodiscard]] ID _parseKey(const spk::JSON::Reader &reader) const override;
		[[nodiscard]] Definition _parseElement(const spk::JSON::Reader &reader) const override;

	public:
		Builder() = default;
		void bind(const Shape::Registry &shapes);
		void add(ID id, Definition definition);
		[[nodiscard]] Registry build() const;
	};
}
