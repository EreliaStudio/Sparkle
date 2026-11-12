#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "graphics/material.hpp"
#include "rendering/engine/rendering_component.hpp"

namespace spk
{
	class Skybox final : public RenderingComponent3D
	{
	public:
		enum class Face : std::uint8_t
		{
			PositiveX,
			NegativeX,
			PositiveY,
			NegativeY,
			PositiveZ,
			NegativeZ
		};

		static constexpr std::size_t FaceCount = 6;
		using Materials = std::array<Material, FaceCount>;

	private:
		struct Resolved
		{
			Texture::Handle albedo;
			std::array<Texture::Section, FaceCount> sections;
		};

		Materials _materials;

		[[nodiscard]] static std::size_t _index(Face face);
		[[nodiscard]] Resolved _resolve() const;

	public:
		using RenderingComponent3D::RenderingComponent3D;

		void setMaterial(Face face, Material material);
		[[nodiscard]] const Material &material(Face face) const;

		void buildViewRenderSnapshot(
			RenderSnapshot::Builder &builder,
			const EngineView::CaptureContext &context) override;
	};
}
