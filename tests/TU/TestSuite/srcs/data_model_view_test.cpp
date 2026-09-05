#include <gtest/gtest.h>

#include "ui/data_model_view.hpp"
#include "ui/text_model_view.hpp"
#include <cstdlib>
#include <set>

namespace
{
	using Model = spk::DataModel<int>;
	struct Item : spk::Widget
	{
		int value = 0;
		Model::RowID id = 0;
		bool enabled = true;
		bool selected = false;
		using Widget::Widget;
	};
	struct Delegate : Model::View::Delegate
	{
		int creations = 0;
		std::unique_ptr<spk::Widget> createItem(std::string name, spk::Widget *parent) override
		{
			++creations;
			return std::make_unique<Item>(std::move(name), parent);
		}
		void bindItem(spk::Widget &widget, const Model &model, std::size_t row, bool selected) override
		{
			auto &item = dynamic_cast<Item &>(widget);
			item.value = model.data(row);
			item.id = model.rowID(row);
			item.enabled = model.isEnabled(row);
			item.selected = selected;
		}
		unsigned rowExtent(const Model &, std::size_t) const override
		{
			return 20;
		}
	};
	std::vector<Item *> activeItems(Model::View &view)
	{
		std::vector<Item *> result;
		for (auto *child : view.children())
		{
			if (child->isActive())
			{
				result.push_back(dynamic_cast<Item *>(child));
			}
		}
		std::ranges::sort(result, {}, [](const auto *item) {
			return item->geometry().y;
		});
		return result;
	}
}

TEST(DataModelViewTest, BindingSelectionScrollingAndDelegateReusePreserveIdentity)
{
	Model model{10, 20, 30, 40, 50};
	Delegate delegate;
	Model::View view("View");
	view.setGeometry({.anchor = {10, 10}, .size = {100, 40}});
	view.setDelegate(&delegate);
	view.setModel(&model);
	ASSERT_EQ(activeItems(view).size(), 2u);
	EXPECT_EQ(delegate.creations, 2);
	EXPECT_EQ(activeItems(view)[0]->value, 10);
	EXPECT_EQ(activeItems(view)[1]->geometry(), (spk::Rect2D{.anchor = {0, 20}, .size = {100, 20}}));
	for (auto *item : activeItems(view))
	{
		EXPECT_EQ(item->parent(), &view);
	}
	int selections = 0;
	auto contract = view.subscribeToSelection([&](auto selection) {
		++selections;
		EXPECT_EQ(selection, view.selection());
	});
	view.setSelectedRow(1);
	auto *selected = view.selectedWidget();
	EXPECT_EQ(view.selectedRowID(), model.rowID(1));
	EXPECT_TRUE(dynamic_cast<Item *>(selected)->selected);
	view.setSelectedRow(1);
	EXPECT_EQ(selections, 1);
	view.scrollTo(2);
	EXPECT_EQ(view.selectedWidget(), selected);
	EXPECT_EQ(delegate.creations, 2);
	view.scrollTo(4);
	EXPECT_EQ(dynamic_cast<Item *>(view.selectedWidget())->id, model.rowID(1));
	EXPECT_EQ(delegate.creations, 3); // Offscreen selection retains its item.
	EXPECT_EQ(static_cast<const Model::View &>(view).selectedWidget(), view.selectedWidget());
	view.setSelectedRow(std::nullopt);
	EXPECT_EQ(view.selectedWidget(), nullptr);
	EXPECT_EQ(selections, 2);
}

TEST(DataModelViewTest, ReactiveEditsPreserveRowIDAndClearRemovedSelection)
{
	Model model{10, 20, 30};
	Delegate delegate;
	Model::View view("View");
	view.setGeometry({.anchor = {0, 0}, .size = {100, 60}});
	view.setDelegate(&delegate);
	view.setModel(&model);
	view.setSelectedRow(1);
	const auto id = view.selectedRowID();
	int notifications = 0;
	auto contract = view.subscribeToSelection([&](auto) {
		++notifications;
	});
	model.insert(0, 5);
	EXPECT_EQ(view.selectedRow(), 2u);
	EXPECT_EQ(view.selectedRowID(), id);
	model.setData(2, 99);
	EXPECT_EQ(dynamic_cast<Item *>(view.selectedWidget())->value, 99);
	model.setEnabled(2, false);
	EXPECT_FALSE(dynamic_cast<Item *>(view.selectedWidget())->enabled);
	EXPECT_EQ(notifications, 0);
	model.erase(2);
	EXPECT_FALSE(view.selection().has_value());
	EXPECT_EQ(notifications, 1);
	model.clear();
	EXPECT_TRUE(view.children().empty());
	EXPECT_EQ(view.preferredSize().y, 0);
}

TEST(DataModelViewTest, EmptyLargeZeroPartialAndExactPageGeometry)
{
	Model model;
	Delegate delegate;
	Model::View view("View");
	view.setDelegate(&delegate);
	view.setModel(&model);
	EXPECT_TRUE(view.children().empty());
	view.setModel(nullptr);
	for (int i = 0; i < 1000; ++i)
	{
		model.append(i);
	}
	view.setModel(&model);
	EXPECT_TRUE(view.children().empty());
	for (const auto [height, count] : {std::pair{1u, 1u}, {19u, 1u}, {20u, 1u}, {21u, 2u}, {40u, 2u}, {0u, 0u}})
	{
		view.setGeometry({.anchor = {0, 0}, .size = {1, height}});
		EXPECT_EQ(activeItems(view).size(), count) << height;
	}
	view.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	view.scrollTo(999);
	ASSERT_EQ(activeItems(view).size(), 2u);
	EXPECT_EQ(activeItems(view).back()->value, 999);
	EXPECT_LE(delegate.creations, 2);
	view.scrollTo(0);
	EXPECT_EQ(activeItems(view).front()->value, 0);
}

TEST(DataModelViewTest, NullReplacementInvalidRowsAndDisabledMouseSelection)
{
	Model model{10, 20};
	Model other{30};
	Delegate delegate;
	Model::View view("View");
	EXPECT_THROW(view.setSelectedRow(0), std::out_of_range);
	EXPECT_THROW(view.scrollTo(0), std::out_of_range);
	view.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	view.setDelegate(&delegate);
	view.setModel(&model);
	EXPECT_THROW(view.setSelectedRow(2), std::out_of_range);
	EXPECT_THROW(view.scrollTo(2), std::out_of_range);
	model.setEnabled(0, false);
	spk::Mouse mouse;
	mouse.position = {5, 5};
	spk::MouseButtonPressedRecord record{};
	record.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent event(record, mouse);
	view.dispatch(event);
	EXPECT_TRUE(event.consumed);
	EXPECT_FALSE(view.selection().has_value());
	view.setSelectedRow(0); // Programmatic selection explicitly permits disabled rows.
	EXPECT_EQ(view.selectedRow(), 0u);
	view.setModel(&other);
	EXPECT_FALSE(view.selection().has_value());
	model.clear();
	EXPECT_EQ(activeItems(view).front()->value, 30);
	view.setModel(nullptr);
	EXPECT_TRUE(view.children().empty());
	view.setDelegate(nullptr);
	EXPECT_EQ(view.delegate(), nullptr);
}

TEST(DataModelViewTest, InvalidDelegateProductsThrowAndAllowRecovery)
{
	struct Invalid : Delegate
	{
		bool foreign = false;
		std::unique_ptr<spk::Widget> createItem(std::string name, spk::Widget *) override
		{
			return foreign ? std::make_unique<spk::Widget>(name, nullptr) : nullptr;
		}
	};
	Model model{1};
	Delegate valid;
	Invalid invalid;
	Model::View view("View");
	view.setGeometry({.anchor = {0, 0}, .size = {100, 20}});
	view.setModel(&model);
	EXPECT_THROW(view.setDelegate(&invalid), std::invalid_argument);
	view.setDelegate(&valid);
	ASSERT_EQ(activeItems(view).size(), 1u);
	invalid.foreign = true;
	EXPECT_THROW(view.setDelegate(&invalid), std::invalid_argument);
	view.setDelegate(&valid);
	EXPECT_EQ(activeItems(view).front()->value, 1);
	// createItem transfers unique ownership: returning the same widget twice violates that API contract.
}

// The view currently leaves its old scroll offset after model shrink or viewport growth.
TEST(DataModelViewTest, DISABLED_ReactiveModelShrinkAndResizeClampScrollOffset)
{
	Model model{1, 2, 3, 4, 5};
	Delegate delegate;
	Model::View view("View");
	view.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	view.setDelegate(&delegate);
	view.setModel(&model);
	view.scrollTo(4);
	model.erase(1, 4);
	ASSERT_EQ(activeItems(view).size(), 1u);
	EXPECT_EQ(activeItems(view).front()->geometry().y, 0);
	model.append(2);
	model.append(3);
	view.scrollTo(2);
	view.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	ASSERT_EQ(activeItems(view).size(), 3u);
	EXPECT_EQ(activeItems(view).front()->geometry().y, 0);
}

TEST(TextModelViewTest, DefaultDelegatePresentsUnicodeEmptyAndDuplicateRowsWithStableSelection)
{
	spk::TextModel model{"", "same", "same", "\xc3\xa9"};
	spk::TextModel::View view("Text");
	view.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
	view.setModel(&model);
	EXPECT_EQ(view.delegate(), &view.defaultDelegate());
	view.setSelectedRow(1);
	const auto firstID = view.selectedRowID();
	ASSERT_NE(dynamic_cast<spk::TextLabel *>(view.selectedWidget()), nullptr);
	EXPECT_EQ(dynamic_cast<spk::TextLabel *>(view.selectedWidget())->text(), U"same");
	view.setSelectedRow(2);
	EXPECT_NE(view.selectedRowID(), firstID);
	view.scrollTo(3);
	view.setSelectedRow(3);
	EXPECT_EQ(dynamic_cast<spk::TextLabel *>(view.selectedWidget())->text(), U"\u00e9");
	const auto id = view.selectedRowID();
	model.insert(0, "front");
	EXPECT_EQ(view.selectedRowID(), id);
	EXPECT_EQ(view.selectedRow(), 4u);
	model.setData(4, "changed");
	EXPECT_EQ(dynamic_cast<spk::TextLabel *>(view.selectedWidget())->text(), U"changed");
	model.erase(4);
	EXPECT_FALSE(view.selectedRow().has_value());
}

TEST(TextModelDelegateTest, PresentationSettingsAndInvalidItemsHaveDefinedBehavior)
{
	spk::TextModel model{"text", ""};
	spk::TextModel::Delegate delegate;
	spk::Widget parent("Parent", nullptr);
	delegate.setTextSize({25, 1});
	delegate.setPadding({7, 3});
	delegate.setRowHeight(42);
	delegate.setGlyphColor({1, 0, 0, 1});
	delegate.setOutlineColor({0, 1, 0, 1});
	auto item = delegate.createItem("Item", &parent);
	ASSERT_EQ(item->parent(), &parent);
	delegate.bindItem(*item, model, 0, false);
	auto &label = dynamic_cast<spk::TextLabel &>(*item);
	EXPECT_EQ(label.text(), U"text");
	EXPECT_EQ(label.textSize(), (spk::Font::Size{25, 1}));
	EXPECT_EQ(label.padding(), spk::Vector2UInt(7, 3));
	EXPECT_EQ(delegate.rowExtent(model, 0), 42u);
	EXPECT_EQ(label.glyphColor(), spk::Color(1, 0, 0, 1));
	EXPECT_EQ(label.outlineColor(), spk::Color(0, 1, 0, 1));
	model.setEnabled(0, false);
	delegate.bindItem(*item, model, 0, true);
	EXPECT_EQ(label.text(), U"text"); // Disabled rows remain displayed; the view gates mouse selection.
	delegate.bindItem(*item, model, 1, false);
	EXPECT_TRUE(label.text().empty());
	EXPECT_THROW(delegate.setFont(nullptr), std::invalid_argument);
	EXPECT_THROW(delegate.bindItem(parent, model, 0, false), std::invalid_argument);
	EXPECT_THROW(delegate.bindItem(*item, model, 2, false), std::out_of_range);
}

// Run this invalid ownership product in a subprocess: accepting it would otherwise
// double-delete the item when the view is destroyed. Exit before that unsafe cleanup.
TEST(DataModelViewTest, DISABLED_DuplicateDelegateWidgetIsRejected)
{
	EXPECT_EXIT(([] {
					struct Duplicate : Delegate
					{
						spk::Widget *first = nullptr;
						std::unique_ptr<spk::Widget> createItem(std::string name, spk::Widget *parent) override
						{
							if (first == nullptr)
							{
								auto item = Delegate::createItem(std::move(name), parent);
								first = item.get();
								return item;
							}
							return std::unique_ptr<spk::Widget>(first);
						}
					};
					Model model{1, 2};
					Duplicate delegate;
					Model::View view("Duplicate");
					view.setGeometry({.anchor = {0, 0}, .size = {100, 40}});
					view.setModel(&model);
					try
					{
						view.setDelegate(&delegate);
					} catch (const std::invalid_argument &)
					{
						std::_Exit(0);
					}
					std::_Exit(1);
				}()),
				testing::ExitedWithCode(0),
				"");
}

TEST(DataModelViewTest, DISABLED_InvalidDelegateReplacementPreservesExistingItems)
{
	struct Invalid : Delegate
	{
		bool foreign = false;
		std::unique_ptr<spk::Widget> createItem(std::string name, spk::Widget *) override
		{
			return foreign ? std::make_unique<spk::Widget>(std::move(name), nullptr) : nullptr;
		}
	};
	for (bool foreign : {false, true})
	{
		Model model{1};
		Delegate valid;
		Invalid invalid;
		invalid.foreign = foreign;
		Model::View view("View");
		view.setGeometry({.anchor = {0, 0}, .size = {100, 20}});
		view.setDelegate(&valid);
		view.setModel(&model);
		view.setSelectedRow(0);
		const auto *item = view.selectedWidget();
		EXPECT_THROW(view.setDelegate(&invalid), std::invalid_argument);
		EXPECT_EQ(view.delegate(), &valid);
		EXPECT_EQ(view.selectedWidget(), item);
		EXPECT_EQ(view.selectedRowID(), model.rowID(0));
	}
}
