#pragma once

#include <stdexcept>

#include <string>

#include "structure/graphics/lumina/compiler/spk_token.hpp"
#include "structure/graphics/lumina/compiler/spk_source_manager.hpp"

namespace spk::Lumina
{
	class TokenException : public std::runtime_error
    {
    public:
        TokenException(const std::wstring& p_msg, const Token& p_token, const SourceManager& p_sourceManager);

    private:
        static std::wstring compose(const std::wstring& p_msg, const Token& p_token, const SourceManager& p_sourceManager);
    };
}