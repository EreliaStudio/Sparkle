#include "lighting/system.hpp"

#include <cmath>
#include <limits>
#include <span>

#include "exception.hpp"

namespace spk::Lighting
{
	System::System(const Light::Selection &selection, RGB ambient) :
		_lights(BindingPoint, sizeof(Header), sizeof(LightData))
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
