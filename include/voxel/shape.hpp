#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "container/json/object.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace spk::Voxel
{
	class Shape
	{
	public:
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
		[[nodiscard]] const std::vector<Polygon> &polygons() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] bool operator==(const Shape &other) const noexcept;
		[[nodiscard]] const Slot &slot(std::string_view name) const;
	};
}
