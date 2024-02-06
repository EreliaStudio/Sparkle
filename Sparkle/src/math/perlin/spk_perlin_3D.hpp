#pragma once

#include "math/perlin/spk_perlin_2D.hpp"

#include "math/spk_vector3.hpp"

namespace spk
{
	class Perlin3D : public Perlin2D
	{
	private:
		float _computeWaveLength(const float& p_x, const float& p_y, const float& p_z, const float& p_frequency) const;

	public:
		Perlin3D();
		Perlin3D(const unsigned long& p_seed);
		Perlin3D(const spk::JSON::Object& p_object);

		float sample(const float& p_x, const float& p_y, const float& p_z) const;

		template<typename TVectorType>
		float sample(const spk::IVector3<TVectorType>& p_position) const
		{
			return (sample(
					static_cast<float>(p_position.x),
					static_cast<float>(p_position.y),
					static_cast<float>(p_position.z))
				);
		}
	};
}