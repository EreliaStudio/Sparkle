#pragma once

#include <string>

#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/texture/spk_image.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/graphics/texture/spk_font.hpp"

namespace spk
{
	class AssetAtlas
	{
	private:
		using Resource = std::variant<std::shared_ptr<spk::Image>, std::shared_ptr<spk::SpriteSheet>, std::shared_ptr<spk::Font>>;

		std::unordered_map<std::wstring, Resource> _allocatedResources;

	public:
		AssetAtlas() = default;

		void load(const std::filesystem::path &p_path);

		spk::SafePointer<spk::Image> image(const std::wstring &p_name) const;
		spk::SafePointer<spk::SpriteSheet> spriteSheet(const std::wstring &p_name) const;
		spk::SafePointer<spk::Font> font(const std::wstring &p_name) const;
	};
}