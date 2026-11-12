#include "rendering/engine/skybox.hpp"

#include <cmath>
#include <string>
#include <utility>

#include "exception.hpp"
#include "rendering/command/draw_skybox_command.hpp"

namespace spk
{
	namespace
	{
		[[nodiscard]] bool validSection(const Texture::Section &section)
		{
			const auto normalized = [](float value) {
				return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
			};
			return normalized(section.anchor.x) && normalized(section.anchor.y) &&
				   normalized(section.size.x) && normalized(section.size.y) &&
				   section.size.x > 0.0f && section.size.y > 0.0f &&
				   section.anchor.x + section.size.x <= 1.0f &&
				   section.anchor.y + section.size.y <= 1.0f;
		}
	}

	std::size_t Skybox::_index(Face face)
	{
		const auto result = static_cast<std::size_t>(face);
		if (result >= FaceCount)
		{
			throw spk::Exception("Skybox: invalid face");
		}
		return result;
	}

	void Skybox::setMaterial(Face face, Material material)
	{
		_materials[_index(face)] = std::move(material);
	}

	const Material &Skybox::material(Face face) const
	{
		return _materials[_index(face)];
	}

	Skybox::Resolved Skybox::_resolve() const
	{
		Resolved resolved{};
		for (std::size_t i = 0; i < FaceCount; ++i)
		{
			const auto *binding = _materials[i].find("albedo");
			if (binding == nullptr || !binding->texture)
			{
				throw spk::Exception("Skybox: face " + std::to_string(i) + " has no valid albedo");
			}
			if (!validSection(binding->section))
			{
				throw spk::Exception("Skybox: face " + std::to_string(i) + " has invalid UV coordinates");
			}
			if (resolved.albedo && (resolved.albedo.identifier() != binding->texture.identifier() ||
									&resolved.albedo.state() != &binding->texture.state()))
			{
				throw spk::Exception("Skybox: face " + std::to_string(i) + " uses a different albedo source");
			}
			resolved.albedo = binding->texture;
			resolved.sections[i] = binding->section;
		}
		return resolved;
	}

	void Skybox::buildViewRenderSnapshot(
		RenderSnapshot::Builder &builder,
		const EngineView::CaptureContext &context)
	{
		const auto captured = _resolve();
		builder.renderPass(EngineView::SkyboxPassKey).emplace<DrawSkyboxCommand>(captured.albedo, captured.sections, context.projectionMatrix, context.viewMatrix);
	}
}
