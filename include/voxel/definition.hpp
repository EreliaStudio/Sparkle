#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "voxel/shape.hpp"

namespace spk::Voxel
{
	class Definition
	{
	public:
		struct SlotMapping
		{
			std::string geometric;
			std::string semantic;
		};

	private:
		struct Data;
		std::shared_ptr<const Data> _data;

	public:
		Definition();
		Definition(Shape shape, std::vector<SlotMapping> mapping);

		[[nodiscard]] static const Definition &air();
		[[nodiscard]] const Shape &shape() const noexcept;
		[[nodiscard]] const std::vector<SlotMapping> &slots() const noexcept;
		[[nodiscard]] const std::string &semanticSlot(std::string_view geometric) const;
		[[nodiscard]] bool sameIdentity(const Definition &other) const noexcept;
		[[nodiscard]] bool isAir() const noexcept;
	};
}
