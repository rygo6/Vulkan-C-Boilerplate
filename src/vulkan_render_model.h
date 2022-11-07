#ifndef FABRIC_VULKAN_RENDER_MODEL_H
#define FABRIC_VULKAN_RENDER_MODEL_H

#include "vulkan_utility.h"
#include "vulkan/vulkan.h"
#include "Matrices.h"
#include "openvr.h"

class VulkanRenderModel
{
public:
    VulkanRenderModel( const std::string & sRenderModelName );
    ~VulkanRenderModel();

    bool BInit(VkDevice pDevice,
               const VkPhysicalDeviceMemoryProperties &memoryProperties,
               VkCommandBuffer pCommandBuffer,
               vr::TrackedDeviceIndex_t unTrackedDeviceIndex,
               VkDescriptorSet pDescriptorSets[3],
               const vr::RenderModel_t &vrModel,
               const vr::RenderModel_TextureMap_t &vrDiffuseTexture);
    void Cleanup();
    void Draw( RenderTarget target, VkCommandBuffer pCommandBuffer, VkPipelineLayout pPipelineLayout, const Matrix4 &matMVP );

    const std::string & GetName() const { return m_sModelName; }

private:
    VkDevice m_pDevice;
    VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
    VkBuffer m_pVertexBuffer;
    VkDeviceMemory m_pVertexBufferMemory;
    VkBuffer m_pIndexBuffer;
    VkDeviceMemory m_pIndexBufferMemory;
    VkImage m_pImage;
    VkDeviceMemory m_pImageMemory;
    VkImageView m_pImageView;
    VkBuffer m_pImageStagingBuffer;
    VkDeviceMemory m_pImageStagingBufferMemory;
    VkBuffer m_pConstantBuffer[ 3 ];
    VkDeviceMemory m_pConstantBufferMemory[ 3 ];
    void *m_pConstantBufferData[ 3 ];
    VkDescriptorSet m_pDescriptorSets[ 3 ];
    VkSampler m_pSampler;

    size_t m_unVertexCount;
    vr::TrackedDeviceIndex_t m_unTrackedDeviceIndex;
    std::string m_sModelName;
};

#endif //FABRIC_VULKAN_RENDER_MODEL_H
