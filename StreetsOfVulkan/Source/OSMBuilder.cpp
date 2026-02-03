#include "OSMBuilder.h"

void BuildingHandler::GenerateBuildingWall(Mesh& mesh, s32 CurrentNanoDegX, s32 CurrentNanoDegY, s32 NextNanoDegX, s32 NextNanoDegY, f32 Height, f32 MinHeight)
{
	const u32 BaseVertex = (u32)mesh.vertices.size();

	const glm::vec3 wallColor(
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f
	);

	mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), Height, wallColor });
	mesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), MinHeight, wallColor });
	mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), MinHeight, wallColor });
	mesh.vertices.push_back({ glm::ivec2(NextNanoDegX, NextNanoDegY), Height, wallColor });

	mesh.Indices.push_back(BaseVertex + 2);
	mesh.Indices.push_back(BaseVertex + 3);
	mesh.Indices.push_back(BaseVertex + 0);
	mesh.Indices.push_back(BaseVertex + 0);
	mesh.Indices.push_back(BaseVertex + 1);
	mesh.Indices.push_back(BaseVertex + 2);
}

bool BuildingHandler::GetBuildingData(const osmium::OSMObject& Object, f32& OutHeight, f32& OutMinHeight)
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

	if (OutHeight == 0.0f)
	{
		return false;
	}

	return true;
}

void RemoveColinearPoints(std::vector<std::array<s32, 2>>& Ring)
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

	const u32 RingPoints = Ring.size();

	const bool IsClockwise = (Area < 0);

	if (!Way.tags().has_key("building") && !Way.tags().has_key("building:part"))
	{
		Ways[Way.id()] = WayGeometry{ IsClockwise, Ring };
		return;
	}

	if (IsClockwise)
	{
		std::reverse(Ring.begin(), Ring.end());
	}

	const f32 LevelHeight = 3.0f;
	f32 Height = 3.0f;
	f32 MinHeight = 0.0f;

	GetBuildingData(Way, Height, MinHeight);

	u32 BaseVertex = TestMesh.vertices.size();

	const glm::vec3 roofColor(
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f,
		(f32)(rand() % 256) / 255.0f
	);

	for (u32 i = 0; i < Ring.size(); ++i)
	{
		const s32 CurrentNanoDegX = Ring[i][0];
		const s32 CurrentNanoDegY = Ring[i][1];

		TestMesh.vertices.push_back({ glm::ivec2(CurrentNanoDegX, CurrentNanoDegY), Height, roofColor });
	}

	std::vector<std::vector<std::array<s32, 2>>> Polygon;
	Polygon.push_back(Ring);

	std::vector<u32> Indices = mapbox::earcut<u32>(Polygon);

	for (u32 i = 0; i < Indices.size(); ++i)
	{
		TestMesh.Indices.push_back(Indices[i] + BaseVertex);
	}

	for (u32 i = 0; i < RingPoints; ++i)
	{
		const s32 CurrentNanoDegX = Ring[i][0];
		const s32 CurrentNanoDegY = Ring[i][1];

		const s32 NextNanoDegX = Ring[(i + 1) % RingPoints][0];
		const s32 NextNanoDegY = Ring[(i + 1) % RingPoints][1];

		GenerateBuildingWall(TestMesh, CurrentNanoDegX, CurrentNanoDegY, NextNanoDegX, NextNanoDegY, Height, MinHeight);
	}
}

void BuildingHandler::relation(const osmium::Relation& rel)
{
	if (!rel.tags().has_tag("type", "multipolygon"))
	{
		return;
	}

	if (!rel.tags().has_key("building"))
	{
		return;
	}

	f32 Height = 3.0f;
	f32 MinHeight = 0.0f;

	GetBuildingData(rel, Height, MinHeight);

	std::vector<std::vector<std::array<s32, 2>>> OuterRings;
	std::vector<std::vector<std::array<s32, 2>>> InnerRings;

	for (const osmium::RelationMember& Member : rel.members())
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
				TestMesh.vertices.push_back({ glm::ivec2(ring[i][0], ring[i][1]), Height, roofColor });
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
}
