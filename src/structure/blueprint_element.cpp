#include "structure/blueprint.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	void validatePosition(const spk::Vector3 &position)
	{
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
		{
			throw spk::Exception("Blueprint::Element position must contain finite coordinates");
		}
	}

	spk::Matrix4x4 rigidTransform(const spk::Vector3 &position, const spk::Quaternion &orientation)
	{
		validatePosition(position);
		if (!std::isfinite(orientation.x) || !std::isfinite(orientation.y) ||
			!std::isfinite(orientation.z) || !std::isfinite(orientation.w) ||
			orientation.dot(orientation) == 0.0f)
		{
			throw spk::Exception("Blueprint::Element orientation must be a non-zero finite quaternion");
		}
		return spk::Matrix4x4::translation(position) * spk::Matrix4x4::rotation(orientation);
	}
}

namespace spk::Assembly
{
	Blueprint::Element::AttachmentPoint::AttachmentPoint(std::string name) :
		NameTrait(std::move(name))
	{
	}

	Blueprint::Element::AttachmentPoint::~AttachmentPoint() = default;
	Blueprint::Element::AttachmentPoint::AttachmentPoint(AttachmentPoint &&) noexcept = default;
	Blueprint::Element::AttachmentPoint &Blueprint::Element::AttachmentPoint::operator=(AttachmentPoint &&) = default;

	const Matrix4x4 &Blueprint::Element::AttachmentPoint::transform() const noexcept
	{
		return _transform;
	}

	const Blueprint::Element *Blueprint::Element::AttachmentPoint::child() const noexcept
	{
		return _child.get();
	}

	void Blueprint::Element::AttachmentPoint::setPosition(const Vector3 &position)
	{
		validatePosition(position);
		_transform[3][0] = position.x;
		_transform[3][1] = position.y;
		_transform[3][2] = position.z;
	}

	void Blueprint::Element::AttachmentPoint::setOrientation(const Quaternion &orientation)
	{
		const Vector3 position{_transform[3][0], _transform[3][1], _transform[3][2]};
		setTransform(position, orientation);
	}

	void Blueprint::Element::AttachmentPoint::setTransform(const Vector3 &position, const Quaternion &orientation)
	{
		_transform = rigidTransform(position, orientation);
	}

	Blueprint::Element::Element(std::string name) :
		NameTrait(std::move(name))
	{
	}

	Blueprint::Element::~Element() = default;
	Blueprint::Element::Element(Blueprint::Element &&) noexcept = default;

	const Matrix4x4 &Blueprint::Element::anchor() const noexcept
	{
		return _anchor;
	}

	void Blueprint::Element::setAnchor(const Vector3 &position, const Quaternion &orientation)
	{
		_anchor = rigidTransform(position, orientation);
	}

	Blueprint::Element::AttachmentPoint &Blueprint::Element::addAttachmentPoint(
		std::string name,
		const Vector3 &position,
		const Quaternion &orientation)
	{
		const auto existing = std::ranges::find_if(_attachmentPoints, [&name](const AttachmentPoint &entry) {
			return entry.name() == name;
		});
		if (existing != _attachmentPoints.end())
		{
			throw spk::Exception("Duplicate attachment point '" + name + "' in Blueprint::Element '" + this->name() + "'");
		}

		AttachmentPoint entry(std::move(name));
		entry.setTransform(position, orientation);
		_attachmentPoints.push_back(std::move(entry));
		return _attachmentPoints.back();
	}

	void Blueprint::Element::reserveAttachmentPoints(std::size_t count)
	{
		_attachmentPoints.reserve(count);
	}

	Blueprint::Element::AttachmentPoint &Blueprint::Element::attachmentPoint(const std::string &name)
	{
		const auto it = std::ranges::find_if(_attachmentPoints, [&name](const AttachmentPoint &entry) {
			return entry.name() == name;
		});
		if (it == _attachmentPoints.end())
		{
			throw spk::Exception("Unknown attachment point '" + name + "' in Blueprint::Element '" + this->name() + "'");
		}
		return *it;
	}

	const Blueprint::Element::AttachmentPoint &Blueprint::Element::attachmentPoint(const std::string &name) const
	{
		return const_cast<Blueprint::Element *>(this)->attachmentPoint(name);
	}

	const std::vector<Blueprint::Element::AttachmentPoint> &Blueprint::Element::attachmentPoints() const noexcept
	{
		return _attachmentPoints;
	}

	Blueprint::Element::AttachmentPoint &Blueprint::Element::_availableAttachmentPoint(const std::string &name)
	{
		AttachmentPoint &entry = attachmentPoint(name);
		if (entry._child != nullptr)
		{
			throw spk::Exception("AttachmentPoint point '" + name + "' in Blueprint::Element '" + this->name() + "' already has a child");
		}
		return entry;
	}

	void Blueprint::Element::removeChild(const std::string &name)
	{
		AttachmentPoint &entry = attachmentPoint(name);
		if (entry._child == nullptr)
		{
			throw spk::Exception("AttachmentPoint point '" + name + "' has no child to remove");
		}
		entry._child.reset();
	}
}
