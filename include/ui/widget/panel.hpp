#pragma once

#include <string>

#include "graphics/sprite_sheet.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class Panel : public Widget
	{
	private:
		const SpriteSheet *_spriteSheet = nullptr;
		Vector2Int _cornerSize{0, 0};
		float _depth = 0.0f;

		void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		explicit Panel(std::string name, Widget *parent = nullptr);
		Panel(std::string name, const SpriteSheet *spriteSheet, Widget *parent = nullptr);

		void setSpriteSheet(const SpriteSheet *spriteSheet);
		void setCornerSize(const Vector2Int &cornerSize);
		void setDepth(float depth);

		[[nodiscard]] const SpriteSheet *spriteSheet() const noexcept;
		[[nodiscard]] const Vector2Int &cornerSize() const noexcept;
		[[nodiscard]] float depth() const noexcept;
	};
}
