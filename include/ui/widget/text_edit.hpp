#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

#include "design_pattern/contract_provider.hpp"
#include "graphics/color.hpp"
#include "graphics/font.hpp"
#include "graphics/sprite_sheet.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class TextEdit : public Widget
	{
	public:
		struct Selection
		{
			std::size_t start = 0;
			std::size_t end = 0;
			[[nodiscard]] bool empty() const noexcept;
			[[nodiscard]] std::size_t length() const noexcept;
			bool operator==(const Selection &) const = default;
		};

		enum class ValidationState
		{
			Valid,
			Undefined,
			Invalid
		};

		using ValidationCallback = std::function<ValidationState(const Font::Text &)>;
		using EditionProvider = ContractProvider<const Font::Text &>;
		using EditionCallback = EditionProvider::callback_type;
		using EditionContract = EditionProvider::Contract;
		using SelectionProvider = ContractProvider<const Selection &>;
		using SelectionCallback = SelectionProvider::callback_type;
		using SelectionContract = SelectionProvider::Contract;

	private:
		const SpriteSheet *_spriteSheet = nullptr;
		Vector2Int _cornerSize{};
		Font *_font = nullptr;
		Font::Size _textSize{16};
		Color _glyphColor{1.0f, 1.0f, 1.0f, 1.0f};
		Color _outlineColor{0.0f, 0.0f, 0.0f, 1.0f};
		Color _cursorColor{1.0f, 1.0f, 1.0f, 1.0f};
		Color _selectionColor{0.2f, 0.45f, 0.9f, 0.55f};
		float _depth = 0.0f;

		Font::Text _text;
		Font::Text _placeholder = U"Enter text here";
		bool _obscured = false;
		bool _editEnabled = true;
		bool _focused = false;
		bool _hovered = false;
		std::size_t _cursor = 0;
		std::size_t _selectionAnchor = 0;
		Selection _selectionState{};
		bool _dragSelecting = false;
		bool _copyObscuredTextEnabled = false;
		bool _shiftPressed = false;
		std::size_t _visibleStart = 0;
		std::size_t _visibleEnd = 0;
		bool _caretVisible = true;
		std::chrono::steady_clock::duration _caretElapsed{};
		ValidationCallback _validationCallback;
		EditionProvider _editionProvider;
		SelectionProvider _selectionProvider;

		[[nodiscard]] Vector2UInt _innerSize() const noexcept;
		[[nodiscard]] Font::Text _editableRepresentation() const;
		[[nodiscard]] unsigned int _measure(const Font::Text &text) const;
		[[nodiscard]] Font::Text _fittingPrefix(const Font::Text &text, unsigned int width) const;
		[[nodiscard]] ValidationState _validate(const Font::Text &candidate) const;
		void _resetCaretBlink() noexcept;
		void _recomputeVisibleRange();
		void _placeCursorFromClick(int globalX);
		void _updateSelectionState();
		void _setAnchorAndCursor(std::size_t anchor, std::size_t cursor);
		bool _replaceSelection(const Font::Text &replacement);
		void _notifyEdition();
		void _handleMousePress(EventBase &event, Mouse::Button button, const Vector2Int &position, bool extendSelection = false);
		void _updateSizeHint() override;
		void _updateState(UpdateContext &context) override;
		void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;
		void _onGeometryChange() override;
		void _onFocusAcquired(FocusMode::Channel channel) noexcept override;
		void _onFocusReleased(FocusMode::Channel channel) noexcept override;
		void _onWindowFocusLostEvent(WindowFocusLostEvent &event) override;
		void _onMouseLeftEvent(MouseLeftEvent &event) override;
		void _onMouseMovedEvent(MouseMovedEvent &event) override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;
		void _onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event) override;
		void _onPassiveMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onKeyPressedEvent(KeyPressedEvent &event) override;
		void _onKeyReleasedEvent(KeyReleasedEvent &event) override;
		void _onPassiveKeyPressedEvent(KeyPressedEvent &event) override;
		void _onPassiveKeyReleasedEvent(KeyReleasedEvent &event) override;
		void _onTextInputEvent(TextInputEvent &event) override;

	public:
		explicit TextEdit(std::string name, Widget *parent = nullptr);
		TextEdit(std::string name, Font *font, Widget *parent = nullptr);
		TextEdit(std::string name, const SpriteSheet *spriteSheet, Font *font, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		[[nodiscard]] EditionContract subscribeToEdition(EditionCallback callback);
		[[nodiscard]] SelectionContract subscribeToSelection(SelectionCallback callback);

		void setSpriteSheet(const SpriteSheet *spriteSheet);
		void setCornerSize(const Vector2Int &cornerSize);
		void setFont(Font *font);
		void setText(const Font::Text &text);
		void setText(std::string_view text);
		void setPlaceholder(const Font::Text &placeholder);
		void setPlaceholder(std::string_view placeholder);
		void setObscured(bool obscured);
		void enableEdit();
		void disableEdit();
		void setValidationCallback(ValidationCallback callback);
		void setTextSize(const Font::Size &textSize);
		void setGlyphColor(const Color &color);
		void setOutlineColor(const Color &color);
		void setCursorColor(const Color &color);
		void setDepth(float depth);
		void setSelection(std::size_t start, std::size_t end);
		void clearSelection();
		void selectAll();
		void setSelectionColor(const Color &color);
		void setCopyObscuredTextEnabled(bool enabled);
		bool copySelection() const;
		bool cutSelection();
		bool pasteClipboard();

		[[nodiscard]] ValidationState validationState() const;
		[[nodiscard]] bool hasText() const noexcept;
		[[nodiscard]] bool isObscured() const noexcept;
		[[nodiscard]] bool isEditEnabled() const noexcept;
		[[nodiscard]] bool isFocused() const noexcept;
		[[nodiscard]] bool isHovered() const noexcept;
		[[nodiscard]] bool isCaretVisible() const noexcept;
		[[nodiscard]] bool hasSelection() const noexcept;
		[[nodiscard]] Selection selection() const noexcept;
		[[nodiscard]] Font::Text selectedText() const;
		[[nodiscard]] const Color &selectionColor() const noexcept;
		[[nodiscard]] bool isCopyObscuredTextEnabled() const noexcept;
		[[nodiscard]] Font::Text renderedText() const;
		[[nodiscard]] Font::Text visibleText() const;
		[[nodiscard]] const Font::Text &text() const noexcept;
		[[nodiscard]] std::string textAsUTF8() const;
		[[nodiscard]] const Font::Text &placeholder() const noexcept;
		[[nodiscard]] std::size_t cursor() const noexcept;
		[[nodiscard]] std::size_t visibleStart() const noexcept;
		[[nodiscard]] std::size_t visibleEnd() const noexcept;
		[[nodiscard]] const SpriteSheet *spriteSheet() const noexcept;
		[[nodiscard]] const Vector2Int &cornerSize() const noexcept;
		[[nodiscard]] Font *font() noexcept;
		[[nodiscard]] const Font *font() const noexcept;
		[[nodiscard]] const Font::Size &textSize() const noexcept;
		[[nodiscard]] const Color &glyphColor() const noexcept;
		[[nodiscard]] const Color &outlineColor() const noexcept;
		[[nodiscard]] const Color &cursorColor() const noexcept;
		[[nodiscard]] float depth() const noexcept;
	};
}
