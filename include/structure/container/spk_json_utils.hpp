#pragma once

#include "structure/container/spk_json_object.hpp"

namespace spk::JSON::utils
{
	std::wstring extractUnitSubstring(const std::wstring &p_content, size_t &p_index);
	void loadUnitNumbers(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);
	std::wstring getAttributeName(const std::wstring &p_content, size_t &p_index);
	std::wstring applyGrammar(const std::wstring &p_fileContent);
	void loadUnit(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
	void loadObject(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
	void loadArray(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
	void loadContent(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
	void loadUnitString(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);
	void loadUnitBoolean(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);
	void loadUnitNull(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);
} // namespace