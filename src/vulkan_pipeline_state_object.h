//
// Created by rygo6 on 11/6/2022.
//

#ifndef FABRIC_VULKAN_PIPELINE_STATE_OBJECT_H
#define FABRIC_VULKAN_PIPELINE_STATE_OBJECT_H

class VulkanPipelineStateObject {
public:
    VulkanPipelineStateObject( const std::string & sShaderName );
    ~VulkanPipelineStateObject();

    bool BInit(VkDevice pDevice,
               VkDescriptorSetLayout pDescriptorSetLayout,
               VkPipelineLayout pPipelineLayout,
               VkPipelineCache pPipelineCache,
               VkRenderPass pRenderPass,
               VkPrimitiveTopology primitiveTopology,
               VkBool32 depthTestEnableWrite,
               VkVertexInputAttributeDescription attributeDescription[ 3 ],
               size_t stride);

    bool CreateAllShaders();

    const VkPipeline & GetPipeline() const { return m_pPipeline; }

private:
    std::string m_sShaderName;

    VkDevice m_pDevice;
    VkPipelineLayout m_pPipelineLayout;
    VkDescriptorSetLayout m_pDescriptorSetLayout;
    VkPipelineCache m_pPipelineCache;
    VkPipeline m_pPipeline;
    VkRenderPass m_pRenderPass;
    VkShaderModule m_pShaderModules[ 2 ];

    VkPrimitiveTopology m_PrimitiveTopology;
    VkBool32 m_DepthTestEnableWrite;
    VkVertexInputAttributeDescription m_AttributeDescription[ 3 ];
    size_t m_nStride;

    int m_nMSAASampleCount = 4;
};


#endif //FABRIC_VULKAN_PIPELINE_STATE_OBJECT_H
