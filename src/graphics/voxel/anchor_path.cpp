#include "voxel/model.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	Model::AnchorPath::AnchorPath(std::initializer_list<std::string> segments) :
		AnchorPath(std::vector<std::string>(segments))
	{
	}

	Model::AnchorPath::AnchorPath(std::vector<std::string> segments) :
		_segments(std::move(segments))
	{
		for (const auto &segment : _segments)
		{
			if (segment.empty())
			{
				throw spk::Exception("Voxel AnchorPath: empty segment");
			}
		}
	}

	const std::vector<std::string> &Model::AnchorPath::segments() const noexcept
	{
		return _segments;
	}

	bool Model::AnchorPath::empty() const noexcept
	{
		return _segments.empty();
	}

	std::size_t Model::AnchorPath::size() const noexcept
	{
		return _segments.size();
	}

	bool Model::AnchorPath::operator<(const AnchorPath &other) const noexcept
	{
		return _segments < other._segments;
	}

	bool Model::AnchorPath::operator==(const AnchorPath &other) const noexcept
	{
		return _segments == other._segments;
	}

	std::vector<std::string>::const_iterator Model::AnchorPath::begin() const noexcept
	{
		return _segments.begin();
	}

	std::vector<std::string>::const_iterator Model::AnchorPath::end() const noexcept
	{
		return _segments.end();
	}

	void Model::AnchorPath::push_back(std::string segment)
	{
		if (segment.empty())
		{
			throw spk::Exception("Voxel AnchorPath: empty segment");
		}
		_segments.push_back(std::move(segment));
	}

	void Model::AnchorPath::pop_back()
	{
		if (_segments.empty())
		{
			throw spk::Exception("Voxel AnchorPath: empty path");
		}
		_segments.pop_back();
	}
}
