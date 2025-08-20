#pragma once

#include "structure/container/spk_json_object.hpp"
#include <cfenv>
#include <cmath>
#include <filesystem>

namespace spk
{
	namespace JSON
	{
		class File
		{
		private:
			spk::JSON::Object _root;

		public:
			File();

			File(const std::filesystem::path &p_filePath);

			void load(const std::filesystem::path &p_filePath);

			void save(const std::filesystem::path &p_filePath) const;

			bool contains(const std::wstring &p_key) const;

			spk::JSON::Object &operator[](const std::wstring &p_key);

			spk::JSON::Object &operator[](size_t p_index);

			const spk::JSON::Object &operator[](const std::wstring &p_key) const;

			const spk::JSON::Object &operator[](size_t p_index) const;

			template <typename TType>
			const TType &as()
			{
				return (_root.as<TType>());
			}

			spk::JSON::Object &root();
			const spk::JSON::Object &root() const;

			friend std::wostream &operator<<(std::wostream &p_os, const File &p_file);
		};
	}
}