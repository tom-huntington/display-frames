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
// VulkanHpp Samples : PhysicalDeviceMemoryProperties
//                     Get memory properties per physical device.

#include "../utils/utils.hpp"
#include "vulkan/vulkan.hpp"

#include <sstream>
#include <vector>

static char const * AppName    = "PhysicalDeviceMemoryProperties";
static char const * EngineName = "Vulkan.hpp";

std::string formatSize( vk::DeviceSize size )
{
  std::ostringstream oss;
  if ( size < 1024 )
  {
    oss << size << " B";
  }
  else if ( size < 1024 * 1024 )
  {
    oss << size / 1024.f << " KB";
  }
  else if ( size < 1024 * 1024 * 1024 )
  {
    oss << size / ( 1024.0f * 1024.0f ) << " MB";
  }
  else
  {
    oss << size / ( 1024.0f * 1024.0f * 1024.0f ) << " GB";
  }
  return oss.str();
}

int main( int /*argc*/, char ** /*argv*/ )
{
  try
  {
    std::unique_ptr<vk::ContextHandle<>> contextHandle = std::make_unique<vk::ContextHandle<>>();
    std::unique_ptr<vk::InstanceHandle<>> instanceHandle =
      vk::su::makeUniqueInstanceHandle( *contextHandle, AppName, EngineName, {}, {}, VK_API_VERSION_1_1 );
#if !defined( NDEBUG )
    std::unique_ptr<vk::DebugUtilsMessengerEXTHandle<>> debugUtilsMessenger =
      vk::su::makeUniqueDebugUtilsMessengerEXTHandle( *instanceHandle );
#endif

    // enumerate the physicalDevices
    std::unique_ptr<vk::PhysicalDeviceHandles<>> physicalDeviceHandles =
      std::make_unique<vk::PhysicalDeviceHandles<>>( *instanceHandle );

    /* VULKAN_KEY_START */

    for ( size_t i = 0; i < physicalDeviceHandles->size(); i++ )
    {
      // some properties are only valid, if a corresponding extension is available!
      std::vector<vk::ExtensionProperties> extensionProperties =
        (*physicalDeviceHandles)[i].enumerateDeviceExtensionProperties();
      bool containsMemoryBudget = vk::su::contains( extensionProperties, "VK_EXT_memory_budget" );

      std::cout << "PhysicalDevice " << i << " :\n";
      auto memoryProperties2 =
        (*physicalDeviceHandles)[i]
          .getMemoryProperties2<vk::PhysicalDeviceMemoryProperties2, vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();
      vk::PhysicalDeviceMemoryProperties const & memoryProperties =
        memoryProperties2.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties;
      vk::PhysicalDeviceMemoryBudgetPropertiesEXT const & memoryBudgetProperties =
        memoryProperties2.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();
      std::cout << "memoryHeapCount: " << memoryProperties.memoryHeapCount << "\n";
      for ( uint32_t j = 0; j < memoryProperties.memoryHeapCount; j++ )
      {
        std::cout << "  " << j << ": size = " << formatSize( memoryProperties.memoryHeaps[j].size )
                  << ", flags = " << vk::to_string( memoryProperties.memoryHeaps[j].flags ) << "\n";
        if ( containsMemoryBudget )
        {
          std::cout << "     heapBudget = " << formatSize( memoryBudgetProperties.heapBudget[j] )
                    << ", heapUsage = " << formatSize( memoryBudgetProperties.heapUsage[j] ) << "\n";
        }
      }
      std::cout << "memoryTypeCount: " << memoryProperties.memoryTypeCount << "\n";
      for ( uint32_t j = 0; j < memoryProperties.memoryTypeCount; j++ )
      {
        std::cout << "  " << j << ": heapIndex = " << memoryProperties.memoryTypes[j].heapIndex
                  << ", flags = " << vk::to_string( memoryProperties.memoryTypes[j].propertyFlags ) << "\n";
      }
    }

    /* VULKAN_KEY_END */
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