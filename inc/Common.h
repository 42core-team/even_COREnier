#ifndef COMMON_H
#define COMMON_H

enum class MovementDirection
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

struct Position
{
	unsigned int x;
	unsigned int y;

	bool operator==(const Position& other) const
	{
		return x == other.x && y == other.y;
	}
	bool isValid(unsigned int maxX, unsigned int maxY) const
	{
		return x < maxX && y < maxY;
	}
	Position operator+(const MovementDirection& dir) const
	{
		Position newPos = *this;
		switch (dir)
		{
			case MovementDirection::UP:
				newPos.y--;
				break;
			case MovementDirection::DOWN:
				newPos.y++;
				break;
			case MovementDirection::LEFT:
				newPos.x--;
				break;
			case MovementDirection::RIGHT:
				newPos.x++;
				break;
		}
		return newPos;
	}
	Position operator+(const Position& other) const
	{
		return {x + other.x, y + other.y};
	}

	Position(unsigned int x, unsigned int y) : x(x), y(y) {}
};

#endif // COMMON_H