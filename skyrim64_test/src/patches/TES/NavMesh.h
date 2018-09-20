#pragma once

#include <unordered_map>
#include "BSTArray.h"

class NavMesh
{
public:
	constexpr static uint16_t BAD_NAVMESH_TRIANGLE = 0xFFFF;
	constexpr static uint16_t BAD_NAVMESH_VERTEX = 0xFFFF;
	constexpr static uint32_t CUSTOM_NAVMESH_PSEUDODELTE_FLAG = 0x8;

	class BSNavmeshTriangle
	{
	public:
		uint16_t m_Vertices[3];	// Triangle vertices pointing into the mesh vertex array
		uint16_t m_Edges[3];	// Edges pointing to triangles in the mesh
		uint32_t m_ExtraInfo;

		uint16_t GetVertexIndex(uint32_t Vertex) const
		{
			return m_Vertices[Vertex];
		}

		uint16_t GetEdgeIndex(uint32_t Edge) const
		{
			return m_Edges[Edge];
		}

		uint16_t GetEdgeIndexChecked(uint32_t Edge) const
		{
			if (IsEdgePresent(Edge))
				return m_Edges[Edge];

			Assert(false);
			return BAD_NAVMESH_TRIANGLE;
		}

		bool IsEdgePresent(uint32_t Edge) const
		{
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
	};

public:
	char _pad[0x58];
	BSTArray<BSNavmeshTriangle> m_Triangles;

	void hk_DeleteTriangle(uint16_t TriangleIndex)
	{
		if (false)
		{
			// Proceed with normal deletion
			(this->*DeleteTriangle)(TriangleIndex);
		}
		else
		{
			BSNavmeshTriangle& tri = m_Triangles.at(TriangleIndex);
			tri.m_ExtraInfo |= CUSTOM_NAVMESH_PSEUDODELTE_FLAG;

			// Kill all edges referencing the index
			for (uint32_t i = 0; i < m_Triangles.QSize(); i++)
			{
				for (uint32_t edge = 0; edge < 3; edge++)
				{
					if (m_Triangles[i].GetEdgeIndex(edge) == TriangleIndex)
						m_Triangles[i].ClearEdge(edge);
				}
			}

			// Kill edges of this triangle
			tri.ClearEdge(0);
			tri.ClearEdge(1);
			tri.ClearEdge(2);

			// If possible, avoid orphaning vertices when making degenerate tris
			std::unordered_map<uint16_t, bool> orphans
			{
				{ tri.GetVertexIndex(0), true },
				{ tri.GetVertexIndex(1), true },
				{ tri.GetVertexIndex(2), true },
			};

			for (uint32_t i = 0; i < m_Triangles.QSize(); i++)
			{
				if (i == TriangleIndex)
					continue;

				orphans[m_Triangles[i].GetVertexIndex(0)] = false;
				orphans[m_Triangles[i].GetVertexIndex(1)] = false;
				orphans[m_Triangles[i].GetVertexIndex(2)] = false;
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
static_assert(sizeof(NavMesh::BSNavmeshTriangle) == 0x10);
static_assert_offset(NavMesh::BSNavmeshTriangle, m_Vertices, 0x0);
static_assert_offset(NavMesh::BSNavmeshTriangle, m_Edges, 0x6);
static_assert_offset(NavMesh::BSNavmeshTriangle, m_ExtraInfo, 0xC);

static_assert_offset(NavMesh, m_Triangles, 0x58);