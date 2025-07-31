#include "structure/container/spk_JSON_file.hpp"

#include <algorithm>
#include <fstream>

#include "utils/spk_file_utils.hpp"
#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk::JSON
{
	File::File() :
		_root(L"Root")
	{
	}

	File::File(const std::filesystem::path &p_filePath) :
		_root(L"Root")
	{
		load(p_filePath);
	}

	void File::load(const std::filesystem::path &p_filePath)
	{
		std::wstring fileContent = spk::FileUtils::readFileAsWString(p_filePath);

		if (fileContent.empty())
		{
			GENERATE_ERROR("Empty file: " + p_filePath.string());
		}
		_root = spk::JSON::Object::fromString(fileContent);
	}

	void File::save(const std::filesystem::path &p_filePath) const
	{
		std::wofstream file;

		file.open(p_filePath);
		if (file.is_open() == false)
		{
			GENERATE_ERROR("Unable to open file: " + p_filePath.string());
		}
		file << _root;
		file.close();
	}

	bool File::contains(const std::wstring &p_key) const
	{
		return (_root.contains(p_key));
	}

	spk::JSON::Object &File::operator[](const std::wstring &p_key)
	{
		return (_root[p_key]);
	}

	spk::JSON::Object &File::operator[](size_t p_index)
	{
		return (_root[p_index]);
	}

	const spk::JSON::Object &File::operator[](const std::wstring &p_key) const
	{
		return (_root[p_key]);
	}

	const spk::JSON::Object &File::operator[](size_t p_index) const
	{
		return (_root[p_index]);
	}

	spk::JSON::Object &File::root()
	{
		return (_root);
	}

	const spk::JSON::Object &File::root() const
	{
		return (_root);
	}

	std::wostream &operator<<(std::wostream &p_os, const File &p_file)
	{
		p_os << p_file._root;
		return (p_os);
	}
}