#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "graphics/palette.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"
#include "rendering/render_snapshot.hpp"
#include "voxel/geometry.hpp"

namespace spk::Voxel
{
	class RenderBatch final
	{
	private:
		struct Key
		{
			const std::vector<Definition::Slot> *slots;
			const spk::Palette::Materials *materials;

			[[nodiscard]] bool operator==(const Key &) const = default;
		};

		struct KeyHash
		{
			[[nodiscard]] std::size_t operator()(const Key &key) const noexcept;
		};

		struct Bucket
		{
			Geometry geometry;
			spk::Palette palette;
			std::vector<std::uint32_t> materialRemap;
			std::vector<spk::Matrix4x4> models;
		};

		std::vector<Bucket> _buckets;
		std::unordered_map<Key, std::size_t, KeyHash> _indices;

	public:
		static void validateScale(const spk::Vector3 &scale);

		void append(
			Geometry geometry,
			spk::Palette palette,
			std::vector<std::uint32_t> materialRemap,
			const spk::Matrix4x4 &model);
		void publish(spk::RenderSnapshot::Builder &builder, const spk::Matrix4x4 &viewProjection);
		[[nodiscard]] std::size_t bucketCount() const noexcept;
	};
}
