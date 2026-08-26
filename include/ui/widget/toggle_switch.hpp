#pragma once

#include <chrono>
#include <string>

#include "design_pattern/contract_provider.hpp"
#include "graphics/opengl/texture.hpp"
#include "type/orientation.hpp"
#include "ui/widget/panel.hpp"

namespace spk
{
	class ToggleSwitch : public Widget
	{
	public:
		using Duration = std::chrono::steady_clock::duration;
		using StateProvider = ContractProvider<bool>;
		using StateCallback = StateProvider::callback_type;
		using StateContract = StateProvider::Contract;

	private:
		bool _checked = false;
		bool _animating = false;
		bool _pressed = false;
		float _visualRatio = 0.0f;
		float _animationStart = 0.0f;
		Duration _animationElapsed{};
		Duration _animationDuration = std::chrono::milliseconds(150);
		Orientation _orientation = Orientation::Horizontal;
		Vector2UInt _padding{2, 2};
		Vector2UInt _thumbSize{16, 16};
		const Texture *_uncheckedTexture = nullptr;
		Texture::Section _uncheckedSection = Texture::Section::whole;
		const SpriteSheet *_uncheckedNineSlice = nullptr;
		const Texture *_checkedTexture = nullptr;
		Texture::Section _checkedSection = Texture::Section::whole;
		const SpriteSheet *_checkedNineSlice = nullptr;
		Panel _outline;
		Panel _thumb;
		StateProvider _stateProvider;

		void _updateVisualGeometry();
		void _updateSizeHint() override;
		void _updateState(UpdateContext &context) override;
		void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;
		void _onGeometryChange() override;
		void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;
		void _onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event) override;

	public:
		explicit ToggleSwitch(std::string name, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setChecked(bool checked);
		void toggle();
		void setOrientation(Orientation orientation);
		void setPadding(const Vector2UInt &padding);
		void setThumbSize(const Vector2UInt &size);
		void setAnimationDuration(Duration duration);
		void setUncheckedBackgroundTexture(const Texture *texture, const Texture::Section &section = Texture::Section::whole);
		void setUncheckedBackgroundTexture(const SpriteSheet *texture);
		void setCheckedBackgroundTexture(const Texture *texture, const Texture::Section &section = Texture::Section::whole);
		void setCheckedBackgroundTexture(const SpriteSheet *texture);
		void setOutlineTexture(const SpriteSheet *texture);
		void setOutlineCornerSize(const Vector2Int &cornerSize);
		void setThumbTexture(const SpriteSheet *texture);
		void setThumbCornerSize(const Vector2Int &cornerSize);

		[[nodiscard]] bool isChecked() const noexcept;
		[[nodiscard]] bool isAnimating() const noexcept;
		[[nodiscard]] float visualRatio() const noexcept;
		[[nodiscard]] Orientation orientation() const noexcept;
		[[nodiscard]] const Vector2UInt &padding() const noexcept;
		[[nodiscard]] const Vector2UInt &thumbSize() const noexcept;
		[[nodiscard]] Duration animationDuration() const noexcept;
		[[nodiscard]] const Texture *uncheckedBackgroundTexture() const noexcept;
		[[nodiscard]] const Texture::Section &uncheckedBackgroundSection() const noexcept;
		[[nodiscard]] const Texture *checkedBackgroundTexture() const noexcept;
		[[nodiscard]] const Texture::Section &checkedBackgroundSection() const noexcept;
		[[nodiscard]] const SpriteSheet *outlineTexture() const noexcept;
		[[nodiscard]] const Vector2Int &outlineCornerSize() const noexcept;
		[[nodiscard]] const SpriteSheet *thumbTexture() const noexcept;
		[[nodiscard]] const Vector2Int &thumbCornerSize() const noexcept;
		[[nodiscard]] StateContract subscribeToState(StateCallback callback);
	};
}
