#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "container/json/catalog.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace spk::Voxel
{
	class Shape
	{
	public:
		class Registry;

		struct Vertex
		{
			spk::Vector3 position{};
			spk::Vector2 uv{};
			std::optional<spk::Vector3> normal;
		};

		struct Slot
		{
			std::string name;
			float opacity = 1.0f;
		};

		enum class Shell
		{
			Outer,
			Inner
		};

		enum class Face
		{
			NegativeX,
			PositiveX,
			NegativeY,
			PositiveY,
			NegativeZ,
			PositiveZ
		};

		struct Polygon
		{
			std::vector<Vertex> vertices;
			std::string slotKey;
			spk::Vector3 geometricNormal{};
			Shell shell = Shell::Inner;
			std::optional<Face> boundary;
		};

		enum class HorizontalOrientation : std::uint16_t
		{
			XPositive = 0,
			ZPositive = 270,
			XNegative = 180,
			ZNegative = 90
		};

		enum class VerticalOrientation : std::uint8_t
		{
			YPositive,
			YNegative
		};

		struct Orientation
		{
			HorizontalOrientation horizontal = HorizontalOrientation::XPositive;
			VerticalOrientation vertical = VerticalOrientation::YPositive;
		};

		class Instance
		{
		private:
			std::vector<Polygon> _polygons;

		public:
			Instance() = default;
			explicit Instance(std::vector<Polygon> polygons);

			[[nodiscard]] const std::vector<Polygon> &polygons() const noexcept;
			[[nodiscard]] bool empty() const noexcept;
		};

		class Builder
		{
		private:
			std::vector<Slot> _slots;
			std::vector<Polygon> _polygons;

		public:
			Builder() = default;
			explicit Builder(const spk::JSON::Value &json);

			void addSlot(std::string name, float opacity = 1.0f);
			void addPolygon(std::string slotKey, std::vector<Vertex> vertices);
			void addFromJSON(const spk::JSON::Value &value);
			[[nodiscard]] Shape build();
		};

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

		explicit Shape(std::shared_ptr<const Data> data) noexcept;

	public:
		Shape();

		[[nodiscard]] const std::vector<Slot> &slots() const noexcept;
		[[nodiscard]] const Instance &instance() const;
		[[nodiscard]] const Instance &instance(Orientation orientation) const;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] bool operator==(const Shape &other) const noexcept;
		[[nodiscard]] const Slot &slot(std::string_view name) const;
	};

	class Shape::Registry
	{
	public:
		using ID = std::string;
		class Builder;

	private:
		struct Data
		{
			std::unordered_map<ID, Shape> shapes;
		};

		std::shared_ptr<const Data> _data;

		explicit Registry(std::shared_ptr<const Data> data) noexcept;

	public:
		Registry();

		[[nodiscard]] const Shape &at(const ID &id) const;
		[[nodiscard]] const Shape &operator[](const ID &id) const;
		[[nodiscard]] const Shape *tryGet(const ID &id) const noexcept;
		[[nodiscard]] bool contains(const ID &id) const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
	};

	class Shape::Registry::Builder final : public spk::JSON::Catalog<Shape, std::string>
	{
	private:
		[[nodiscard]] ID _parseKey(const spk::JSON::Reader &reader) const override;
		[[nodiscard]] Shape _parseElement(const spk::JSON::Reader &reader) const override;

	public:
		Builder() = default;
		void add(ID id, Shape shape);
		[[nodiscard]] Registry build() const;
	};
}
