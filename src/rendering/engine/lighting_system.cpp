#include "lighting/system.hpp"

#include <cmath>
#include <limits>
#include <span>
#include <string>

#include "core/context/render_context.hpp"
#include "light/channel.hpp"

#include "exception.hpp"

namespace spk::Lighting
{
	System::System(const Light::Selection &selection, RGB ambient,
		std::shared_ptr<Light::Channel> shadowChannel, std::uint64_t captureOrdinal) :
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
		bindings->samplers.reserve(MaxShadowSamplers);
		const auto append = [&](const Light::Selection::Entry &light) {
			ShadowData shadow;
			if (_shadowChannel && context.targetSurface != nullptr &&
				light.type != Light::Selection::Type::Point && bindings->samplers.size() < MaxShadowSamplers)
			{
				const auto resolved = _shadowChannel->resolveShadowResult(*context.targetSurface, light, _captureOrdinal);
				if (resolved && resolved->faceCount == 1 && resolved->resolution > 0)
				{
					const std::size_t index = bindings->samplers.size();
					shadow.viewProjection = resolved->viewProjection[0];
					shadow.parameters = {resolved->depthBias, resolved->normalBias,
						1.0f / static_cast<float>(resolved->resolution), static_cast<float>(index + 1)};
					bindings->samplers.emplace_back(1 + index, Sampler::Filtering::Nearest,
						Sampler::Wrap::ClampToBorder, Sampler::MipmapFiltering::Disabled);
					bindings->samplers.back().setTexture(resolved->depthTexture);
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
