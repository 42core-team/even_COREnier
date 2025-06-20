#include "JigsawWorldGenerator.h"

JigsawWorldGenerator::JigsawWorldGenerator(unsigned int seed)
{
	if (seed == 0)
		seed = std::chrono::system_clock::now().time_since_epoch().count();
	Logger::Log("JigsawWorldGenerator seed: " + std::to_string(seed));
	eng_ = std::default_random_engine(seed);
	loadTemplates();
}

void JigsawWorldGenerator::loadTemplates()
{
	for (const auto &entry : std::filesystem::directory_iterator("./data/jigsaw-templates/"))
	{
		if (entry.path().extension() != ".txt")
		{
			Logger::Log(LogLevel::WARNING, "Skipping non-txt file: " + entry.path().string());
			continue;
		}
		try
		{
			templates_.emplace_back(entry.path().string());
		}
		catch (const std::exception &e)
		{
			Logger::Log(LogLevel::ERROR, "Error loading template " + entry.path().string() + ": " + e.what());
		}
	}
	if (templates_.empty())
	{
		Logger::Log(LogLevel::ERROR, "No valid templates found in template folder.");
		exit(EXIT_FAILURE);
	}
}

bool JigsawWorldGenerator::canPlaceTemplate(Game *game, const MapTemplate &temp, int posX, int posY)
{
	int minSpacing = Config::getInstance().worldGeneratorConfig.value("minTemplateSpacing", 1);
	int minCoreDistance = Config::getInstance().worldGeneratorConfig.value("minCoreDistance", 5);

	for (int y = 0; y < temp.height; ++y)
	{
		for (int x = 0; x < temp.width; ++x)
		{
			char cell = temp.grid[y][x];
			if (cell == ' ')
				continue;

			int targetX = posX + x;
			int targetY = posY + y;
			Position targetPos(targetX, targetY);

			if (game->getObjectAtPos(targetPos) != nullptr)
				return false;

			for (int sy = -minSpacing; sy <= minSpacing; ++sy)
			{
				for (int sx = -minSpacing; sx <= minSpacing; ++sx)
				{
					Position neighbor(targetX + sx, targetY + sy);
					if (game->getObjectAtPos(neighbor) != nullptr)
						return false;
				}
			}

			for (const auto &core : game->getCores())
				if (core.getPosition().distance(targetPos) < minCoreDistance)
					return false;
		}
	}
	return true;
}
bool JigsawWorldGenerator::tryPlaceTemplate(Game *game, const MapTemplate &temp, int posX, int posY, bool force = false)
{
	if (!canPlaceTemplate(game, temp, posX, posY) && !force)
		return false;

	const std::unordered_map<char, int> moneyLikelihoodMap = {
		{'K', 0},
		{'L', 1},
		{'N', 2},
		{'O', 3},
		{'P', 4},
		{'Q', 5},
		{'S', 6},
		{'T', 7},
		{'U', 8},
		{'V', 9}};

	for (int y = 0; y < temp.height; ++y)
	{
		for (int x = 0; x < temp.width; ++x)
		{
			char cell = temp.grid[y][x];
			if (cell == ' ')
				continue;

			if (posX + x < 0 || posX + x >= (int)Config::getInstance().width || posY + y < 0 || posY + y >= (int)Config::getInstance().height)
				continue;

			Position targetPos(posX + x, posY + y);
			if (cell == 'X')
				game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), targetPos));
			else if (cell == 'R')
				game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), targetPos));
			else if (cell == 'M')
				game->getObjects().push_back(std::make_unique<Money>(game->getNextObjectId(), targetPos));
			else if (std::string("0123456789").find(cell) != std::string::npos)
			{
				int wallLikelihood = cell - '0';
				std::uniform_int_distribution<int> dist(0, 9);
				if (dist(eng_) < wallLikelihood)
					game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), targetPos));
			}
			else if (std::string("abcdefghij").find(cell) != std::string::npos)
			{
				int resourceLikelihood = cell - 'a';
				std::uniform_int_distribution<int> dist(0, 9);
				if (dist(eng_) < resourceLikelihood)
					game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), targetPos));
			}
			else if (std::string("ABCDEFGHIJ").find(cell) != std::string::npos)
			{
				int moneyLikelihood = cell - 'A';
				std::uniform_int_distribution<int> dist(0, 9);
				if (dist(eng_) < moneyLikelihood)
					game->getObjects().push_back(std::make_unique<Money>(game->getNextObjectId(), targetPos));
			}
			else if (std::string("klmnopqrst").find(cell) != std::string::npos)
			{
				int wallLikelihood = cell - 'k';
				std::uniform_int_distribution<int> dist(0, 9);
				if (dist(eng_) < wallLikelihood)
					game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), targetPos));
				else
					game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), targetPos));
			}
			else if (std::string("uvwxyz!/$%").find(cell) != std::string::npos)
			{
				int moneyLikelihood = cell - 'u';
				if (cell == '!')
					moneyLikelihood = 6;
				else if (cell == '/')
					moneyLikelihood = 7;
				else if (cell == '$')
					moneyLikelihood = 8;
				else if (cell == '%')
					moneyLikelihood = 9;
				std::uniform_int_distribution<int> dist(0, 9);
				if (dist(eng_) < moneyLikelihood)
					game->getObjects().push_back(std::make_unique<Money>(game->getNextObjectId(), targetPos));
				else
					game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), targetPos));
			}
			else if (std::string("KLNOPQSTUV").find(cell) != std::string::npos)
			{
				auto it = moneyLikelihoodMap.find(cell);
				if (it != moneyLikelihoodMap.end())
				{
					int moneyLikelihood = it->second;
					std::uniform_int_distribution<int> dist(0, 9);
					if (dist(eng_) < moneyLikelihood)
						game->getObjects().push_back(std::make_unique<Money>(game->getNextObjectId(), targetPos));
					else
						game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), targetPos));
				}
			}
		}
	}
	return true;
}

void JigsawWorldGenerator::balanceObjectType(Game *game, ObjectType type, int amount)
{
	int minCoreDistance = Config::getInstance().worldGeneratorConfig.value("minCoreDistance", 5);

	std::vector<size_t> objectIndices;
	for (size_t i = 0; i < game->getObjects().size(); ++i)
		if (game->getObjects()[i]->getType() == type)
			objectIndices.push_back(i);
	int currentObjCount = objectIndices.size();

	if (currentObjCount > amount)
	{
		int removeCount = currentObjCount - amount;

		std::shuffle(objectIndices.begin(), objectIndices.end(), eng_);

		std::vector<size_t> indicesToRemove(objectIndices.begin(), objectIndices.begin() + removeCount);
		std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<size_t>());

		for (size_t idx : indicesToRemove)
		{
			game->getObjects().erase(game->getObjects().begin() + idx);
		}
	}
	else if (currentObjCount < amount)
	{
		int addCount = amount - currentObjCount;
		std::uniform_int_distribution<int> distX(0, Config::getInstance().width - 1);
		std::uniform_int_distribution<int> distY(0, Config::getInstance().height - 1);

		int max_iter = 10000;
		while (addCount > 0 && --max_iter > 0)
		{
			int x = distX(eng_);
			int y = distY(eng_);
			Position pos(x, y);

			bool nearCore = false;
			for (const auto &core : game->getCores())
			{
				if (core.getPosition().distance(pos) < minCoreDistance)
				{
					nearCore = true;
					break;
				}
			}
			if (nearCore)
				continue;

			// likelihood tweaking based on surroundings for more interesting world gen
			int rerollLikeliness = 100;

			// higher likelihood around walls
			int wallsCount = 0;
			for (int xW = -1; xW <= 1; xW++)
				for (int yW = -1; yW <= 1; yW++)
					if (game->getObjectAtPos(Position(x + xW, y + yW)) != nullptr && \
						game->getObjectAtPos(Position(x + xW, y + yW))->getType() == ObjectType::Wall)
						wallsCount++;
			rerollLikeliness -= wallsCount * 5;

			// higher likelihood near non-core corners
			int bottomLeftDistance = pos.distance(Position(0, Config::getInstance().height));
			int topRightDistance = pos.distance(Position(Config::getInstance().width, 0));
			int cornerDist = bottomLeftDistance < topRightDistance ? bottomLeftDistance : topRightDistance;
			rerollLikeliness -= Config::getInstance().width - cornerDist;

			if (rerollLikeliness < 0)
				rerollLikeliness = 0;

			std::uniform_int_distribution<int> pick(0, 100);
			if (pick(eng_) < rerollLikeliness)
				continue ;

			if (game->getObjectAtPos(pos) == nullptr)
			{
				if (type == ObjectType::Resource)
					game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), pos));
				else
					game->getObjects().push_back(std::make_unique<Money>(game->getNextObjectId(), pos));
				addCount--;
			}
		}
	}
}

void JigsawWorldGenerator::clearPathBetweenCores(Game *game)
{
	const auto &cores = game->getCores();

	const Position start = cores[0].getPosition();
	const Position end = cores[1].getPosition();

	int W = Config::getInstance().width;
	int H = Config::getInstance().height;

	auto getCost = [game](int x, int y) -> int
	{
		Position pos(x, y);
		Object *obj = game->getObjectAtPos(pos);
		return (obj == nullptr || obj->getType() == ObjectType::Core) ? 0 : 1;
	};

	std::vector<int> dist(W * H, INT_MAX);
	std::vector<std::pair<int, int>> prev(W * H, {-1, -1});
	auto index = [W](int x, int y) -> int
	{ return y * W + x; };

	struct Node
	{
		int x, y, cost;
	};
	auto cmp = [](const Node &a, const Node &b)
	{ return a.cost > b.cost; };
	std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);

	dist[index(start.x, start.y)] = 0;
	pq.push({start.x, start.y, 0});

	std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

	bool found = false;
	while (!pq.empty())
	{
		Node cur = pq.top();
		pq.pop();

		if (cur.x == end.x && cur.y == end.y)
		{
			found = true;
			break;
		}

		if (cur.cost > dist[index(cur.x, cur.y)])
			continue;

		for (const auto &d : directions)
		{
			int nx = cur.x + d.first;
			int ny = cur.y + d.second;
			if (nx < 0 || ny < 0 || nx >= W || ny >= H)
				continue;
			int cost = getCost(nx, ny);
			int newCost = cur.cost + cost;
			if (newCost < dist[index(nx, ny)])
			{
				dist[index(nx, ny)] = newCost;
				prev[index(nx, ny)] = {cur.x, cur.y};
				pq.push({nx, ny, newCost});
			}
		}
	}

	if (!found)
	{
		Logger::Log(LogLevel::ERROR, "No viable path found between cores.");
		return;
	}

	std::vector<Position> path;
	int cx = end.x, cy = end.y;
	while (!(cx == start.x && cy == start.y))
	{
		path.push_back(Position(cx, cy));
		auto p = prev[index(cx, cy)];
		cx = p.first;
		cy = p.second;
	}
	path.push_back(start);
	std::reverse(path.begin(), path.end());

	for (const auto &pos : path)
	{
		auto &objects = game->getObjects();

		objects.erase(std::remove_if(objects.begin(), objects.end(),
									 [&](const std::unique_ptr<Object> &o)
									 {
										 return o->getPosition() == pos && o->getType() != ObjectType::Core;
									 }),
					  objects.end());

		if (Config::getInstance().worldGeneratorConfig.value("mirrorMap", true))
		{
			Position mirrorPos(
				(W - 1) - pos.x,
				(H - 1) - pos.y);
			objects.erase(std::remove_if(objects.begin(), objects.end(),
										 [&](const std::unique_ptr<Object> &o)
										 {
											 return o->getPosition() == mirrorPos &&
													o->getType() != ObjectType::Core;
										 }),
						  objects.end());
		}
	}
}

void JigsawWorldGenerator::placeWalls(Game *game)
{
	int additionalWallPlaceAttemptCount = Config::getInstance().worldGeneratorConfig.value("additionalWallPlaceAttemptCount", 100);
	int minCoreDistance = Config::getInstance().worldGeneratorConfig.value("minCoreDistance", 5);

	std::uniform_int_distribution<int> distX(0, Config::getInstance().width - 1);
	std::uniform_int_distribution<int> distY(0, Config::getInstance().height - 1);
	std::uniform_real_distribution<double> probDist(0.0, 1.0);

	for (int i = 0; i < additionalWallPlaceAttemptCount; ++i)
	{
		int x = distX(eng_);
		int y = distY(eng_);
		Position pos(x, y);

		if (game->getObjectAtPos(pos) != nullptr)
			continue;

		bool nearCore = false;
		for (const auto &core : game->getCores())
		{
			if (core.getPosition().distance(pos) < minCoreDistance)
			{
				nearCore = true;
				break;
			}
		}
		if (nearCore)
			continue;

		int adjacentObjects = 0;
		for (int sy = -1; sy <= 1; ++sy)
		{
			for (int sx = -1; sx <= 1; ++sx)
			{
				if (sx == 0 && sy == 0)
					continue;
				Position neighbor(x + sx, y + sy);
				if (game->getObjectAtPos(neighbor) != nullptr)
					adjacentObjects++;
			}
		}

		if (adjacentObjects >= 4)
			continue;

		double placementProbability = 0.5;
		if (adjacentObjects == 0)
			placementProbability = 1;
		else if (adjacentObjects == 1)
			placementProbability = 0.6;
		else if (adjacentObjects == 2)
			placementProbability = 0.9;
		else if (adjacentObjects == 3)
			placementProbability = 0.2;

		if (probDist(eng_) < placementProbability)
			game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), pos));
	}
}

void JigsawWorldGenerator::mirrorWorld(Game *game)
{
	unsigned int W = Config::getInstance().width;
	unsigned int H = Config::getInstance().height;

	auto &objs = game->getObjects();

	std::vector<Object *> base;
	base.reserve(objs.size());

	for (const auto &uptr : objs)
	{
		Object *o = uptr.get();
		if (o->getType() == ObjectType::Core)
			continue; // never mirror cores

		const Position p = o->getPosition();
		const Position q(W - 1 - p.x, H - 1 - p.y);

		// lexicographic compare: pick the 'smaller' of p and q
		bool isBase = (p.y < q.y) ||
					  (p.y == q.y && p.x < q.x);
		if (isBase)
			base.push_back(o);
	}

	// 2) Erase every non-core object not in the base set
	objs.erase(
		std::remove_if(objs.begin(), objs.end(),
					   [&](const std::unique_ptr<Object> &uptr)
					   {
						   Position p = uptr->getPosition();
						   Position q(W - 1 - p.x, H - 1 - p.y);
						   bool isCore = uptr->getType() == ObjectType::Core;
						   bool isBase =
							   (p.y < q.y) ||
							   (p.y == q.y && p.x < q.x);
						   return (!isCore && !isBase);
					   }),
		objs.end());

	// 3) Clone each base object into its 180° partner
	for (Object *o : base)
	{
		Position p = o->getPosition();
		Position mirrorPos(W - 1 - p.x, H - 1 - p.y);
		o->clone(mirrorPos, game);
	}
}

void JigsawWorldGenerator::generateWorld(Game *game)
{
	int templatePlaceAttemptCount = Config::getInstance().worldGeneratorConfig.value("templatePlaceAttemptCount", 1000);
	bool mirrorMap = Config::getInstance().worldGeneratorConfig.value("mirrorMap", true);

	Logger::Log("Generating world with JigsawWorldGenerator");

	game->visualizeGameState(0);

	unsigned int width = Config::getInstance().width;
	unsigned int height = Config::getInstance().height;

	std::uniform_int_distribution<int> distX(0, width + 10);
	std::uniform_int_distribution<int> distY(0, height + 10);
	std::uniform_int_distribution<size_t> templateDist(0, templates_.size() - 1);

	Logger::Log("Step 1: Placing templates");
	for (int i = 0; i < templatePlaceAttemptCount; ++i)
	{
		const MapTemplate &original = templates_[templateDist(eng_)];
		MapTemplate temp = original.getTransformedTemplate(eng_);
		int posX = distX(eng_) - 5;
		int posY = distY(eng_) - 5;
		if (tryPlaceTemplate(game, temp, posX, posY))
			Logger::Log("Placed template " + original.name + " at (" + std::to_string(posX) + ", " + std::to_string(posY) + ")");
	}
	game->visualizeGameState(0);

	Logger::Log("Step 2: Placing walls");
	placeWalls(game);
	game->visualizeGameState(0);

	Logger::Log("Step 3: Balancing resources");
	balanceObjectType(game, ObjectType::Resource, Config::getInstance().worldGeneratorConfig.value("resourceCount", 20));
	game->visualizeGameState(0);

	Logger::Log("Step 4: Balancing moneys");
	balanceObjectType(game, ObjectType::Money, Config::getInstance().worldGeneratorConfig.value("moneysCount", 20));
	game->visualizeGameState(0);

	if (mirrorMap)
	{
		Logger::Log("Step 5: Mirroring world");
		mirrorWorld(game);
		game->visualizeGameState(0);
	}

	Logger::Log("Step 6: Clearing at least 1 path between cores.");
	clearPathBetweenCores(game);
	game->visualizeGameState(0);

	Logger::Log("World generation complete");
}
