//
// Created by rygo6 on 11/6/2022.
//

#include "vulkan_utility.h"
#include "vulkan_pipeline_state_object.h"
#include "pathtools.h"
#include "Vectors.h"

VulkanPipelineStateObject::VulkanPipelineStateObject( const std::string & sShaderName )
        : m_sShaderName( sShaderName )
        , m_pDevice( VK_NULL_HANDLE )
        , m_pPipelineLayout( VK_NULL_HANDLE )
        , m_pDescriptorSetLayout( VK_NULL_HANDLE )
        , m_pPipelineCache( VK_NULL_HANDLE )
        , m_pPipeline( VK_NULL_HANDLE )
        , m_pRenderPass( VK_NULL_HANDLE )
        , m_PrimitiveTopology( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST )
{
    dprintf( "VulkanPipelineStateObject" );
    memset( &m_pShaderModules[ 0 ], 0, sizeof( m_pShaderModules ) );
    memset( &m_AttributeDescription[ 0 ], 0, sizeof( m_AttributeDescription ) );
}

VulkanPipelineStateObject::~VulkanPipelineStateObject()
{
    dprintf( "VulkanPipelineStateObject::~VulkanPipelineStateObject()" );
    vkDestroyPipeline( m_pDevice, m_pPipeline, nullptr );
    for ( uint32_t nShader = 0; nShader < _countof( m_pShaderModules); nShader++ )
    {
        vkDestroyShaderModule( m_pDevice, m_pShaderModules[ nShader ], nullptr );
    }
}

bool VulkanPipelineStateObject::BInit(VkDevice pDevice,
                                      VkDescriptorSetLayout pDescriptorSetLayout,
                                      VkPipelineLayout pPipelineLayout,
                                      VkPipelineCache pPipelineCache,
                                      VkRenderPass pRenderPass,
                                      VkPrimitiveTopology primitiveTopology,
                                      VkBool32 depthTestEnableWrite,
                                      VkVertexInputAttributeDescription attributeDescription[ 3 ],
                                      size_t stride)
{
    m_pDevice = pDevice;
    m_pDescriptorSetLayout = pDescriptorSetLayout;
    m_pPipelineLayout = pPipelineLayout;
    m_pPipelineCache = pPipelineCache;
    m_pRenderPass = pRenderPass;
    m_PrimitiveTopology = primitiveTopology;
    m_DepthTestEnableWrite = depthTestEnableWrite;
    m_AttributeDescription[0] = attributeDescription[0];
    m_AttributeDescription[1] = attributeDescription[1];
    m_AttributeDescription[3] = attributeDescription[3];
    m_nStride = stride;

    return true;
}

bool VulkanPipelineStateObject::CreateAllShaders()
{
    VkResult nResult;
    std::string sExecutableDirectory = Path_StripFilename( Path_GetExecutablePath() );

    const char *pStageNames[ 2 ] =
            {
                    "vs",
                    "ps"
            };

    // Load the SPIR-V into shader modules
    for ( int32_t nStage = 0; nStage <= 1; nStage++ )
    {
        char shaderFileName[ 1024 ];
        sprintf( shaderFileName, "../shaders/%s_%s.spv", m_sShaderName.c_str(), pStageNames[ nStage ] );
        std::string shaderPath =  Path_MakeAbsolute( shaderFileName, sExecutableDirectory );

        FILE *fp = fopen( shaderPath.c_str(), "rb" );
        if ( fp == NULL )
        {
            dprintf( "Error opening SPIR-V file: %s\n", shaderPath.c_str() );
            return false;
        }
        fseek( fp, 0, SEEK_END );
        size_t nSize = ftell( fp );
        fseek( fp, 0, SEEK_SET );

        char *pBuffer = new char[ nSize ];
        if ( fread( pBuffer, 1, nSize, fp ) != nSize )
        {
            dprintf( "Error reading SPIR-V file: %s\n", shaderPath.c_str() );
            return false;
        }
        fclose( fp );

        // Create the shader module
        VkShaderModuleCreateInfo shaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shaderModuleCreateInfo.codeSize = nSize;
        shaderModuleCreateInfo.pCode = ( const uint32_t *) pBuffer;
        nResult = vkCreateShaderModule( m_pDevice, &shaderModuleCreateInfo, nullptr, &m_pShaderModules[ nStage ] );
        if ( nResult != VK_SUCCESS )
        {
            dprintf( "Error creating shader module for %s, error %d\n", shaderPath.c_str(), nResult );
            return false;
        }

        delete [] pBuffer;
    }

    // Create the PSOs
//    for ( uint32_t nPSO = 0; nPSO < PSO_COUNT; nPSO++ )
//    {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

        // VkPipelineVertexInputStateCreateInfo
        VkVertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = m_nStride;

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        for ( uint32_t nAttr = 0; nAttr < 3; nAttr++ )
        {
            if ( m_AttributeDescription[nAttr].format != VK_FORMAT_UNDEFINED )
            {
                vertexInputCreateInfo.vertexAttributeDescriptionCount++;
            }
        }
        vertexInputCreateInfo.pVertexAttributeDescriptions = &m_AttributeDescription[0];
        vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;

        // VkPipelineDepthStencilStateCreateInfo
        VkPipelineDepthStencilStateCreateInfo dsState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        dsState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//        dsState.depthTestEnable = (nPSO != PSO_SCREEN ) ? VK_TRUE : VK_FALSE;
//        dsState.depthWriteEnable = (nPSO != PSO_SCREEN ) ? VK_TRUE : VK_FALSE;
        dsState.depthTestEnable = m_DepthTestEnableWrite;
        dsState.depthWriteEnable = m_DepthTestEnableWrite;
//        dsState.depthTestEnable = VK_TRUE;
//        dsState.depthWriteEnable = VK_TRUE;
        dsState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        dsState.depthBoundsTestEnable = VK_FALSE;
        dsState.stencilTestEnable = VK_FALSE;
        dsState.minDepthBounds = 0.0f;
        dsState.maxDepthBounds = 0.0f;

        // VkPipelineColorBlendStateCreateInfo
        VkPipelineColorBlendStateCreateInfo cbState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cbState.logicOpEnable = VK_FALSE;
        cbState.logicOp = VK_LOGIC_OP_COPY;
        VkPipelineColorBlendAttachmentState cbAttachmentState = {};
        cbAttachmentState.blendEnable = VK_FALSE;
        cbAttachmentState.colorWriteMask = 0xf;
        cbState.attachmentCount = 1;
        cbState.pAttachments = &cbAttachmentState;

        // VkPipelineColorBlendStateCreateInfo
        VkPipelineRasterizationStateCreateInfo rsState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rsState.polygonMode = VK_POLYGON_MODE_FILL;
        rsState.cullMode = VK_CULL_MODE_BACK_BIT;
        rsState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rsState.lineWidth = 1.0f;

        // VkPipelineInputAssemblyStateCreateInfo
        VkPipelineInputAssemblyStateCreateInfo iaState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
//        iaState.topology = ( nPSO == PSO_AXES ) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        iaState.topology = m_PrimitiveTopology;
//        iaState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        iaState.primitiveRestartEnable = VK_FALSE;

        // VkPipelineMultisampleStateCreateInfo
        VkPipelineMultisampleStateCreateInfo msState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
//        msState.rasterizationSamples = (nPSO == PSO_SCREEN ) ? VK_SAMPLE_COUNT_1_BIT : ( VkSampleCountFlagBits ) m_nMSAASampleCount;
//        msState.rasterizationSamples = ( VkSampleCountFlagBits ) m_nMSAASampleCount;
        msState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        msState.minSampleShading = 0.0f;
        uint32_t nSampleMask = 0xFFFFFFFF;
        msState.pSampleMask = &nSampleMask;

        // VkPipelineViewportStateCreateInfo
        VkPipelineViewportStateCreateInfo vpState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vpState.viewportCount = 1;
        vpState.scissorCount = 1;

        VkPipelineShaderStageCreateInfo shaderStages[ 2 ] = { };
        shaderStages[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[ 0 ].module = m_pShaderModules[0];
        shaderStages[ 0 ].pName = "VSMain";

        shaderStages[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[ 1 ].module = m_pShaderModules[1];
        shaderStages[ 1 ].pName = "PSMain";

        pipelineCreateInfo.layout = m_pPipelineLayout;

        // Set pipeline states
        pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &iaState;
        pipelineCreateInfo.pViewportState = &vpState;
        pipelineCreateInfo.pRasterizationState = &rsState;
        pipelineCreateInfo.pMultisampleState = &msState;
        pipelineCreateInfo.pDepthStencilState = &dsState;
        pipelineCreateInfo.pColorBlendState = &cbState;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = &shaderStages[ 0 ];
        pipelineCreateInfo.renderPass = m_pRenderPass;

        static VkDynamicState dynamicStates[] =
                {
                        VK_DYNAMIC_STATE_VIEWPORT,
                        VK_DYNAMIC_STATE_SCISSOR,
                };

        static VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.pNext = NULL;
        dynamicStateCreateInfo.dynamicStateCount = _countof( dynamicStates );
        dynamicStateCreateInfo.pDynamicStates = &dynamicStates[ 0 ];
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        // Create the pipeline
        nResult = vkCreateGraphicsPipelines( m_pDevice, m_pPipelineCache, 1, &pipelineCreateInfo, NULL, &m_pPipeline );
        if ( nResult != VK_SUCCESS )
        {
            dprintf( "vkCreateGraphicsPipelines failed with error %d\n", nResult );
            return false;
        }
//    }

    return true;
}
