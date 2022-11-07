#ifndef FABRIC_VULKAN_UTILITY_H
#define FABRIC_VULKAN_UTILITY_H

#include <SDL.h>
#include <SDL_syswm.h>
#include <stdio.h>
#include "vulkan/vulkan.h"
#include "openvr.h"
#include "Vectors.h"

static bool g_bPrintf = true;

struct FramebufferDesc
{
    VkImage m_pImage;
    VkImageLayout m_nImageLayout;
    VkDeviceMemory m_pDeviceMemory;
    VkImageView m_pImageView;
    VkImage m_pDepthStencilImage;
    VkImageLayout m_nDepthStencilImageLayout;
    VkDeviceMemory m_pDepthStencilDeviceMemory;
    VkImageView m_pDepthStencilImageView;
    VkRenderPass m_pRenderPass;
    VkFramebuffer m_pFramebuffer;
};

struct VertexDataScene
{
    Vector3 position;
    Vector2 texCoord;
};

struct VertexDataWindow
{
    Vector2 position;
    Vector2 texCoord;

    VertexDataWindow( const Vector2 & pos, const Vector2 tex ) :  position(pos), texCoord(tex) {	}
};

// Pipeline state objects
enum PipelineStateObjectEnum_t
{
    PSO_SCENE = 0,
    PSO_AXES,
    PSO_RENDERMODEL,
    PSO_SCREEN,
    PSO_COUNT
};

// Indices of descriptor sets for rendering
enum DescriptorSetIndex_t
{
    DESCRIPTOR_SET_LEFT_EYE_SCENE = 0,
    DESCRIPTOR_SET_RIGHT_EYE_SCENE,
    DESCRIPTOR_SET_SCREEN_SCENE,
    DESCRIPTOR_SET_SCREEN_TEXTURE,
    DESCRIPTOR_SET_LEFT_EYE_RENDER_MODEL0,
    DESCRIPTOR_SET_LEFT_EYE_RENDER_MODEL_MAX = DESCRIPTOR_SET_LEFT_EYE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    DESCRIPTOR_SET_RIGHT_EYE_RENDER_MODEL0,
    DESCRIPTOR_SET_RIGHT_EYE_RENDER_MODEL_MAX = DESCRIPTOR_SET_RIGHT_EYE_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    DESCRIPTOR_SET_SCREEN_RENDER_MODEL0,
    DESCRIPTOR_SET_SCREEN_RENDER_MODEL_MAX = DESCRIPTOR_SET_SCREEN_RENDER_MODEL0 + vr::k_unMaxTrackedDeviceCount,
    NUM_DESCRIPTOR_SETS
};

enum RenderTarget
{
    EYE_LEFT = 0,
    EYE_RIGHT = 1,
    SCREEN = 2,
    NUM_RENDER_TARGETS = 3
};

static void GenMipMapRGBA( const uint8_t *pSrc, uint8_t *pDst, int nSrcWidth, int nSrcHeight, int *pDstWidthOut, int *pDstHeightOut )
{
    *pDstWidthOut = nSrcWidth / 2;
    if ( *pDstWidthOut <= 0 )
    {
        *pDstWidthOut = 1;
    }
    *pDstHeightOut = nSrcHeight / 2;
    if ( *pDstHeightOut <= 0 )
    {
        *pDstHeightOut = 1;
    }

    for ( int y = 0; y < *pDstHeightOut; y++ )
    {
        for ( int x = 0; x < *pDstWidthOut; x++ )
        {
            int nSrcIndex[4];
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            float a = 0.0f;

            nSrcIndex[0] = ( ( ( y * 2 ) * nSrcWidth ) + ( x * 2 ) ) * 4;
            nSrcIndex[1] = ( ( ( y * 2 ) * nSrcWidth ) + ( x * 2 + 1 ) ) * 4;
            nSrcIndex[2] = ( ( ( ( y * 2 ) + 1 ) * nSrcWidth ) + ( x * 2 ) ) * 4;
            nSrcIndex[3] = ( ( ( ( y * 2 ) + 1 ) * nSrcWidth ) + ( x * 2 + 1 ) ) * 4;

            // Sum all pixels
            for ( int nSample = 0; nSample < 4; nSample++ )
            {
                r += pSrc[ nSrcIndex[ nSample ] ];
                g += pSrc[ nSrcIndex[ nSample ] + 1 ];
                b += pSrc[ nSrcIndex[ nSample ] + 2 ];
                a += pSrc[ nSrcIndex[ nSample ] + 3 ];
            }

            // Average results
            r /= 4.0;
            g /= 4.0;
            b /= 4.0;
            a /= 4.0;

            // Store resulting pixels
            pDst[ ( y * ( *pDstWidthOut ) + x ) * 4 ] = ( uint8_t ) ( r );
            pDst[ ( y * ( *pDstWidthOut ) + x ) * 4 + 1] = ( uint8_t ) ( g );
            pDst[ ( y * ( *pDstWidthOut ) + x ) * 4 + 2] = ( uint8_t ) ( b );
            pDst[ ( y * ( *pDstWidthOut ) + x ) * 4 + 3] = ( uint8_t ) ( a );
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Outputs a set of optional arguments to debugging output, using
//          the printf format setting specified in fmt*.
//-----------------------------------------------------------------------------
static void dprintf( const char *fmt, ... )
{
    va_list args;
    char buffer[ 2048 ];

    va_start( args, fmt );
    vsnprintf( buffer, sizeof( buffer ), fmt, args );
    va_end( args );

    if ( g_bPrintf )
        printf( "%s", buffer );

    OutputDebugStringA( buffer );
}

//-----------------------------------------------------------------------------
// Purpose: Determine the memory type index from the memory requirements
// and type bits
//-----------------------------------------------------------------------------
static bool MemoryTypeFromProperties( const VkPhysicalDeviceMemoryProperties &memoryProperties, uint32_t nMemoryTypeBits, VkMemoryPropertyFlags nMemoryProperties, uint32_t *pTypeIndexOut )
{
    for ( uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++ )
    {
        if ( ( nMemoryTypeBits & 1 ) == 1)
        {
            // Type is available, does it match user properties?
            if ( ( memoryProperties.memoryTypes[i].propertyFlags & nMemoryProperties ) == nMemoryProperties )
            {
                *pTypeIndexOut = i;
                return true;
            }
        }
        nMemoryTypeBits >>= 1;
    }

    // No memory types matched, return failure
    return false;
}


//-----------------------------------------------------------------------------
// Purpose: Helper function to create Vulkan static VB/IBs
//-----------------------------------------------------------------------------
static bool CreateVulkanBuffer( VkDevice pDevice, const VkPhysicalDeviceMemoryProperties &memoryProperties, const void *pBufferData, VkDeviceSize nSize, VkBufferUsageFlags nUsage, VkBuffer *ppBufferOut, VkDeviceMemory *ppDeviceMemoryOut )
{
    // Create the vertex buffer and fill with data
    VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferCreateInfo.size = nSize;
    bufferCreateInfo.usage = nUsage;
    VkResult nResult = vkCreateBuffer( pDevice, &bufferCreateInfo, nullptr, ppBufferOut );
    if ( nResult != VK_SUCCESS )
    {
        dprintf( "%s - vkCreateBuffer failed with error %d\n", __FUNCTION__, nResult );
        return false;
    }

    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements( pDevice, *ppBufferOut, &memoryRequirements );

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    if ( !MemoryTypeFromProperties( memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex ) )
    {
        dprintf( "%s - failed to find matching memoryTypeIndex for buffer\n", __FUNCTION__ );
        return false;
    }
    allocInfo.allocationSize = memoryRequirements.size;

    nResult = vkAllocateMemory( pDevice, &allocInfo, nullptr, ppDeviceMemoryOut );
    if ( nResult != VK_SUCCESS )
    {
        dprintf( "%s - vkCreateBuffer failed with error %d\n", __FUNCTION__, nResult );
        return false;
    }

    nResult = vkBindBufferMemory( pDevice, *ppBufferOut, *ppDeviceMemoryOut, 0 );
    if ( nResult != VK_SUCCESS )
    {
        dprintf( "%s vkBindBufferMemory failed with error %d\n", __FUNCTION__, nResult );
        return false;
    }

    if ( pBufferData != nullptr )
    {
        void *pData;
        nResult = vkMapMemory( pDevice, *ppDeviceMemoryOut, 0, VK_WHOLE_SIZE, 0, &pData );
        if ( nResult != VK_SUCCESS )
        {
            dprintf( "%s - vkMapMemory returned error %d\n", __FUNCTION__, nResult );
            return false;
        }
        memcpy( pData, pBufferData, nSize );
        vkUnmapMemory( pDevice, *ppDeviceMemoryOut );

        VkMappedMemoryRange memoryRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        memoryRange.memory = *ppDeviceMemoryOut;
        memoryRange.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges( pDevice, 1, &memoryRange );

    }
    return true;
}

#endif //FABRIC_VULKAN_UTILITY_H
