#include <GL/glew.h>
#include <gtest/gtest.h>
#include <stb_image_write.h>

#include "sparkle_test.hpp"
#include "ui/widget/check_box.hpp"
#include "ui/widget/container_widget.hpp"
#include "ui/widget/debug_overlay.hpp"
#include "ui/widget/image_label.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/progress_bar.hpp"
#include "ui/widget/prompt_panel.hpp"
#include "ui/widget/radio_button.hpp"
#include "ui/widget/text_area.hpp"
#include "ui/widget/text_edit.hpp"
#include "ui/widget/text_label.hpp"
#include "ui/widget/toggle_switch.hpp"
#include "ui/widget/tooltip.hpp"

namespace
{
	class SolidTexture : public spk::Texture
	{
	public:
		explicit SolidTexture(std::array<std::uint8_t, 4> color) :
			Texture(Target::Texture2D)
		{
			setPixels(color.data(), {1, 1}, Format::RGBA);
			setMipmap(Mipmap::Disable);
			validate();
		}
	};

	spk::SpriteSheet solidSheet()
	{
		std::array<std::uint8_t, 36> pixels;
		pixels.fill(255);
		std::vector<std::uint8_t> png;
		stbi_write_png_to_func([](void *context, void *data, int size) {
			auto &bytes = *static_cast<std::vector<std::uint8_t> *>(context);
			const auto *begin = static_cast<std::uint8_t *>(data);
			bytes.insert(bytes.end(), begin, begin + size);
		},
							   &png,
							   3,
							   3,
							   4,
							   pixels.data(),
							   12);
		return spk::SpriteSheet(png, {3, 3});
	}

	sparkle_test::FramebufferImage render(spk::Widget &widget)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(context.renderContext());
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		return context.capture();
	}

	std::size_t visiblePixels(const sparkle_test::FramebufferImage &image)
	{
		std::size_t result = 0;
		for (std::size_t i = 3; i < image.pixels.size(); i += 4)
		{
			result += image.pixels[i] != 0;
		}
		return result;
	}

	void expectClipped(const sparkle_test::FramebufferImage &image, const spk::Rect2D &bounds)
	{
		std::size_t outside = 0;
		for (unsigned y = 0; y < image.size.y; ++y)
		{
			for (unsigned x = 0; x < image.size.x; ++x)
			{
				if (!bounds.contains({static_cast<int>(x), static_cast<int>(y)}) && image.pixel({x, y})[3] != 0)
				{
					++outside;
				}
			}
		}
		EXPECT_EQ(outside, 0u);
	}
}

TEST(WidgetRenderIntegrationTest, OverlappingImagesRespectDepthInEitherInsertionOrderAndClipWideGeometry)
{
	SolidTexture red({255, 0, 0, 255}), green({0, 255, 0, 255});
	spk::Widget root("Root", nullptr);
	root.activate();
	root.setGeometry({.anchor = {20, 20}, .size = {80, 60}});
	spk::ImageLabel back("Back", &red, &root), front("Front", &green, &root);
	back.setGeometry({.anchor = {-10, 0}, .size = {150, 60}});
	front.setGeometry({.anchor = {10, 10}, .size = {40, 30}});
	back.setDepth(-0.5f);
	front.setDepth(0.5f);
	for (float order : {-1.0f, 1.0f})
	{
		front.setZOrder(order);
		const auto image = render(root);
		EXPECT_EQ(image.pixel({40, 40})[1], 255);
		EXPECT_EQ(image.pixel({25, 25})[0], 255);
		EXPECT_EQ(visiblePixels(image), 80u * 60u);
		expectClipped(image, root.geometry());
	}
}

TEST(WidgetRenderIntegrationTest, ContainerClipsOversizedOffsetContentToItsVisibleRegion)
{
	SolidTexture white({255, 255, 255, 255});
	spk::ContainerWidget container("Container");
	spk::ImageLabel content("Content", &white, &container);
	container.setGeometry({.anchor = {30, 40}, .size = {40, 30}});
	container.setContent(&content);
	container.setContentSize({200, 200});
	container.setContentAnchor({-20, -20});
	const auto image = render(container);
	EXPECT_EQ(visiblePixels(image), 1200u);
	expectClipped(image, container.geometry());
}

TEST(WidgetRenderIntegrationTest, PanelResourcesCornersHintsAndStyleChangePixels)
{
	spk::Panel panel("Panel");
	panel.setGeometry({.anchor = {20, 20}, .size = {100, 60}});
	panel.setCornerSize({8, 8});
	panel.setDepth(0.25f);
	const auto before = render(panel);
	EXPECT_GT(visiblePixels(before), 0u);
	expectClipped(before, panel.geometry());
	const auto *sheet = panel.spriteSheet();
	EXPECT_THROW(panel.setSpriteSheet(spk::Widget::defaultStyle->iconset.get()), std::invalid_argument);
	EXPECT_EQ(panel.spriteSheet(), sheet);
	panel.setSpriteSheet(spk::Widget::defaultStyle->darkNineSlice.get());
	const auto after = render(panel);
	EXPECT_NE(before.pixels, after.pixels);
	EXPECT_EQ(panel.minimalSize(), spk::Vector2(16, 16));
	panel.applyStyle(spk::Widget::defaultStyle);
	EXPECT_EQ(render(panel).pixels, before.pixels);
}

TEST(WidgetRenderIntegrationTest, TextAlignmentsOutlinePaddingUnicodeAndEmptyGeometry)
{
	spk::TextLabel label("Text");
	label.setGeometry({.anchor = {30, 30}, .size = {160, 80}});
	label.setText(U"A\u00e9\U0010ffff");
	label.setPadding({8, 6});
	label.setGlyphColor({1, 0, 0, 1});
	label.setOutlineColor({0, 1, 0, 1});
	label.setDepth(0.25f);
	std::vector<std::uint8_t> previous;
	for (unsigned outline : {0u, 2u})
	{
		for (auto h : {spk::Alignment::Horizontal::Left, spk::Alignment::Horizontal::Center, spk::Alignment::Horizontal::Right})
		{
			for (auto v : {spk::Alignment::Vertical::Top, spk::Alignment::Vertical::Center, spk::Alignment::Vertical::Bottom})
			{
				label.setTextSize({24, outline});
				label.setAlignment({h, v});
				const auto image = render(label);
				EXPECT_GT(visiblePixels(image), 0u);
				expectClipped(image, label.geometry());
				if (!previous.empty())
				{
					EXPECT_NE(image.pixels, previous);
				}
				previous = image.pixels;
			}
		}
	}
	label.setText("");
	EXPECT_EQ(visiblePixels(render(label)), 0u);
	label.setText("Text");
	label.setGeometry({.anchor = {0, 0}, .size = {0, 0}});
	EXPECT_EQ(visiblePixels(render(label)), 0u);
}

TEST(WidgetRenderIntegrationTest, WrappedMultilineAreaAlignmentsWidthsAndLinePadding)
{
	spk::TextArea area("Area");
	area.setText(U"one two\n\u00e9 longwordwithoutspaces");
	area.setTextSize({20, 1});
	area.setLinePadding(4);
	area.setGlyphColor({1, 0, 0, 1});
	area.setOutlineColor({0, 1, 0, 1});
	area.setDepth(0.2f);
	for (auto h : {spk::Alignment::Horizontal::Left, spk::Alignment::Horizontal::Center, spk::Alignment::Horizontal::Right})
	{
		for (auto v : {spk::Alignment::Vertical::Top, spk::Alignment::Vertical::Center, spk::Alignment::Vertical::Bottom})
		{
			area.setAlignment({h, v});
			area.setGeometry({.anchor = {30, 30}, .size = {160, 200}});
			const auto image = render(area);
			EXPECT_GT(visiblePixels(image), 0u);
			expectClipped(image, area.geometry());
		}
	}
	EXPECT_GT(area.computePreferredSize(60).y, area.computePreferredSize(200).y);
	for (unsigned width : {0u, 1u, 5u})
	{
		area.setGeometry({.anchor = {30, 30}, .size = {width, 40}});
		expectClipped(render(area), area.geometry());
	}
	area.setText("");
	EXPECT_EQ(visiblePixels(render(area)), 0u);
}

TEST(WidgetRenderIntegrationTest, AlternateUncheckedSpriteAndClippedCheckBoxAndOverlay)
{
	spk::CheckableIconButton button("Checkable");
	button.setGeometry({.anchor = {30, 30}, .size = {50, 50}});
	button.setUncheckedSpriteID(1);
	button.setCheckedSpriteID(2);
	button.setChecked(false);
	const auto unchecked = render(button);
	EXPECT_GT(visiblePixels(unchecked), 0u);
	button.setChecked(true);
	EXPECT_NE(render(button).pixels, unchecked.pixels);
	spk::Widget clip("Clip", nullptr);
	clip.setGeometry({.anchor = {30, 30}, .size = {40, 40}});
	spk::CheckBox box("Box", &clip);
	box.setText("Overflowing label");
	box.setSpacing(18);
	box.setIndicatorSize({32, 32});
	box.setGeometry({.anchor = {0, 0}, .size = {300, 64}});
	const auto boxImage = render(box);
	EXPECT_GT(visiblePixels(boxImage), 0u);
	expectClipped(boxImage, clip.geometry());
	spk::DebugOverlay overlay("Overlay", &clip);
	spk::TextLabel text("Text", &overlay);
	text.setMaximalSize({1000, 1000});
	text.setText("Overflowing debug content");
	text.setAlignment({spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top});
	overlay.setWidget(0, 0, &text);
	overlay.setGeometry({.anchor = {0, 0}, .size = {300, 64}});
	overlay.activate();
	const auto overlayImage = render(overlay);
	EXPECT_GT(visiblePixels(overlayImage), 0u);
	expectClipped(overlayImage, clip.geometry());
}

TEST(WidgetRenderIntegrationTest, ProgressFillPixelsForEveryDirectionAndBoundary)
{
	auto white = solidSheet();
	spk::ProgressBar bar("Progress");
	bar.setFillTexture(&white);
	bar.setCornerSize({0, 0});
	bar.setGeometry({.anchor = {20, 20}, .size = {100, 40}});
	bar.setRatio(0.25f);
	for (auto direction : {spk::ProgressBar::FillDirection::LeftToRight, spk::ProgressBar::FillDirection::RightToLeft, spk::ProgressBar::FillDirection::TopToBottom, spk::ProgressBar::FillDirection::BottomToTop})
	{
		bar.setFillDirection(direction);
		const auto image = render(bar);
		EXPECT_EQ(visiblePixels(image), 1000u);
		expectClipped(image, bar.children().front()->viewRegion().viewport);
	}
	bar.setRatio(0);
	EXPECT_EQ(visiblePixels(render(bar)), 0u);
	bar.setRatio(1);
	EXPECT_EQ(visiblePixels(render(bar)), 4000u);
	bar.setGeometry({.anchor = {0, 0}, .size = {1, 1}});
	EXPECT_EQ(visiblePixels(render(bar)), 1u);
	bar.setGeometry({.anchor = {0, 0}, .size = {0, 0}});
	EXPECT_EQ(visiblePixels(render(bar)), 0u);
}

TEST(WidgetRenderIntegrationTest, TextEditSelectionCaretObscuringAndStyleAffectPixels)
{
	spk::TextEdit edit("Edit");
	edit.setGeometry({.anchor = {30, 30}, .size = {200, 40}});
	edit.setText("editable");
	const auto plain = render(edit);
	EXPECT_GT(visiblePixels(plain), 0u);
	edit.notifyFocusAcquired(spk::FocusMode::Channel::Keyboard);
	EXPECT_NE(render(edit).pixels, plain.pixels);
	edit.selectAll();
	const auto selected = render(edit);
	EXPECT_NE(selected.pixels, plain.pixels);
	edit.setObscured(true);
	EXPECT_NE(render(edit).pixels, selected.pixels);
	edit.setTextSize({24, 1});
	edit.setGlyphColor({1, 0, 0, 1});
	edit.setCursorColor({0, 1, 0, 1});
	edit.setSelectionColor({0, 0, 1, 1});
	const auto styled = render(edit);
	EXPECT_NE(styled.pixels, selected.pixels);
	expectClipped(styled, edit.geometry());
	edit.font()->atlas(edit.textSize()).loadAllRenderableGlyphs();
	EXPECT_EQ(render(edit).pixels, styled.pixels);
}

TEST(WidgetRenderIntegrationTest, ComposedControlsRenderTheirDifferentStatesAndPromptBackground)
{
	spk::PushButton push("Push");
	push.setGeometry({.anchor = {20, 20}, .size = {140, 40}});
	push.setText("Click");
	const auto released = render(push);
	spk::Mouse mouse;
	mouse.position = {30, 30};
	spk::MouseButtonPressedRecord record{};
	record.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent event(record, mouse);
	push.dispatch(event);
	EXPECT_NE(render(push).pixels, released.pixels);
	spk::RadioButton radio("Radio");
	radio.setText("Option");
	radio.setGeometry({.anchor = {20, 20}, .size = {140, 40}});
	const auto unchecked = render(radio);
	radio.setChecked(true);
	EXPECT_NE(render(radio).pixels, unchecked.pixels);
	spk::ToggleSwitch toggle("Toggle");
	toggle.setAnimationDuration({});
	toggle.setGeometry({.anchor = {20, 20}, .size = {80, 40}});
	const auto off = render(toggle);
	toggle.setChecked(true);
	EXPECT_NE(render(toggle).pixels, off.pixels);
	spk::PromptPanel prompt("Prompt");
	prompt.setMessage("Message");
	prompt.addButton("ok", "OK");
	prompt.setGeometry({.anchor = {20, 20}, .size = {250, 140}});
	const auto promptImage = render(prompt);
	EXPECT_GT(visiblePixels(promptImage), 0u);
	expectClipped(promptImage, prompt.geometry());
}

// TextRenderCommand currently stores UVs without subscribing to atlas rescaling.
TEST(WidgetRenderIntegrationTest, DISABLED_ExistingTextSnapshotSurvivesAtlasGrowthAndFontMove)
{
	const auto path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path().parent_path() / "resources/fonts/arial.ttf";
	spk::Font font(path);
	spk::Font moved;
	spk::TextLabel label("Text", &font);
	label.setText("A");
	label.setTextSize({48, 2});
	label.setGeometry({.anchor = {30, 30}, .size = {100, 80}});
	auto &context = sparkle_test::OpenGLTestContext::instance();
	context.reset();
	spk::RenderSnapshot::Builder builder;
	label.buildRenderSnapshot(builder);
	const auto snapshot = builder.build();
	snapshot.execute(context.renderContext());
	const auto before = context.capture();
	font.atlas({48, 2}).loadAllRenderableGlyphs();
	moved = std::move(font);
	label.setFont(&moved);
	context.reset();
	snapshot.execute(context.renderContext());
	EXPECT_EQ(context.capture().pixels, before.pixels);
	// The source font remains alive until commands referencing its atlas are destroyed.
}

TEST(WidgetRenderIntegrationTest, ButtonTextIconFlatAndAlignmentMatrixFitsGeometry)
{
	for (bool text : {false, true})
	{
		for (bool icon : {false, true})
		{
			for (bool flat : {false, true})
			{
				spk::PushButton button("Button");
				button.setText(text ? "Text" : "");
				button.setFlat(flat);
				if (icon)
				{
					button.setIcon(spk::Widget::defaultStyle->iconset.get(), 1);
				}
				button.setTextPadding({3, 4});
				button.setIconSize({20, 18});
				button.setIconPadding({5, 6});
				for (auto h : {spk::Alignment::Horizontal::Left, spk::Alignment::Horizontal::Center, spk::Alignment::Horizontal::Right})
				{
					for (auto v : {spk::Alignment::Vertical::Top, spk::Alignment::Vertical::Center, spk::Alignment::Vertical::Bottom})
					{
						button.setAlignment({h, v});
						EXPECT_EQ(button.releasedLabel().alignment(), (spk::Alignment{h, v}));
						for (unsigned width : {0u, 1u, 160u})
						{
							button.setGeometry({.anchor = {20, 20}, .size = {width, 40}});
							EXPECT_LE(button.releasedLabel().geometry().width, width);
							EXPECT_LE(button.releasedIcon().geometry().width, width);
						}
					}
				}
				const auto image = render(button);
				expectClipped(image, button.geometry());
				if (flat && !text && !icon)
				{
					EXPECT_EQ(visiblePixels(image), 0u);
				}
				else
				{
					EXPECT_GT(visiblePixels(image), 0u);
				}
			}
		}
	}
}

TEST(WidgetRenderIntegrationTest, ToggleRawBackgroundsAndNineSliceStylesRenderBothOrientations)
{
	SolidTexture red({255, 0, 0, 255}), green({0, 255, 0, 255});
	for (auto orientation : {spk::Orientation::Horizontal, spk::Orientation::Vertical})
	{
		spk::ToggleSwitch toggle("Toggle");
		toggle.setOrientation(orientation);
		toggle.setAnimationDuration({});
		toggle.setGeometry({.anchor = {20, 20}, .size = {80, 80}});
		toggle.setPadding({5, 7});
		toggle.setThumbSize({20, 30});
		toggle.setUncheckedBackgroundTexture(static_cast<const spk::Texture *>(&red));
		toggle.setCheckedBackgroundTexture(static_cast<const spk::Texture *>(&green));
		const auto off = render(toggle);
		toggle.setChecked(true);
		const auto on = render(toggle);
		EXPECT_NE(off.pixels, on.pixels);
		expectClipped(on, toggle.geometry());
		toggle.applyStyle(spk::Widget::defaultStyle);
		EXPECT_NE(render(toggle).pixels, on.pixels);
		toggle.setPadding({100, 100});
		toggle.setThumbSize({1000, 1000});
		expectClipped(render(toggle), toggle.geometry());
	}
}

TEST(WidgetRenderIntegrationTest, TooltipUsesInheritedChildPassAndRootClipping)
{
	spk::Widget root("Root", nullptr), target("Target", &root);
	root.setGeometry({.anchor = {20, 20}, .size = {100, 60}});
	target.setGeometry({.anchor = {80, 40}, .size = {20, 20}});
	spk::Tooltip tooltip("Tooltip", &root);
	tooltip.setTarget(&target);
	tooltip.setText("A tooltip that wraps");
	tooltip.setMaximumWidth(80);
	tooltip.show();
	const auto image = render(tooltip);
	EXPECT_GT(visiblePixels(image), 0u);
	expectClipped(image, root.geometry());
	EXPECT_EQ(tooltip.textArea().targetRenderPass().name, spk::Widget::TooltipKey.name);
	tooltip.hide();
	EXPECT_EQ(visiblePixels(render(tooltip)), 0u);
}
