// Copyright(c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// VulkanHpp Samples : CopyBlitImage
//                     Draw a cube

#include "../utils/utils.hpp"

#include <thread>
#include <string_view>

static char const * AppName    = "CopyBlitImage";
static char const * EngineName = "Vulkan.hpp";

int main( int argc, char ** argv )
{
  const std::vector<std::string_view> args(argv + 1, argv + argc);
  auto it = std::find(args.begin(), args.end(), std::string_view{"--size"});
  if (it == args.end())
  {
    std::cout << "No --size {width}x{height} command line argument provided" << std::endl;
    return 1;
  }
  std::advance(it, 1);
  int width_input_ = 1280;//720][1280
  int height_input_ = 720;
  sscanf( it->data(), "%dx%d", &width_input_, &height_input_);
  const unsigned width_input = width_input_;
  const unsigned height_input = height_input_;
  const int width_swapchain = 1280;
  const int height_swapchain = 720;

  auto input_data = std::vector<uint8_t>(3*width_input*height_input);
  size_t read_size;
  for (int i = 0; i < 1; i++)
  {
    read_size = fread(input_data.data(), 1, input_data.size(), stdin);
    assert(input_data.size() == read_size);
  }

  std::printf("read\n");

  try
  {
    vk::Instance instance = vk::su::createInstance( AppName, EngineName, {}, vk::su::getInstanceExtensions() );
#if !defined( NDEBUG )
    vk::DebugUtilsMessengerEXT debugUtilsMessenger = instance.createDebugUtilsMessengerEXT( vk::su::makeDebugUtilsMessengerCreateInfoEXT() );
#endif

    vk::PhysicalDevice physicalDevice = instance.enumeratePhysicalDevices().front();

    vk::su::SurfaceData surfaceData( instance, AppName, vk::Extent2D( width_swapchain, height_swapchain ) );

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( surfaceData.surface );
    if ( !( surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst ) )
    {
      std::cout << "Surface cannot be destination of blit - abort \n";
      exit( -1 );
    }

    std::pair<uint32_t, uint32_t> graphicsAndPresentQueueFamilyIndex = vk::su::findGraphicsAndPresentQueueFamilyIndex( physicalDevice, surfaceData.surface );
    vk::Device                    device = vk::su::createDevice( physicalDevice, graphicsAndPresentQueueFamilyIndex.first, vk::su::getDeviceExtensions() );

    vk::CommandPool   commandPool = device.createCommandPool( { {}, graphicsAndPresentQueueFamilyIndex.first } );
    vk::CommandBuffer commandBuffer =
      device.allocateCommandBuffers( vk::CommandBufferAllocateInfo( commandPool, vk::CommandBufferLevel::ePrimary, 1 ) ).front();

    vk::Queue graphicsQueue = device.getQueue( graphicsAndPresentQueueFamilyIndex.first, 0 );
    vk::Queue presentQueue  = device.getQueue( graphicsAndPresentQueueFamilyIndex.second, 0 );

    vk::su::SwapChainData swapChainData( physicalDevice,
                                         device,
                                         surfaceData.surface,
                                         surfaceData.extent,
                                         vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
                                         {},
                                         graphicsAndPresentQueueFamilyIndex.first,
                                         graphicsAndPresentQueueFamilyIndex.second );


    /* VULKAN_KEY_START */
#if 1 // !defined( NDEBUG )
    //auto f = swapChainData.colorFormat;
    //assert(f);
    vk::FormatProperties formatProperties = physicalDevice.getFormatProperties( /*vk::Format::eB8G8R8A8Unorm*/swapChainData.colorFormat );
    //auto b = bool(formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst);
    auto b2 = bool( formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc );
    assert( b2 && "Format cannot be used as transfer source" );
    //assert(b);
#endif

    vk::Semaphore imageAcquiredSemaphore = device.createSemaphore( vk::SemaphoreCreateInfo() );

    // Get the index of the next available swapchain image:
    vk::ResultValue<uint32_t> nextImage = device.acquireNextImageKHR( swapChainData.swapChain, vk::su::FenceTimeout, imageAcquiredSemaphore, nullptr );
    assert( nextImage.result == vk::Result::eSuccess );
    assert( nextImage.value < swapChainData.images.size() );
    uint32_t imageIndex = nextImage.value;

    commandBuffer.begin( vk::CommandBufferBeginInfo() );
    vk::su::setImageLayout(
      commandBuffer, swapChainData.images[imageIndex], swapChainData.colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal );

    // Create an image, map it, and write some values to the image
    vk::ImageCreateInfo imageCreateInfo( vk::ImageCreateFlags(),
                                         vk::ImageType::e2D,
                                         swapChainData.colorFormat,
                                         vk::Extent3D( width_input, height_input, 1 ),
                                         1,
                                         1,
                                         vk::SampleCountFlagBits::e1,
                                         vk::ImageTiling::eLinear,
                                         vk::ImageUsageFlagBits::eTransferSrc );
    vk::Image           blitSourceImage = device.createImage( imageCreateInfo );

    vk::PhysicalDeviceMemoryProperties memoryProperties   = physicalDevice.getMemoryProperties();
    vk::MemoryRequirements             memoryRequirements = device.getImageMemoryRequirements( blitSourceImage );
    uint32_t memoryTypeIndex = vk::su::findMemoryType( memoryProperties, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible );

    vk::DeviceMemory deviceMemory = device.allocateMemory( vk::MemoryAllocateInfo( memoryRequirements.size, memoryTypeIndex ) );
    device.bindImageMemory( blitSourceImage, deviceMemory, 0 );

    vk::su::setImageLayout( commandBuffer, blitSourceImage, swapChainData.colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral );


    vk::ImageCreateInfo imageCreateInfo2( vk::ImageCreateFlags(),
                                         vk::ImageType::e2D,
                                         swapChainData.colorFormat,
                                         vk::Extent3D( width_input, height_input, 1 ),
                                         1,
                                         1,
                                         vk::SampleCountFlagBits::e1,
                                         vk::ImageTiling::eOptimal,
                                         vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst);
    vk::Image           blitSourceImage2 = device.createImage( imageCreateInfo2 );

    vk::PhysicalDeviceMemoryProperties memoryProperties2   = physicalDevice.getMemoryProperties();
    vk::MemoryRequirements             memoryRequirements2 = device.getImageMemoryRequirements( blitSourceImage2 );
    uint32_t memoryTypeIndex2 = vk::su::findMemoryType( memoryProperties2, memoryRequirements2.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible );

    vk::DeviceMemory deviceMemory2 = device.allocateMemory( vk::MemoryAllocateInfo( memoryRequirements2.size, memoryTypeIndex2 ) );
    device.bindImageMemory( blitSourceImage2, deviceMemory2, 0 );

    vk::su::setImageLayout( commandBuffer, blitSourceImage2, swapChainData.colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral );


    commandBuffer.end();

    /* Queue the command buffer for execution */
    vk::Fence              commandFence = device.createFence( {} );
    vk::PipelineStageFlags pipeStageFlags( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    graphicsQueue.submit( vk::SubmitInfo( imageAcquiredSemaphore, pipeStageFlags, commandBuffer ), commandFence );

    /* Make sure command buffer is finished before mapping */
    while ( device.waitForFences( commandFence, true, vk::su::FenceTimeout ) == vk::Result::eTimeout )
      ;

    unsigned char * pImageMemory = static_cast<unsigned char *>( device.mapMemory( deviceMemory, 0, memoryRequirements.size ) );
    auto layout = device.getImageSubresourceLayout( blitSourceImage, vk::ImageSubresource( vk::ImageAspectFlagBits::eColor ) );
    
    if (not (swapChainData.colorFormat == vk::Format::eB8G8R8A8Unorm || swapChainData.colorFormat == vk::Format::eR8G8B8A8Unorm))
    {
      std::cout << "Device has unsupported color format. Only B8G8R8A8Unorm and R8G8B8A8Unorm supported" << std::endl;
      return 1;
    }
    auto color_index0 = vk::Format::eB8G8R8A8Unorm == swapChainData.colorFormat ? 0 : 2;
    auto color_index2 = vk::Format::eB8G8R8A8Unorm == swapChainData.colorFormat ? 2 : 0;
    // Checkerboard of 8x8 pixel squares
    for ( uint32_t row = 0; row < height_input; row++ )
    {
      for ( uint32_t col = 0; col < width_input; col++ )
      {
        //unsigned char rgb = ( ( ( row & 0x4 ) == 0 ) ^ ( ( col & 0x4 ) == 0 ) ) * 255;
        //unsigned char rgb = (((row / 4) % 2) == ((col / 4) % 2 )) * 255;
        auto offset = ((layout.offset  + col)*4 + row * layout.rowPitch);
        auto ImageMemory = pImageMemory + offset;
        auto color = input_data[(row * width_input + col) * 3 + color_index0];
        ImageMemory[0]   = color;
        color = input_data[(row * width_input + col) * 3 + 1];
        ImageMemory[1]   = color;
        color = input_data[(row * width_input + col) * 3 + color_index2];
        ImageMemory[2]   = color;
        ImageMemory[3]   = 255;
        //pImageMemory += 4;
      }
    }

    // Flush the mapped memory and then unmap it. Assume it isn't coherent since we didn't really confirm
    device.flushMappedMemoryRanges( vk::MappedMemoryRange( deviceMemory, 0, memoryRequirements.size ) );
    device.unmapMemory( deviceMemory );

    // reset the command buffer by resetting the complete command pool
    device.resetCommandPool( commandPool );

    commandBuffer.begin( vk::CommandBufferBeginInfo() );

    // Intend to blit from this image, set the layout accordingly
    vk::su::setImageLayout( commandBuffer, blitSourceImage, swapChainData.colorFormat, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal );
    vk::su::setImageLayout( commandBuffer, blitSourceImage2, swapChainData.colorFormat, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferDstOptimal );

    vk::ImageSubresourceLayers imageSubresourceLayers( vk::ImageAspectFlagBits::eColor, 0, 0, 1 );
    vk::ImageCopy imageCopy( imageSubresourceLayers, vk::Offset3D(), imageSubresourceLayers, vk::Offset3D( 0, 0, 0 ), vk::Extent3D( width_input, height_input, 1 ) );
    commandBuffer.copyImage( blitSourceImage, vk::ImageLayout::eTransferSrcOptimal, blitSourceImage2, vk::ImageLayout::eTransferDstOptimal, imageCopy );


    vk::Image blitDestinationImage = swapChainData.images[imageIndex];

    // Do a 32x32 blit to all of the dst image - should get big squares
    vk::ImageBlit              imageBlit( imageSubresourceLayers,
                                          { { vk::Offset3D( 0, 0, 0 ), vk::Offset3D( width_input, height_input, 1 ) } },
                             imageSubresourceLayers,
                                          { { vk::Offset3D( 0, 0, 0 ), vk::Offset3D( surfaceData.extent.width, surfaceData.extent.height, 1 ) } } );
    
    vk::su::setImageLayout( commandBuffer, blitSourceImage2, swapChainData.colorFormat, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal );
    commandBuffer.blitImage(
      blitSourceImage2, vk::ImageLayout::eTransferSrcOptimal, blitDestinationImage, vk::ImageLayout::eTransferDstOptimal, imageBlit, vk::Filter::eLinear );

    // Use a barrier to make sure the blit is finished before the copy starts
    // Note: for a layout of vk::ImageLayout::eTransferDstOptimal, the access mask is supposed to be vk::AccessFlagBits::eTransferWrite
    vk::ImageMemoryBarrier memoryBarrier( vk::AccessFlagBits::eTransferWrite,
                                          vk::AccessFlagBits::eTransferWrite,
                                          vk::ImageLayout::eTransferDstOptimal,
                                          vk::ImageLayout::eTransferDstOptimal,
                                          VK_QUEUE_FAMILY_IGNORED,
                                          VK_QUEUE_FAMILY_IGNORED,
                                          blitDestinationImage,
                                          vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 ) );
    commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), nullptr, nullptr, memoryBarrier );

    // Do a image copy to part of the dst image - checks should stay small
    //vk::ImageCopy imageCopy2( imageSubresourceLayers, vk::Offset3D(), imageSubresourceLayers, vk::Offset3D( 256, 256, 0 ), vk::Extent3D( width_input, height_input, 1 ) );
    //commandBuffer.copyImage( blitSourceImage, vk::ImageLayout::eTransferSrcOptimal, blitDestinationImage, vk::ImageLayout::eTransferDstOptimal, imageCopy2 );

    // Note: for a layout of vk::ImageLayout::ePresentSrcKHR, the access mask is supposed to be empty
    vk::ImageMemoryBarrier prePresentBarrier( vk::AccessFlagBits::eTransferWrite,
                                              {},
                                              vk::ImageLayout::eTransferDstOptimal,
                                              vk::ImageLayout::ePresentSrcKHR,
                                              VK_QUEUE_FAMILY_IGNORED,
                                              VK_QUEUE_FAMILY_IGNORED,
                                              swapChainData.images[imageIndex],
                                              vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 ) );
    commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlags(), nullptr, nullptr, prePresentBarrier );
    commandBuffer.end();

    vk::Fence drawFence = device.createFence( {} );
    graphicsQueue.submit( vk::SubmitInfo( {}, {}, commandBuffer ), drawFence );
    graphicsQueue.waitIdle();

    /* Make sure command buffer is finished before presenting */
    while ( device.waitForFences( drawFence, true, vk::su::FenceTimeout ) == vk::Result::eTimeout )
      ;

    /* Now present the image in the window */
    vk::Result result = presentQueue.presentKHR( vk::PresentInfoKHR( {}, swapChainData.swapChain, imageIndex, {} ) );
    switch ( result )
    {
      case vk::Result::eSuccess: break;
      case vk::Result::eSuboptimalKHR: std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n"; break;
      default: assert( false );  // an unexpected result is returned !
    }
    std::this_thread::sleep_for( std::chrono::milliseconds( 10000 ) );

    /* VULKAN_KEY_END */

    device.destroyFence( drawFence );
    device.destroyFence( commandFence );
    device.destroyImage( blitSourceImage );  // destroy the image before the bound device memory to prevent some validation layer warning
    device.freeMemory( deviceMemory );
    device.destroySemaphore( imageAcquiredSemaphore );
    swapChainData.clear( device );
    device.freeCommandBuffers( commandPool, commandBuffer );
    device.destroyCommandPool( commandPool );
    device.destroy();
    instance.destroySurfaceKHR( surfaceData.surface );
#if !defined( NDEBUG )
    instance.destroyDebugUtilsMessengerEXT( debugUtilsMessenger );
#endif
    instance.destroy();
  }
  catch ( vk::SystemError & err )
  {
    std::cout << "vk::SystemError: " << err.what() << std::endl;
    exit( -1 );
  }
  catch ( std::exception & err )
  {
    std::cout << "std::exception: " << err.what() << std::endl;
    exit( -1 );
  }
  catch ( ... )
  {
    std::cout << "unknown error\n";
    exit( -1 );
  }
  return 0;
}
