#include "AttackAction.h"

#include "Unit.h"

AttackAction::AttackAction(json msg) : Action(ActionType::MOVE)
{
	decodeJSON(msg);
}

void AttackAction::decodeJSON(json msg)
{
	if (!msg.contains("unit_id") || !msg.contains("target_pos_x") || !msg.contains("target_pos_y"))
	{
		is_valid_ = false;
		return;
	}

	unit_id_ = msg["unit_id"];
	target_pos_.x = msg["target_pos_x"];
	target_pos_.y = msg["target_pos_y"];

	if (!target_pos_.isValid(Config::instance().width, Config::instance().height))
	{
		is_valid_ = false;
		return;
	}
}
json AttackAction::encodeJSON()
{
	json js;

	js["type"] = "move";
	js["unit_id"] = unit_id_;
	js["target_pos_x"] = target_pos_.x;
	js["target_pos_y"] = target_pos_.y;
	js["damage"] = damage_;

	return js;
}

bool AttackAction::attackObj(Object *obj, Unit *unit) // returns object new hp, 1 if no object present
{
	if (!obj)
		return false;
	unit->resetMoveCooldown();
	if (obj->getType() == ObjectType::Unit)
	{
		obj->setHP(obj->getHP() - Config::instance().units[unit->getUnitType()].damageUnit);
		damage_ = Config::instance().units[unit->getUnitType()].damageUnit;
	}
	else if (obj->getType() == ObjectType::Core)
	{
		obj->setHP(obj->getHP() - Config::instance().units[unit->getUnitType()].damageCore);
		damage_ = Config::instance().units[unit->getUnitType()].damageCore;
	}
	else if (obj->getType() == ObjectType::Resource)
	{
		obj->setHP(obj->getHP() - Config::instance().units[unit->getUnitType()].damageResource);
		if (obj->getHP() <= 0)
			((Resource *)obj)->getMined(unit);
		damage_ = Config::instance().units[unit->getUnitType()].damageResource;
	}
	else if (obj->getType() == ObjectType::Wall)
	{
		obj->setHP(obj->getHP() - Config::instance().units[unit->getUnitType()].damageWall);
		damage_ = Config::instance().units[unit->getUnitType()].damageWall;
	}
	else if (obj->getType() == ObjectType::Money)
	{
		unit->setBalance(unit->getBalance() + ((Money *)obj)->getBalance());
		Board::instance().removeObjectById(obj->getId());
		damage_ = 1;
	}
	else if (obj->getType() == ObjectType::Bomb)
	{
		Bomb *bomb = (Bomb *)obj;
		bomb->explode();
		damage_ = 1;
	}

	return true;
}

bool AttackAction::execute(Core *core)
{
	if (!is_valid_)
		return false;

	Object *unitObj = Board::instance().getObjectById(getUnitId());

	if (!unitObj || unitObj->getType() != ObjectType::Unit)
		return false;
	Unit *unit = (Unit *)unitObj;
	if (unit->getMoveCooldown() > 0)
		return false;
	if (unit->getTeamId() != core->getTeamId())
		return false;

	Object *obj = Board::instance().getObjectAtPos(target_pos_);
	if (!obj)
		return false;

	if (!attackObj(obj, unit))
		return false;

	return true;
}
