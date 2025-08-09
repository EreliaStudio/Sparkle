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
                _max(0LL, TimeUnit::Nanosecond)
        {
        }

        void Profiler::DurationMeasurement::start()
        {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                _chronometer.restart();
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

        spk::JSON::Object Profiler::DurationMeasurement::exportData() const
        {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                spk::JSON::Object result(_name);
                result.addAttribute(L"Type");
                result.addAttribute(L"Occurence");
                result.addAttribute(L"TotalMilliseconds");
                result.addAttribute(L"MinMilliseconds");
                result.addAttribute(L"MaxMilliseconds");
                result.addAttribute(L"AverageMilliseconds");
                result[L"Type"] = std::wstring(L"Duration");
                result[L"Occurence"] = static_cast<long>(_count);
                double totalMs = static_cast<double>(_total.nanoseconds) / 1'000'000.0;
                double minMs = static_cast<double>(_min.nanoseconds) / 1'000'000.0;
                double maxMs = static_cast<double>(_max.nanoseconds) / 1'000'000.0;
                double avgMs = (_count == 0) ? 0.0 : totalMs / static_cast<double>(_count);
                result[L"TotalMilliseconds"] = totalMs;
                result[L"MinMilliseconds"] = minMs;
                result[L"MaxMilliseconds"] = maxMs;
                result[L"AverageMilliseconds"] = avgMs;
                return result;
        }

        Profiler::CounterMeasurement::CounterMeasurement(const std::wstring &p_name) :
                Measurement(p_name),
                _count(0)
        {
        }

        void Profiler::CounterMeasurement::increment()
        {
                _count++;
        }

        spk::JSON::Object Profiler::CounterMeasurement::exportData() const
        {
                spk::JSON::Object result(_name);
                result.addAttribute(L"Type");
                result.addAttribute(L"Value");
                result[L"Type"] = std::wstring(L"Counter");
                result[L"Value"] = static_cast<long>(_count.load());
                return result;
        }

        Profiler::Profiler()
        {
        }

        Profiler::~Profiler()
        {
                for (auto &pair : _measurements)
                {
                        delete pair.second;
                }
        }

        spk::SafePointer<Profiler::DurationMeasurement> Profiler::duration(const std::wstring &p_name)
        {
                return _get<DurationMeasurement>(p_name, [&]() { return new DurationMeasurement(p_name); });
        }

        spk::SafePointer<Profiler::CounterMeasurement> Profiler::counter(const std::wstring &p_name)
        {
                return _get<CounterMeasurement>(p_name, [&]() { return new CounterMeasurement(p_name); });
        }

        spk::SafePointer<Profiler::Measurement> Profiler::measurement(const std::wstring &p_name)
        {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                auto it = _measurements.find(p_name);
                if (it == _measurements.end())
                {
                        return spk::SafePointer<Measurement>(nullptr);
                }
                return spk::SafePointer<Measurement>(it->second);
        }

        spk::JSON::Object Profiler::exportData() const
        {
                std::lock_guard<std::recursive_mutex> lock(_mutex);
                spk::JSON::Object result(L"Profiler");
                for (const auto &pair : _measurements)
                {
                        result.addAttribute(pair.first);
                        result[pair.first] = pair.second->exportData();
                }
                return result;
        }
}
