#include "MoveAction.h"
#include "StatsTracker.h"
void attackStats(Game *game, Unit * unit, Object * obj);

MoveAction::MoveAction(json msg) : Action(ActionType::MOVE), attacked_(false)
{
	decodeJSON(msg);
}

void MoveAction::decodeJSON(json msg)
{
	if (!msg.contains("unit_id") || !msg.contains("dir"))
	{
		is_valid_ = false;
		return;
	}

	unit_id_ = msg["unit_id"];

	std::string dir = msg["dir"];
	if (dir == "u")
		dir_ = MovementDirection::UP;
	else if (dir == "d")
		dir_ = MovementDirection::DOWN;
	else if (dir == "l")
		dir_ = MovementDirection::LEFT;
	else if (dir == "r")
		dir_ = MovementDirection::RIGHT;
	else
		is_valid_ = false;
}
json MoveAction::encodeJSON()
{
	json js;

	js["type"] = "move";
	js["unit_id"] = unit_id_;
	switch (dir_)
	{
	case MovementDirection::UP:
		js["dir"] = "u";
		break;
	case MovementDirection::DOWN:
		js["dir"] = "d";
		break;
	case MovementDirection::LEFT:
		js["dir"] = "l";
		break;
	case MovementDirection::RIGHT:
		js["dir"] = "r";
		break;
	}
	if (attacked_)
		js["attacked"] = attacked_;

	return js;
}

// returns object new hp, 1 if no object present 
bool MoveAction::attackObj(Object *obj, Unit * unit)
{
	if (!obj)
		return false;
	if (obj->getType() == ObjectType::Unit)
	{
		obj->setHP(obj->getHP() - Config::getInstance().units[unit->getTypeId()].damageUnit);
		if (obj->getHP() <= 0)
		{
			unit->addBalance(((Unit *)obj)->getBalance());
			((Unit *)obj)->setBalance(0);
		}
	}
	else if (obj->getType() == ObjectType::Core)
		obj->setHP(obj->getHP() - Config::getInstance().units[unit->getTypeId()].damageCore);
	else if (obj->getType() == ObjectType::Resource)
		((Resource *)obj)->getMined(unit);
	else if (obj->getType() == ObjectType::Wall)
		obj->setHP(obj->getHP() - Config::getInstance().units[unit->getTypeId()].damageWall);

	attacked_ = true;

	return true;
}

bool MoveAction::execute(Game *game, Core * core)
{
	if (!is_valid_)
		return false;

	Object * unitObj = game->getObject(getUnitId());

	if (!unitObj || unitObj->getType() != ObjectType::Unit)
		return false;
	Unit * unit = (Unit *)unitObj;
	if (!unit->canTravel())
		return false;
	if (unit->getTeamId() != core->getTeamId())
		return false;

	Position unitPos = unit->getPosition();
	Position newPos = unitPos + getDirection();
	if (!newPos.isValid(Config::getInstance().width, Config::getInstance().height))
		return false;

	AttackType attackType = Config::getInstance().units[unit->getTypeId()].attackType;

	Object * obj = game->getObjectAtPos(newPos);
	if (!obj)
	{
		unit->setPosition(newPos);
		// movement stat tracking
		game->statsTracker_.incrementRightStats(core->getTeamId(), MOVEMENT, 1);
		unit->resetNextMoveOpp();
		return true;
	}

	if (attackType == AttackType::DIRECT_HIT) // direct hit attack
	{
		attackStats(game, unit, obj);
		//int tempBalance = unit->getBalance();
		if (!attackObj(obj, unit))
			return false;
		// moneyStats(game, unit, tempBalance);
	}
	else if (attackType == AttackType::DIRECTION_SHOT)
	{
		int deltaX = newPos.x - unitPos.x;
		int deltaY = newPos.y - unitPos.y;
		unsigned int attackReach = Config::getInstance().units[unit->getTypeId()].attackReach;

		for (int i = -((int)attackReach); i <= (int)attackReach; i++)
		{
			if (i == 0)
				continue;

			Position posI(unitPos.x + deltaX * i, unitPos.y + deltaY * i);
			if (!posI.isValid(Config::getInstance().width, Config::getInstance().height))
				continue;

			Object* shotObj = game->getObjectAtPos(posI);
			if (!attackObj(shotObj, unit))
				return false;
		}
	}
	else
	{
		std::cerr << "Unknown attack type" << std::endl;
	}

	unit->resetNextMoveOpp();

	return true;
}


void attackStats(Game *game, Unit * unit, Object * obj)
{
	if (!obj)
		return;
	if (obj->getType() == ObjectType::Unit) {
		if ((int)Config::getInstance().units[unit->getTypeId()].damageUnit < obj->getHP()) {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, Config::getInstance().units[unit->getTypeId()].damageUnit);
			game->statsTracker_.incrementRightStats(unit->getTeamId(), DAMAGE_ON_UNITS, Config::getInstance().units[unit->getTypeId()].damageUnit);
		}
		else {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), UNITS_KILLED, 1);
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, obj->getHP());
			game->statsTracker_.incrementRightStats(unit->getTeamId(), DAMAGE_ON_UNITS, obj->getHP());
		}
	}
	if (obj->getType() == ObjectType::Core) {
		if ((int)Config::getInstance().units[unit->getTypeId()].damageCore < obj->getHP()) {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, Config::getInstance().units[unit->getTypeId()].damageCore);
			game->statsTracker_.incrementRightStats(unit->getTeamId(), DAMAGE_ON_CORES, Config::getInstance().units[unit->getTypeId()].damageCore);
		}
		else {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), CORES_DESTROYED, 1);
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, obj->getHP());
			game->statsTracker_.incrementRightStats(unit->getTeamId(), DAMAGE_ON_CORES, obj->getHP());
		}
	}
	if (obj->getType() == ObjectType::Wall) {
		if ((int)Config::getInstance().units[unit->getTypeId()].damageWall < obj->getHP()) {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, Config::getInstance().units[unit->getTypeId()].damageWall);
		}
		else {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), WALLS_DESTROYED, 1);
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, obj->getHP());
		}
	}
	if (obj->getType() == ObjectType::Resource) {
		if ((int)Config::getInstance().units[unit->getTypeId()].damageResource < (int)obj->getHP()) {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, Config::getInstance().units[unit->getTypeId()].damageResource);
		}
		else {
			game->statsTracker_.incrementRightStats(unit->getTeamId(), RECOURCES_MINED, 1);
			game->statsTracker_.incrementRightStats(unit->getTeamId(), TOTAL_DAMAGE, obj->getHP());
		}
		game->statsTracker_.incrementRightStats(unit->getTeamId(), RECOURCES_MINED, 1);
	}
}

// void moneyStats(Game *game, Unit *unit, int tempBalance)
// {
// 	if (tempBalance < unit->getBalance())
// 		game->statsTracker_.incrementRightStats(unit->getTeamId(), MONEY_SPENT, unit->getBalance() - tempBalance);
// 	if (tempBalance > unit->getBalance())
// 		game->statsTracker_.incrementRightStats(unit->getTeamId(), MONEY_GAINED, tmepBalance - unit->getBalance());
// }