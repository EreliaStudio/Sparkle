#pragma once

#include <chrono>
#include <functional>
#include <string>

#include "ui/widget/text_label.hpp"

namespace spk
{
	class DynamicTextLabel : public TextLabel
	{
	public:
		using Duration = std::chrono::steady_clock::duration;
		using TextProducer = std::function<std::string()>;

	private:
		TextProducer _producer;
		Duration _refreshDuration = std::chrono::seconds(1);
		Duration _elapsed{};

		void _updateState(UpdateContext &context) override;

	public:
		explicit DynamicTextLabel(std::string name, Widget *parent = nullptr);
		DynamicTextLabel(std::string name, Font *font, TextProducer producer = {}, Widget *parent = nullptr);

		void setTextProducer(TextProducer producer);
		void setRefreshDuration(Duration duration);
		void refresh();

		[[nodiscard]] const TextProducer &textProducer() const noexcept;
		[[nodiscard]] Duration refreshDuration() const noexcept;
	};
}
