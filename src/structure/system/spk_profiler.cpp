#include "structure/system/spk_profiler.hpp"

namespace spk
{
	Profiler::Measurement::Measurement(const std::wstring &p_name) :
		_name(p_name)
	{
	}

	const std::wstring &Profiler::Measurement::name() const
	{
		return _name;
	}

	Profiler::DurationMeasurement::DurationMeasurement(const std::wstring &p_name) :
		Measurement(p_name),
		_count(0),
		_total(0LL, TimeUnit::Nanosecond),
		_min(0LL, TimeUnit::Nanosecond),
		_max(0LL, TimeUnit::Nanosecond),
		_hasSample(false)
	{
	}

	void Profiler::DurationMeasurement::start()
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_chronometer.start();
	}

	void Profiler::DurationMeasurement::stop()
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_chronometer.pause();
		Duration elapsed = _chronometer.elapsedTime();
		_chronometer.stop();
		if (_count == 0)
		{
			_min = elapsed;
			_max = elapsed;
		}
		else
		{
			if (elapsed < _min)
			{
				_min = elapsed;
			}
			if (elapsed > _max)
			{
				_max = elapsed;
			}
		}
		_total += elapsed;
		_count++;
	}

	size_t Profiler::DurationMeasurement::measurementCount() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _count;
	}

	spk::Duration Profiler::DurationMeasurement::totalDuration() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _total;
	}

	spk::Duration Profiler::DurationMeasurement::minimumDuration() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _hasSample ? _min : spk::Duration{};
	}

	spk::Duration Profiler::DurationMeasurement::maximumDuration() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return _hasSample ? _max : spk::Duration{};
	}

	spk::Duration Profiler::DurationMeasurement::averageDuration() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		return ((_count == 0) ? spk::Duration{} : spk::Duration(_total.nanoseconds / _count, spk::TimeUnit::Nanosecond));
	}

	void Profiler::DurationMeasurement::reset()
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_count = 0;
		_total = spk::Duration{};
		_min = spk::Duration{};
		_max = spk::Duration{};
		_hasSample = false;
	}

	spk::JSON::Object Profiler::DurationMeasurement::exportData() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		spk::JSON::Object result(_name);
		result[L"Type"] = std::wstring(L"Duration");
		result[L"Occurence"] = static_cast<long>(_count);
		result[L"TotalMilliseconds"] = static_cast<double>(_total.nanoseconds) / 1'000'000.0;
		result[L"MinMilliseconds"] = static_cast<double>(_min.nanoseconds) / 1'000'000.0;
		result[L"MaxMilliseconds"] = static_cast<double>(_max.nanoseconds) / 1'000'000.0;
		result[L"AverageMilliseconds"] = (_count == 0) ? 0.0 : _total.nanoseconds / 1'000'000.0 / static_cast<double>(_count);
		;
		return result;
	}

	Profiler::CounterMeasurement::CounterMeasurement(const std::wstring &p_name) :
		Measurement(p_name),
		_count(0)
	{
	}

	void Profiler::CounterMeasurement::increment()
	{
		_count.fetch_add(1, std::memory_order_relaxed);
	}

	size_t Profiler::CounterMeasurement::value() const
	{
		return _count.load(std::memory_order_relaxed);
	}

	void Profiler::CounterMeasurement::reset()
	{
		_count.store(0, std::memory_order_relaxed);
	}

	spk::JSON::Object Profiler::CounterMeasurement::exportData() const
	{
		spk::JSON::Object obj;
		obj[L"Name"] = _name;
		obj[L"Type"] = L"Counter";
		obj[L"Value"] = static_cast<long>(_count.load(std::memory_order_relaxed));
		return obj;
	}

	Profiler::Profiler() = default;

	Profiler::~Profiler()
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		_measurements.clear();
	}

	spk::SafePointer<Profiler::DurationMeasurement> Profiler::duration(const std::wstring &p_name)
	{
		return _get<DurationMeasurement>(p_name, [&]() { return std::make_unique<DurationMeasurement>(p_name); });
	}

	spk::SafePointer<Profiler::CounterMeasurement> Profiler::counter(const std::wstring &p_name)
	{
		return _get<CounterMeasurement>(p_name, [&]() { return std::make_unique<CounterMeasurement>(p_name); });
	}

	spk::SafePointer<Profiler::Measurement> Profiler::measurement(const std::wstring &p_name)
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		auto it = _measurements.find(p_name);
		if (it == _measurements.end())
		{
			GENERATE_ERROR("Measurement [" + spk::StringUtils::wstringToString(p_name) + "] does not exist");
		}
		return spk::SafePointer<Measurement>(it->second.get());
	}

	spk::JSON::Object Profiler::exportData() const
	{
		std::lock_guard<std::recursive_mutex> lock(_mutex);
		spk::JSON::Object root;
		for (const auto &[key, value] : _measurements)
		{
			root[key] = value->exportData();
		}
		return root;
	}
}
