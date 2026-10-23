#pragma once

#include "engine/server_entity_hierarchy.hpp"

#include <functional>
#include <utility>
#include <vector>

class TestServerEntityHierarchy : public spk::ServerEntityHierarchy
{
public:
	using Selection = std::function<std::vector<spk::PublishableEntity *>(const spk::Connection &)>;

private:
	Selection _selection;

protected:
	std::vector<spk::PublishableEntity *> selectEntities(const spk::Connection &connection) const override
	{
		return _selection(connection);
	}

public:
	TestServerEntityHierarchy(Selection selection, MessageConfiguration messages) :
		ServerEntityHierarchy(messages),
		_selection(std::move(selection))
	{
	}
};
