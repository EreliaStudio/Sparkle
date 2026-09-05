#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/combo_box.hpp"

namespace
{
	class NullComboDelegate final : public spk::ComboBox::ModelView::Delegate
	{
	public:
		std::unique_ptr<spk::Widget> createItem(std::string, spk::Widget *) override
		{
			return nullptr;
		}
		void bindItem(spk::Widget &, const spk::ComboBox::Model &, std::size_t, bool) override
		{
		}
		unsigned int rowExtent(const spk::ComboBox::Model &, std::size_t) const override
		{
			return 20;
		}
	};

	class ForeignComboDelegate final : public spk::ComboBox::ModelView::Delegate
	{
	public:
		std::unique_ptr<spk::Widget> createItem(std::string name, spk::Widget *) override
		{
			return std::make_unique<spk::TextLabel>(std::move(name));
		}
		void bindItem(spk::Widget &, const spk::ComboBox::Model &, std::size_t, bool) override
		{
		}
		unsigned int rowExtent(const spk::ComboBox::Model &, std::size_t) const override
		{
			return 20;
		}
	};

	void expectWidgetImage(spk::Widget &widget, const std::filesystem::path &category, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();

		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		spk::RenderSnapshot snapshot = builder.build();
		snapshot.execute(context.renderContext());

		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);

		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing golden image: " << expected << "\n"
			<< "The current render was saved to: " << actual;

		const sparkle_test::ImageComparisonResult result =
			sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches)
			<< "Image mismatch for [" << category.string() << "/" << name << "]\n"
			<< "Different pixels: " << result.differentPixelCount << "\n"
			<< "Actual size: " << result.actualWidth << "x" << result.actualHeight << "\n"
			<< "Expected size: " << result.expectedWidth << "x" << result.expectedHeight << "\n"
			<< "Difference image: " << difference;
	}

	[[nodiscard]] const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}
}

TEST(ComboBoxTest, DefaultState)
{
	spk::ComboBox combo("Combo");
	EXPECT_FALSE(combo.isOpen());
	EXPECT_EQ(combo.model(), nullptr);
	EXPECT_FALSE(combo.selectedRow().has_value());
	EXPECT_FALSE(combo.selectedRowID().has_value());
	EXPECT_FALSE(combo.selection().has_value());
	EXPECT_EQ(combo.maximumVisibleRows(), 0u);
	EXPECT_EQ(combo.openButtonWidth(), 32u);
}

TEST(ComboBoxTest, ModelSelectionAndClearSelection)
{
	spk::ComboBox::Model model{"Alpha", "Beta", "Gamma"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	EXPECT_EQ(combo.model(), &model);
	combo.setSelectedRow(1);
	ASSERT_TRUE(combo.selectedRow().has_value());
	EXPECT_EQ(*combo.selectedRow(), 1u);
	EXPECT_EQ(combo.selectedRowID(), model.rowID(1));
	EXPECT_TRUE(combo.selection().has_value());
	EXPECT_NE(combo.selectedWidget(), nullptr);
	combo.clearSelection();
	EXPECT_FALSE(combo.selectedRow().has_value());
	EXPECT_EQ(combo.selectedWidget(), nullptr);
}

TEST(ComboBoxTest, SelectionSubscriptionReportsChanges)
{
	spk::ComboBox::Model model{"Alpha", "Beta"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	int notifications = 0;
	auto contract = combo.subscribeToSelection([&](const auto &) {
		++notifications;
	});
	combo.setSelectedRow(0);
	combo.setSelectedRow(1);
	combo.clearSelection();
	EXPECT_EQ(notifications, 3);
}

TEST(ComboBoxTest, OpenAndCloseRoundTrip)
{
	spk::ComboBox::Model model{"Alpha", "Beta"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.open();
	EXPECT_TRUE(combo.isOpen());
	combo.close();
	EXPECT_FALSE(combo.isOpen());
}

TEST(ComboBoxTest, PlaceholderOverloadsRoundTrip)
{
	spk::ComboBox combo("Combo");
	combo.setPlaceholder("Choose one");
	EXPECT_EQ(combo.placeholder(), spk::Font::textFromUTF8("Choose one"));
	const spk::Font::Text text = U"Nothing selected";
	combo.setPlaceholder(text);
	EXPECT_EQ(combo.placeholder(), text);
}

TEST(ComboBoxTest, MaximumVisibleRowsAndOpenButtonWidthRoundTrip)
{
	spk::ComboBox combo("Combo");
	combo.setMaximumVisibleRows(4);
	combo.setOpenButtonWidth(48);
	EXPECT_EQ(combo.maximumVisibleRows(), 4u);
	EXPECT_EQ(combo.openButtonWidth(), 48u);
}

TEST(ComboBoxTest, ModelMutationKeepsSelectionByStableRowId)
{
	spk::ComboBox::Model model{"A", "B", "C"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setSelectedRow(1);
	const auto id = combo.selectedRowID();
	model.insert(0, "Before");
	EXPECT_EQ(combo.selectedRowID(), id);
	ASSERT_TRUE(combo.selectedRow().has_value());
	EXPECT_EQ(*combo.selectedRow(), 2u);
	model.erase(2);
	EXPECT_FALSE(combo.selectedRow().has_value());
}

TEST(ComboBoxTest, SetModelNullDetachesAndClearsSelection)
{
	spk::ComboBox::Model model{"A", "B"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setSelectedRow(0);
	combo.setModel(nullptr);
	EXPECT_EQ(combo.model(), nullptr);
	EXPECT_FALSE(combo.selectedRow().has_value());
}

TEST(ComboBoxTest, InvalidSelectionIsRejected)
{
	spk::ComboBox::Model model{"A"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	EXPECT_THROW(combo.setSelectedRow(5), std::out_of_range);
}

TEST(ComboBoxTest, ChildAccessorsAreStable)
{
	spk::ComboBox combo("Combo");
	const spk::ComboBox &constant = combo;
	EXPECT_EQ(&constant.background(), &combo.background());
	EXPECT_EQ(&constant.displayLabel(), &combo.displayLabel());
	EXPECT_EQ(&constant.openButton(), &combo.openButton());
	EXPECT_EQ(&constant.view(), &combo.view());
	EXPECT_EQ(&constant.popupBackground(), &combo.popupBackground());
	EXPECT_EQ(&constant.popup(), &combo.popup());
	EXPECT_EQ(constant.displayItem(), combo.displayItem());
}

TEST(ComboBoxTest, ApplyStyleAndNullDelegateAreSupported)
{
	spk::ComboBox combo("Combo");
	combo.applyStyle(defaultStyle());
	EXPECT_NO_THROW(combo.setItemDelegate(nullptr));
}

TEST(ComboBoxTest, InvalidDisplayDelegatesAreRejected)
{
	spk::ComboBox::Model model{"Alpha"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setSelectedRow(0);
	NullComboDelegate nullDelegate;
	EXPECT_THROW(combo.setItemDelegate(&nullDelegate), std::invalid_argument);
	ForeignComboDelegate foreignDelegate;
	EXPECT_THROW(combo.setItemDelegate(&foreignDelegate), std::invalid_argument);
}

TEST(ComboBoxTest, EmptyModelAndVisibleRowBoundaryRemainCoherent)
{
	spk::ComboBox::Model model;
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setMaximumVisibleRows(1);
	combo.open();
	EXPECT_TRUE(combo.isOpen());
	EXPECT_FALSE(combo.selection().has_value());
	combo.setMaximumVisibleRows(0);
	combo.close();
	EXPECT_FALSE(combo.isOpen());
}

TEST(ComboBoxTest, OpenButtonClickTogglesPopup)
{
	spk::ComboBox::Model model{"Alpha", "Beta"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setGeometry({.anchor = {0, 0}, .size = {240, 40}});
	auto &button = combo.openButton();
	spk::Mouse mouse;
	mouse.position = button.viewRegion().viewport.anchor + spk::Vector2Int{1, 1};
	auto click = [&] {
		spk::MouseButtonPressedRecord p{};
		p.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent pressed(p, mouse);
		button.dispatch(pressed);
		spk::MouseButtonReleasedRecord r{};
		r.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent released(r, mouse);
		button.dispatch(released);
	};
	click();
	EXPECT_TRUE(combo.isOpen());
	click();
	EXPECT_FALSE(combo.isOpen());
}

TEST(ComboBoxTest, SelectingAViewRowUpdatesDisplayAndClosesPopup)
{
	spk::ComboBox::Model model{"Alpha", "Beta", "Gamma"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setGeometry({.anchor = {0, 0}, .size = {240, 40}});
	combo.open();
	spk::Mouse mouse;
	mouse.position = combo.view().viewRegion().viewport.anchor + spk::Vector2Int{2, 22};
	spk::MouseButtonPressedRecord record{};
	record.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent event(record, mouse);
	combo.view().dispatch(event);
	ASSERT_TRUE(combo.selectedRow().has_value());
	EXPECT_EQ(*combo.selectedRow(), 1u);
	EXPECT_NE(combo.displayItem(), nullptr);
	EXPECT_FALSE(combo.isOpen());
}

TEST(ComboBoxRenderTest, DISABLED_Placeholder)
{
	spk::ComboBox combo("Combo");
	combo.setPlaceholder("Select an item");
	combo.setGeometry({.anchor = {60, 60}, .size = {300, 48}});
	combo.activate();
	expectWidgetImage(combo, "ui/widget/combo_box", "placeholder");
}

TEST(ComboBoxRenderTest, DISABLED_SelectedItem)
{
	spk::ComboBox::Model model{"Alpha", "Beta", "Gamma"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setSelectedRow(1);
	combo.setGeometry({.anchor = {60, 60}, .size = {300, 48}});
	combo.activate();
	expectWidgetImage(combo, "ui/widget/combo_box", "selected_item");
}

TEST(ComboBoxRenderTest, DISABLED_OpenPopup)
{
	spk::ComboBox::Model model{"Alpha", "Beta", "Gamma", "Delta", "Epsilon"};
	spk::ComboBox combo("Combo");
	combo.setModel(&model);
	combo.setSelectedRow(2);
	combo.setMaximumVisibleRows(3);
	combo.setGeometry({.anchor = {60, 60}, .size = {300, 48}});
	combo.open();
	combo.activate();
	expectWidgetImage(combo, "ui/widget/combo_box", "open_popup");
}
