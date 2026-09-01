#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "container/cached_data.hpp"
#include "core/event/record.hpp"
#include "core/event/event_dispatcher.hpp"
#include "design_pattern/trait/activable_trait.hpp"
#include "design_pattern/trait/inherence_trait.hpp"
#include "design_pattern/trait/name_trait.hpp"
#include "design_pattern/trait/resizeable_trait.hpp"
#include "graphics/color.hpp"
#include "graphics/font.hpp"
#include "math/rect2d.hpp"
#include "rendering/render_snapshot.hpp"
#include "type/alignment.hpp"
#include "type/focus_mode.hpp"
#include "ui/view_region.hpp"

namespace spk
{
	struct UpdateContext;
	class Image;
	class SpriteSheet;
	class Widget;

	struct WidgetChildComparator
	{
		bool operator()(const Widget *lhs, const Widget *rhs) const;
	};

	class Widget : public spk::NameTrait,
				   public spk::InherenceTrait<Widget, WidgetChildComparator>,
				   public spk::ResizeableTrait,
				   public spk::ActivableTrait,
				   public spk::EventDispatcher
	{
	public:
		//Need to refactor this to make it more manageable
		struct Style
		{
			Font::Size textLabelTextSize{16};
			Color textLabelGlyphColor{0.94f, 0.96f, 1.0f, 1.0f};
			Color textLabelOutlineColor{0.03f, 0.04f, 0.08f, 1.0f};

			Vector2Int pushButtonCornerSize{8, 8};
			Font::Size pushButtonTextSize{16};
			Color pushButtonReleasedGlyphColor{0.88f, 0.92f, 1.0f, 1.0f};
			Color pushButtonPressedGlyphColor{1.0f, 1.0f, 1.0f, 1.0f};
			Alignment pushButtonAlignment{Alignment::Horizontal::Center, Alignment::Vertical::Center};

			Vector2UInt iconButtonIconSize{22, 22};
			Vector2UInt iconButtonIconPadding{6, 6};

			Vector2Int textEditCornerSize{7, 7};
			Font::Size textEditTextSize{17};
			Color textEditGlyphColor{0.92f, 0.95f, 1.0f, 1.0f};
			Color textEditOutlineColor{0.03f, 0.04f, 0.08f, 1.0f};
			Color textEditCursorColor{1.0f, 1.0f, 1.0f, 1.0f};

			Vector2Int sliderBarBackgroundCornerSize{5, 5};
			Vector2Int sliderBarBodyCornerSize{8, 8};

			Vector2UInt scrollBarButtonIconSize{16, 16};

			Vector2Int interfaceWindowBackgroundCornerSize{12, 12};
			Font::Size interfaceWindowMenuTitleTextSize{18};
			Color interfaceWindowMenuTitleGlyphColor{0.94f, 0.96f, 1.0f, 1.0f};
			Color interfaceWindowMenuTitleOutlineColor{0.03f, 0.04f, 0.08f, 1.0f};
			Alignment interfaceWindowMenuTitleAlignment{Alignment::Horizontal::Left, Alignment::Vertical::Center};
			Vector2UInt interfaceWindowMenuButtonIconSize{12, 12};
			Vector2UInt interfaceWindowMenuButtonIconPadding{4, 4};
			Vector2Int interfaceWindowMenuButtonCornerSize{2, 2};

			std::unique_ptr<Font> font;
			std::unique_ptr<Image> iconsetImage;
			std::unique_ptr<SpriteSheet> iconset;
			std::unique_ptr<SpriteSheet> nineSlice;
			std::unique_ptr<SpriteSheet> darkNineSlice;
			std::unique_ptr<SpriteSheet> darkerNineSlice;
			std::unique_ptr<SpriteSheet> lightNineSlice;
			std::unique_ptr<SpriteSheet> sliderBody;
			std::unique_ptr<SpriteSheet> menuBreak;
			std::unique_ptr<SpriteSheet> toggleSwitchOutline;
			std::unique_ptr<SpriteSheet> toggleSwitchThumb;
			std::unique_ptr<SpriteSheet> toggleSwitchOffBackground;
			std::unique_ptr<SpriteSheet> toggleSwitchOnBackground;

			Style();
			~Style();
			Style(Style &&) noexcept;
			Style &operator=(Style &&) noexcept;
			Style(const Style &) = delete;
			Style &operator=(const Style &) = delete;
		};

		static inline spk::RenderPass::Key BackgroundKey = {
			.name = "sparkle.Background",
			.order = -300};
		static inline spk::RenderPass::Key OverlayKey = {
			.name = "sparkle.Overlay",
			.order = 0};
		static inline spk::RenderPass::Key PopupKey = {
			.name = "sparkle.Popup",
			.order = 50};
		static inline spk::RenderPass::Key TooltipKey = {
			.name = "sparkle.Tooltip",
			.order = 100};
		static CachedData<Style> defaultStyle;

		using ZOrder = float;

	private:
		using Inherence = InherenceTrait<Widget, WidgetChildComparator>;

		bool _acceptChildSizeHintEditions = false;
		ActivableTrait::ActivationContract _initialActivationContract;
		Inherence::OnParentEditionContract _onParentEditedContract;
		std::unordered_map<Widget *, ResizeableTrait::Contract> _childSizeHintEditionContracts;
		ZOrder _zOrder = 0;
		spk::CachedData<ZOrder> _absoluteZOrder;
		spk::Rect2D _geometry{};
		spk::Vector2 _anchorRatio{0.0f, 0.0f};
		spk::Vector2 _sizeRatio{1.0f, 1.0f};
		spk::CachedData<ViewRegion> _viewRegion;
		std::optional<RenderPass::Key> _targetRenderPassOverride;

		void _invalidateViewRegion();
		void _invalidateAbsoluteZOrder();
		void _computeRatio();
		[[nodiscard]] spk::Rect2D _geometryFromRatio(const Widget &child) const;
		void _resize(const spk::Rect2D &geometry);
		void _onChildSizeHintEdition();

		void _onChildAdded(Widget *child) override final;
		void _onChildRemoved(Widget *child) override final;

		[[nodiscard]] bool _isAcceptingInteraction() const override;
		void _propagateInteraction(
			const std::function<void(EventDispatcher *)> &callback) override;


		void _buildViewRegionCommands(spk::RenderSnapshot::Builder &builder);

		virtual void _updateState(UpdateContext &context);
		virtual void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
		virtual void _updateSizeHint();
		virtual void _onGeometryChange();
		virtual void _onFocusAcquired(FocusMode::Channel channel) noexcept;
		virtual void _onFocusReleased(FocusMode::Channel channel) noexcept;


	public:
		Widget(std::string name, Widget *parent);
		virtual ~Widget();

		virtual void applyStyle(const Style &style);

		void setZOrder(ZOrder zOrder);
		[[nodiscard]] ZOrder zOrder() const;
		[[nodiscard]] ZOrder absoluteZOrder() const;

		void setGeometry(const spk::Rect2D &geometry);
		void resize(const spk::Rect2D &geometry);
		[[nodiscard]] const spk::Rect2D &geometry() const noexcept;
		[[nodiscard]] const ViewRegion &viewRegion() const;
		[[nodiscard]] Widget &root() noexcept;
		[[nodiscard]] const Widget &root() const noexcept;

		void setTargetRenderPass(const RenderPass::Key &key);
		void inheritTargetRenderPass();
		[[nodiscard]] bool hasTargetRenderPassOverride() const noexcept;
		[[nodiscard]] const RenderPass::Key &targetRenderPass() const noexcept;


		void updateState(UpdateContext &context);
		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder);
		void notifyFocusAcquired(FocusMode::Channel channel) noexcept;
		void notifyFocusReleased(FocusMode::Channel channel) noexcept;
	};
}
