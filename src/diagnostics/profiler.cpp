#include "diagnostics/profiler.hpp"

#include <exception>
#include <utility>

namespace spk
{
	Profiler::Measurement::Measurement(const Identifier &identifier) :
		_identifier(identifier)
	{
	}

	Profiler::Measurement::~Measurement() = default;

	const Profiler::Identifier &Profiler::Measurement::identifier() const
	{
		return _identifier;
	}

	Profiler::TimeMeasurement::Scope::Scope(TimeMeasurement &measurement) :
		_measurement(measurement)
	{
		try
		{
			_measurement.start();
		} catch (...)
		{
			throw spk::Exception(
				"Exception while creating a TimeMeasurement::Scope",
				std::current_exception());
		}
	}

	Profiler::TimeMeasurement::Scope::~Scope() noexcept
	{
		try
		{
			if (_measurement.isRunning() == true)
			{
				_measurement.stop();
			}
		} catch (...)
		{
		}
	}

	Profiler::TimeMeasurement::TimeMeasurement(const Identifier &identifier) :
		Measurement(identifier)
	{
	}

	void Profiler::TimeMeasurement::start()
	{
		if (_running == true)
		{
			throw spk::Exception("TimeMeasurement already started");
		}

		_chronometer.reset();
		_chronometer.start();
		_running = true;
	}

	void Profiler::TimeMeasurement::stop()
	{
		if (_running == false)
		{
			throw spk::Exception("TimeMeasurement isn't running");
		}

		const Duration elapsedTime = _chronometer.stop();
		_running = false;

		_stats.insert(elapsedTime);
	}

	bool Profiler::TimeMeasurement::isRunning() const
	{
		return _running;
	}

	Profiler::TimeMeasurement::Statistics &Profiler::TimeMeasurement::stats()
	{
		return _stats;
	}

	const Profiler::TimeMeasurement::Statistics &Profiler::TimeMeasurement::stats() const
	{
		return _stats;
	}

	Profiler::Measurement *Profiler::MeasurementCollection::find(
		const Identifier &identifier)
	{
		return this->template getElement<Measurement>(
			[&identifier](const Measurement *measurement) {
				return measurement->identifier() == identifier;
			});
	}

	Profiler::TimeMeasurement &Profiler::timeMeasurement(
		const Identifier &identifier)
	{
		return _measurement<TimeMeasurement>(identifier);
	}

	Profiler &Profiler::defaultProfiler()
	{
		static Profiler result;
		return result;
	}

}
