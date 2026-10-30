#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "voxel/shape.hpp"

namespace spk::Voxel
{
	class Definition
	{
	public:
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
}
