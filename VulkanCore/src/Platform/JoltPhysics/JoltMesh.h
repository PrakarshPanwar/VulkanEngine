#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Core/Reference.h"

#include "VulkanCore/Mesh/Mesh.h"

namespace VulkanCore {

	class JoltMesh : public JPH::RefTargetVirtual
	{
	public:
		JoltMesh(const JPH::DebugRenderer::Vertex* verticesPtr, int vertexCount, const JPH::uint32* indicesPtr, int indexCount);

		void AddRef() override { ++m_Refs; }
		void Release() override { if (--m_Refs == 0) delete this; }

		inline std::shared_ptr<Mesh> GetMesh() const { return m_JoltMesh; }
	private:
		void CalculateTangentBasis(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	private:
		std::shared_ptr<Mesh> m_JoltMesh;

		std::atomic<uint32_t> m_Refs = 0;
	};

}
