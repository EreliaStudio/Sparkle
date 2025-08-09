#pragma once

#include <map>
#include <string>
#include <mutex>
#include <atomic>

#include "structure/design_pattern/spk_singleton.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/container/spk_json_object.hpp"
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
                        Measurement(const std::wstring &p_name);
                        virtual ~Measurement() = default;
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
                        std::recursive_mutex _mutex;

                public:
                        DurationMeasurement(const std::wstring &p_name);

                        void start();
                        void stop();

                        spk::JSON::Object exportData() const override;
                };

                class CounterMeasurement : public Measurement
                {
                private:
                        std::atomic<size_t> _count;

                public:
                        CounterMeasurement(const std::wstring &p_name);

                        void increment();

                        spk::JSON::Object exportData() const override;
                };

        private:
                std::map<std::wstring, Measurement *> _measurements;
                mutable std::recursive_mutex _mutex;

                template <typename TMeasurement, typename Creator>
                spk::SafePointer<TMeasurement> _get(const std::wstring &p_name, const Creator &p_creator)
                {
                        std::lock_guard<std::recursive_mutex> lock(_mutex);
                        auto it = _measurements.find(p_name);
                        if (it == _measurements.end())
                        {
                                Measurement *m = p_creator();
                                _measurements[p_name] = m;
                                return spk::SafePointer<TMeasurement>(static_cast<TMeasurement *>(m));
                        }
                        TMeasurement *result = dynamic_cast<TMeasurement *>(it->second);
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
