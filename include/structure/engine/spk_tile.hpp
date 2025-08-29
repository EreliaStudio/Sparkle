#pragma once

#include "structure/container/spk_flags.hpp"
#include "structure/math/spk_vector2.hpp"

namespace spk
{
	template <typename TFlagEnum>
	class Tile final
	{
	public:
		using ID = short;
		enum class Type
		{
			Monotile,
			Autotile
		};

	private:
		spk::Vector2Int _spriteAnchor = 0;
		spk::Flags<TFlagEnum> _flags = {};
		Type _type = Type::Monotile;

	public:
		Tile() = default;
		Tile(const spk::Vector2Int &p_spriteAnchor, Type p_type, spk::Flags<TFlagEnum> p_flags = {}) :
			_spriteAnchor(p_spriteAnchor),
			_flags(p_flags),
			_type(p_type)
		{
		}

		const spk::Vector2Int &spriteAnchor() const
		{
			return _spriteAnchor;
		}

		Type type() const
		{
			return _type;
		}

		const spk::Flags<TFlagEnum> &flags() const
		{
			return _flags;
		}
	};
}
