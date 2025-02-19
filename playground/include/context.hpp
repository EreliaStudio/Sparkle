#pragma once

#include "sparkle.hpp"

#include "event.hpp"
#include "map_manager.hpp"
#include "node.hpp"
#include "texture_manager.hpp"
#include "control_mapper.hpp"
#include "player_controller.hpp"

struct Context : public spk::Singleton<Context>
{
private:
	TextureManager::Instanciator textureManagerInstanciator;
	EventCenter::Instanciator eventCenterInstanciator;

public:
	NodeMap nodeMap;

	spk::GameEngine gameEngine;
	
	spk::Entity worldEntity;
	spk::Entity playerEntity;
	spk::Entity cameraEntity;
	
	WorldManager& worldManager;
	MapManager& mapManager;
	ControlMapper& controlMapper;
	PlayerController& playerController;
	CameraManager& cameraManager;
	const spk::Camera& mainCamera;

	Context() :
		textureManagerInstanciator(),
		eventCenterInstanciator(),
		nodeMap(),
		gameEngine(),
		worldEntity(L"World manager"),
		playerEntity(L"Player"),
		cameraEntity(L"Camera", &playerEntity),
		worldManager(worldEntity.addComponent<WorldManager>(L"World manager component")),
		mapManager(worldEntity.addComponent<MapManager>(L"Map manager component")),
		controlMapper(worldEntity.addComponent<ControlMapper>(L"Control mapper component")),
		playerController(playerEntity.addComponent<PlayerController>(L"Player controller component")),
		cameraManager(cameraEntity.addComponent<CameraManager>(L"Camera manager component")),
		mainCamera(cameraManager.camera())
	{
		playerEntity.transform().place(spk::Vector3(0, 0, 0));

		cameraEntity.transform().place(spk::Vector3(0, 0, 20));
		cameraEntity.transform().lookAt(playerEntity.transform().position());

		gameEngine.addEntity(&worldEntity);
		gameEngine.addEntity(&playerEntity);
	}

	void configure(const std::filesystem& p_configurationPath)
	{
		spk::JSON::File configurationFile(p_configurationPath);

		
	}
};