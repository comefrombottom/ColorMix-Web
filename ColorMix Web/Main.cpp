# include <Siv3D.hpp> // Siv3D v0.6.14

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

struct Game
{
	static constexpr Size gridSize = { 6,128 };
	Grid<Optional<FixedColorNode>> fixedNodeGrid;
	Grid<Optional<ColorEnemy>> enemyGrid;
	Array<Array<ColorNode>> nodesLanes;
	static constexpr double width = 360.0;
	static constexpr double oneLaneWidth = width / gridSize.x;
	double laneHeight = 500.0;
	double enemySpeed = 5.0;
	double nodeSpeed = 20.0;

	double stageProgress = 0.0;
	int32 enemySetIndexY = 10;
	static constexpr double enemySpanLength = 60;
	static constexpr double enemyAppearY = -100;

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
		waitNodeSetTimer.restart();
		nextNodes.resize(3);
		for (auto& node : nextNodes)
		{
			node = ColorType(Random(0, 2));
		}

		stageProgress = 600;
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

	void update(double delta)
	{
		Transformer2D tf(Mat3x2::Translate((Scene::Width() - width) / 2, upSpaceY), TransformCursor::Yes);

		stageProgress += delta * enemySpeed;

		for (auto [lane_i, lane] : IndexedRef(nodesLanes))
		{
			for (auto& node : lane)
			{
				node.y -= delta * nodeSpeed;

				//find collision
				Point findIndex = { lane_i,nodeIndexAtY(node.y - enemySpanLength / 2) };
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
					node.y = fixedNodeCenterY(pushIndex.y);
					node.beFixed = true;
					if (fixedNodeGrid.inBounds(pushIndex)) {
						fixedNodeGrid[pushIndex] = FixedColorNode{ node.type };

						bool foundAtDown = false;

						Point downIndex = pushIndex + Point(0, 1);
						if (enemyGrid.inBounds(downIndex)) {
							if (auto& o = enemyGrid[downIndex])
							{
								if (o->type == node.type)
								{
									o.reset();
									fixedNodeGrid[pushIndex].reset();
									foundAtDown = true;
								}
							}
							else if (auto& o = fixedNodeGrid[downIndex])
							{
								if (o->type == node.type)
								{
									o.reset();
									fixedNodeGrid[pushIndex].reset();
									foundAtDown = true;
								}
							}
						}

						if (foundAtDown)
						{
							for (int32 i = pushIndex.y - 1; i >= 0; i--) {
								Point p = { pushIndex.x,i };
								if (enemyGrid.inBounds(p)) {
									if (enemyGrid[p])
									{
										break;
									}
								}
								if (fixedNodeGrid.inBounds(p)) {
									if (auto& o = fixedNodeGrid[p])
									{
										nodesLanes[lane_i].push_back({ fixedNodeCenterY(i), o->type });
										o.reset();
									}
								}
							}
						}
					}


				}
			}

			lane.remove_if([](const ColorNode& node) {return node.beFixed; });
		}



		while (nodeIndexAtY(enemyAppearY) >= enemySetIndexY)
		{
			int32 n = 3;
			//配列からランダムにｎ個選ぶ処理
			Array<int32> indexes = step(static_cast<int32>(gridSize.x));
			indexes.shuffle();
			indexes.resize(n);
			for (auto i : indexes)
			{
				Point p = { i,enemySetIndexY };
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


			predictedPos = Vec2(laneIndex * oneLaneWidth + oneLaneWidth / 2, Cursor::Pos().y);
			if (MouseL.up())
			{
				if (mixable) {
					nodesLanes[laneIndex][minedIndex].type = mixedColor;
					pickingNode.reset();
					pickingUpperLimitY.reset();
				}
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

		for (auto& p : step(enemyGrid.size())) {
			if (auto& enemy = enemyGrid[p]) {
				drawEnemy({ laneCenterX(p.x), fixedNodeCenterY(p.y) }, enemy->type);
			}
		}
		for (auto& p : step(fixedNodeGrid.size())) {
			if (auto& fixedNode = fixedNodeGrid[p]) {
				drawNode({ laneCenterX(p.x), fixedNodeCenterY(p.y) }, fixedNode->type);
			}
		}


		for (auto [i, lane] : Indexed(nodesLanes))
		{
			for (auto& node : lane)
			{
				drawNode({ laneCenterX(i), node.y }, node.type);
			}
		}
		for (auto i : step(gridSize.x + 1))
		{
			Line(i * oneLaneWidth, 0, i * oneLaneWidth, laneHeight).draw(2, Palette::Black);
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
			Circle(predictedPos, 20).draw(getColor(*pickingNode).withAlpha(128));
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
