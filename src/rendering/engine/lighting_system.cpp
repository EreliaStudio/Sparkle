#include "lighting/system.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <string>

#include "core/context/render_context.hpp"
#include "exception.hpp"
#include "light/channel.hpp"

namespace spk::Lighting
{
	System::System(
		const Light::Selection &selection,
		RGB ambient,
		std::shared_ptr<Light::Channel> shadowChannel,
		std::uint64_t captureOrdinal) :
		_lights(BindingPoint, sizeof(Header), sizeof(LightData)),
		_selection(selection),
		_shadowChannel(std::move(shadowChannel)),
		_captureOrdinal(captureOrdinal == 0 ? selection.captureOrdinal : captureOrdinal)
	{
		const std::size_t count = selection.local.size() + selection.directional.size();
		if (count < selection.local.size() || count > std::numeric_limits<std::uint32_t>::max())
		{
			throw spk::Exception("Forward lighting: too many selected lights for the GPU");
		}

		std::vector<LightData> data;
		data.reserve(count);
		const auto append = [&data](const Light::Selection::Entry &source) {
			LightData light;
			light.position = source.position;
			light.range = source.range;
			light.direction = source.direction;
			light.coneCosine = source.type == Light::Selection::Type::Spot ? std::cos(source.angle) : 1.0f;
			light.color = source.color;
			light.intensity = source.intensity;
			switch (source.type)
			{
			case Light::Selection::Type::Point:
				light.type = 0;
				break;
			case Light::Selection::Type::Spot:
				light.type = 1;
				break;
			case Light::Selection::Type::Directional:
				light.type = 2;
				break;
			}
			data.push_back(light);
		};
		for (const auto &light : selection.local)
		{
			append(light);
		}
		for (const auto &light : selection.directional)
		{
			append(light);
		}
		Header header;
		header.ambient = ambient;
		header.count = static_cast<std::uint32_t>(data.size());
		header.localCount = static_cast<std::uint32_t>(selection.local.size());
		header.directionalCount = static_cast<std::uint32_t>(selection.directional.size());

		_header = header;
		_lightData = data;
		_lights.resize(data.size());
		_lights.setFixedData(header);
		_lights.setDynamicData(std::span<const LightData>(data));
		_lights.validate();
	}

	const ShaderStorageBuffer &System::buffer() const noexcept
	{
		return _lights;
	}

	void System::activate(RenderContext &context) const
	{
		_lights.activate(context);
	}
}

namespace spk::Lighting
{
	std::unique_ptr<System::ShadowBindings> System::activateShadows(RenderContext &context) const
	{
		auto bindings = std::make_unique<ShadowBindings>();
		std::vector<ShadowData> data;
		data.reserve(_selection.local.size() + _selection.directional.size());
		bindings->samplers.reserve(MaxShadowSamplers + MaxPointShadowSamplers);
		std::size_t map2DCount = 0;
		std::size_t cubeCount = 0;
		std::vector<LightData> historicalLights = _lightData;
		bool requiresHistoricalLights = false;
		const auto append = [&](const Light::Selection::Entry &light) {
			ShadowData shadow;
			if (_shadowChannel && context.targetSurface != nullptr)
			{
				const auto resolved = _shadowChannel->resolveShadowResult(*context.targetSurface, light, _captureOrdinal);
				if (resolved && resolved->resolution > 0 && resolved->depthTexture)
				{
					const bool point = light.type == Light::Selection::Type::Point;
					const bool pointCompatible = resolved->faceCount == 6 && resolved->depthTexture->target() == Texture::Target::TextureCubeMap && resolved->depthTexture->format() == Texture::Format::Depth32F && resolved->depthTexture->size().x == resolved->resolution && resolved->depthTexture->size().y == resolved->resolution;
					const bool compatible = point ? pointCompatible : (resolved->faceCount == 1 && resolved->depthTexture->target() == Texture::Target::Texture2D);
					if (compatible && point && cubeCount < MaxPointShadowSamplers)
					{
						const auto &geometry = resolved->light;
						LightData &historic = historicalLights[data.size()];
						if (historic.position != geometry.position || historic.range != geometry.range)
						{
							historic.position = geometry.position;
							historic.range = geometry.range;
							requiresHistoricalLights = true;
						}
						const std::size_t index = cubeCount++;
						shadow.viewProjection = resolved->viewProjection;
						shadow.parameters[0] = resolved->depthBias;
						shadow.parameters[1] = resolved->normalBias;
						shadow.parameters[3] = static_cast<float>(index + 1);
						shadow.pointPositionNear = {resolved->light.position.x, resolved->light.position.y, resolved->light.position.z, resolved->nearPlane};
						shadow.pointFarResolution = {resolved->farPlane, static_cast<float>(resolved->resolution), light.shadowFilter == Light::ShadowFilter::PCF3x3 ? 1.0f : 0.0f, 1.0f};
						bindings->samplers.emplace_back(
							PointShadowBindingBase + index,
							Sampler::Filtering::Nearest,
							Sampler::Wrap::ClampToEdge,
							Sampler::MipmapFiltering::Disabled);
						bindings->samplers.back().setTexture(resolved->depthTexture);
						bindings->samplers.back().setDepthComparison(Sampler::DepthComparison::LessEqual);
					}
					else if (compatible && !point && resolved->cascadeSplits.back() > 0.0f &&
							 map2DCount + resolved->cascadeTextures.size() + static_cast<std::size_t>(std::count_if(
								 resolved->cascadeDynamicTextures.begin(), resolved->cascadeDynamicTextures.end(),
								 [](const auto &texture) { return static_cast<bool>(texture); })) <= MaxShadowSamplers)
					{
						shadow.viewProjection = resolved->viewProjection;
						shadow.viewProjection[4] = resolved->cascadeCameraView;
						shadow.cascadeSplits = resolved->cascadeSplits;
						shadow.parameters[0] = resolved->cascadeNear;
						shadow.cascadeDepthBias = resolved->cascadeDepthBias;
						shadow.cascadeNormalBias = resolved->cascadeNormalBias;
						const bool soft = light.shadowFilter == Light::ShadowFilter::PCF3x3;
						shadow.parameters[2] = soft ? 1.0f / static_cast<float>(resolved->resolution) : 0.0f;
						shadow.parameters[3] = static_cast<float>(map2DCount + 1);
						for (std::size_t i = 0; i < resolved->cascadeTextures.size(); ++i)
						{
							const auto &texture = resolved->cascadeTextures[i];
							// With an independent dynamic map, combine binary
							// comparisons per tap before filtering.
							const bool linear = soft && !resolved->cascadeDynamicTextures[i];
							bindings->samplers.emplace_back(
								1 + map2DCount++,
								linear ? Sampler::Filtering::Linear : Sampler::Filtering::Nearest,
								Sampler::Wrap::ClampToBorder,
								Sampler::MipmapFiltering::Disabled);
							bindings->samplers.back().setTexture(texture);
							bindings->samplers.back().setDepthComparison(Sampler::DepthComparison::LessEqual);
						}
						for (std::size_t i = 0; i < resolved->cascadeDynamicTextures.size(); ++i)
						{
							const auto &texture = resolved->cascadeDynamicTextures[i];
							if (!texture)
							{
								continue;
							}
							shadow.cascadeDynamicSlots[i] = static_cast<float>(map2DCount + 1);
							bindings->samplers.emplace_back(
								1 + map2DCount++, Sampler::Filtering::Nearest,
								Sampler::Wrap::ClampToBorder, Sampler::MipmapFiltering::Disabled);
							bindings->samplers.back().setTexture(texture);
							bindings->samplers.back().setDepthComparison(Sampler::DepthComparison::LessEqual);
						}
					}
					else if (compatible && !point && resolved->cascadeSplits.back() == 0.0f && map2DCount < MaxShadowSamplers)
					{
						const std::size_t index = map2DCount++;
						shadow.viewProjection[0] = resolved->viewProjection[0];
						shadow.parameters[0] = resolved->depthBias;
						shadow.parameters[1] = resolved->normalBias;
						const bool soft = light.shadowFilter == Light::ShadowFilter::PCF3x3;
						shadow.parameters[2] = soft ? 1.0f / static_cast<float>(resolved->resolution) : 0.0f;
						shadow.parameters[3] = static_cast<float>(index + 1);
						bindings->samplers.emplace_back(
							1 + index,
							soft ? Sampler::Filtering::Linear : Sampler::Filtering::Nearest,
							Sampler::Wrap::ClampToBorder,
							Sampler::MipmapFiltering::Disabled);
						bindings->samplers.back().setTexture(resolved->depthTexture);
						bindings->samplers.back().setDepthComparison(Sampler::DepthComparison::LessEqual);
					}
				}
			}
			data.push_back(shadow);
		};
		for (const auto &light : _selection.local)
		{
			append(light);
		}
		for (const auto &light : _selection.directional)
		{
			append(light);
		}
		if (requiresHistoricalLights)
		{
			bindings->historicalLights = std::make_unique<ShaderStorageBuffer>(
				BindingPoint, sizeof(Header), sizeof(LightData));
			bindings->historicalLights->resize(historicalLights.size());
			bindings->historicalLights->setFixedData(_header);
			bindings->historicalLights->setDynamicData(std::span<const LightData>(historicalLights));
			bindings->historicalLights->validate();
			bindings->historicalLights->activate(context);
		}
		bindings->buffer.resize(data.size());
		bindings->buffer.setDynamicData(std::span<const ShadowData>(data));
		bindings->buffer.validate();
		bindings->buffer.activate(context);
		for (auto &sampler : bindings->samplers)
		{
			sampler.validate();
			sampler.activate(context);
		}
		return bindings;
	}
}
