#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <variant>

#include "core/context/render_context.hpp"
#include "core/context/update_context.hpp"
#include "core/event/platform_request.hpp"
#include "core/event/record.hpp"
#include "core/event/render_request.hpp"
#include "core/event/update_request.hpp"
#include "engine/reference_frame.hpp"
#include "graphics/color.hpp"
#include "graphics/opengl/padding.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "type/activation_status.hpp"
#include "type/alignment.hpp"
#include "type/focus_mode.hpp"
#include "type/orientation.hpp"
#include "ui/view_region.hpp"
#include "ui/widget/scalable_widget.hpp"

namespace
{

	spk::Rect2D makeRect(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height)
	{
		spk::Rect2D result;
		result.anchor = {x, y};
		result.size = {width, height};
		return result;
	}
	template <typename TVariant, typename TAlternative>
	inline constexpr bool VariantContains = []<std::size_t... Indices>(std::index_sequence<Indices...>)
	{
		return (std::is_same_v<std::variant_alternative_t<Indices, TVariant>, TAlternative> || ...);
	}(std::make_index_sequence<std::variant_size_v<TVariant>>{});

	static_assert(std::is_aggregate_v<spk::Alignment>);
	static_assert(std::is_trivially_copyable_v<spk::Alignment>);
	static_assert(std::is_aggregate_v<spk::Color>);
	static_assert(std::is_trivially_copyable_v<spk::Color>);
	static_assert(std::is_aggregate_v<spk::ViewRegion>);
	static_assert(std::is_trivially_copyable_v<spk::ViewRegion>);
	static_assert(std::is_aggregate_v<spk::RenderContext>);
	static_assert(std::is_aggregate_v<spk::UpdateContext>);
	static_assert(std::is_trivially_copyable_v<spk::Padding<1>>);
	static_assert(std::is_trivially_copyable_v<spk::Padding<4>>);
	static_assert(sizeof(spk::Padding<4>) == sizeof(float) * 4);

	static_assert(std::variant_size_v<spk::PlatformRequest> == 3);
	static_assert(VariantContains<spk::PlatformRequest, spk::NativeRegistrationRequest>);
	static_assert(VariantContains<spk::PlatformRequest, spk::NativeDeletionRequest>);
	static_assert(VariantContains<spk::PlatformRequest, spk::MousePositionRequest>);

	static_assert(std::variant_size_v<spk::RenderRequest> == 4);
	static_assert(VariantContains<spk::RenderRequest, spk::SurfaceRegistrationRequest>);
	static_assert(VariantContains<spk::RenderRequest, spk::SurfaceCreationRequest>);
	static_assert(VariantContains<spk::RenderRequest, spk::SurfaceResizeRequest>);
	static_assert(VariantContains<spk::RenderRequest, spk::SurfaceDeletionRequest>);

	static_assert(std::variant_size_v<spk::UpdateRequest> == 2);
	static_assert(VariantContains<spk::UpdateRequest, spk::StateRegistrationRequest>);
	static_assert(VariantContains<spk::UpdateRequest, spk::StateDeletionRequest>);

	static_assert(std::variant_size_v<spk::EventRecord> == 14);
	static_assert(VariantContains<spk::EventRecord, spk::WindowResizedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::WindowMovedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::WindowFocusGainedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::WindowFocusLostRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::MouseEnteredRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::MouseLeftRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::MouseMovedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::MouseWheelScrolledRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::MouseButtonPressedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::MouseButtonReleasedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::MouseButtonDoubleClickedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::KeyPressedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::KeyReleasedRecord>);
	static_assert(VariantContains<spk::EventRecord, spk::TextInputRecord>);

	TEST(PublicTypesTest, StandardUsageInitializesAndComparesCommonEnumsAndRecords)
	{
		const spk::Alignment defaultAlignment{};
		const spk::Alignment centered{spk::Alignment::Horizontal::Center, spk::Alignment::Vertical::Center};
		const spk::Color defaultColor{};
		const spk::Color color{0.25f, 0.5f, 0.75f, 0.125f};
		const spk::ViewRegion region{
			makeRect(10, 20, 100u, 50u),
			makeRect(20, 25, 80u, 40u)};

		EXPECT_EQ(defaultAlignment, (spk::Alignment{spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top}));
		EXPECT_EQ(centered.horizontal, spk::Alignment::Horizontal::Center);
		EXPECT_EQ(centered.vertical, spk::Alignment::Vertical::Center);
		EXPECT_EQ(defaultColor, (spk::Color{0.0f, 0.0f, 0.0f, 1.0f}));
		EXPECT_EQ(color, (spk::Color{0.25f, 0.5f, 0.75f, 0.125f}));
		EXPECT_EQ(region.viewport, makeRect(10, 20, 100u, 50u));
		EXPECT_EQ(region.scissor, makeRect(20, 25, 80u, 40u));
	}

	TEST(PublicTypesTest, EnumNumericMappingsRemainStable)
	{
		EXPECT_EQ(static_cast<int>(spk::Alignment::Horizontal::Left), 0);
		EXPECT_EQ(static_cast<int>(spk::Alignment::Horizontal::Center), 1);
		EXPECT_EQ(static_cast<int>(spk::Alignment::Horizontal::Right), 2);
		EXPECT_EQ(static_cast<int>(spk::Alignment::Vertical::Top), 0);
		EXPECT_EQ(static_cast<int>(spk::Alignment::Vertical::Center), 1);
		EXPECT_EQ(static_cast<int>(spk::Alignment::Vertical::Bottom), 2);

		EXPECT_EQ(static_cast<int>(spk::ActivationStatus::Activated), 0);
		EXPECT_EQ(static_cast<int>(spk::ActivationStatus::Deactivated), 1);
		EXPECT_EQ(static_cast<int>(spk::Orientation::Vertical), 0);
		EXPECT_EQ(static_cast<int>(spk::Orientation::Horizontal), 1);
		EXPECT_EQ(static_cast<int>(spk::ReferenceFrame::Local), 0);
		EXPECT_EQ(static_cast<int>(spk::ReferenceFrame::World), 1);
	}

	TEST(PublicTypesTest, ValueInitializedEnumsSelectTheirFirstEnumerator)
	{
		EXPECT_EQ(spk::ActivationStatus{}, spk::ActivationStatus::Activated);
		EXPECT_EQ(spk::Orientation{}, spk::Orientation::Vertical);
		EXPECT_EQ(spk::ReferenceFrame{}, spk::ReferenceFrame::Local);
		EXPECT_EQ(spk::FocusMode::Channel{}, spk::FocusMode::Channel::Keyboard);
		EXPECT_EQ(spk::FocusMode::ChangeType{}, spk::FocusMode::ChangeType::Take);
	}

	TEST(PublicTypesTest, FocusModeChannelsAndDefaultTargetsHaveStableLayout)
	{
		spk::FocusMode mode{};

		EXPECT_EQ(spk::FocusMode::ChannelCount, 2u);
		EXPECT_EQ(static_cast<std::size_t>(spk::FocusMode::Channel::Keyboard), 0u);
		EXPECT_EQ(static_cast<std::size_t>(spk::FocusMode::Channel::Mouse), 1u);
		EXPECT_EQ(mode.targetWidgets[0], nullptr);
		EXPECT_EQ(mode.targetWidgets[1], nullptr);

		const spk::FocusMode::Record record{spk::FocusMode::ChangeType::Release, nullptr};
		EXPECT_EQ(record.type, spk::FocusMode::ChangeType::Release);
		EXPECT_EQ(record.widget, nullptr);
	}

	TEST(PublicTypesTest, PaddingValueInitializesEveryFloatToZero)
	{
		const spk::Padding<4> padding{};

		for (const float value : padding.values)
		{
			EXPECT_FLOAT_EQ(value, 0.0f);
		}
	}

	TEST(PublicTypesTest, PaddingMacroContributesTheRequestedNumberOfFloats)
	{
		struct PaddedRecord
		{
			float before = 1.0f;
			SPK_PADDING(3);
			float after = 2.0f;
		};

		static_assert(std::is_trivially_copyable_v<PaddedRecord>);
		static_assert(sizeof(PaddedRecord) == sizeof(float) * 5);

		const PaddedRecord value{};
		EXPECT_FLOAT_EQ(value.before, 1.0f);
		EXPECT_FLOAT_EQ(value.after, 2.0f);
	}

	TEST(PublicTypesTest, RenderContextAggregateValueInitializationUsesNullTargetSurface)
	{
		const spk::RenderContext context{};

		EXPECT_EQ(context.targetSurface, nullptr);
	}

	TEST(PublicTypesTest, UpdateContextExposesDurationAndInputReferenceMembers)
	{
		static_assert(std::is_same_v<decltype(spk::UpdateContext::time), std::chrono::steady_clock::duration>);
		static_assert(std::is_same_v<decltype(spk::UpdateContext::deltaTime), std::chrono::steady_clock::duration>);
		static_assert(std::is_same_v<decltype(spk::UpdateContext::keyboard), const spk::Keyboard &>);
		static_assert(std::is_same_v<decltype(spk::UpdateContext::mouse), const spk::Mouse &>);

		SUCCEED();
	}

	TEST(PublicTypesTest, PlatformRequestSimpleRecordsAggregateInitializeAndSelectExpectedVariant)
	{
		const spk::NativeDeletionRequest deletion{"main"};
		const spk::MousePositionRequest mousePosition{"main", {12, -8}};
		const spk::PlatformRequest first = deletion;
		const spk::PlatformRequest second = mousePosition;

		ASSERT_TRUE(std::holds_alternative<spk::NativeDeletionRequest>(first));
		EXPECT_EQ(std::get<spk::NativeDeletionRequest>(first).windowIdentifier, "main");
		ASSERT_TRUE(std::holds_alternative<spk::MousePositionRequest>(second));
		EXPECT_EQ(std::get<spk::MousePositionRequest>(second).position, spk::Vector2Int(12, -8));
	}

	TEST(PublicTypesTest, RenderRequestSimpleRecordsAggregateInitializeAndSelectExpectedVariant)
	{
		const spk::SurfaceCreationRequest creation{"main", {}};
		const spk::SurfaceResizeRequest resize{"main", {1920u, 1080u}};
		const spk::SurfaceDeletionRequest deletion{"main"};

		const spk::RenderRequest creationVariant = creation;
		const spk::RenderRequest resizeVariant = resize;
		const spk::RenderRequest deletionVariant = deletion;

		EXPECT_TRUE(std::holds_alternative<spk::SurfaceCreationRequest>(creationVariant));
		ASSERT_TRUE(std::holds_alternative<spk::SurfaceResizeRequest>(resizeVariant));
		EXPECT_EQ(std::get<spk::SurfaceResizeRequest>(resizeVariant).newSize, spk::Vector2UInt(1920u, 1080u));
		EXPECT_TRUE(std::holds_alternative<spk::SurfaceDeletionRequest>(deletionVariant));
	}

	TEST(PublicTypesTest, UpdateRequestDeletionRecordAggregateInitializesAndSelectsExpectedVariant)
	{
		const spk::StateDeletionRequest deletion{"main"};
		const spk::UpdateRequest request = deletion;

		ASSERT_TRUE(std::holds_alternative<spk::StateDeletionRequest>(request));
		EXPECT_EQ(std::get<spk::StateDeletionRequest>(request).windowIdentifier, "main");
	}

	TEST(PublicTypesTest, EventRecordsAggregateInitializeAndSelectExpectedVariant)
	{
		const spk::WindowResizedRecord resized{{"main"}, {800u, 600u}};
		const spk::MouseMovedRecord moved{{"main"}, {10, 20}};
		const spk::MouseWheelScrolledRecord wheel{{"main"}, {0.0f, -1.0f}};
		const spk::TextInputRecord text{{"main"}, U'X'};

		const spk::EventRecord resizedVariant = resized;
		const spk::EventRecord movedVariant = moved;
		const spk::EventRecord wheelVariant = wheel;
		const spk::EventRecord textVariant = text;

		ASSERT_TRUE(std::holds_alternative<spk::WindowResizedRecord>(resizedVariant));
		EXPECT_EQ(std::get<spk::WindowResizedRecord>(resizedVariant).size, spk::Vector2UInt(800u, 600u));
		ASSERT_TRUE(std::holds_alternative<spk::MouseMovedRecord>(movedVariant));
		EXPECT_EQ(std::get<spk::MouseMovedRecord>(movedVariant).position, spk::Vector2Int(10, 20));
		ASSERT_TRUE(std::holds_alternative<spk::MouseWheelScrolledRecord>(wheelVariant));
		EXPECT_EQ(std::get<spk::MouseWheelScrolledRecord>(wheelVariant).value, spk::Vector2(0.0f, -1.0f));
		ASSERT_TRUE(std::holds_alternative<spk::TextInputRecord>(textVariant));
		EXPECT_EQ(std::get<spk::TextInputRecord>(textVariant).glyph, U'X');
	}

	TEST(PublicTypesTest, ClearRenderCommandMaskSupportsBitwiseComposition)
	{
		using Mask = spk::ClearRenderCommand::Mask;

		EXPECT_EQ(static_cast<std::uint8_t>(Mask::None), 0u);
		EXPECT_EQ(static_cast<std::uint8_t>(Mask::Color), 1u);
		EXPECT_EQ(static_cast<std::uint8_t>(Mask::Depth), 2u);
		EXPECT_EQ(static_cast<std::uint8_t>(Mask::Stencil), 4u);
		EXPECT_EQ(static_cast<std::uint8_t>(Mask::All), 7u);
		EXPECT_EQ(Mask::Color | Mask::Depth | Mask::Stencil, Mask::All);
		EXPECT_EQ((Mask::All & Mask::Depth), Mask::Depth);
		EXPECT_EQ((Mask::Color & Mask::Depth), Mask::None);
	}

	TEST(PublicTypesTest, ScalableWidgetEdgesUseIndependentComposableBits)
	{
		using Widget = spk::ScalableWidget;
		const Widget::Edges leftTop = static_cast<Widget::Edges>(Widget::Left | Widget::Top);
		const Widget::Edges all = static_cast<Widget::Edges>(Widget::Left | Widget::Right | Widget::Top | Widget::Bottom);

		EXPECT_EQ(static_cast<std::uint8_t>(Widget::None), 0u);
		EXPECT_EQ(static_cast<std::uint8_t>(Widget::Left), 1u);
		EXPECT_EQ(static_cast<std::uint8_t>(Widget::Right), 2u);
		EXPECT_EQ(static_cast<std::uint8_t>(Widget::Top), 4u);
		EXPECT_EQ(static_cast<std::uint8_t>(Widget::Bottom), 8u);
		EXPECT_TRUE(Widget::containsEdge(leftTop, Widget::Left));
		EXPECT_TRUE(Widget::containsEdge(leftTop, Widget::Top));
		EXPECT_FALSE(Widget::containsEdge(leftTop, Widget::Right));
		EXPECT_FALSE(Widget::containsEdge(leftTop, Widget::Bottom));
		EXPECT_TRUE(Widget::containsEdge(all, Widget::Left));
		EXPECT_TRUE(Widget::containsEdge(all, Widget::Right));
		EXPECT_TRUE(Widget::containsEdge(all, Widget::Top));
		EXPECT_TRUE(Widget::containsEdge(all, Widget::Bottom));
		EXPECT_FALSE(Widget::containsEdge(Widget::None, Widget::Left));
	}
}
