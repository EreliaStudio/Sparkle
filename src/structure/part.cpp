#include "structure/part.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	void validatePosition(const spk::Vector3 &position)
	{
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
		{
			throw spk::Exception("Part position must contain finite coordinates");
		}
	}

	spk::Matrix4x4 rigidTransform(const spk::Vector3 &position, const spk::Quaternion &orientation)
	{
		validatePosition(position);
		if (!std::isfinite(orientation.x) || !std::isfinite(orientation.y) ||
			!std::isfinite(orientation.z) || !std::isfinite(orientation.w) ||
			orientation.dot(orientation) == 0.0f)
		{
			throw spk::Exception("Part orientation must be a non-zero finite quaternion");
		}
		return spk::Matrix4x4::translation(position) * spk::Matrix4x4::rotation(orientation);
	}
}

namespace spk
{
	Part::Attachment::Attachment(std::string name) :
		_name(std::move(name))
	{
	}

	Part::Attachment::~Attachment() = default;
	Part::Attachment::Attachment(Attachment &&) noexcept = default;
	Part::Attachment &Part::Attachment::operator=(Attachment &&) noexcept = default;

	const std::string &Part::Attachment::name() const noexcept
	{
		return _name;
	}

	const Matrix4x4 &Part::Attachment::transform() const noexcept
	{
		return _transform;
	}

	const Part *Part::Attachment::child() const noexcept
	{
		return _child.get();
	}

	void Part::Attachment::setPosition(const Vector3 &position)
	{
		validatePosition(position);
		_transform[3][0] = position.x;
		_transform[3][1] = position.y;
		_transform[3][2] = position.z;
	}

	void Part::Attachment::setOrientation(const Quaternion &orientation)
	{
		const Vector3 position{_transform[3][0], _transform[3][1], _transform[3][2]};
		setTransform(position, orientation);
	}

	void Part::Attachment::setTransform(const Vector3 &position, const Quaternion &orientation)
	{
		_transform = rigidTransform(position, orientation);
	}

	Part::Part(std::string name) :
		NameTrait(std::move(name))
	{
	}

	Part::~Part() = default;
	Part::Part(Part &&) noexcept = default;

	const Matrix4x4 &Part::anchor() const noexcept
	{
		return _anchor;
	}

	void Part::setAnchor(const Vector3 &position, const Quaternion &orientation)
	{
		_anchor = rigidTransform(position, orientation);
	}

	Part::Attachment &Part::addAttachmentPoint(
		std::string name,
		const Vector3 &position,
		const Quaternion &orientation)
	{
		const auto existing = std::ranges::find_if(_attachments, [&name](const Attachment &entry) {
			return entry.name() == name;
		});
		if (existing != _attachments.end())
		{
			throw spk::Exception("Duplicate attachment point '" + name + "' in Part '" + this->name() + "'");
		}

		Attachment entry(std::move(name));
		entry.setTransform(position, orientation);
		_attachments.push_back(std::move(entry));
		return _attachments.back();
	}

	void Part::reserveAttachments(std::size_t count)
	{
		_attachments.reserve(count);
	}

	Part::Attachment &Part::attachment(const std::string &name)
	{
		const auto it = std::ranges::find_if(_attachments, [&name](const Attachment &entry) {
			return entry.name() == name;
		});
		if (it == _attachments.end())
		{
			throw spk::Exception("Unknown attachment point '" + name + "' in Part '" + this->name() + "'");
		}
		return *it;
	}

	const Part::Attachment &Part::attachment(const std::string &name) const
	{
		return const_cast<Part *>(this)->attachment(name);
	}

	const std::vector<Part::Attachment> &Part::attachments() const noexcept
	{
		return _attachments;
	}

	Part::Attachment &Part::_availableAttachment(const std::string &name)
	{
		Attachment &entry = attachment(name);
		if (entry._child != nullptr)
		{
			throw spk::Exception("Attachment point '" + name + "' in Part '" + this->name() + "' already has a child");
		}
		return entry;
	}

	void Part::removeChild(const std::string &name)
	{
		Attachment &entry = attachment(name);
		if (entry._child == nullptr)
		{
			throw spk::Exception("Attachment point '" + name + "' has no child to remove");
		}
		entry._child.reset();
	}
}
