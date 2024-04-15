# include <Siv3D.hpp> // Siv3D v0.6.14

class NeighbourSearcher {
	struct DistancePoint {
		double distance;
		int32 x;
		int32 y;

		auto operator<=>(const DistancePoint&) const = default;
	};
	std::set<DistancePoint> distancePoints;
	Vec2 oneEdge{};
	Vec2 center{};

	double distanceOfIndex(const Point& index) {
		return Geometry2D::Distance(RectF(index * oneEdge, oneEdge), center);
	}
	void insertDistancePoint(const Point& index) {
		distancePoints.insert({ distanceOfIndex(index), index.x,index.y });
	}
public:
	NeighbourSearcher() = default;
	NeighbourSearcher(const Vec2& oneEdge, const Vec2& center) :oneEdge(oneEdge), center(center) {
		insertDistancePoint(Floor(center / oneEdge).asPoint());
	}
	Point current() {
		auto& dp = *distancePoints.begin();
		return Point(dp.x, dp.y);
	}
	Point pop() {
		Point origin = Floor(center / oneEdge).asPoint();
		Point index = current();
		distancePoints.erase(distancePoints.begin());

		bool isOriginUpDown = index.x == origin.x and abs(index.y - origin.y) <= 1;
		bool isOrigin = index == origin;

		if (index.x > origin.x or isOriginUpDown)insertDistancePoint(index + Point(1, 0));
		if (index.x < origin.x or isOriginUpDown)insertDistancePoint(index + Point(-1, 0));
		if (index.y > origin.y or isOrigin)insertDistancePoint(index + Point(0, 1));
		if (index.y < origin.y or isOrigin)insertDistancePoint(index + Point(0, -1));

		return index;
	}

};

enum class ColorType
{
	red,
	yellow,
	blue,
	orange,
	green,
	purple,
	black,
};

Optional<ColorType> getMixedColor(ColorType a, ColorType b)
{
	if (a == ColorType::red and b == ColorType::yellow or a == ColorType::yellow and b == ColorType::red)
	{
		return ColorType::orange;
	}
	if (a == ColorType::red and b == ColorType::blue or a == ColorType::blue and b == ColorType::red)
	{
		return ColorType::purple;
	}
	if (a == ColorType::yellow and b == ColorType::blue or a == ColorType::blue and b == ColorType::yellow)
	{
		return ColorType::green;
	}
	if (a == ColorType::red and b == ColorType::green or a == ColorType::green and b == ColorType::red)
	{
		return ColorType::black;
	}
	if (a == ColorType::yellow and b == ColorType::purple or a == ColorType::purple and b == ColorType::yellow)
	{
		return ColorType::black;
	}
	if (a == ColorType::blue and b == ColorType::orange or a == ColorType::orange and b == ColorType::blue)
	{
		return ColorType::black;
	}
	return none;
}

Color getColor(ColorType type)
{
	switch (type)
	{
	case ColorType::red:
		return Palette::Red;
	case ColorType::yellow:
		return Palette::Yellow;
	case ColorType::blue:
		return Palette::Blue;
	case ColorType::orange:
		return Palette::Orange;
	case ColorType::green:
		return Palette::Green;
	case ColorType::purple:
		return Palette::Purple;
	case ColorType::black:
		return Palette::Black;
	default:
		return Palette::White;
	}
}

struct ColorNode
{
	double y;
	ColorType type;
	bool beFixed = false;
	bool bePicked = false;
	bool wasEnemy = false;
};

struct ColorEnemy
{
	ColorType type;
	bool beDissapear = false;
};

struct FixedColorNode {
	ColorType type;
	bool wasEnemy = false;
	bool beDisappear = false;
};

struct NodePoper {
	double y;
	ColorType type;
	double speed = 0;
};

struct Game
{
	Grid<Optional<FixedColorNode>> fixedNodeGrid;
	Grid<Optional<ColorEnemy>> enemyGrid;
	Array<Array<ColorNode>> nodesLanes;
	Array<Array<NodePoper>> nodePopers;

	static constexpr double width = 360.0;
	static constexpr double laneHeight = 500.0;
	static constexpr double enemySpanLength = 70;
	static constexpr Size gridSize = { 5,static_cast<int32>(laneHeight / enemySpanLength * 2) };
	static constexpr double oneLaneWidth = width / gridSize.x;
	static constexpr int32 startEnemySetIndexY = static_cast<int32>(laneHeight / enemySpanLength * 1.5);
	static constexpr double firstEenemySpeed = 5.0;
	double enemySpeed = firstEenemySpeed;

	static constexpr double nodeSpeed = 20.0;


	double stageProgress = startEnemySetIndexY * enemySpanLength;
	int32 enemySetIndexY = startEnemySetIndexY;
	static constexpr double enemyAppearY = -enemySpanLength * 2;
	int32 progressIndex = 0;

	static constexpr double upSpaceY = 50;


	static constexpr Vec2 pickWaitingPos = Vec2(width / 2, 510);
	Optional<ColorType> waitingNode;
	Array<ColorType> nextNodes;
	Stopwatch waitNodeSetTimer;

	static constexpr double waitingNodeRadius = oneLaneWidth * 0.45;

	Array<ColorType> NodeSetToShuffle = { ColorType::red,ColorType::yellow,ColorType::blue,ColorType::red,ColorType::yellow,ColorType::blue };
	Array<ColorType> shuffledNodeStack;

	Optional<ColorType> pickingNode;
	Vec2 predictedPos;
	Optional<double> pickingUnderLimitY = 0.0;
	int32 prevLaneIndex;


	int32 score = 0;

	Font font{ 30 };

	Game()
	{
		fixedNodeGrid.resize(gridSize);
		enemyGrid.resize(gridSize);
		nodesLanes.resize(gridSize.x);
		nodePopers.resize(gridSize.x);
		waitNodeSetTimer.restart();
		nextNodes.resize(3);
		shuffledNodeStack = NodeSetToShuffle.shuffled();
		for (auto& node : nextNodes)
		{
			node = shuffledNodeStack.back();
			shuffledNodeStack.pop_back();
		}
	}

	void init() {
		fixedNodeGrid.clear();
		fixedNodeGrid.resize(gridSize);
		enemyGrid.clear();
		enemyGrid.resize(gridSize);
		nodesLanes.clear();
		nodesLanes.resize(gridSize.x);
		nodePopers.clear();
		nodePopers.resize(gridSize.x);
		waitNodeSetTimer.restart();
		waitingNode.reset();
		shuffledNodeStack = NodeSetToShuffle.shuffled();
		for (auto& node : nextNodes)
		{
			node = shuffledNodeStack.back();
			shuffledNodeStack.pop_back();
		}
		enemySpeed = firstEenemySpeed;
		stageProgress = startEnemySetIndexY * enemySpanLength;
		enemySetIndexY = startEnemySetIndexY;
		progressIndex = 0;
		score = 0;
		pickingNode.reset();
		pickingUnderLimitY.reset();
	}

	double laneCenterX(size_t laneIndex) const
	{
		return laneIndex * oneLaneWidth + oneLaneWidth / 2;
	}

	double fixedNodeCenterY(int32 n) const
	{
		return -enemySpanLength * n - enemySpanLength / 2 + stageProgress;
	}

	int32 nodeIndexAtY(double y) const
	{
		return static_cast<int32>(Floor((stageProgress - y) / enemySpanLength));
	}

	int32 nodeIndexAtYReal(double y) const
	{
		return nodeIndexAtY(y) - progressIndex;
	}

	double fixedNodeCenterYReal(int32 n) const
	{
		return fixedNodeCenterY(n + progressIndex);
	}

	void drawEnemy(const Vec2& pos, ColorType c) const
	{
		double oneEdge = oneLaneWidth * 0.8;
		RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).draw(getColor(c));
	}

	void drawNode(const Vec2& pos, ColorType c) const
	{
		double r = oneLaneWidth * 0.4;
		Circle(pos, r).draw(getColor(c));
	}

	void drawNodePoper(const Vec2& pos, ColorType c) const
	{
		double oneEdge = oneLaneWidth * 0.8;
		Color color = c == ColorType::black ? HSV(0, 0, 0.6) : HSV(HSV(getColor(c)).h, 0.2, 0.8);
		RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).draw(color);
	}

	void progressGrid() {
		progressIndex++;
		enemyGrid.remove_row(0);
		enemyGrid.push_back_row(none);
		fixedNodeGrid.remove_row(0);
		fixedNodeGrid.push_back_row(none);
	}

	void tellGridBecomeEmpty(const Point& index) {
		Point downIndex = index - Point(0, 1);
		if (fixedNodeGrid.inBounds(downIndex)) {
			if (auto& fixedNode = fixedNodeGrid[downIndex]) {
				nodesLanes[index.x].push_back({ fixedNodeCenterYReal(downIndex.y), fixedNode->type });
				fixedNode.reset();
				tellGridBecomeEmpty(downIndex);
			}
		}
	}

	void update(double delta)
	{
		Transformer2D tf(Mat3x2::Translate((Scene::Width() - width) / 2, upSpaceY), TransformCursor::Yes);

		enemySpeed += delta * 0.03;

		stageProgress += delta * enemySpeed;

		while (static_cast<int32>(stageProgress / enemySpanLength) - startEnemySetIndexY > progressIndex)
		{
			progressGrid();
		}


		Array<Point> emptyGrids;

		for (auto [lane_i, lane] : IndexedRef(nodesLanes))
		{



			for (auto& node : lane)
			{
				node.y -= delta * nodeSpeed;

				for (auto& other : lane) {
					if (&node == &other)continue;
					double sub = node.y - other.y;
					if (Abs(sub) < enemySpanLength)
					{
						if (sub > 0)
						{
							double over = enemySpanLength - sub;
							node.y += over / 2;
							other.y += -over / 2;
						}
						else
						{
							double over = enemySpanLength + sub;
							node.y += -over / 2;
							other.y += over / 2;
						}
					}
				}

				double upperLimitY = fixedNodeCenterYReal(nodeIndexAtYReal(0)) + enemySpanLength;
				if (node.y < upperLimitY)
				{
					node.y = upperLimitY;
				}

				//find collision
				Point findIndex = { lane_i,nodeIndexAtYReal(node.y - enemySpanLength / 2) };
				bool found = false;

				if (fixedNodeGrid.inBounds(findIndex)) {
					if (fixedNodeGrid[findIndex]) {
						found = true;
					}
					if (enemyGrid[findIndex]) {
						found = true;
					}
				}

				if (found)
				{
					Point pushIndex = findIndex + Point(0, -1);
					node.y = fixedNodeCenterYReal(pushIndex.y);
					node.beFixed = true;
					if (fixedNodeGrid.inBounds(pushIndex)) {
						fixedNodeGrid[pushIndex] = FixedColorNode{ node.type,node.wasEnemy };

						//find around
						bool foundAround = false;
						for (Point rp : {Point{0, 1}, Point{ 1,0 }, Point{ 0,-1 }, Point{ -1,0 }}) {
							Point aroundIndex = pushIndex + rp;
							if (enemyGrid.inBounds(aroundIndex)) {
								if (auto& o = enemyGrid[aroundIndex])
								{
									if (o->type == node.type)
									{
										o.reset();
										emptyGrids.push_back(aroundIndex);
										nodePopers[aroundIndex.x].push_back({ fixedNodeCenterYReal(aroundIndex.y) ,node.type });
										score += 1;
										foundAround = true;
									}
								}
								else if (auto& o = fixedNodeGrid[aroundIndex])
								{
									if (o->type == node.type)
									{
										o.reset();
										emptyGrids.push_back(aroundIndex);
										foundAround = true;
									}
								}
							}
						}
						if (foundAround) {
							if (fixedNodeGrid[pushIndex]->wasEnemy)score += 1;
							fixedNodeGrid[pushIndex].reset();
							emptyGrids.push_back(pushIndex);
						}
					}


				}
			}

			lane.remove_if([](const ColorNode& node) {return node.beFixed; });
		}

		for (auto& index : emptyGrids) {
			tellGridBecomeEmpty(index);
		}

		for (auto [lane_i, lane] : IndexedRef(nodePopers))
		{
			for (auto& p : lane)
			{
				int32 preN = nodeIndexAtYReal(p.y);
				p.y += delta * (enemySpeed + p.speed);
				p.speed += delta * 1500;
				int32 postN = nodeIndexAtYReal(p.y);

				for (int32 i = preN; i > postN; i--)
				{
					Point findIndex = { lane_i,i };
					if (enemyGrid.inBounds(findIndex)) {
						if (auto& o = enemyGrid[findIndex])
						{
							ColorNode node = { fixedNodeCenterYReal(i), o->type };
							node.wasEnemy = true;
							nodesLanes[lane_i].push_back(node);
							o.reset();
							tellGridBecomeEmpty(findIndex);
						}
					}
				}
			}
		}

		while (nodeIndexAtY(enemyAppearY) >= enemySetIndexY)
		{
			int32 n = Random(4, gridSize.x);
			n = gridSize.x;
			//配列からランダムにｎ個選ぶ処理
			Array<int32> indexes = step(static_cast<int32>(gridSize.x));
			indexes.shuffle();
			indexes.resize(n);
			for (auto i : indexes)
			{
				Point p = { i,enemySetIndexY - progressIndex };
				if (enemyGrid.inBounds(p))
				{
					enemyGrid[p] = ColorEnemy{ ColorType(Random(0, 6)) };
				}
			}
			enemySetIndexY++;
		}

		if (not pickingNode and waitingNode and Circle(pickWaitingPos, waitingNodeRadius).leftClicked())
		{
			pickingNode = waitingNode;
			waitingNode.reset();
			waitNodeSetTimer.restart();
		}

		if (not pickingNode) {
			bool picked = false;
			size_t laneIndex = static_cast<size_t>(Clamp<int32>(Cursor::PosF().x / oneLaneWidth, 0, gridSize.x - 1));
			for (auto& node : nodesLanes[laneIndex])
			{
				if (Abs(node.y - Cursor::PosF().y) < 20 and MouseL.down())
				{
					pickingNode = node.type;
					node.bePicked = true;
					pickingUnderLimitY = node.y - stageProgress;
					picked = true;
					break;
				}
			}
			if (picked) {
				nodesLanes[laneIndex].remove_if([](const ColorNode& node) {return node.bePicked; });
			}
		}

		if (waitNodeSetTimer > 0.1s and not waitingNode) {
			if (nextNodes) {
				waitingNode = nextNodes[0];
				nextNodes.erase(nextNodes.begin());

				if (not shuffledNodeStack) {
					shuffledNodeStack = NodeSetToShuffle.shuffled();
				}

				nextNodes.push_back(shuffledNodeStack.back());
				shuffledNodeStack.pop_back();
			}
			else {
				waitingNode = ColorType(Random(0, 2));
			}
		}

		if (pickingNode) {
			int32 laneIndex = static_cast<size_t>(Clamp<int32>(Cursor::PosF().x / oneLaneWidth, 0, gridSize.x - 1));

			double cursorY = Clamp(Cursor::PosF().y, fixedNodeCenterYReal(nodeIndexAtYReal(0)) + enemySpanLength, laneHeight);

			if (pickingUnderLimitY) {
				cursorY = Clamp(cursorY, fixedNodeCenterYReal(nodeIndexAtYReal(0)) + enemySpanLength, *pickingUnderLimitY + stageProgress);
			}

			bool mixable = false;
			size_t minedIndex = 0;
			ColorType mixedColor;
			for (auto [i, node] : Indexed(nodesLanes[laneIndex]))
			{
				if (Abs(node.y - cursorY) < 20)
				{
					if (auto mixed = getMixedColor(node.type, *pickingNode))
					{
						mixable = true;
						minedIndex = i;
						mixedColor = *mixed;
						break;
					}
				}
			}

			//fixedNode mixable check 
			/*Point findIndex = { laneIndex,nodeIndexAtY(cursorY) - progressIndex };
			bool gridMixable = false;
			Point minedGridIndex = { 0,0 };
			ColorType mixedGridColor;
			if (fixedNodeGrid.inBounds(findIndex)) {
				if (fixedNodeGrid[findIndex]) {
					if (auto mixed = getMixedColor(fixedNodeGrid[findIndex]->type, *pickingNode))
					{
						gridMixable = true;
						minedGridIndex = findIndex;
						mixedGridColor = *mixed;
					}
				}
			}*/
			Vec2 prevPredictedPos = predictedPos;

			Point centerIndex = { laneIndex,nodeIndexAtYReal(cursorY) };
			if (fixedNodeGrid.inBounds(centerIndex)) {
				if (fixedNodeGrid[centerIndex] or enemyGrid[centerIndex]) {

					bool canShift = false;
					Point shiftedIndex = { laneIndex,nodeIndexAtYReal(prevPredictedPos.y) };
					if (fixedNodeGrid.inBounds(shiftedIndex)) {
						if (not fixedNodeGrid[shiftedIndex] and not enemyGrid[shiftedIndex]) {
							canShift = true;
						}
					}
					if (not canShift)laneIndex = prevLaneIndex;
				}
			}

			predictedPos = Vec2(laneIndex * oneLaneWidth + oneLaneWidth / 2, cursorY);

			bool collision = false;
			Point headIndex = { laneIndex,nodeIndexAtYReal(cursorY - enemySpanLength / 2) };
			if (fixedNodeGrid.inBounds(headIndex)) {
				if (fixedNodeGrid[headIndex] or enemyGrid[headIndex]) {
					collision = true;
				}
			}/*
			Point tailIndex = { laneIndex,nodeIndexAtYReal(cursorY + enemySpanLength / 2) };
			if (fixedNodeGrid.inBounds(tailIndex)) {
				if (fixedNodeGrid[tailIndex] or enemyGrid[tailIndex]) {
					collision = true;
				}
			}*/


			if (collision) {


				//find down empty grid
				headIndex.y--;
				while (fixedNodeGrid.inBounds(headIndex)) {
					if (not fixedNodeGrid[headIndex] and not enemyGrid[headIndex]) {
						break;
					}
					headIndex.y--;
				}
				predictedPos = Vec2(headIndex.x * oneLaneWidth + oneLaneWidth / 2, fixedNodeCenterYReal(headIndex.y));
			}

			prevLaneIndex = laneIndex;


			if (MouseL.up())
			{
				if (mixable) {
					nodesLanes[laneIndex][minedIndex].type = mixedColor;
					pickingNode.reset();
					pickingUnderLimitY.reset();
				}
				/*else if (gridMixable) {
					fixedNodeGrid[minedGridIndex].reset();
					nodesLanes[laneIndex].push_back({ fixedNodeCenterY(minedGridIndex.y + progressIndex), mixedGridColor });
					pickingNode.reset();
					pickingUpperLimitY.reset();
				}*/
				else {
					nodesLanes[laneIndex].push_back({ predictedPos.y, *pickingNode });
					pickingNode.reset();
					pickingUnderLimitY.reset();
				}
			}
		}
	}

	void draw() const
	{
		Transformer2D tf(Mat3x2::Translate((Scene::Width() - width) / 2, upSpaceY), TransformCursor::Yes);

		for (auto& p : step(enemyGrid.size())) {
			if (auto& enemy = enemyGrid[p]) {
				drawEnemy({ laneCenterX(p.x), fixedNodeCenterYReal(p.y) }, enemy->type);
			}
		}
		for (auto& p : step(fixedNodeGrid.size())) {
			if (auto& fixedNode = fixedNodeGrid[p]) {
				drawNode({ laneCenterX(p.x), fixedNodeCenterYReal(p.y) }, fixedNode->type);
			}
		}


		for (auto [i, lane] : Indexed(nodesLanes))
		{
			for (auto& node : lane)
			{
				drawNode({ laneCenterX(i), node.y }, node.type);
			}
		}


		for (auto [i, lane] : Indexed(nodePopers))
		{
			for (auto& p : lane)
			{
				drawNodePoper({ laneCenterX(i), p.y }, p.type);
			}
		}

		for (auto i : step(gridSize.x + 1))
		{
			Line(i * oneLaneWidth, 0, i * oneLaneWidth, laneHeight).draw(2, Palette::Black.withAlpha(100));
		}

		Circle(pickWaitingPos, waitingNodeRadius + 3).drawFrame(2, Palette::Black);
		if (waitingNode)
		{
			Circle(pickWaitingPos, waitingNodeRadius).draw(getColor(*waitingNode));
		}
		for (auto [i, node] : Indexed(nextNodes))
		{
			Circle(pickWaitingPos + Vec2(-50.0 - 50.0 * i, 0), 10).draw(getColor(node));
		}


		if (pickingNode)
		{
			Circle(predictedPos, oneLaneWidth * 0.45).draw(getColor(*pickingNode).withAlpha(128));
		}

		//draw upper limit
		RectF(0, fixedNodeCenterYReal(nodeIndexAtYReal(0)) + enemySpanLength / 2 - 20, width, 20).draw(Arg::top = ColorF(0, 1, 1, 0), Arg::bottom = ColorF(0, 1, 1, 0.5));

		//draw under limit line
		if (pickingUnderLimitY)
		{
			Line(0, *pickingUnderLimitY + stageProgress + enemySpanLength / 2, Arg::direction(width, 0)).draw(LineStyle::SquareDot.offset(Scene::Time()), 2, Palette::Black);
		}

		RectF(-100, -100, width + 200, 100).drawShadow({ 0,0 }, 20).draw(Palette::White).drawFrame(2, Palette::Gray);

		//draw score
		font(U"Score: ", score).draw(Arg::topRight = Vec2(width - 10, -50), Palette::Black);

	}
};

void Main()
{
	Scene::SetBackground(Palette::White);
	Game field;


	Window::Resize(400, 600);

	while (System::Update())
	{
		ClearPrint();



		field.update(Scene::DeltaTime());
		{
			field.draw();
		}

		if (SimpleGUI::Button(U"reset", { 0,0 })) {
			field.init();
		}
	}
}

//
// - Debug ビルド: プログラムの最適化を減らす代わりに、エラーやクラッシュ時に詳細な情報を得られます。
//
// - Release ビルド: 最大限の最適化でビルドします。
//
// - [デバッグ] メニュー → [デバッグの開始] でプログラムを実行すると、[出力] ウィンドウに詳細なログが表示され、エラーの原因を探せます。
//
// - Visual Studio を更新した直後は、プログラムのリビルド（[ビルド]メニュー → [ソリューションのリビルド]）が必要な場合があります。
//
