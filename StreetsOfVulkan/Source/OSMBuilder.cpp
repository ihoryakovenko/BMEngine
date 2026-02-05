#include "OSMBuilder.h"

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

		const std::vector<std::array<s32, 2>>& OuterRing = OuterRings[o];

		Polygon.push_back(OuterRing);
		for (const auto& inner : InnerRings)
		{
			Polygon.push_back(inner);
		}

		std::vector<u32> Indices = mapbox::earcut<u32>(Polygon);

		u32 BaseVertex = TestMesh.vertices.size();
		for (const auto& ring : Polygon)
		{
			for (size_t i = 0; i < ring.size(); ++i)
			{
				TestMesh.vertices.push_back({ glm::ivec2(ring[i][0], ring[i][1]), Height, roofColor, glm::vec3(0.0f, 1.0f, 0.0f) });
			}
		}

		for (size_t i = 0; i < Indices.size(); ++i)
		{
			TestMesh.Indices.push_back(Indices[i] + BaseVertex);
		}

		u32 NodeCount = (u32)OuterRing.size();
		for (u32 i = 0; i < NodeCount; ++i)
		{
			const s32 CurrentNanoDegX = OuterRing[i][0];
			const s32 CurrentNanoDegY = OuterRing[i][1];

			const s32 NextNanoDegX = OuterRing[(i + 1) % NodeCount][0];
			const s32 NextNanoDegY = OuterRing[(i + 1) % NodeCount][1];

			GenerateBuildingWall(TestMesh, CurrentNanoDegX, CurrentNanoDegY, NextNanoDegX, NextNanoDegY, Height, MinHeight);
		}

		for (const auto& innerRing : InnerRings)
		{
			NodeCount = (u32)innerRing.size();
			for (u32 i = 0; i < NodeCount; ++i)
			{
				const s32 CurrentNanoDegX = innerRing[i][0];
				const s32 CurrentNanoDegY = innerRing[i][1];

				const s32 NextNanoDegX = innerRing[(i + 1) % NodeCount][0];
				const s32 NextNanoDegY = innerRing[(i + 1) % NodeCount][1];

				GenerateBuildingWall(TestMesh, CurrentNanoDegX, CurrentNanoDegY, NextNanoDegX, NextNanoDegY, Height, MinHeight);
			}
		}
	}

	Range.IndexCount = TestMesh.Indices.size() - Range.FirstIndex;
	TestMesh.Ranges.push_back(Range);
	TestMesh.Instances.push_back({ (u32)Material });
}

void BuildingHandler::GenerateBuildingWall(Mesh& Mesh, s32 CurrentNanoDegX, s32 CurrentNanoDegY, s32 NextNanoDegX, s32 NextNanoDegY, f32 Height, f32 MinHeight)
{
	const u32 BaseVertex = (u32)Mesh.vertices.size();

	const glm::vec3 wallColor(
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f
	);

	const s32 dx = NextNanoDegX - CurrentNanoDegX;
	const s32 dy = NextNanoDegY - CurrentNanoDegY;

	glm::vec3 Normal(-(f32)dy, 0.0f, (f32)dx);

	const f32 Len = glm::length(Normal);
	Normal = Len > 0.0f ? Normal / Len : glm::vec3(0.0f, 0.0f, 1.0f);

	Mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), Height, wallColor, Normal });
	Mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), MinHeight, wallColor, Normal });
	Mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), MinHeight, wallColor, Normal });
	Mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), Height, wallColor, Normal });

	Mesh.Indices.push_back(BaseVertex + 2);
	Mesh.Indices.push_back(BaseVertex + 3);
	Mesh.Indices.push_back(BaseVertex + 0);
	Mesh.Indices.push_back(BaseVertex + 0);
	Mesh.Indices.push_back(BaseVertex + 1);
	Mesh.Indices.push_back(BaseVertex + 2);
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

	RemoveColinearPoints(Ring);

	const bool IsClockwise = (Area < 0);

	if (Way.tags().has_key("building") || Way.tags().has_key("building:part"))
	{
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
		{
			continue;
		}

		if (BWay.Way.IsClockwise)
		{
			std::reverse(BWay.Way.Ring.begin(), BWay.Way.Ring.end());
		}

		const u32 RingPoints = BWay.Way.Ring.size();
		u32 BaseVertex = TestMesh.vertices.size();

		StreetsRender_3DObjectRange Range;
		Range.FirstIndex = TestMesh.Indices.size();

		const glm::vec3 RoofColor(
			(f32)(rand() % 256) / 255.0f,
			(f32)(rand() % 256) / 255.0f,
			(f32)(rand() % 256) / 255.0f
		);

		for (u32 i = 0; i < BWay.Way.Ring.size(); ++i)
		{
			const s32 CurrentNanoDegX = BWay.Way.Ring[i][0];
			const s32 CurrentNanoDegY = BWay.Way.Ring[i][1];

			TestMesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), BWay.Height, RoofColor, glm::vec3(0.0f, 1.0f, 0.0f) });
		}

		std::vector<std::vector<std::array<s32, 2>>> Polygon;
		Polygon.push_back(BWay.Way.Ring);

		std::vector<u32> Indices = mapbox::earcut<u32>(Polygon);

		for (u32 i = 0; i < Indices.size(); ++i)
		{
			TestMesh.Indices.push_back(Indices[i] + BaseVertex);
		}

		for (u32 i = 0; i < RingPoints; ++i)
		{
			const s32 CurrentNanoDegX = BWay.Way.Ring[i][0];
			const s32 CurrentNanoDegY = BWay.Way.Ring[i][1];

			const s32 NextNanoDegX = BWay.Way.Ring[(i + 1) % RingPoints][0];
			const s32 NextNanoDegY = BWay.Way.Ring[(i + 1) % RingPoints][1];

			GenerateBuildingWall(TestMesh, CurrentNanoDegX, CurrentNanoDegY, NextNanoDegX, NextNanoDegY, BWay.Height, BWay.MinHeight);
		}

		Range.IndexCount = TestMesh.Indices.size() - Range.FirstIndex;
		TestMesh.Ranges.push_back(Range);
		TestMesh.Instances.push_back({ (u32)BWay.Material });
	}
}