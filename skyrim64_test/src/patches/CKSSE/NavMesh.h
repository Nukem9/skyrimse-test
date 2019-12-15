#pragma once

#include <unordered_map>
#include "../TES/BSTArray.h"

class BSNavmesh
{
public:
	constexpr static uint16_t BAD_NAVMESH_TRIANGLE = 0xFFFF;
	constexpr static uint16_t BAD_NAVMESH_VERTEX = 0xFFFF;

	constexpr static uint32_t CUSTOM_NAVMESH_PSEUDODELTE_FLAG = 0x8;
	constexpr static uint32_t NAVMESH_FOUND_FLAG = 0x800;

	class VertexData
	{
	public:
		float m_Values[3];
	};

	class BSNavmeshTriangle
	{
	public:
		uint16_t m_Vertices[3];	// Triangle vertices pointing into the mesh vertex array
		uint16_t m_Edges[3];	// Index into the triangle array of the navmesh OR an index into the edge links array
		uint32_t m_ExtraInfo;

		uint16_t GetVertexIndex(uint32_t Vertex) const
		{
			return m_Vertices[Vertex];
		}

		uint16_t GetEdgeIndex(uint32_t Edge) const
		{
			return m_Edges[Edge];
		}

		uint16_t GetEdgeLinkIndex(uint32_t Edge) const
		{
			if (HasExtraInfo(Edge))
				return m_Edges[Edge];

			Assert(false);
			return BAD_NAVMESH_TRIANGLE;
		}

		bool HasExtraInfo(uint32_t Edge) const
		{
			// If set, m_Edge[Edge] points in to m_ExtraInfo[]
			// If not set, triangle is in the same mesh
			return (m_ExtraInfo & (1 << Edge)) != 0;
		}

		void ClearEdge(uint32_t Edge)
		{
			m_ExtraInfo &= ~(1 << Edge);
			m_Edges[Edge] = BAD_NAVMESH_TRIANGLE;
		}

		uint16_t hk_GetVertexIndex_DegenerateCheck(uint32_t Vertex)
		{
			// If special flag is set: return an invalid value to make the == comparison fail
			if (m_ExtraInfo & CUSTOM_NAVMESH_PSEUDODELTE_FLAG)
				return BAD_NAVMESH_VERTEX;

			return GetVertexIndex(Vertex);
		}

		uint16_t hk_GetVertexIndex_VertexCheck(uint32_t Vertex)
		{
			// Same as degenerate check, but can't return 2 identical values
			static uint16_t fakeCounter = 0;

			if (m_ExtraInfo & CUSTOM_NAVMESH_PSEUDODELTE_FLAG)
			{
				fakeCounter = (fakeCounter + 1) % 0xFFFF;
				return fakeCounter;
			}

			return GetVertexIndex(Vertex);
		}
	};

	class EdgeExtraInfo
	{
	public:
		uint32_t m_Type;
		uint32_t m_NavmeshID;
		uint16_t m_TriangleIndex;
	};

	char _pad0[0x10];
	BSTArray<VertexData> m_Vertices;
	BSTArray<BSNavmeshTriangle> m_Triangles;
	BSTArray<EdgeExtraInfo> m_ExtraInfo;

	EdgeExtraInfo *GetEdgeExtraInfo(uint16_t TriangleIndex, uint32_t EdgeIndex)
	{
		AssertMsg(TriangleIndex < m_Triangles.QSize(), "Invalid triangle index");
		AssertMsg(EdgeIndex < 3, "Invalid edge index");

		if (m_Triangles[TriangleIndex].HasExtraInfo(EdgeIndex))
		{
			uint16_t index = m_Triangles[TriangleIndex].GetEdgeLinkIndex(EdgeIndex);

			if (index < m_ExtraInfo.QSize())
				return &m_ExtraInfo.at(index);
		}

		return nullptr;
	}
};
static_assert(sizeof(BSNavmesh::VertexData) == 0xC);

static_assert(sizeof(BSNavmesh::BSNavmeshTriangle) == 0x10);
static_assert_offset(BSNavmesh::BSNavmeshTriangle, m_Vertices, 0x0);
static_assert_offset(BSNavmesh::BSNavmeshTriangle, m_Edges, 0x6);
static_assert_offset(BSNavmesh::BSNavmeshTriangle, m_ExtraInfo, 0xC);

static_assert(sizeof(BSNavmesh::EdgeExtraInfo) == 0xC);

static_assert_offset(BSNavmesh, m_Vertices, 0x10);
static_assert_offset(BSNavmesh, m_Triangles, 0x28);
static_assert_offset(BSNavmesh, m_ExtraInfo, 0x40);

class NavMesh /*: TESForm, BSNavmesh */
{
public:
	char _pad0[0x30];
	BSNavmesh m_Data;

	void hk_DeleteTriangle(uint16_t TriangleIndex)
	{
		if (false)
		{
			// Proceed with normal deletion
			(this->*DeleteTriangle)(TriangleIndex);
		}
		else
		{
			auto PathingGetSingleton = (void *(__fastcall *)())(g_ModuleBase + 0x1354BC0);
			auto PathingGetForm = (NavMesh *(__fastcall *)(void *, uint32_t))(g_ModuleBase + 0x1D9C8A0);

			BSNavmesh::BSNavmeshTriangle& tri = m_Data.m_Triangles.at(TriangleIndex);

			// Mark with custom flag
			tri.m_ExtraInfo |= BSNavmesh::CUSTOM_NAVMESH_PSEUDODELTE_FLAG;

			// Kill all edges referencing this triangle & kill edges this triangle references
			auto removeEdgeReferences = [this, TriangleIndex](BSNavmesh::BSNavmeshTriangle& Triangle)
			{
				for (uint32_t i = 0; i < 3; i++)
				{
					if (!Triangle.HasExtraInfo(i) && Triangle.GetEdgeIndex(i) == TriangleIndex)
						Triangle.ClearEdge(i);
				}
			};

			for (int edge = 0; edge < 3; edge++)
			{
				BSNavmesh::EdgeExtraInfo *extraInfo = m_Data.GetEdgeExtraInfo(TriangleIndex, edge);
				uint16_t index = (extraInfo) ? extraInfo->m_TriangleIndex : tri.GetEdgeIndex(edge);

				if (index != BSNavmesh::BAD_NAVMESH_TRIANGLE)
				{
					if (extraInfo)
					{
						// Separated mesh link
						Assert(extraInfo->m_NavmeshID != 0);
						NavMesh *externalMesh = PathingGetForm(PathingGetSingleton(), extraInfo->m_NavmeshID);

						Assert(externalMesh);
						removeEdgeReferences(externalMesh->m_Data.m_Triangles.at(index));

						extraInfo->m_NavmeshID = 0;
						extraInfo->m_TriangleIndex = BSNavmesh::BAD_NAVMESH_TRIANGLE;
					}
					else
					{
						removeEdgeReferences(m_Data.m_Triangles.at(index));
					}
				}

				tri.ClearEdge(edge);
			}

			// If possible, avoid orphaning vertices when making it degenerate
			std::unordered_map<uint16_t, bool> orphans
			{
				{ tri.GetVertexIndex(0), true },
				{ tri.GetVertexIndex(1), true },
				{ tri.GetVertexIndex(2), true },
			};

			for (uint32_t i = 0; i < m_Data.m_Triangles.QSize(); i++)
			{
				if (i == TriangleIndex)
					continue;

				orphans[m_Data.m_Triangles[i].GetVertexIndex(0)] = false;
				orphans[m_Data.m_Triangles[i].GetVertexIndex(1)] = false;
				orphans[m_Data.m_Triangles[i].GetVertexIndex(2)] = false;
			}

			uint16_t vert = tri.GetVertexIndex(0);

			for (auto[orphanVert, orphaned] : orphans)
			{
				if (!orphaned)
					continue;

				vert = orphanVert;
				break;
			}

			tri.m_Vertices[0] = vert;
			tri.m_Vertices[1] = vert;
			tri.m_Vertices[2] = vert;
		}
	}

	static inline decltype(&hk_DeleteTriangle) DeleteTriangle;
};
static_assert_offset(NavMesh, m_Data, 0x30);