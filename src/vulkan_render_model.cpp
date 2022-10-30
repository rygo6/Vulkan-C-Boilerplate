#include "vulkan_render_model.h"

#include <vector>

//-----------------------------------------------------------------------------
// Purpose: Create/destroy Vulkan Render Models
//-----------------------------------------------------------------------------
VulkanRenderModel::VulkanRenderModel( const std::string & sRenderModelName )
        : m_sModelName( sRenderModelName )
        , m_pDevice( VK_NULL_HANDLE )
        , m_pVertexBuffer( VK_NULL_HANDLE )
        , m_pVertexBufferMemory( VK_NULL_HANDLE )
        , m_pIndexBuffer( VK_NULL_HANDLE )
        , m_pIndexBufferMemory( VK_NULL_HANDLE )
        , m_pImage( VK_NULL_HANDLE )
        , m_pImageMemory( VK_NULL_HANDLE )
        , m_pImageView( VK_NULL_HANDLE )
        , m_pImageStagingBuffer( VK_NULL_HANDLE )
        , m_pImageStagingBufferMemory( VK_NULL_HANDLE )
        , m_pSampler( VK_NULL_HANDLE )
{
    memset( m_pConstantBuffer, 0, sizeof( m_pConstantBuffer ) );
    memset( m_pConstantBufferMemory, 0, sizeof( m_pConstantBufferMemory ) );
    memset( m_pConstantBufferData, 0, sizeof( m_pConstantBufferData ) );
    memset( m_pDescriptorSets, 0, sizeof( m_pDescriptorSets ) );
}

VulkanRenderModel::~VulkanRenderModel()
{
    Cleanup();
}

//-----------------------------------------------------------------------------
// Purpose: Allocates and populates the Vulkan resources for a render model
//-----------------------------------------------------------------------------
bool VulkanRenderModel::BInit( VkDevice pDevice, const VkPhysicalDeviceMemoryProperties &memoryProperties, VkCommandBuffer pCommandBuffer, vr::TrackedDeviceIndex_t unTrackedDeviceIndex, VkDescriptorSet pDescriptorSets[ 3 ], const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture )
{
    m_pDevice = pDevice;
    m_physicalDeviceMemoryProperties = memoryProperties;
    m_unTrackedDeviceIndex = unTrackedDeviceIndex;
    m_pDescriptorSets[ 0 ] = pDescriptorSets[ 0 ];
    m_pDescriptorSets[ 1 ] = pDescriptorSets[ 1 ];
    m_pDescriptorSets[ 2 ] = pDescriptorSets[ 2 ];

    // Create and populate the vertex buffer
    {
        if ( !CreateVulkanBuffer( m_pDevice, m_physicalDeviceMemoryProperties, vrModel.rVertexData, sizeof( vr::RenderModel_Vertex_t ) * vrModel.unVertexCount,
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &m_pVertexBuffer, &m_pVertexBufferMemory ) )
        {
            return false;
        }
    }

    // Create and populate the index buffer
    {
        if ( !CreateVulkanBuffer( m_pDevice, m_physicalDeviceMemoryProperties, vrModel.rIndexData, sizeof( uint16_t ) * vrModel.unTriangleCount * 3,
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &m_pIndexBuffer, &m_pIndexBufferMemory ) )
        {
            return false;
        }
    }

    // create and populate the texture
    {
        int nImageWidth = vrDiffuseTexture.unWidth;
        int nImageHeight = vrDiffuseTexture.unHeight;

        // Copy the base level to a buffer, reserve space for mips (overreserve by a bit to avoid having to calc mipchain size ahead of time)
        VkDeviceSize nBufferSize = 0;
        uint8_t *pBuffer = new uint8_t[ nImageWidth * nImageHeight * 4 * 2 ];
        uint8_t *pPrevBuffer = pBuffer;
        uint8_t *pCurBuffer = pBuffer;
        memcpy( pCurBuffer, vrDiffuseTexture.rubTextureMapData, sizeof( uint8_t ) * nImageWidth * nImageHeight * 4 );
        pCurBuffer += sizeof( uint8_t ) * nImageWidth * nImageHeight * 4;

        std::vector< VkBufferImageCopy > bufferImageCopies;
        VkBufferImageCopy bufferImageCopy = {};
        bufferImageCopy.bufferOffset = 0;
        bufferImageCopy.bufferRowLength = 0;
        bufferImageCopy.bufferImageHeight = 0;
        bufferImageCopy.imageSubresource.baseArrayLayer = 0;
        bufferImageCopy.imageSubresource.layerCount = 1;
        bufferImageCopy.imageSubresource.mipLevel = 0;
        bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferImageCopy.imageOffset.x = 0;
        bufferImageCopy.imageOffset.y = 0;
        bufferImageCopy.imageOffset.z = 0;
        bufferImageCopy.imageExtent.width = nImageWidth;
        bufferImageCopy.imageExtent.height = nImageHeight;
        bufferImageCopy.imageExtent.depth = 1;
        bufferImageCopies.push_back( bufferImageCopy );

        int nMipWidth = nImageWidth;
        int nMipHeight = nImageHeight;

        while( nMipWidth > 1 && nMipHeight > 1 )
        {
            GenMipMapRGBA( pPrevBuffer, pCurBuffer, nMipWidth, nMipHeight, &nMipWidth, &nMipHeight );
            bufferImageCopy.bufferOffset = pCurBuffer - pBuffer;
            bufferImageCopy.imageSubresource.mipLevel++;
            bufferImageCopy.imageExtent.width = nMipWidth;
            bufferImageCopy.imageExtent.height = nMipHeight;
            bufferImageCopies.push_back( bufferImageCopy );
            pPrevBuffer = pCurBuffer;
            pCurBuffer += ( nMipWidth * nMipHeight * 4 * sizeof( uint8_t ) );
        }
        nBufferSize = pCurBuffer - pBuffer;

        // Create the image
        VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = nImageWidth;
        imageCreateInfo.extent.height = nImageHeight;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = ( uint32_t ) bufferImageCopies.size();
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCreateInfo.flags = 0;
        vkCreateImage( m_pDevice, &imageCreateInfo, nullptr, &m_pImage );

        VkMemoryRequirements memoryRequirements = {};
        vkGetImageMemoryRequirements( m_pDevice, m_pImage, &memoryRequirements );

        VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        MemoryTypeFromProperties( m_physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex );
        vkAllocateMemory( m_pDevice, &memoryAllocateInfo, nullptr, &m_pImageMemory );
        vkBindImageMemory( m_pDevice, m_pImage, m_pImageMemory, 0 );

        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = m_pImage;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = imageCreateInfo.format;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = imageCreateInfo.mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        vkCreateImageView( m_pDevice, &imageViewCreateInfo, nullptr, &m_pImageView );

        // Create a staging buffer
        VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size = nBufferSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vkCreateBuffer( m_pDevice, &bufferCreateInfo, nullptr, &m_pImageStagingBuffer );
        vkGetBufferMemoryRequirements( m_pDevice, m_pImageStagingBuffer, &memoryRequirements );

        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        MemoryTypeFromProperties( m_physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex );
        allocInfo.allocationSize = memoryRequirements.size;

        vkAllocateMemory( m_pDevice, &allocInfo, nullptr, &m_pImageStagingBufferMemory );
        vkBindBufferMemory( m_pDevice, m_pImageStagingBuffer, m_pImageStagingBufferMemory, 0 );

        // Copy memory to the staging buffer
        void *pData;
        VkResult nResult = vkMapMemory( m_pDevice, m_pImageStagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &pData );
        if ( nResult != VK_SUCCESS )
        {
            dprintf( "vkMapMemory returned error %d\n", nResult );
            return false;
        }
        memcpy( pData, pBuffer, nBufferSize );
        vkUnmapMemory( m_pDevice, m_pImageStagingBufferMemory );

        VkMappedMemoryRange memoryRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        memoryRange.memory = m_pImageStagingBufferMemory;
        memoryRange.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges( m_pDevice, 1, &memoryRange );

        // Transition the image to TRANSFER_DST to receive image
        VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.image = m_pImage;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = imageCreateInfo.mipLevels;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier( pCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

        // Issue the copy to fill the image data
        vkCmdCopyBufferToImage( pCommandBuffer, m_pImageStagingBuffer, m_pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ( uint32_t ) bufferImageCopies.size(), &bufferImageCopies[ 0 ] );

        // Transition the image to SHADER_READ_OPTIMAL for reading
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier( pCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );

        // Create a sampler
        VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy = 16.0f;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = ( float ) imageCreateInfo.mipLevels;
        vkCreateSampler( m_pDevice, &samplerCreateInfo, nullptr, &m_pSampler );
    }

    // Create a constant buffer to hold the transform (one for each eye)
    for (uint32_t renderTarget = 0; renderTarget < RenderTarget::NUM_RENDER_TARGETS; renderTarget++ )
    {
        VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size = sizeof( Matrix4 );
        bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        vkCreateBuffer( m_pDevice, &bufferCreateInfo, nullptr, &m_pConstantBuffer[ renderTarget ] );

        VkMemoryRequirements memoryRequirements = {};
        vkGetBufferMemoryRequirements(m_pDevice, m_pConstantBuffer[ renderTarget ], &memoryRequirements );
        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        MemoryTypeFromProperties( m_physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, &allocInfo.memoryTypeIndex );
        allocInfo.allocationSize = memoryRequirements.size;

        vkAllocateMemory( m_pDevice, &allocInfo, nullptr, &m_pConstantBufferMemory[ renderTarget ] );
        vkBindBufferMemory(m_pDevice, m_pConstantBuffer[ renderTarget ], m_pConstantBufferMemory[ renderTarget ], 0 );

        // Map and keep mapped persistently
        vkMapMemory(m_pDevice, m_pConstantBufferMemory[ renderTarget ], 0, VK_WHOLE_SIZE, 0, &m_pConstantBufferData[ renderTarget ] );

        // Bake the descriptor set
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_pConstantBuffer[ renderTarget ];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageView = m_pImageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = m_pSampler;

        VkWriteDescriptorSet writeDescriptorSets[ 3 ] = { };
        writeDescriptorSets[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[ 0 ].dstSet = m_pDescriptorSets[ renderTarget ];
        writeDescriptorSets[ 0 ].dstBinding = 0;
        writeDescriptorSets[ 0 ].descriptorCount = 1;
        writeDescriptorSets[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[ 0 ].pBufferInfo = &bufferInfo;
        writeDescriptorSets[ 1 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[ 1 ].dstSet = m_pDescriptorSets[ renderTarget ];
        writeDescriptorSets[ 1 ].dstBinding = 1;
        writeDescriptorSets[ 1 ].descriptorCount = 1;
        writeDescriptorSets[ 1 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSets[ 1 ].pImageInfo = &imageInfo;
        writeDescriptorSets[ 2 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[ 2 ].dstSet = m_pDescriptorSets[ renderTarget ];
        writeDescriptorSets[ 2 ].dstBinding = 2;
        writeDescriptorSets[ 2 ].descriptorCount = 1;
        writeDescriptorSets[ 2 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writeDescriptorSets[ 2 ].pImageInfo = &samplerInfo;

        vkUpdateDescriptorSets( m_pDevice, _countof( writeDescriptorSets ), writeDescriptorSets, 0, nullptr );
    }

    m_unVertexCount = vrModel.unTriangleCount * 3;

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Frees the Vulkan resources for a render model
//-----------------------------------------------------------------------------
void VulkanRenderModel::Cleanup()
{
    if ( m_pVertexBuffer != VK_NULL_HANDLE )
    {
        vkDestroyBuffer( m_pDevice, m_pVertexBuffer, nullptr );
        m_pVertexBuffer = VK_NULL_HANDLE;
    }

    if ( m_pVertexBufferMemory != VK_NULL_HANDLE )
    {
        vkFreeMemory( m_pDevice, m_pVertexBufferMemory, nullptr );
        m_pVertexBufferMemory = VK_NULL_HANDLE;
    }

    if ( m_pIndexBuffer != VK_NULL_HANDLE )
    {
        vkDestroyBuffer( m_pDevice, m_pIndexBuffer, nullptr );
        m_pIndexBuffer = VK_NULL_HANDLE;
    }

    if ( m_pIndexBufferMemory != VK_NULL_HANDLE )
    {
        vkFreeMemory( m_pDevice, m_pIndexBufferMemory, nullptr );
        m_pIndexBufferMemory = VK_NULL_HANDLE;
    }

    if ( m_pImage != VK_NULL_HANDLE )
    {
        vkDestroyImage( m_pDevice, m_pImage, nullptr );
        m_pImage = VK_NULL_HANDLE;
    }

    if ( m_pImageMemory != VK_NULL_HANDLE )
    {
        vkFreeMemory( m_pDevice, m_pImageMemory, nullptr );
        m_pImageMemory = VK_NULL_HANDLE;
    }

    if ( m_pImageView != VK_NULL_HANDLE )
    {
        vkDestroyImageView( m_pDevice, m_pImageView, nullptr );
        m_pImageView = VK_NULL_HANDLE;
    }

    if ( m_pImageStagingBuffer != VK_NULL_HANDLE )
    {
        vkDestroyBuffer( m_pDevice, m_pImageStagingBuffer, nullptr );
        m_pImageStagingBuffer = VK_NULL_HANDLE;
    }

    if ( m_pImageStagingBufferMemory != VK_NULL_HANDLE )
    {
        vkFreeMemory( m_pDevice, m_pImageStagingBufferMemory, nullptr );
        m_pImageStagingBufferMemory = VK_NULL_HANDLE;
    }

    for ( uint32_t nEye = 0; nEye < 2; nEye++ )
    {
        if ( m_pConstantBuffer[ nEye ] != VK_NULL_HANDLE )
        {
            vkDestroyBuffer( m_pDevice, m_pConstantBuffer[ nEye ], nullptr );
            m_pConstantBuffer[ nEye ] = VK_NULL_HANDLE;
        }

        if ( m_pConstantBufferMemory != VK_NULL_HANDLE )
        {
            vkFreeMemory( m_pDevice, m_pConstantBufferMemory[ nEye ], nullptr );
            m_pConstantBufferMemory[ nEye ] = VK_NULL_HANDLE;
        }
    }

    if ( m_pSampler != VK_NULL_HANDLE )
    {
        vkDestroySampler( m_pDevice, m_pSampler, nullptr );
        m_pSampler = VK_NULL_HANDLE;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Draws the render model
//-----------------------------------------------------------------------------
void VulkanRenderModel::Draw( RenderTarget target, VkCommandBuffer pCommandBuffer, VkPipelineLayout pPipelineLayout, const Matrix4 &matMVP )
{
    // Update the CB with the transform
    memcpy( m_pConstantBufferData[ target ], &matMVP, sizeof( matMVP ) );

    // Bind the descriptor set
    vkCmdBindDescriptorSets( pCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineLayout, 0, 1, &m_pDescriptorSets[ target ], 0, nullptr );

    // Bind the VB/IB and draw
    VkDeviceSize nOffsets[ 1 ] = { 0 };
    vkCmdBindVertexBuffers( pCommandBuffer, 0, 1, &m_pVertexBuffer, &nOffsets[ 0 ] );
    vkCmdBindIndexBuffer( pCommandBuffer, m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT16 );
    vkCmdDrawIndexed( pCommandBuffer, m_unVertexCount, 1, 0, 0, 0 );
}