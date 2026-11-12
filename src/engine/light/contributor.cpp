#include "light/contributor.hpp"

#include <cmath>
#include <limits>
#include <utility>

#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "math/vector4.hpp"

namespace spk::Light
{
	Contributor::Contributor(const std::string &name, bool castShadows) :
		Component3D(name),
		_castsShadows(castShadows)
	{
		_onContextEditionContract = subscribeToContextEdition(
			[this](EngineHeadless *const &, EngineHeadless *const &newContext) {
				if (newContext == nullptr)
				{
					_registrationOrder = 0;
				}
			});
	}

	void Contributor::_setRegistrationOrder(std::uint64_t order) noexcept
	{
		_registrationOrder = order;
	}

	void Contributor::_notifyEdition()
	{
		_onEditionContractProvider.trigger(*this);
	}

	Vector3 Contributor::_worldPosition() const
	{
		if (owner() == nullptr)
		{
			throw spk::Exception("A direct light must have an Entity3D owner to access its world position");
		}

		const Vector3 position = owner()->transform().position(ReferenceFrame::World);
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
		{
			throw spk::Exception("Direct light world position must be finite");
		}
		return position;
	}

	Vector3 Contributor::_worldForward() const
	{
		if (owner() == nullptr)
		{
			throw spk::Exception("A direct light must have an Entity3D owner to access its world direction");
		}
		const auto &rotation = owner()->transform().rotation(ReferenceFrame::World);
		const Vector4 forward = Matrix4x4::rotation(rotation) * Vector4{0.0f, 0.0f, -1.0f, 0.0f};
		return Vector3{forward.x, forward.y, forward.z}.normalized();
	}

	RGB Contributor::color() const noexcept
	{
		return _color;
	}

	void Contributor::setColor(RGB color)
	{
		const auto valid = [](float v) {
			return std::isfinite(v) && v >= 0.0f && v <= 1.0f;
		};
		if (!valid(color.r) || !valid(color.g) || !valid(color.b))
		{
			throw spk::Exception("Direct light color must be finite linear RGB within [0,1]");
		}
		if (_color == color)
		{
			return;
		}
		_color = color;
		_notifyEdition();
	}

	float Contributor::intensity() const noexcept
	{
		return _intensity;
	}

	void Contributor::setIntensity(float intensity)
	{
		if (!std::isfinite(intensity) || intensity < 0.0f)
		{
			throw spk::Exception("Direct light intensity must be finite and nonnegative");
		}
		if (_intensity == intensity)
		{
			return;
		}
		_intensity = intensity;
		_notifyEdition();
	}

	float Contributor::priorityWeight() const noexcept
	{
		return _priorityWeight;
	}

	void Contributor::setPriorityWeight(float weight)
	{
		if (!std::isfinite(weight) || weight < 0.0f)
		{
			throw spk::Exception("Direct light priorityWeight must be finite and nonnegative");
		}
		if (_priorityWeight == weight)
		{
			return;
		}
		_priorityWeight = weight;
		_notifyEdition();
	}

	bool Contributor::castsShadows() const noexcept
	{
		return _castsShadows;
	}

	void Contributor::setCastsShadows(bool enabled)
	{
		if (_castsShadows == enabled)
		{
			return;
		}
		_castsShadows = enabled;
		_notifyEdition();
	}

	std::uint32_t Contributor::shadowResolution() const noexcept
	{
		return _shadowResolution;
	}

	void Contributor::setShadowResolution(std::uint32_t resolution)
	{
		// OpenGL texture dimensions use signed GLsizei; actual device limits are checked on allocation.
		if (resolution == 0 || resolution > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
		{
			throw spk::Exception("Direct light shadowResolution must be a positive representable texture dimension");
		}
		if (_shadowResolution == resolution)
		{
			return;
		}
		_shadowResolution = resolution;
		_notifyEdition();
	}

	std::uint64_t Contributor::registrationOrder() const noexcept
	{
		return _registrationOrder;
	}

	Contributor::OnEditionContract Contributor::subscribeToEdition(OnEditionCallback callback)
	{
		return _onEditionContractProvider.subscribe(std::move(callback));
	}
}
