#include "structure/assembly.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	void validatePosition(const spk::Vector3 &position)
	{
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
		{
			throw spk::Exception("Assembly::Element position must contain finite coordinates");
		}
	}

	spk::Matrix4x4 rigidTransform(const spk::Vector3 &position, const spk::Quaternion &orientation)
	{
		validatePosition(position);
		if (!std::isfinite(orientation.x) || !std::isfinite(orientation.y) ||
			!std::isfinite(orientation.z) || !std::isfinite(orientation.w) ||
			orientation.dot(orientation) == 0.0f)
		{
			throw spk::Exception("Assembly::Element orientation must be a non-zero finite quaternion");
		}
		return spk::Matrix4x4::translation(position) * spk::Matrix4x4::rotation(orientation);
	}
}

namespace spk
{
	Assembly::Element::AttachmentPoint::AttachmentPoint(std::string name) :
		NameTrait(std::move(name))
	{
	}

	Assembly::Element::AttachmentPoint::~AttachmentPoint() = default;
	Assembly::Element::AttachmentPoint::AttachmentPoint(AttachmentPoint &&) noexcept = default;
	Assembly::Element::AttachmentPoint &Assembly::Element::AttachmentPoint::operator=(AttachmentPoint &&) = default;

	const Matrix4x4 &Assembly::Element::AttachmentPoint::transform() const noexcept
	{
		return _transform;
	}

	const Assembly::Element *Assembly::Element::AttachmentPoint::child() const noexcept
	{
		return _child.get();
	}

	void Assembly::Element::AttachmentPoint::setPosition(const Vector3 &position)
	{
		validatePosition(position);
		_transform[3][0] = position.x;
		_transform[3][1] = position.y;
		_transform[3][2] = position.z;
	}

	void Assembly::Element::AttachmentPoint::setOrientation(const Quaternion &orientation)
	{
		const Vector3 position{_transform[3][0], _transform[3][1], _transform[3][2]};
		setTransform(position, orientation);
	}

	void Assembly::Element::AttachmentPoint::setTransform(const Vector3 &position, const Quaternion &orientation)
	{
		_transform = rigidTransform(position, orientation);
	}

	Assembly::Element::Element(std::string name) :
		NameTrait(std::move(name))
	{
	}

	Assembly::Element::~Element() = default;
	Assembly::Element::Element(Assembly::Element &&) noexcept = default;

	const Matrix4x4 &Assembly::Element::anchor() const noexcept
	{
		return _anchor;
	}

	void Assembly::Element::setAnchor(const Vector3 &position, const Quaternion &orientation)
	{
		_anchor = rigidTransform(position, orientation);
	}

	Assembly::Element::AttachmentPoint &Assembly::Element::addAttachmentPoint(
		std::string name,
		const Vector3 &position,
		const Quaternion &orientation)
	{
		const auto existing = std::ranges::find_if(_attachmentPoints, [&name](const AttachmentPoint &entry) {
			return entry.name() == name;
		});
		if (existing != _attachmentPoints.end())
		{
			throw spk::Exception("Duplicate attachmentPoint point '" + name + "' in Assembly::Element '" + this->name() + "'");
		}

		AttachmentPoint entry(std::move(name));
		entry.setTransform(position, orientation);
		_attachmentPoints.push_back(std::move(entry));
		return _attachmentPoints.back();
	}

	void Assembly::Element::reserveAttachmentPoints(std::size_t count)
	{
		_attachmentPoints.reserve(count);
	}

	Assembly::Element::AttachmentPoint &Assembly::Element::attachmentPoint(const std::string &name)
	{
		const auto it = std::ranges::find_if(_attachmentPoints, [&name](const AttachmentPoint &entry) {
			return entry.name() == name;
		});
		if (it == _attachmentPoints.end())
		{
			throw spk::Exception("Unknown attachmentPoint point '" + name + "' in Assembly::Element '" + this->name() + "'");
		}
		return *it;
	}

	const Assembly::Element::AttachmentPoint &Assembly::Element::attachmentPoint(const std::string &name) const
	{
		return const_cast<Assembly::Element *>(this)->attachmentPoint(name);
	}

	const std::vector<Assembly::Element::AttachmentPoint> &Assembly::Element::attachmentPoints() const noexcept
	{
		return _attachmentPoints;
	}

	Assembly::Element::AttachmentPoint &Assembly::Element::_availableAttachmentPoint(const std::string &name)
	{
		AttachmentPoint &entry = attachmentPoint(name);
		if (entry._child != nullptr)
		{
			throw spk::Exception("AttachmentPoint point '" + name + "' in Assembly::Element '" + this->name() + "' already has a child");
		}
		return entry;
	}

	void Assembly::Element::removeChild(const std::string &name)
	{
		AttachmentPoint &entry = attachmentPoint(name);
		if (entry._child == nullptr)
		{
			throw spk::Exception("AttachmentPoint point '" + name + "' has no child to remove");
		}
		entry._child.reset();
	}
}
