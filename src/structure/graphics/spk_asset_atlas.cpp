#include "structure/graphics/spk_asset_atlas.hpp"

#include "structure/container/spk_json_file.hpp"

namespace spk
{
	void AssetAtlas::load(const std::filesystem::path &p_path)
	{
		spk::JSON::File atlasFileConfiguration = spk::JSON::File(p_path);

		if (atlasFileConfiguration.contains(L"Images"))
		{
			const auto &imagesNode = atlasFileConfiguration[L"Images"];
			if (imagesNode.isArray() == false)
			{
				throw std::runtime_error("The \"Images\" entry must be a JSON array");
			}

			for (const auto &imagePtr : imagesNode.asArray()) // <- shared_ptr<Object>
			{
				const auto &image = *imagePtr; // <- deref to Object

				const std::wstring name = image[L"Name"].as<std::wstring>();
				const std::wstring path = image[L"Path"].as<std::wstring>();

				_allocatedResources[name] = std::make_shared<spk::Image>(path);
			}
		}

		if (atlasFileConfiguration.contains(L"SpriteSheets"))
		{
			const auto &sheetsNode = atlasFileConfiguration[L"SpriteSheets"];
			if (sheetsNode.isArray() == false)
			{
				throw std::runtime_error("The \"SpriteSheets\" entry must be a JSON array");
			}

			for (const auto &sheetPtr : sheetsNode.asArray())
			{
				const auto &sheet = *sheetPtr;

				const std::wstring name = sheet[L"Name"].as<std::wstring>();
				const std::wstring path = sheet[L"Path"].as<std::wstring>();
				const spk::Vector2Int size = spk::Vector2Int(sheet[L"Size"]);

				_allocatedResources[name] = std::make_shared<spk::SpriteSheet>(path, size);
			}
		}

		if (atlasFileConfiguration.contains(L"Fonts"))
		{
			const auto &fontsNode = atlasFileConfiguration[L"Fonts"];
			if (fontsNode.isArray() == false)
			{
				throw std::runtime_error("The \"Fonts\" entry must be a JSON array");
			}

			for (const auto &fontPtr : fontsNode.asArray())
			{
				const auto &font = *fontPtr;

				const std::wstring name = font[L"Name"].as<std::wstring>();
				const std::wstring path = font[L"Path"].as<std::wstring>();

				_allocatedResources[name] = std::make_shared<spk::Font>(path);
			}
		}
	}

	spk::SafePointer<spk::Image> AssetAtlas::image(const std::wstring &p_name) const
	{
		if (_allocatedResources.contains(p_name) == false)
		{
			GENERATE_ERROR("Resource [" + spk::StringUtils::wstringToString(p_name) + "] not allocated");
		}
		const auto &res = _allocatedResources.at(p_name);
		if (auto p = std::get_if<std::shared_ptr<spk::Image>>(&res))
		{
			return p->get();
		}
		GENERATE_ERROR("Resource [" + spk::StringUtils::wstringToString(p_name) + "] is not an Image");
	}

	spk::SafePointer<spk::SpriteSheet> AssetAtlas::spriteSheet(const std::wstring &p_name) const
	{
		if (_allocatedResources.contains(p_name) == false)
		{
			GENERATE_ERROR("Resource [" + spk::StringUtils::wstringToString(p_name) + "] not allocated");
		}
		const auto &res = _allocatedResources.at(p_name);
		if (auto p = std::get_if<std::shared_ptr<spk::SpriteSheet>>(&res))
		{
			return p->get();
		}
		GENERATE_ERROR("Resource [" + spk::StringUtils::wstringToString(p_name) + "] is not a SpriteSheet");
	}

	spk::SafePointer<spk::Font> AssetAtlas::font(const std::wstring &p_name) const
	{
		if (_allocatedResources.contains(p_name) == false)
		{
			GENERATE_ERROR("Resource [" + spk::StringUtils::wstringToString(p_name) + "] not allocated");
		}
		const auto &res = _allocatedResources.at(p_name);
		if (auto p = std::get_if<std::shared_ptr<spk::Font>>(&res))
		{
			return p->get();
		}
		GENERATE_ERROR("Resource [" + spk::StringUtils::wstringToString(p_name) + "] is not a Font");
	}

}