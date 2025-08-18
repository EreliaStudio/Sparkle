#pragma once

#include "structure/container/spk_json_object.hpp"

namespace spk::JSON::utils
{
std::wstring applyGrammar(const std::wstring &p_fileContent);
void loadContent(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
} // namespace spk::JSON::utils

