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
	Assembly::Element::Attachment::Attachment(std::string name) :
		NameTrait(std::move(name))
	{
	}

	Assembly::Element::Attachment::~Attachment() = default;
	Assembly::Element::Attachment::Attachment(Attachment &&) noexcept = default;
	Assembly::Element::Attachment &Assembly::Element::Attachment::operator=(Attachment &&) = default;

	const Matrix4x4 &Assembly::Element::Attachment::transform() const noexcept
	{
		return _transform;
	}

	const Assembly::Element *Assembly::Element::Attachment::child() const noexcept
	{
		return _child.get();
	}

	void Assembly::Element::Attachment::setPosition(const Vector3 &position)
	{
		validatePosition(position);
		_transform[3][0] = position.x;
		_transform[3][1] = position.y;
		_transform[3][2] = position.z;
	}

	void Assembly::Element::Attachment::setOrientation(const Quaternion &orientation)
	{
		const Vector3 position{_transform[3][0], _transform[3][1], _transform[3][2]};
		setTransform(position, orientation);
	}

	void Assembly::Element::Attachment::setTransform(const Vector3 &position, const Quaternion &orientation)
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

	Assembly::Element::Attachment &Assembly::Element::addAttachmentPoint(
		std::string name,
		const Vector3 &position,
		const Quaternion &orientation)
	{
		const auto existing = std::ranges::find_if(_attachments, [&name](const Attachment &entry) {
			return entry.name() == name;
		});
		if (existing != _attachments.end())
		{
			throw spk::Exception("Duplicate attachment point '" + name + "' in Assembly::Element '" + this->name() + "'");
		}

		Attachment entry(std::move(name));
		entry.setTransform(position, orientation);
		_attachments.push_back(std::move(entry));
		return _attachments.back();
	}

	void Assembly::Element::reserveAttachments(std::size_t count)
	{
		_attachments.reserve(count);
	}

	Assembly::Element::Attachment &Assembly::Element::attachment(const std::string &name)
	{
		const auto it = std::ranges::find_if(_attachments, [&name](const Attachment &entry) {
			return entry.name() == name;
		});
		if (it == _attachments.end())
		{
			throw spk::Exception("Unknown attachment point '" + name + "' in Assembly::Element '" + this->name() + "'");
		}
		return *it;
	}

	const Assembly::Element::Attachment &Assembly::Element::attachment(const std::string &name) const
	{
		return const_cast<Assembly::Element *>(this)->attachment(name);
	}

	const std::vector<Assembly::Element::Attachment> &Assembly::Element::attachments() const noexcept
	{
		return _attachments;
	}

	Assembly::Element::Attachment &Assembly::Element::_availableAttachment(const std::string &name)
	{
		Attachment &entry = attachment(name);
		if (entry._child != nullptr)
		{
			throw spk::Exception("Attachment point '" + name + "' in Assembly::Element '" + this->name() + "' already has a child");
		}
		return entry;
	}

	void Assembly::Element::removeChild(const std::string &name)
	{
		Attachment &entry = attachment(name);
		if (entry._child == nullptr)
		{
			throw spk::Exception("Attachment point '" + name + "' has no child to remove");
		}
		entry._child.reset();
	}
}
