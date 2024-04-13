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
};

struct ColorEnemy
{
	ColorType type;
	bool beDissapear = false;
};

struct FixedColorNode {
	ColorType type;
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
	Array<int32> fixedNodeBottoms;

	static constexpr double width = 360.0;
	static constexpr double laneHeight = 500.0;
	static constexpr double enemySpanLength = 50;
	static constexpr Size gridSize = { 7,static_cast<int32>(laneHeight / enemySpanLength * 2) };
	static constexpr double oneLaneWidth = width / gridSize.x;
	static constexpr int32 startEnemySetIndexY = static_cast<int32>(laneHeight / enemySpanLength * 1.5);
	double enemySpeed = 5.0;
	double nodeSpeed = 20.0;


	double stageProgress = startEnemySetIndexY * enemySpanLength;
	int32 enemySetIndexY = startEnemySetIndexY;
	static constexpr double enemyAppearY = -enemySpanLength * 2;
	int32 progressIndex = 0;

	static constexpr double upSpaceY = 50;


	static constexpr Vec2 pickWaitingPos = Vec2(width / 2, 510);
	Optional<ColorType> waitingNode;
	Array<ColorType> nextNodes;
	Stopwatch waitNodeSetTimer;

	Array<ColorType> NodeSetToShuffle = { ColorType::red,ColorType::yellow,ColorType::blue,ColorType::red,ColorType::yellow,ColorType::blue };
	Array<ColorType> shuffledNodeStack;

	Optional<ColorType> pickingNode;
	Vec2 predictedPos;
	Optional<double> pickingUpperLimitY = 0.0;



	Game()
	{
		fixedNodeGrid.resize(gridSize);
		enemyGrid.resize(gridSize);
		nodesLanes.resize(gridSize.x);
		nodePopers.resize(gridSize.x);
		fixedNodeBottoms.resize(gridSize.x);
		waitNodeSetTimer.restart();
		nextNodes.resize(3);
		shuffledNodeStack = NodeSetToShuffle.shuffled();
		for (auto& node : nextNodes)
		{
			node = shuffledNodeStack.back();
			shuffledNodeStack.pop_back();
		}
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

		stageProgress += delta * enemySpeed;

		while (static_cast<int32>(stageProgress / enemySpanLength) - startEnemySetIndexY > progressIndex)
		{
			progressGrid();
		}

		//update fixedNodeBottoms
		{
			int32 startIndex = nodeIndexAtYReal(0);
			for (auto [i, bottom] : IndexedRef(fixedNodeBottoms)) {
				bottom = startIndex;
				for (int32 j = startIndex; j >= 0; j--) {
					if (fixedNodeGrid[{i, j}] or enemyGrid[{i, j}]) {
						bottom = j;
					}
				}
			}
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
						fixedNodeGrid[pushIndex] = FixedColorNode{ node.type };

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
							nodesLanes[lane_i].push_back({ fixedNodeCenterYReal(i), o->type });
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

		if (not pickingNode and waitingNode and Circle(pickWaitingPos, 20).leftClicked())
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
					pickingUpperLimitY = node.y - stageProgress;
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
			size_t laneIndex = static_cast<size_t>(Clamp<int32>(Cursor::PosF().x / oneLaneWidth, 0, gridSize.x - 1));

			bool mixable = false;
			size_t minedIndex = 0;
			ColorType mixedColor;
			for (auto [i, node] : Indexed(nodesLanes[laneIndex]))
			{
				if (Abs(node.y - Cursor::PosF().y) < 20)
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
			/*Point findIndex = { laneIndex,nodeIndexAtY(Cursor::PosF().y) - progressIndex };
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

			predictedPos = Vec2(laneIndex * oneLaneWidth + oneLaneWidth / 2, Cursor::Pos().y);

			bool collision = false;
			Point headIndex = { laneIndex,nodeIndexAtYReal(predictedPos.y - enemySpanLength / 2) };
			if (enemyGrid.inBounds(headIndex))
			{
				if (enemyGrid[headIndex] or fixedNodeGrid[headIndex]) {
					collision = true;
				}
			}
			Point tailIndex = { laneIndex,nodeIndexAtYReal(predictedPos.y + enemySpanLength / 2) };
			if (enemyGrid.inBounds(tailIndex))
			{
				if (enemyGrid[tailIndex] or fixedNodeGrid[tailIndex]) {
					collision = true;
				}
			}
			if (collision) {
				NeighbourSearcher ns({ oneLaneWidth,enemySpanLength }, (Cursor::PosF() - Vec2(0, fixedNodeCenterYReal(0) + enemySpanLength / 2)) * Vec2(1, -1));
				for (auto& p : step(100))
				{
					Point index = ns.pop();
					if (not enemyGrid.inBounds(index)) {
						continue;
					}
					if (enemyGrid[index] or fixedNodeGrid[index]) {
						continue;
					}
					predictedPos = Vec2(laneCenterX(index.x), fixedNodeCenterYReal(index.y));
					laneIndex = index.x;
					break;
				}
			}


			if (MouseL.up())
			{
				if (mixable) {
					nodesLanes[laneIndex][minedIndex].type = mixedColor;
					pickingNode.reset();
					pickingUpperLimitY.reset();
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
					pickingUpperLimitY.reset();
				}
			}
		}
	}

	void draw() const
	{
		Transformer2D tf(Mat3x2::Translate((Scene::Width() - width) / 2, upSpaceY), TransformCursor::Yes);

		//draw bottoms
		for (auto [i, bottom] : Indexed(fixedNodeBottoms))
		{
			double y = fixedNodeCenterYReal(bottom) + enemySpanLength / 2;
			Line(i * oneLaneWidth, y, (i + 1) * oneLaneWidth, y).draw(2, Palette::Black);
		}

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

		Circle(pickWaitingPos, 20).drawFrame(2, Palette::Black);
		if (waitingNode)
		{
			Circle(pickWaitingPos, 20).draw(getColor(*waitingNode));
		}
		for (auto [i, node] : Indexed(nextNodes))
		{
			Circle(pickWaitingPos + Vec2(-50.0 - 50.0 * i, 0), 10).draw(getColor(node));
		}


		if (pickingNode)
		{
			Circle(predictedPos, oneLaneWidth * 0.45).draw(getColor(*pickingNode).withAlpha(128));
			Print << Cursor::PosF();
		}

		//draw upper limit line
		if (pickingUpperLimitY)
		{
			//Line(0, *pickingUpperLimitY + stageProgress + enemySpanLength / 2, Arg::direction(width(), 0)).draw(LineStyle::SquareDot.offset(Scene::Time()), 2, Palette::Black);
		}
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
