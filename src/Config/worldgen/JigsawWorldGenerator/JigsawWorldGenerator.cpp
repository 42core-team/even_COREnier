#include "JigsawWorldGenerator.h"

JigsawWorldGenerator::JigsawWorldGenerator(unsigned int seed)
{
	if (seed == 0)
		seed = std::chrono::system_clock::now().time_since_epoch().count();
	Logger::Log(LogLevel::INFO, "JigsawWorldGenerator seed: " + std::to_string(seed));
	eng_ = std::default_random_engine(seed);
	loadTemplates();
}

void JigsawWorldGenerator::loadTemplates()
{
	for (const auto & entry : std::filesystem::directory_iterator("./core/src/Config/worldgen/JigsawWorldGenerator/templates")) {
		if (entry.path().extension() == ".txt")
		{
			try
			{
				templates_.emplace_back(entry.path().string());
			}
			catch (const std::exception &e)
			{
				std::cerr << "Error loading template " << entry.path() << ": " << e.what() << std::endl;
			}
		}
	}
	if (templates_.empty())
		throw std::runtime_error("No valid templates found in template folder.");

	for (const auto &entry : std::filesystem::directory_iterator("./core/src/Config/worldgen/JigsawWorldGenerator/core_wall_templates")) {
		if (entry.path().extension() == ".txt")
		{
			try {
				coreWallTemplates_.emplace_back(entry.path().string());
			}
			catch (const std::exception &e) {
				std::cerr << "Error loading core wall template " << entry.path() << ": " << e.what() << std::endl;
			}
		}
	}
	if (coreWallTemplates_.empty())
		throw std::runtime_error("No valid core wall templates found in core_wall_template folder.");
}

bool JigsawWorldGenerator::areAllCoresConnected(Game* game)
{
	const auto& cores = game->getCores();
	if (cores.empty())
		return true;

	const int width = Config::getInstance().width;
	const int height = Config::getInstance().height;

	std::vector<bool> visited(width * height, false);
	auto index = [width](int x, int y) -> int { return y * width + x; };

	std::queue<Position> queue;
	Position start = cores.front().getPosition();
	queue.push(start);
	visited[index(start.x, start.y)] = true;

	while (!queue.empty())
	{
		Position current = queue.front();
		queue.pop();

		std::vector<Position> directions = {
			{ current.x,     current.y - 1 },
			{ current.x,     current.y + 1 },
			{ current.x - 1, current.y     },
			{ current.x + 1, current.y     }
		};

		for (const auto& next : directions)
		{
			if (next.x < 0 || next.x >= width || next.y < 0 || next.y >= height)
				continue;

			if (visited[index(next.x, next.y)])
				continue;

			if (auto obj = game->getObjectAtPos(next))
				if (obj->getType() == ObjectType::Wall || obj->getType() == ObjectType::Resource)
					continue;

			visited[index(next.x, next.y)] = true;
			queue.push(next);
		}
	}

	for (const auto& core : cores)
	{
		Position corePos = core.getPosition();
		if (!visited[index(corePos.x, corePos.y)])
			return false;
	}

	return true;
}

bool JigsawWorldGenerator::canPlaceTemplate(Game* game, const MapTemplate &temp, int posX, int posY)
{
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

			if (targetX < 0 || targetX >= (int)Config::getInstance().width ||
				targetY < 0 || targetY >= (int)Config::getInstance().height)
				return false;

			if (game->getObjectAtPos(targetPos) != nullptr)
				return false;

			for (int sy = -minSpacing_; sy <= minSpacing_; ++sy)
			{
				for (int sx = -minSpacing_; sx <= minSpacing_; ++sx)
				{
					Position neighbor(targetX + sx, targetY + sy);
					if (game->getObjectAtPos(neighbor) != nullptr)
						return false;
				}
			}

			for (const auto &region : coreWallRegions_)
			{
				if (targetX >= region.x && targetX < region.x + region.width &&
					targetY >= region.y && targetY < region.y + region.height)
					return false;
			}
		}
	}
	return true;
}
bool JigsawWorldGenerator::tryPlaceTemplate(Game* game, const MapTemplate &temp, int posX, int posY, bool force = false)
{
	if (!canPlaceTemplate(game, temp, posX, posY) && !force)
		return false;

	for (int y = 0; y < temp.height; ++y)
	{
		for (int x = 0; x < temp.width; ++x)
		{
			char cell = temp.grid[y][x];
			if (cell == ' ')
				continue;

			Position targetPos(posX + x, posY + y);
			if (cell == 'X')
				game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), targetPos));
			else if (cell == 'R')
				game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), targetPos));
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
				int resourceLikelihood = cell - 'u';
				if (cell == '!')
					resourceLikelihood = 6;
				else if (cell == '/')
					resourceLikelihood = 7;
				else if (cell == '$')
					resourceLikelihood = 8;
				else if (cell == '%')
					resourceLikelihood = 9;
				std::uniform_int_distribution<int> dist(0, 9);
				if (dist(eng_) < resourceLikelihood)
					game->getObjects().push_back(std::make_unique<Resource>(game->getNextObjectId(), targetPos));
				else
					game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), targetPos));
			}
		}
	}
	return true;
}

void JigsawWorldGenerator::placeWalls(Game* game)
{
	std::uniform_int_distribution<int> distX(0, Config::getInstance().width - 1);
	std::uniform_int_distribution<int> distY(0, Config::getInstance().height - 1);
	std::uniform_real_distribution<double> probDist(0.0, 1.0);

	for (int i = 0; i < 75; ++i)
	{
		int x = distX(eng_);
		int y = distY(eng_);
		Position pos(x, y);
		
		if (game->getObjectAtPos(pos) != nullptr)
			continue;

		bool nearCore = false;
		for (const auto &safeZone : coreWallRegions_)
		{
			if (pos.x >= safeZone.x && pos.x < safeZone.x + safeZone.width &&
				pos.y >= safeZone.y && pos.y < safeZone.y + safeZone.height)
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

		if (adjacentObjects >= 3)
			continue;

		double placementProbability = 0.5;
		if (adjacentObjects == 1)
			placementProbability = 0.6;
		else if (adjacentObjects == 2)
			placementProbability = 0.9;

		if (probDist(eng_) < placementProbability)
		{
			game->getObjects().push_back(std::make_unique<Wall>(game->getNextObjectId(), pos));
			if (!areAllCoresConnected(game))
				game->getObjects().pop_back();
		}
	}
}

void JigsawWorldGenerator::placeCoreWalls(Game* game)
{
	std::uniform_int_distribution<size_t> dist(0, coreWallTemplates_.size() - 1);
	const MapTemplate selectedTemplate = coreWallTemplates_[dist(eng_)];

	int worldWidth = Config::getInstance().width;
	int worldHeight = Config::getInstance().height;

	for (const auto &core : game->getCores())
	{
		Position corePos = core.getPosition();
		bool isLeft = (corePos.x < worldWidth / 2);
		bool isTop  = (corePos.y < worldHeight / 2);
		
		MapTemplate orientedTemplate = selectedTemplate;
		int placeX = 0;
		int placeY = 0;

		if (isTop && isLeft)
		{
			placeX = 0;
			placeY = 0;
		}
		else if (isTop && !isLeft)
		{
			orientedTemplate = orientedTemplate.rotate90();
			placeX = worldWidth - orientedTemplate.width;
			placeY = 0;
		}
		else if (!isTop && isLeft)
		{
			orientedTemplate = orientedTemplate.rotate90();
			orientedTemplate = orientedTemplate.rotate90();
			orientedTemplate = orientedTemplate.rotate90();
			placeX = 0;
			placeY = worldHeight - orientedTemplate.height;
		}
		else if (!isTop && !isLeft)
		{
			orientedTemplate = orientedTemplate.rotate90();
			orientedTemplate = orientedTemplate.rotate90();
			placeX = worldWidth - orientedTemplate.width;
			placeY = worldHeight - orientedTemplate.height;
		}

		tryPlaceTemplate(game, orientedTemplate, placeX, placeY, true);
		coreWallRegions_.push_back({ placeX, placeY, orientedTemplate.width, orientedTemplate.height });
	}
}

void JigsawWorldGenerator::generateWorld(Game* game)
{
	unsigned int width = Config::getInstance().width;
	unsigned int height = Config::getInstance().height;

	Logger::Log(LogLevel::INFO, "Generating world map dimensions " + std::to_string(width) + "x" + std::to_string(height));
	game->visualizeGameState(0);

	Logger::Log(LogLevel::INFO, "Step 1: Placing core walls");
	placeCoreWalls(game);
	game->visualizeGameState(0);

	Logger::Log(LogLevel::INFO, "Step 2: Placing templates to create structures");
	std::uniform_int_distribution<int> distX(0, width - 1);
	std::uniform_int_distribution<int> distY(0, height - 1);
	std::uniform_int_distribution<size_t> templateDist(0, templates_.size() - 1);
	for (int i = 0; i < 1000; ++i)
	{
		const MapTemplate &original = templates_[templateDist(eng_)];
		MapTemplate temp = original.getTransformedTemplate(eng_);
		int posX = distX(eng_);
		int posY = distY(eng_);
		tryPlaceTemplate(game, temp, posX, posY);
	}
	game->visualizeGameState(0);

	Logger::Log(LogLevel::INFO, "Step 3: Placing random walls to fill empty spaces");
	placeWalls(game);
	game->visualizeGameState(0);

	Logger::Log(LogLevel::INFO, "Step 4: Balancing resource count");
	balanceResourceAmount(game);
	game->visualizeGameState(0);
	
	Logger::Log(LogLevel::INFO, "Step 5: Balancing resource distribution around cores");
	balanceResourceDistribution(game);
	game->visualizeGameState(0);

	Logger::Log(LogLevel::INFO, "World generation complete");
}
