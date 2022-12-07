/*#include <engine/graphics/AccelerationStructure.hpp>

namespace en
{
	BotLevAccStruct::BotLevAccStruct(const Model& model)
	{
		VkDevice device = VulkanAPI::GetDevice();

		VkDeviceOrHostAddressConstKHR vertexBufDevAddr = {}; // TODO
		VkDeviceOrHostAddressConstKHR indexBufDevAddr = {};

		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.pNext = nullptr;
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.vertexData = vertexBufDevAddr;
		geometry.geometry.triangles.vertexStride = sizeof(en::PNTVertex);
		geometry.geometry.triangles.maxVertex = vertexCount;
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.indexData = indexBufDevAddr;
		geometry.geometry.triangles.transformData.deviceAddress = 0;
		geometry.flags = 0;

		VkAccelerationStructureBuildGeometryInfoKHR buildGeomInfo = {};
		buildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeomInfo.pNext = nullptr;
		buildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildGeomInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeomInfo.srcAccelerationStructure;
		buildGeomInfo.dstAccelerationStructure;
		buildGeomInfo.geometryCount = 1;
		buildGeomInfo.pGeometries = &geometry;
		buildGeomInfo.ppGeometries = nullptr;
		buildGeomInfo.scratchData.deviceAddress = 0;

		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		buildSizesInfo.pNext = nullptr;

		vkGetAccelerationStructureBuildSizesKHR(
			device, 
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
			&buildGeomInfo, 
			triangleCount, 
			&buildSizesInfo);

		VkAccelerationStructureCreateInfoKHR accStructCI = {};
		accStructCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accStructCI.pNext = nullptr;
		accStructCI.createFlags = 0;
		accStructCI.buffer = buffer;
		accStructCI.offset = 0;
		accStructCI.size = ;
		accStructCI.type;
		accStructCI.deviceAddress;
		vkCreateAccelerationStructureKHR(device, )
	}

	void BotLevAccStruct::Destroy()
	{

	}
}
*/