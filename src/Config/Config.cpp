#include "Config.h"

GameConfig Config::defaultConfig()
{
	GameConfig config;

	config.width = 25;
	config.height = 25;
	config.tickRate = 5;

	config.idleIncome = 1;
	config.idleIncomeTimeOut = 600; // 2 minutes

	config.resourceHp = 50;
	config.resourceIncome = 200;

	config.coreHp = 350;
	config.initialBalance = 200;

	config.wallHp = 100;
	config.wallBuildCost = 20;

	// unit order must match order of units in conn lib unit type enum

	UnitConfig warrior;
	warrior.name = "Warrior";
	warrior.cost = 150;
	warrior.hp = 25;
	warrior.speed = 3;
	warrior.minSpeed = 12;
	warrior.damageCore = 10;
	warrior.damageUnit = 6;
	warrior.damageResource = 2;
	warrior.damageWall = 3;
	warrior.attackType = AttackType::DIRECT_HIT;
	warrior.attackReach = 1;
	warrior.canBuild = false;
	config.units.push_back(warrior);

	UnitConfig miner;
	miner.name = "Miner";
	miner.cost = 100;
	miner.hp = 15;
	miner.speed = 5;
	miner.minSpeed = 15;
	miner.damageCore = 3;
	miner.damageUnit = 2;
	miner.damageResource = 10;
	miner.damageWall = 5;
	miner.canBuild = false;
	miner.attackType = AttackType::DIRECT_HIT;
	miner.attackReach = 1;
	config.units.push_back(miner);

	UnitConfig carrier;
	carrier.name = "Carrier";
	carrier.cost = 125;
	carrier.hp = 20;
	carrier.speed = 4;
	carrier.minSpeed = 10; // no slowdown when carrying money
	carrier.damageCore = 3;
	carrier.damageUnit = 2;
	carrier.damageResource = 4;
	carrier.damageWall = 3;
	carrier.attackType = AttackType::DIRECT_HIT;
	carrier.attackReach = 1;
	carrier.canBuild = false;
	config.units.push_back(carrier);

	UnitConfig builder;
	builder.name = "Builder";
	builder.cost = 300;
	builder.hp = 30;
	builder.speed = 2;
	builder.minSpeed = 8;
	builder.damageCore = 5;
	builder.damageUnit = 3;
	builder.damageResource = 2;
	builder.damageWall = 3;
	builder.attackType = AttackType::DIRECT_HIT;
	builder.attackReach = 1;
	builder.canBuild = true;
	config.units.push_back(builder);

	UnitConfig archer;
	archer.name = "Archer";
	archer.cost = 200;
	archer.hp = 8;
	archer.speed = 2;
	archer.minSpeed = 12;
	archer.damageCore = 3;
	archer.damageUnit = 3;
	archer.damageResource = 0;
	archer.damageWall = 0;
	archer.attackType = AttackType::DIRECTION_SHOT;
	archer.attackReach = 5;
	archer.canBuild = false;
	config.units.push_back(archer);

	config.corePositions.push_back({ 0, 0 });   // top left
	config.corePositions.push_back({ 24, 24 }); // bottom right
	config.corePositions.push_back({ 24, 0 });  // top right
	config.corePositions.push_back({ 0, 24 });  // bottom left
	config.corePositions.push_back({ 0, 12 });  // left
	config.corePositions.push_back({ 24, 12 }); // right
	config.corePositions.push_back({ 12, 0 });  // top
	config.corePositions.push_back({ 12, 24 }); // bottom

	return config;
}

GameConfig & Config::getInstance()
{
	static GameConfig instance = defaultConfig();
	return instance;
}

Position & Config::getCorePosition(unsigned int teamId)
{
	return getInstance().corePositions[teamId];
}

UnitConfig & Config::getUnitConfig(unsigned int typeId)
{
	return getInstance().units[typeId];
}
