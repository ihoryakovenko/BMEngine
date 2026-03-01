#include "OSMBuilder.h"

#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

using Point2i = std::array<s32, 2>;

static double PerpendicularDistanceSq(
	const Point2i& p,
	const Point2i& a,
	const Point2i& b)
{
	const double ax = a[0];
	const double ay = a[1];
	const double bx = b[0];
	const double by = b[1];
	const double px = p[0];
	const double py = p[1];

	const double abx = bx - ax;
	const double aby = by - ay;
	const double apx = px - ax;
	const double apy = py - ay;

	const double ab_len_sq = abx * abx + aby * aby;

	if (ab_len_sq == 0.0) {
		const double dx = px - ax;
		const double dy = py - ay;
		return dx * dx + dy * dy;
	}

	double t = (apx * abx + apy * aby) / ab_len_sq;
	t = std::clamp(t, 0.0, 1.0);

	const double cx = ax + t * abx;
	const double cy = ay + t * aby;

	const double dx = px - cx;
	const double dy = py - cy;

	return dx * dx + dy * dy;
}

static void DouglasPeuckerRecursive(
	const std::vector<Point2i>& input,
	size_t first,
	size_t last,
	double epsilon_sq,
	std::vector<bool>& keep)
{
	if (last <= first + 1)
		return;

	double max_dist_sq = 0.0;
	size_t index = first;

	for (size_t i = first + 1; i < last; ++i) {
		double d = PerpendicularDistanceSq(
			input[i], input[first], input[last]);

		if (d > max_dist_sq) {
			max_dist_sq = d;
			index = i;
		}
	}

	if (max_dist_sq > epsilon_sq) {
		keep[index] = true;
		DouglasPeuckerRecursive(input, first, index, epsilon_sq, keep);
		DouglasPeuckerRecursive(input, index, last, epsilon_sq, keep);
	}
}

void SimplifyDouglasPeucker( std::vector<Point2i>& points, double epsilon)
{
	if (points.size() < 3)
		return;

	const double epsilon_sq = epsilon * epsilon;

	std::vector<bool> keep(points.size(), false);
	keep.front() = true;
	keep.back() = true;

	DouglasPeuckerRecursive(
		points, 0, points.size() - 1, epsilon_sq, keep);

	std::vector<Point2i> result;
	result.reserve(points.size());

	for (size_t i = 0; i < points.size(); ++i) {
		if (keep[i])
			result.push_back(points[i]);
	}

	points.swap(result);
}

bool IsCollinear(
	const Point2i& a,
	const Point2i& b,
	const Point2i& c)
{
	const s32 abx = b[0] - a[0];
	const s32 aby = b[1] - a[1];
	const s32 bcx = c[0] - b[0];
	const s32 bcy = c[1] - b[1];
	return (int64_t)abx * bcy - (int64_t)aby * bcx == 0;
}

StreetsRender_Material BuildingMaterials[(u32)BuildingMaterial::MAX] =
{
	{ glm::vec3(0.0f, 0.0f, 0.0f),  0.0f, 0.0f }, // Empty
	{ glm::vec3(0.55f, 0.52f, 0.5f),  0.0f, 0.92f }, // CementBlock
	{ glm::vec3(0.6f, 0.2f, 0.15f),   0.0f, 0.9f },  // Brick
	{ glm::vec3(0.9f, 0.88f, 0.85f),  0.0f, 0.85f }, // Plaster
	{ glm::vec3(0.45f, 0.3f, 0.2f),   0.0f, 0.8f },  // Wood
	{ glm::vec3(0.5f, 0.5f, 0.5f),    0.0f, 0.95f }, // Concrete
	{ glm::vec3(0.75f, 0.75f, 0.78f), 1.0f, 0.4f },  // Metal
	{ glm::vec3(0.4f, 0.4f, 0.45f),   1.0f, 0.5f },  // Steel
	{ glm::vec3(0.5f, 0.48f, 0.45f),  0.0f, 0.9f },  // Stone
	{ glm::vec3(0.85f, 0.9f, 0.95f),  0.0f, 0.05f }, // Glass
	{ glm::vec3(0.88f, 0.9f, 0.92f),  1.0f, 0.02f }, // Mirror
	{ glm::vec3(0.4f, 0.32f, 0.25f),  0.0f, 0.95f }, // Mud
	{ glm::vec3(0.7f, 0.6f, 0.5f),    0.0f, 0.85f }, // Masonry
	{ glm::vec3(0.65f, 0.65f, 0.68f), 1.0f, 0.45f }, // Tin
	{ glm::vec3(0.7f, 0.72f, 0.75f),  0.0f, 0.35f }, // Plastic
	{ glm::vec3(0.5f, 0.38f, 0.25f),  0.0f, 0.82f }, // TimberFraming
	{ glm::vec3(0.76f, 0.65f, 0.5f),  0.0f, 0.85f }, // Sandstone
	{ glm::vec3(0.6f, 0.45f, 0.35f),  0.0f, 0.9f },  // Clay
	{ glm::vec3(0.72f, 0.6f, 0.4f),  0.0f, 0.88f }, // Reed
	{ glm::vec3(0.52f, 0.42f, 0.32f), 0.0f, 0.95f }, // Loam
	{ glm::vec3(0.9f, 0.89f, 0.88f),  0.0f, 0.3f },  // Marble
	{ glm::vec3(0.72f, 0.45f, 0.2f),  1.0f, 0.3f },  // Copper
	{ glm::vec3(0.3f, 0.32f, 0.35f),  0.0f, 0.8f },  // Slate
	{ glm::vec3(0.85f, 0.85f, 0.88f), 0.0f, 0.25f }, // Vinyl
	{ glm::vec3(0.78f, 0.75f, 0.68f), 0.0f, 0.88f }, // Limestone
	{ glm::vec3(0.8f, 0.4f, 0.3f),   0.0f, 0.6f },  // Tiles
	{ glm::vec3(0.62f, 0.58f, 0.52f), 0.0f, 0.9f }, // Pebbledash
	{ glm::vec3(0.6f, 0.62f, 0.65f),  1.0f, 0.5f },  // MetalPlates
	{ glm::vec3(0.68f, 0.55f, 0.35f), 0.0f, 0.8f }, // Bamboo
	{ glm::vec3(0.55f, 0.4f, 0.3f),   0.0f, 0.95f }, // Adobe
	{ glm::vec3(0.58f, 0.48f, 0.38f), 0.0f, 0.92f }, // RammedEarth
	{ glm::vec3(0.15f, 0.18f, 0.22f), 1.0f, 0.2f }, // SolarPanels
	{ glm::vec3(0.12f, 0.12f, 0.12f), 0.0f, 0.95f }, // Tyres
};

void BuildingHandler::ProcessMultipolygonRelation(const osmium::Relation& Rel)
{
	if (!Rel.tags().has_key("building"))
	{
		return;
	}

	f32 Height = 3.0f;
	f32 MinHeight = 0.0f;
	BuildingMaterial Material = (BuildingMaterial)(rand() % (u32)BuildingMaterial::MAX);

	GetBuildingData(Rel, Height, MinHeight, Material);

	std::vector<std::vector<std::array<s32, 2>>> OuterRings;
	std::vector<std::vector<std::array<s32, 2>>> InnerRings;

	for (const osmium::RelationMember& Member : Rel.members())
	{
		if (Member.type() != osmium::item_type::way)
		{
			continue;
		}

		auto it = Ways.find(Member.ref());
		if (it == Ways.end())
		{
			continue;
		}

		WayGeometry& WG = it->second;
		if (WG.Ring.size() < 3)
		{
			continue;
		}

		const char* role = Member.role();
		if (role && strcmp(role, "outer") == 0)
		{
			if (WG.IsClockwise)
			{
				std::reverse(WG.Ring.begin(), WG.Ring.end());
			}

			OuterRings.push_back(WG.Ring);
		}
		else if (role && strcmp(role, "inner") == 0)
		{
			if (!WG.IsClockwise)
			{
				std::reverse(WG.Ring.begin(), WG.Ring.end());
			}

			InnerRings.push_back(WG.Ring);
		}
	}

	if (OuterRings.empty())
	{
		return;
	}

	StreetsRender_3DObjectRange Range;
	Range.FirstIndex = TestMesh.Indices.size();

	const glm::vec3 roofColor(
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f
	);

	for (size_t o = 0; o < OuterRings.size(); ++o)
	{
		std::vector<std::vector<std::array<s32, 2>>> Polygon;
		Polygon.push_back(OuterRings[o]);

		for (const auto& inner : InnerRings)
		{
			Polygon.push_back(inner);
		}

		AddBuildingPolygonGeometry(TestMesh, Polygon, Height, MinHeight, roofColor);
	}

	Range.IndexCount = TestMesh.Indices.size() - Range.FirstIndex;
	TestMesh.Ranges.push_back(Range);
	TestMesh.Instances.push_back({ (u32)Material });
}

void BuildingHandler::AddBuildingPolygonGeometry(Mesh& Mesh, const std::vector<std::vector<std::array<s32, 2>>>& Polygon, f32 Height, f32 MinHeight, const glm::vec3& RoofColor)
{
	const u32 BaseVertex = (u32)Mesh.vertices.size();

	for (const auto& ring : Polygon)
	{
		for (size_t i = 0; i < ring.size(); ++i)
		{
			Mesh.vertices.push_back({ glm::ivec2(ring[i][0], ring[i][1]), Height, RoofColor, glm::vec3(0.0f, 1.0f, 0.0f) });
		}
	}

	std::vector<u32> Indices = mapbox::earcut<u32>(Polygon);
	for (size_t i = 0; i < Indices.size(); ++i)
	{
		Mesh.Indices.push_back(Indices[i] + BaseVertex);
	}

	for (const auto& ring : Polygon)
	{
		const u32 N = (u32)ring.size();
		if (N < 3) continue;

		const u32 FirstWallVertex = (u32)Mesh.vertices.size();
		bool WasSmooth = false;

		for (u32 i = 0; i < N; ++i)
		{
			const glm::vec3 VertexColor(
				(f32)(rand() % 256) / 255.0f,
				(f32)(rand() % 256) / 255.0f,
				(f32)(rand() % 256) / 255.0f
			);

			const u32 Prev = (i + N - 1) % N;
			const u32 Next = (i + 1) % N;

			const s32 PrevNanoDegX = ring[Prev][0];
			const s32 PrevNanoDegY = ring[Prev][1];

			const s32 CurrentNanoDegX = ring[i][0];
			const s32 CurrentNanoDegY = ring[i][1];

			const s32 NextNanoDegX = ring[Next][0];
			const s32 NextNanoDegY = ring[Next][1];

			glm::vec2 v0((f32)(PrevNanoDegX - CurrentNanoDegX), (f32)(PrevNanoDegY - CurrentNanoDegY));
			glm::vec2 v1((f32)(NextNanoDegX - CurrentNanoDegX),	(f32)(NextNanoDegY - CurrentNanoDegY));

			v0 = glm::normalize(v0);
			v1 = glm::normalize(v1);

			f32 cosAngle = glm::abs(glm::dot(v0, v1));
			cosAngle = glm::clamp(cosAngle, -1.0f, 1.0f);

			f32 CreaseAngleDeg = 30.0f;
			f32 CosThreshold = glm::cos(glm::radians(CreaseAngleDeg));



			bool SmoothCorner = cosAngle > CosThreshold;
			if (SmoothCorner)
			{
				if (WasSmooth)
				{
					Mesh.vertices.pop_back();
					Mesh.vertices.pop_back();
				}

				const glm::vec3 n1 = WallNormal(PrevNanoDegX, PrevNanoDegY, CurrentNanoDegX, CurrentNanoDegY);
				const glm::vec3 n2 = WallNormal(CurrentNanoDegX, CurrentNanoDegY, NextNanoDegX, NextNanoDegY);
				const glm::vec3 nAv = glm::normalize(n1 + n2);

				const u32 BaseWallVertex = (u32)Mesh.vertices.size();
				const u32 a = BaseWallVertex;										// current top 0
				const u32 b = BaseWallVertex + 1;									// current bottom 1
				const u32 c = Next == 0 ? FirstWallVertex : BaseWallVertex + 2;		// next top 3
				const u32 d = Next == 0 ? FirstWallVertex + 1 : BaseWallVertex + 3;	// next bottom 2

				Mesh.Indices.push_back(d);
				Mesh.Indices.push_back(c);
				Mesh.Indices.push_back(a);
				Mesh.Indices.push_back(a);
				Mesh.Indices.push_back(b);
				Mesh.Indices.push_back(d);

				Mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), Height, VertexColor, nAv });
				Mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), MinHeight, VertexColor, nAv });
				Mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), Height, VertexColor, n2 });
				Mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), MinHeight, VertexColor, n2 });
			}
			else
			{
				const u32 BaseWallVertex = (u32)Mesh.vertices.size();
				const u32 a = BaseWallVertex;										// current top 0
				const u32 b = BaseWallVertex + 1;									// current bottom 1
				const u32 c = Next == 0 ? FirstWallVertex : BaseWallVertex + 2;		// next top 3
				const u32 d = Next == 0 ? FirstWallVertex + 1 : BaseWallVertex + 3;	// next bottom 2

				Mesh.Indices.push_back(d);
				Mesh.Indices.push_back(c);
				Mesh.Indices.push_back(a);
				Mesh.Indices.push_back(a);
				Mesh.Indices.push_back(b);
				Mesh.Indices.push_back(d);

				const glm::vec3 Normal = WallNormal(CurrentNanoDegX, CurrentNanoDegY, NextNanoDegX, NextNanoDegY);

				Mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), Height, VertexColor, Normal });
				Mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), MinHeight, VertexColor, Normal });
				Mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), Height, VertexColor, Normal });
				Mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), MinHeight, VertexColor, Normal });
			}

			WasSmooth = SmoothCorner;
		}
	}
}

bool BuildingHandler::GetBuildingData(const osmium::OSMObject& Object, f32& OutHeight, f32& OutMinHeight, BuildingMaterial& OutMaterial)
{
	if (!Object.tags().has_key("building") && !Object.tags().has_key("building:part"))
	{
		return false;
	}

	const f32 LevelHeight = 3.0f;

	if (Object.tags().has_key("height"))
	{
		const char* HeightStr = Object.tags().get_value_by_key("height");
		OutHeight = atof(HeightStr);
	}
	else if (Object.tags().has_key("building:levels"))
	{
		const char* LevelStr = Object.tags().get_value_by_key("building:levels");
		OutHeight = atoi(LevelStr) * LevelHeight;
	}

	if (OutHeight == 0.0f)
	{
		return false;
	}

	if (Object.tags().has_key("min_height"))
	{
		const char* HeightStr = Object.tags().get_value_by_key("min_height");
		OutMinHeight = atof(HeightStr);
	}
	else if (Object.tags().has_key("building:min_level"))
	{
		const char* LevelStr = Object.tags().get_value_by_key("building:min_level");
		OutMinHeight = atoi(LevelStr) * LevelHeight;
	}

	if (Object.tags().has_key("building:material"))
	{
		const char* MaterialStr = Object.tags().get_value_by_key("building:material");
		OutMaterial = MaterialTable[MaterialStr];
	}

	return true;
}

void BuildingHandler::RemoveColinearPoints(std::vector<std::array<s32, 2>>& Ring)
{
	const u64 n = Ring.size();
	std::vector<std::array<s32, 2>> Simplified;
	Simplified.reserve(n);
	for (u64 i = 0; i < n; ++i)
	{
		const u64 prev = (i + n - 1) % n;
		const u64 next = (i + 1) % n;

		const bool sameAsPrev = (Ring[i][0] == Ring[prev][0] && Ring[i][1] == Ring[prev][1]);
		const bool sameAsNext = (Ring[i][0] == Ring[next][0] && Ring[i][1] == Ring[next][1]);
		if (sameAsPrev || sameAsNext)
		{
			Simplified.push_back(Ring[i]);
			continue;
		}

		const s64 cross = (s64)(Ring[i][0] - Ring[prev][0]) * (s64)(Ring[next][1] - Ring[i][1]) -
			(s64)(Ring[i][1] - Ring[prev][1]) * (s64)(Ring[next][0] - Ring[i][0]);

		if (cross != 0)
		{
			Simplified.push_back(Ring[i]);
		}
	}

	if (Simplified.size() >= 3)
	{
		Ring = std::move(Simplified);
	}
}

glm::vec3 BuildingHandler::WallNormal(s32 Ax, s32 Ay, s32 Bx, s32 By)
{
	const s32 dx = Bx - Ax;
	const s32 dy = By - Ay;

	glm::vec3 Normal(-(f32)dy, 0.0f, (f32)dx);

	const f32 Len = glm::length(Normal);
	Normal = Len > 0.0f ? Normal / Len : glm::vec3(0.0f, 0.0f, 1.0f);

	return Normal;
}

void BuildingHandler::way(const osmium::Way& Way)
{
	const u64 NodeCount = Way.nodes().size();
	std::vector<std::array<s32, 2>> Ring;
	Ring.reserve(NodeCount);

	s64 Area = 0;
	bool AllValid = true;

	for (u64 i = 0; i < NodeCount; ++i)
	{
		const osmium::NodeRef& CurrentNode = Way.nodes()[i];
		const osmium::NodeRef& NextNode = Way.nodes()[(i + 1) % NodeCount];

		const osmium::Location& CurrentLoc = CurrentNode.location();
		const osmium::Location& NextLoc = NextNode.location();

		if (!CurrentLoc.valid() || !NextLoc.valid())
		{
			AllValid = false;
			break;
		}

		Area += (s64)CurrentLoc.x() * (s64)NextLoc.y() - (s64)NextLoc.x() * (s64)CurrentLoc.y();

		Ring.push_back({ CurrentLoc.x(), CurrentLoc.y() });
	}

	if (!AllValid || Ring.empty())
	{
		return;
	}

	//Ring = simplify_douglas_peucker(Ring, 10, Way.is_closed());

	//RemoveColinearPoints(Ring);

	const bool IsClockwise = (Area < 0);

	if (Way.tags().has_key("building") || Way.tags().has_key("building:part"))
	{
		if (Way.is_closed())
		{
			Ring.pop_back();
		}

		SimplifyDouglasPeucker(Ring, 10);

		if (Way.is_closed())
			Ring.push_back(Ring.front());

		f32 Height = 3.0f;
		f32 MinHeight = 0.0f;
		BuildingMaterial Material = (BuildingMaterial)(rand() % (u32)BuildingMaterial::MAX);

		GetBuildingData(Way, Height, MinHeight, Material);

		BuildingWay BWay;
		BWay.Way = WayGeometry{ IsClockwise, Ring };
		BWay.Height = Height;
		BWay.MinHeight = MinHeight;
		BWay.Material = Material;
		BWay.IsOutline = false;

		BuildingWays[Way.id()] = BWay;
	}

	Ways[Way.id()] = WayGeometry{ IsClockwise, Ring };
}

void BuildingHandler::relation(const osmium::Relation& Rel)
{

	if (Rel.tags().has_tag("type", "multipolygon"))
	{
		ProcessMultipolygonRelation(Rel);
	}

	if (Rel.tags().has_tag("type", "building"))
	{
		for (const osmium::RelationMember& Member : Rel.members())
		{
			if (Member.type() != osmium::item_type::way)
			{
				continue;
			}

			auto it = BuildingWays.find(Member.ref());
			if (it == BuildingWays.end())
			{
				continue;
			}

			BuildingWay& BW = it->second;
			if (BW.Way.Ring.size() < 3)
			{
				continue;
			}

			const char* role = Member.role();
			if (role && strcmp(role, "outline") == 0)
			{
				BW.IsOutline = true;
			}
			else if (role && strcmp(role, "part") == 0)
			{
				BW.IsOutline = false;
			}
		}
	}
}

void BuildingHandler::InitializeMaterials()
{
	MaterialTable["cement_block"] = BuildingMaterial::CementBlock;
	MaterialTable["brick"] = BuildingMaterial::Brick;
	MaterialTable["plaster"] = BuildingMaterial::Plaster;
	MaterialTable["wood"] = BuildingMaterial::Wood;
	MaterialTable["concrete"] = BuildingMaterial::Concrete;
	MaterialTable["metal"] = BuildingMaterial::Metal;
	MaterialTable["steel"] = BuildingMaterial::Steel;
	MaterialTable["stone"] = BuildingMaterial::Stone;
	MaterialTable["glass"] = BuildingMaterial::Glass;
	MaterialTable["mirror"] = BuildingMaterial::Mirror;
	MaterialTable["mud"] = BuildingMaterial::Mud;
	MaterialTable["masonry"] = BuildingMaterial::Masonry;
	MaterialTable["tin"] = BuildingMaterial::Tin;
	MaterialTable["plastic"] = BuildingMaterial::Plastic;
	MaterialTable["timber_framing"] = BuildingMaterial::TimberFraming;
	MaterialTable["sandstone"] = BuildingMaterial::Sandstone;
	MaterialTable["clay"] = BuildingMaterial::Clay;
	MaterialTable["reed"] = BuildingMaterial::Reed;
	MaterialTable["loam"] = BuildingMaterial::Loam;
	MaterialTable["marble"] = BuildingMaterial::Marble;
	MaterialTable["copper"] = BuildingMaterial::Copper;
	MaterialTable["slate"] = BuildingMaterial::Slate;
	MaterialTable["vinyl"] = BuildingMaterial::Vinyl;
	MaterialTable["limestone"] = BuildingMaterial::Limestone;
	MaterialTable["tiles"] = BuildingMaterial::Tiles;
	MaterialTable["pebbledash"] = BuildingMaterial::Pebbledash;
	MaterialTable["metal_plates"] = BuildingMaterial::MetalPlates;
	MaterialTable["bamboo"] = BuildingMaterial::Bamboo;
	MaterialTable["adobe"] = BuildingMaterial::Adobe;
	MaterialTable["rammed_earth"] = BuildingMaterial::RammedEarth;
	MaterialTable["solar_panels"] = BuildingMaterial::SolarPanels;
	MaterialTable["tyres"] = BuildingMaterial::Tyres;
}

void BuildingHandler::ConstructObjects()
{
	for (auto& BWayIter : BuildingWays)
	{
		BuildingWay& BWay = BWayIter.second;

		if (BWay.IsOutline)
			continue;

		if (BWay.Way.IsClockwise)
			std::reverse(BWay.Way.Ring.begin(), BWay.Way.Ring.end());

		StreetsRender_3DObjectRange Range;
		Range.FirstIndex = (u32)TestMesh.Indices.size();

		const glm::vec3 RoofColor(
			(f32)(rand() % 256) / 255.0f,
			(f32)(rand() % 256) / 255.0f,
			(f32)(rand() % 256) / 255.0f
		);

		std::vector<std::vector<std::array<s32, 2>>> Polygon;
		Polygon.push_back(BWay.Way.Ring);

		AddBuildingPolygonGeometry(TestMesh, Polygon, BWay.Height, BWay.MinHeight, RoofColor);

		Range.IndexCount = (u32)TestMesh.Indices.size() - Range.FirstIndex;
		TestMesh.Ranges.push_back(Range);
		TestMesh.Instances.push_back({ (u32)BWay.Material });
	}
}