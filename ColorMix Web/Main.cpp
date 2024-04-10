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

void drawColorNode(const Vec2& pos, ColorType c)
{
	Circle(pos, 30).draw(getColor(c));
}

struct ColorEnemy
{
	int32 n;
	ColorType type;
	bool beDissapear = false;
};

void drawColorEnemy(const Vec2& pos, ColorType c)
{
	RoundRect(Arg::center = pos, 60, 60, 10).draw(getColor(c));
}

struct FixedColorNode {
	int32 n;
	ColorType type;
	bool beDissapear = false;
};

struct ColorLane
{
	Array<ColorNode> nodes;
	Array<std::variant<ColorEnemy, FixedColorNode>> fixedNodes;
};

int32 getN(const std::variant<ColorEnemy, FixedColorNode>& fixedNode) {
	if (std::holds_alternative<ColorEnemy>(fixedNode))
	{
		return std::get<ColorEnemy>(fixedNode).n;
	}
	else
	{
		return std::get<FixedColorNode>(fixedNode).n;
	}
}

struct Game
{
	Array<ColorLane> lanes;
	double oneLaneWidth = 70.0;
	double laneHeight = 500.0;
	double enemySpeed = 5.0;
	double nodeSpeed = 20.0;

	double stageProgress = 0.0;
	double enemyProgressAccum = 0.0;
	static constexpr double enemySpanLength = 65;
	static constexpr double enemyAppearY = 0;

	static constexpr double upSpaceY = 50;

	Vec2 pickWaitingPos;
	Optional<ColorType> waitingNode;
	Array<ColorType> nextNodes;
	Stopwatch waitNodeSetTimer;

	Optional<ColorType> pickingNode;
	Vec2 predictedPos;
	Optional<double> pickingUpperLimitY = 0.0;

	Game()
	{
		lanes.resize(4);
		pickWaitingPos = Vec2(width() / 2, 510);
		waitNodeSetTimer.restart();
		nextNodes.resize(3);
		for (auto& node : nextNodes)
		{
			node = ColorType(Random(0, 2));
		}

		stageProgress = 100;
		enemyProgressAccum = stageProgress;
	}

	double width() const
	{
		return lanes.size() * oneLaneWidth;
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

	void update(double delta)
	{
		Transformer2D tf(Mat3x2::Translate((Scene::Width() - width()) / 2, upSpaceY), TransformCursor::Yes);

		stageProgress += delta * enemySpeed;
		enemyProgressAccum += delta * enemySpeed;

		for (auto& lane : lanes)
		{
			for (auto& node : lane.nodes)
			{
				node.y -= delta * nodeSpeed;

				//find collision
				int32 findIndex = nodeIndexAtY(node.y - enemySpanLength / 2);
				bool found = false;
				for (auto& fixedNode : lane.fixedNodes) {
					int32 n = getN(fixedNode);
					if (n == findIndex)
					{
						found = true;
						break;
					}
				}
				if (found)
				{
					int32 pushIndex = findIndex - 1;
					node.y = fixedNodeCenterY(pushIndex);
					node.beFixed = true;
					lane.fixedNodes.push_back(FixedColorNode{ pushIndex, node.type });

					bool foundAtDown = false;
					for (auto [i, fixedNode] : IndexedRef(lane.fixedNodes)) {
						int32 n = getN(fixedNode);
						ColorType type = std::holds_alternative<ColorEnemy>(fixedNode) ? std::get<ColorEnemy>(fixedNode).type : std::get<FixedColorNode>(fixedNode).type;
						if (n == pushIndex + 1 && node.type == type)
						{
							foundAtDown = true;

							std::visit([&](auto& o) {o.beDissapear = true; }, fixedNode);
							std::visit([&](auto& o) {o.beDissapear = true; }, lane.fixedNodes.back());
							break;
						}
					}

				}
			}

			lane.nodes.remove_if([](const ColorNode& node) {return node.beFixed; });
			lane.fixedNodes.remove_if([](const std::variant<ColorEnemy, FixedColorNode>& node) {
				if (std::holds_alternative<ColorEnemy>(node))
				{
					return std::get<ColorEnemy>(node).beDissapear;
				}
				else
				{
					return std::get<FixedColorNode>(node).beDissapear;
				}
				});
		}



		while (enemyProgressAccum > enemySpanLength)
		{
			enemyProgressAccum -= enemySpanLength;
			int32 n = 3;
			//配列からランダムにｎ個選ぶ処理
			Array<int32> indexes = step(static_cast<int32>(lanes.size()));
			indexes.shuffle();
			indexes.resize(n);
			for (auto i : indexes)
			{
				lanes[i].fixedNodes.push_back(ColorEnemy{ nodeIndexAtY(enemyAppearY + enemyProgressAccum), ColorType(Random(0, 6)) });
			}
		}
		if (not pickingNode and waitingNode and Circle(pickWaitingPos, 20).leftClicked())
		{
			pickingNode = waitingNode;
			waitingNode.reset();
			waitNodeSetTimer.restart();
		}

		if (not pickingNode) {
			bool picked = false;
			size_t laneIndex = static_cast<size_t>(Clamp<int32>(Cursor::PosF().x / oneLaneWidth, 0, lanes.size() - 1));
			for (auto& node : lanes[laneIndex].nodes)
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
				lanes[laneIndex].nodes.remove_if([](const ColorNode& node) {return node.bePicked; });
			}
		}

		if (waitNodeSetTimer > 0.1s and not waitingNode) {
			if (nextNodes) {
				waitingNode = nextNodes[0];
				nextNodes.erase(nextNodes.begin());
				nextNodes.push_back(ColorType(Random(0, 2)));
			}
			else {
				waitingNode = ColorType(Random(0, 2));
			}
		}

		if (pickingNode) {
			size_t laneIndex = static_cast<size_t>(Clamp<int32>(Cursor::PosF().x / oneLaneWidth, 0, lanes.size() - 1));

			bool mixable = false;
			size_t minedIndex = 0;
			ColorType mixedColor;
			for (auto [i, node] : Indexed(lanes[laneIndex].nodes))
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
					lanes[laneIndex].nodes[minedIndex].type = mixedColor;
					pickingNode.reset();
					pickingUpperLimitY.reset();
				}
				else {
					lanes[laneIndex].nodes.push_back({ predictedPos.y, *pickingNode });
					pickingNode.reset();
					pickingUpperLimitY.reset();
				}
			}
		}
	}

	void draw() const
	{
		Transformer2D tf(Mat3x2::Translate((Scene::Width() - width()) / 2, upSpaceY), TransformCursor::Yes);

		for (auto [i, lane] : Indexed(lanes))
		{
			for (auto& node : lane.nodes)
			{
				drawColorNode({ laneCenterX(i), node.y }, node.type);
			}
			for (auto& enemy : lane.fixedNodes)
			{
				if (std::holds_alternative<ColorEnemy>(enemy))
				{
					auto& e = std::get<ColorEnemy>(enemy);
					drawColorEnemy({ laneCenterX(i),fixedNodeCenterY(e.n) }, e.type);
				}
				else
				{
					auto& e = std::get<FixedColorNode>(enemy);
					drawColorNode({ laneCenterX(i), fixedNodeCenterY(e.n) }, e.type);
				}
			}
		}
		for (auto i : step(lanes.size() + 1))
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
