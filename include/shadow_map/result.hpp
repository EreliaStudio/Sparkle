#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>

#include "graphics/opengl/texture.hpp"
#include "light/selection.hpp"
#include "math/matrix.hpp"

namespace spk::ShadowMap
{
	// One immutable publication per view/surface. Entries are keyed by the
	// Engine's light-registration identity, not by names or component addresses.
	struct Result
	{
		struct Entry
		{
			std::uint64_t registrationOrder = 0;
			std::uint64_t admission = 0;
			std::uint64_t generation = 0;
			std::uint64_t captureOrdinal = 0;

			// The geometric light parameters used when the depth map was built.
			// Shading color and intensity instead come from the consumer snapshot.
			Light::Selection::Entry light;
			Texture::Handle depthTexture;
			std::array<Matrix4x4, 6> viewProjection{};
			std::uint8_t faceCount = 1;
			struct CascadeStatistics
			{
				std::uint64_t staticBuilds = 0;
				std::uint64_t dynamicBuilds = 0;
				std::size_t staticCasters = 0;
				std::size_t dynamicCasters = 0;
			};
			std::array<CascadeStatistics, 4> cascadeStatistics{};
			std::uint8_t cascadeCount = 0;
			float cascadeNear = 0.0f;
			std::array<Texture::Handle, 4> cascadeTextures{};
			// Optional dynamic-only depth maps share their static map projection.
			std::array<Texture::Handle, 4> cascadeDynamicTextures{};
			std::array<float, 4> cascadeSplits{};
			std::array<float, 4> cascadeDepthBias{};
			std::array<float, 4> cascadeNormalBias{};
			Matrix4x4 cascadeCameraView{};
			float nearPlane = 0.0f;
			float farPlane = 0.0f;
			float depthBias = 0.0f;
			float normalBias = 0.0f;
			std::uint32_t resolution = 0;
		};

		std::unordered_map<std::uint64_t, Entry> entries;
	};
}
