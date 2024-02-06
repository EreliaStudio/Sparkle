#pragma once

#include "miscellaneous/JSON/spk_JSON_object.hpp"
#include "spk_basic_functions.hpp"
#include <filesystem>
#include <cfenv>
#include <cmath>

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
			File(const std::filesystem::path& p_filePath);

			void load(const std::filesystem::path& p_filePath);
			void save(const std::filesystem::path& p_filePath) const;

			bool contains(const std::string& p_key) const;

			const spk::JSON::Object &operator[](const std::string &p_key) const;

			const spk::JSON::Object &operator[](size_t p_index) const;

			template <typename TType>
			const TType &as()
			{
				return (_root.as<TType>());
			}

			const spk::JSON::Object &root() const;

			friend std::ostream &operator<<(std::ostream &p_os, const File &p_file);
		};
	}
}
