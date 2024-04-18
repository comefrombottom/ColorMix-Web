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

//Real three color
//Color getColor(ColorType type)
//{
//	switch (type)
//	{
//	case ColorType::red:
//		return Palette::Magenta;
//	case ColorType::yellow:
//		return Palette::Yellow;
//	case ColorType::blue:
//		return Palette::Cyan;
//	case ColorType::orange:
//		return Palette::Red;
//	case ColorType::green:
//		return Palette::Green;
//	case ColorType::purple:
//		return Palette::Blue;
//	case ColorType::black:
//		return Palette::Black;
//	default:
//		return Palette::White;
//	}
//}

struct ColorNode
{
	double y;
	ColorType type;
	int32 wasEnemy = 0;
	bool beFixed = false;
	bool bePicked = false;
	Stopwatch monyuTimer{ StartImmediately::Yes };
};

struct PickedNode
{
	ColorType type;
	int32 wasEnemy = 0;
};

struct ColorEnemy
{
	ColorType type;
	bool beDissapear = false;
};

struct FixedColorNode {
	ColorType type;
	int32 wasEnemy = 0;
	bool beDisappear = false;
};

struct NodePoper {
	double y;
	ColorType type;
	double speed = 0;
	int32 count = 0;
};

struct Halo {
	Vec2 pos;
	Stopwatch lifeTimer{ StartImmediately::Yes };
};

struct Game
{
	Grid<Optional<FixedColorNode>> fixedNodeGrid;
	Grid<Optional<ColorEnemy>> enemyGrid;
	Array<Array<ColorNode>> nodesLanes;
	Array<Array<NodePoper>> nodePopers;


	static constexpr double width = 360.0;
	static constexpr double laneHeight = 500.0;
	static constexpr double enemySpanLength = 65;
	static constexpr Size gridSize = { 5,static_cast<int32>(laneHeight / enemySpanLength * 2) };
	static constexpr double oneLaneWidth = width / gridSize.x;
	static constexpr int32 startEnemySetIndexY = static_cast<int32>(laneHeight / enemySpanLength * 1.5);
	static constexpr double firstEenemySpeed = 4.0;
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

	Optional<PickedNode> pickingNode;
	Vec2 predictedPos;
	Optional<double> pickingUnderLimitY = 0.0;
	int32 prevLaneIndex;


	int32 score = 0;



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

	bool isGameOver() const
	{
		for (auto lane_i : step(gridSize.x)) {
			for (auto y_i : step(gridSize.y)) {
				Point index = { lane_i,y_i };
				if (fixedNodeGrid[index] or enemyGrid[index]) {
					if (fixedNodeCenterYReal(y_i) > laneHeight + enemySpanLength / 2)return true;
					break;
				}
			}
		}
		return false;
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

	double nodeRadius() const
	{
		return oneLaneWidth * 0.4;
	}

	void drawEnemy(const Vec2& pos, ColorType c) const
	{
		double oneEdge = oneLaneWidth * 0.8;

		if (c == ColorType::black) {
			RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).drawShadow({ 0,0 }, 10, 0, ColorF(0.2, 0.7));
			RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).draw(ColorF(0.2));

			RoundRect(Arg::center(pos.x, pos.y + oneEdge * 0.2), oneEdge * 0.8, oneEdge * 0.4, oneEdge * 0.1).draw(ColorF(0, 0.1));
			RoundRect(RectF(Arg::topCenter(pos.x, pos.y - oneEdge / 2 + oneEdge * 0.1), oneEdge * 0.8, oneEdge * 0.15), oneEdge * 0.1).draw(ColorF(1, 0.4));

			RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).drawFrame(4, 0, ColorF(0));
		}
		else {
			HSV hsv = HSV(getColor(c));
			RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).drawShadow({ 0,0 }, 10, 0, HSV(hsv.h, 1, 1, 0.7));
			RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).draw(HSV(hsv.h, 0.8, 1));

			RoundRect(Arg::center(pos.x, pos.y + oneEdge * 0.2), oneEdge * 0.8, oneEdge * 0.4, oneEdge * 0.1).draw(HSV(hsv.h, 0.8, 0.9));
			RoundRect(RectF(Arg::topCenter(pos.x, pos.y - oneEdge / 2 + oneEdge * 0.1), oneEdge * 0.8, oneEdge * 0.15), oneEdge * 0.1).draw(ColorF(1, 0.4));

			RoundRect(Arg::center = pos, oneEdge, oneEdge, 10).drawFrame(4, 0, HSV(hsv.h, 1, 0.9));
		}

	}

	void drawNode(const Vec2& pos, double r, ColorType c) const
	{
		if (c == ColorType::black)
		{
			Circle(pos, r).drawShadow({ 0,0 }, r * 0.3, 0, ColorF(0.2, 0.7));
			Circle(pos, r).draw(ColorF(0.2));
			{
				Vec2 center = pos.movedBy(0, r * 0.45);
				Transformer2D tf(Mat3x2::Scale(1, 0.8, center));
				Circle(center, r * 0.65).drawShadow({}, r * 0.2, 0, ColorF(0, 0.1));
			}

			//Circle(pos, r).drawFrame(4, 0, HSV(hsv.h, 1, 0.9));
			Circle(pos, r * 0.5).drawShadow({ 0,0 }, r * 0.2, 0, ColorF(0.5, 0.1));
			{
				Transformer2D tf(Mat3x2::Rotate(-40_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.7)), r * 0.4, r * 0.2).draw(ColorF(1, 0.4));
			}
			{
				Transformer2D tf(Mat3x2::Rotate(20_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.8)), r * 0.2, r * 0.15).draw(ColorF(1, 0.4));
			}
		}
		else {
			HSV hsv = HSV(getColor(c));
			Circle(pos, r).drawShadow({ 0,0 }, r * 0.3, 0, HSV(hsv.h, 1, 1, 0.7));
			Circle(pos, r).draw(HSV(hsv.h, 0.8, 1));

			{
				Vec2 center = pos.movedBy(0, r * 0.45);
				Transformer2D tf(Mat3x2::Scale(1, 0.8, center));
				Circle(center, r * 0.65).drawShadow({}, r * 0.2, 0, HSV(hsv.h, 0.8, 0.9));
			}
			//Ellipse(pos.movedBy(0, r * 0.45), r * 0.65, r * 0.45).draw(HSV(hsv.h, 0.8, 0.9));

			//Circle(pos, r).drawFrame(4, 0, HSV(hsv.h, 1, 0.9));
			Circle(pos, r * 0.5).drawShadow({ 0,0 }, r * 0.2, 0, ColorF(0.5, 0.1));
			{
				Transformer2D tf(Mat3x2::Rotate(-40_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.7)), r * 0.4, r * 0.2).draw(ColorF(1, 0.4));
			}
			{
				Transformer2D tf(Mat3x2::Rotate(20_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.8)), r * 0.2, r * 0.15).draw(ColorF(1, 0.4));
			}
		}
	}

	void drawNode(const Vec2& pos, double r, ColorType c, const Stopwatch& monyuTimer) const {
		double attenuation = 1 - EaseInOutExpo(monyuTimer.sF());
		Transformer2D scale(Mat3x2::Scale(1 + 0.05 * Sin(monyuTimer.sF() * 20 + Math::HalfPi) * attenuation, 1 + 0.05 * Sin(monyuTimer.sF() * 20) * attenuation, pos));
		drawNode(pos, r, c);
	}

	void drawFixedNode(const Vec2& pos, double r, ColorType c) const
	{

		if (c == ColorType::black)
		{
			Circle(pos, r).drawShadow({ 0,0 }, r * 0.3, 0, ColorF(0.2, 0.7));
			Circle(pos, r).draw(ColorF(0.2));
			{
				Vec2 center = pos.movedBy(0, r * 0.45);
				Transformer2D tf(Mat3x2::Scale(1, 0.8, center));
				Circle(center, r * 0.65).drawShadow({}, r * 0.2, 0, ColorF(0, 0.1));
			}

			Circle(pos, r).drawFrame(4, 0, ColorF(0));
			Circle(pos, r * 0.5).drawShadow({ 0,0 }, r * 0.2, 0, ColorF(0.5, 0.1));
			{
				Transformer2D tf(Mat3x2::Rotate(-40_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.7)), r * 0.4, r * 0.2).draw(ColorF(1, 0.4));
			}
			{
				Transformer2D tf(Mat3x2::Rotate(20_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.8)), r * 0.2, r * 0.15).draw(ColorF(1, 0.4));
			}
		}
		else {
			HSV hsv = HSV(getColor(c));
			//Circle(pos, r).drawShadow({ 0,0 }, r * 0.3, 0, HSV(hsv.h, 1, 1, 0.7));
			Circle(pos, r).draw(HSV(hsv.h, 0.8, 1));

			{
				Vec2 center = pos.movedBy(0, r * 0.45);
				Transformer2D tf(Mat3x2::Scale(1, 0.8, center));
				Circle(center, r * 0.65).drawShadow({}, r * 0.2, 0, HSV(hsv.h, 0.8, 0.9));
			}
			//Ellipse(pos.movedBy(0, r * 0.45), r * 0.65, r * 0.45).draw(HSV(hsv.h, 0.8, 0.9));

			Circle(pos, r).drawFrame(4, 0, HSV(hsv.h, 1, 0.9));
			Circle(pos, r * 0.5).drawShadow({ 0,0 }, r * 0.2, 0, ColorF(0.5, 0.1));
			{
				Transformer2D tf(Mat3x2::Rotate(-40_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.7)), r * 0.4, r * 0.2).draw(ColorF(1, 0.4));
			}
			{
				Transformer2D tf(Mat3x2::Rotate(20_deg, pos));
				Ellipse(pos.movedBy(Vec2(0, -r * 0.8)), r * 0.2, r * 0.15).draw(ColorF(1, 0.4));
			}
		}
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
				nodesLanes[index.x].push_back({ fixedNodeCenterYReal(downIndex.y), fixedNode->type,fixedNode->wasEnemy });
				fixedNode.reset();
				tellGridBecomeEmpty(downIndex);
			}
		}
	}

	void update(double delta)
	{
		Transformer2D tf(Mat3x2::Translate((Scene::Width() - width) / 2, upSpaceY), TransformCursor::Yes);

		enemySpeed += delta * 0.02;

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
										AudioAsset(U"broke").playOneShot(1, 0, Random(0.9, 1.1));
										foundAround = true;
									}
								}
								else if (auto& o = fixedNodeGrid[aroundIndex])
								{
									if (o->type == node.type)
									{
										score += o->wasEnemy;
										o.reset();
										emptyGrids.push_back(aroundIndex);
										AudioAsset(U"pop").playOneShot(1, 0, Random(0.9, 1.1));
										foundAround = true;
									}
								}
							}
						}
						if (foundAround) {
							score += fixedNodeGrid[pushIndex]->wasEnemy;
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
							node.wasEnemy = 1;
							nodesLanes[lane_i].push_back(node);
							AudioAsset(U"pop").playOneShot(1, 0, 1 + p.count * 0.15);
							p.count++;
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
			pickingNode = PickedNode{ *waitingNode };
			waitingNode.reset();
			waitNodeSetTimer.restart();

			AudioAsset(U"pick2").playOneShot(1, 0, Random(0.8, 1.2));
		}

		if (not pickingNode) {
			bool picked = false;
			size_t laneIndex = static_cast<size_t>(Clamp<int32>(Cursor::PosF().x / oneLaneWidth, 0, gridSize.x - 1));
			for (auto& node : nodesLanes[laneIndex])
			{
				if (Abs(node.y - Cursor::PosF().y) < 20 and MouseL.down())
				{
					pickingNode = { node.type,node.wasEnemy };
					node.bePicked = true;
					pickingUnderLimitY = node.y - stageProgress;
					picked = true;
					AudioAsset(U"pick2").playOneShot(1, 0, Random(0.8, 1.2));
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
				if (Abs(node.y - cursorY) < enemySpanLength * 0.8)
				{
					if (auto mixed = getMixedColor(node.type, pickingNode->type))
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
					ColorNode& mixedNode = nodesLanes[laneIndex][minedIndex];
					mixedNode.type = mixedColor;
					mixedNode.wasEnemy += pickingNode->wasEnemy;
					mixedNode.monyuTimer.restart();
					AudioAsset(U"mix").playOneShot(1, 0, Random(0.9, 1.1));
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
					nodesLanes[laneIndex].push_back({ predictedPos.y, pickingNode->type,pickingNode->wasEnemy });
					pickingNode.reset();
					pickingUnderLimitY.reset();
					AudioAsset(U"drop").playOneShot(1, 0, Random(0.9, 1.1));
				}
			}
		}
	}

	void draw() const
	{
		Scene::Rect().draw(Palette::Beige);

		double translateX = (Scene::Width() - width) / 2;

		Transformer2D tf(Mat3x2::Translate(translateX, upSpaceY), TransformCursor::Yes);

		//draw border
		constexpr double borderSize = 40;
		for (auto i : step(static_cast<int32>(Ceil(laneHeight / borderSize))))
		{
			Color c = i % 2 == 0 ? Palette::Mistyrose : Palette::Lavenderblush;
			RectF(-20, i * borderSize, width + 40, borderSize).draw(c);
		}

		RectF(0, 0, width, laneHeight).drawShadow({ 0,0 }, 20);

		for (auto i : step(gridSize.x))
		{
			Color c = i % 2 == 0 ? ColorF(0.8, 1, 1) : ColorF(0.9, 1, 1);
			RectF(i * oneLaneWidth, 0, oneLaneWidth, laneHeight).draw(c);
		}

		//Quad({ 0,laneHeight }, { width,laneHeight }, { width + 100,laneHeight + 200 }, {-100,laneHeight+200}).draw(ColorF(0.6, 0.8, 0.8));

		for (auto [i, lane] : Indexed(nodePopers))
		{
			for (auto& p : lane)
			{
				drawNodePoper({ laneCenterX(i), p.y }, p.type);
			}
		}

		for (auto& p : step(enemyGrid.size())) {
			if (auto& enemy = enemyGrid[p]) {
				drawEnemy({ laneCenterX(p.x), fixedNodeCenterYReal(p.y) }, enemy->type);
			}
		}
		for (auto& p : step(fixedNodeGrid.size())) {
			if (auto& fixedNode = fixedNodeGrid[p]) {
				drawFixedNode({ laneCenterX(p.x), fixedNodeCenterYReal(p.y) }, nodeRadius(), fixedNode->type);
			}
		}


		for (auto [i, lane] : Indexed(nodesLanes))
		{
			for (auto& node : lane)
			{
				drawNode({ laneCenterX(i), node.y }, nodeRadius(), node.type, node.monyuTimer);
			}
		}




		//draw upper limit
		RectF(0, fixedNodeCenterYReal(nodeIndexAtYReal(0)) + enemySpanLength / 2 - 20, width, 20).draw(Arg::top = ColorF(0, 1, 1, 0), Arg::bottom = ColorF(0, 1, 1, 0.5));

		//draw under limit line
		if (pickingUnderLimitY)
		{
			Line(0, *pickingUnderLimitY + stageProgress + enemySpanLength / 2, Arg::direction(width, 0)).draw(LineStyle::SquareDot.offset(Scene::Time() * 6), 2, ColorF(0, 0.8, 0.8));
			//RectF(0, *pickingUnderLimitY + stageProgress + enemySpanLength / 2, width, 20).draw(Arg::top = ColorF(0, 1, 1, 0.5), Arg::bottom = ColorF(0, 1, 1, 0));
		}

		/*for (auto i : step(gridSize.x + 1))
		{
			Line(i * oneLaneWidth, 0, i * oneLaneWidth, laneHeight).draw(2, Palette::Black.withAlpha(100));
		}*/





		RectF(-100, -100, width + 200, 100).drawShadow({ 0,0 }, 20).draw(Palette::Blanchedalmond).drawFrame(2, Palette::Gray);

		RectF(-100, laneHeight, width + 200, 100).drawShadow({ 0,0 }, 20).draw(Palette::Blanchedalmond).drawFrame(2, Palette::Gray);


		Circle(pickWaitingPos, waitingNodeRadius + 6).drawShadow({}, 10).draw(Palette::Beige);
		if (waitingNode)
		{
			drawNode(pickWaitingPos, waitingNodeRadius, *waitingNode);
		}
		for (auto [i, node] : Indexed(nextNodes))
		{
			drawNode(pickWaitingPos + Vec2(-60.0 - 45.0 * i, 0), 10, node);
		}


		if (pickingNode)
		{
			ScopedColorMul2D colorMul(ColorF(1, 0.5));
			drawNode(predictedPos, nodeRadius() * 1.15, pickingNode->type);
		}
	}
};

enum class GameState
{
	title,
	playing,
	gameover,
};

void Main()
{
	Scene::SetBackground(Palette::White);
	Game field;

	GameState state = GameState::title;

	Window::Resize(400, 600);

	Font font = SimpleGUI::GetFont();

	TextureAsset::Register(U"logo", U"asset/ColorMixLogo.png");

	AudioAsset::Register(U"mix", U"asset/SFX_UI_Click_Designed_Liquid_Generic_Open_2.wav");
	AudioAsset::Register(U"pick", U"asset/SFX_UI_Click_Designed_Pop_Generic_1.wav");
	AudioAsset::Register(U"pop", U"asset/SFX_UI_Click_Organic_Pop_Thin_Generic_1.wav");
	AudioAsset::Register(U"pick2", U"asset/SFX_UI_Click_Organic_Pop_Negative_2.wav");
	AudioAsset::Register(U"drop", U"asset/SFX_UI_Click_Organic_Plastic_Soft_Generic_1.wav");
	AudioAsset::Register(U"broke", U"asset/SFX_UI_Click_Designed_Metallic_Negative_1.wav");
	AudioAsset::Register(U"click", U"asset/SFX_UI_Button_Organic_Plastic_Thin_Negative_Back_2.wav");
	AudioAsset::Register(U"finish", U"asset/SFX_UI_Click_Designed_Scifi_Flangy_Thick_Generic_1.wav");



	while (System::Update())
	{
		ClearPrint();

		if (state == GameState::title)
		{
			//font(U"Color Mix").drawAt(Scene::Center().movedBy(0, -100), Palette::Black);
			TextureAsset(U"logo").drawAt(Scene::Center().movedBy(0, -50));
			if (SimpleGUI::ButtonAt(U"start", Scene::CenterF().moveBy(0, 100)))
			{
				field.init();
				state = GameState::playing;
				AudioAsset(U"click").playOneShot();

			}
		}
		else if (state == GameState::playing)
		{
			field.update(Scene::DeltaTime());
			{
				field.draw();
			}

			//font(U"Score:{}"_fmt(field.score)).draw(Arg::topRight(Scene::Rect().tl()), Palette::Black);
			font(U"Score: ", field.score).draw(Arg::topRight = Vec2(Scene::Width() - 10, 10), Palette::Black);

			if (SimpleGUI::Button(U"retry", { 5,5 })) {
				AudioAsset(U"click").playOneShot();
				field.init();
				state = GameState::title;
			}

			if (field.isGameOver())
			{
				state = GameState::gameover;
				AudioAsset(U"finish").playOneShot();
			}
		}
		else if (state == GameState::gameover)
		{
			field.draw();
			Scene::Rect().draw(ColorF(0, 0, 0, 0.5));

			font(U"Score:{}"_fmt(field.score)).drawAt(Scene::Center().movedBy(0, -50), Palette::White);

			if (SimpleGUI::ButtonAt(U"retry", Scene::CenterF()))
			{
				field.init();
				state = GameState::title;
				AudioAsset(U"click").playOneShot();

			}
			if (SimpleGUI::ButtonAt(U"Post score on X(Twitter)", Scene::Rect().topCenter().moveBy(0, 50))) {
				const String text = U"ColorMixでスコア{}を達成しました！\n#ColorMix #Siv3D\nhttps://comefrombottom.github.io/ColorMix-Web/"_fmt(field.score);
				Twitter::OpenTweetWindow(text);
			}

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
