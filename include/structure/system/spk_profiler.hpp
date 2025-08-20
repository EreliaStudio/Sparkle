#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>

#include "structure/container/spk_json_object.hpp"
#include "structure/design_pattern/spk_singleton.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/time/spk_chronometer.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk
{
	class Profiler : public spk::Singleton<Profiler>
	{
	public:
		using Instanciator = spk::Singleton<Profiler>::Instanciator;

		class Measurement
		{
		protected:
			std::wstring _name;

		public:
			explicit Measurement(const std::wstring &p_name);
			virtual ~Measurement() = default;

			// Required API
			virtual void reset() = 0;
			virtual spk::JSON::Object exportData() const = 0;

			const std::wstring &name() const;
		};

		class DurationMeasurement : public Measurement
		{
		private:
			spk::Chronometer _chronometer;
			size_t _count;
			spk::Duration _total;
			spk::Duration _min;
			spk::Duration _max;
			bool _hasSample;
			mutable std::recursive_mutex _mutex;

		public:
			explicit DurationMeasurement(const std::wstring &p_name);

			// Timing API
			void start();
			void stop();

			// Getters (thread-safe)
			size_t measurementCount() const;
			spk::Duration totalDuration() const;
			spk::Duration minimumDuration() const; // returns zero if no sample yet
			spk::Duration maximumDuration() const; // returns zero if no sample yet
			spk::Duration averageDuration() const; // returns zero if no sample yet

			// Maintenance
			void reset() override;

			// Serialization
			spk::JSON::Object exportData() const override;
		};

		class CounterMeasurement : public Measurement
		{
		private:
			std::atomic<size_t> _count;

		public:
			explicit CounterMeasurement(const std::wstring &p_name);

			// Counter API
			void increment();
			size_t value() const;

			// Maintenance
			void reset() override;

			// Serialization
			spk::JSON::Object exportData() const override;
		};

	private:
		std::map<std::wstring, std::unique_ptr<Measurement>> _measurements;
		mutable std::recursive_mutex _mutex;

		template <typename TMeasurement, typename Creator>
		spk::SafePointer<TMeasurement> _get(const std::wstring &p_name, const Creator &p_creator)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			auto it = _measurements.find(p_name);
			if (it == _measurements.end())
			{
				_measurements[p_name] = std::move(p_creator());
				return spk::SafePointer<TMeasurement>(static_cast<TMeasurement *>(_measurements[p_name].get()));
			}
			TMeasurement *result = dynamic_cast<TMeasurement *>(it->second.get());
			if (result == nullptr)
			{
				GENERATE_ERROR("Measurement [" + spk::StringUtils::wstringToString(p_name) + "] is not the correct type");
			}
			return spk::SafePointer<TMeasurement>(result);
		}

	public:
		Profiler();
		~Profiler();

		spk::SafePointer<DurationMeasurement> duration(const std::wstring &p_name);
		spk::SafePointer<CounterMeasurement> counter(const std::wstring &p_name);
		spk::SafePointer<Measurement> measurement(const std::wstring &p_name);

		spk::JSON::Object exportData() const;
	};
}
