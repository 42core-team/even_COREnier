#include "AttackAction.h"

#include <typeinfo>

AttackAction::AttackAction(json msg) : Action(ActionType::ATTACK)
{
	decodeJSON(msg);
}

void AttackAction::decodeJSON(json msg)
{
	unit_id_ = msg["unit_id"];
	int x = msg["x"];
	int y = msg["y"];
	target_pos_ = Position(x, y);
}
json AttackAction::encodeJSON()
{
	json msg = json();

	msg["type"] = "attack";
	msg["unit_id"] = unit_id_;
	msg["x"] = target_pos_.x;
	msg["y"] = target_pos_.y;
	msg["damage_dealt"] = damage_dealt_;

	return msg;
}

bool AttackAction::execute(Game *game, Core * core)
{
	Object *attackObj = game->getObject(unit_id_);
	std::cout << "unit type: " << static_cast<int>(attackObj->getType()) << std::endl;
	std::cout << "attackObj->getType() = " << static_cast<int>(attackObj->getType())
		<< ", ObjectType::Unit = " << static_cast<int>(ObjectType::Unit) << std::endl;

	int attackObjType = static_cast<int>(attackObj->getType());
	int unitType = static_cast<int>(ObjectType::Unit);

	std::cout << "attackObjType = " << attackObjType << ", unitType = " << unitType << std::endl;

	bool attackObjTypeIsUnit = attackObjType == 1;
	bool unitTypeIsUnit = unitType == 1;

	std::cout << "attackObjTypeIsUnit = " << attackObjTypeIsUnit << ", unitTypeIsUnit = " << unitTypeIsUnit << std::endl;

	if (attackObjTypeIsUnit != unitTypeIsUnit)
	{
		printf("not a unit\n");
		return false;
	}
	Unit *unit = dynamic_cast<Unit *>(attackObj);
	if (unit == nullptr)
	{
		printf("unit is null\n");
		return false;
	}
	if (unit->getTeamId() != core->getTeamId())
	{
		printf("not your unit\n");
		return false;
	}

	Object *targetObj = game->getObjectAtPos(target_pos_);
	if (targetObj == nullptr)
	{
		printf("no target\n");
		return false;
	}
	if (targetObj->getType() != ObjectType::Unit)
	{
		printf("not a unit\n");
		return false;
	}
	Unit *targetUnit = dynamic_cast<Unit *>(targetObj);
	if (targetUnit == nullptr)
	{
		printf("target unit is null\n");
		return false;
	}

	int damage = Config::getUnitConfig(unit->getTypeId()).damageUnit;
	targetUnit->setHP(targetUnit->getHP() - damage);
	damage_dealt_ = damage;

	return true;
}
