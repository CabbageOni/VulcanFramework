#pragma once

#include <vector>

#include "vkbase.h"

class VKTextureFinal : public VKBase
{
private:
  struct VertexData
  {
    float x, y, z, w;
    float u, v;
  };
  constexpr static VertexData m_vertex_data[] = {
    {
      -0.7f, -0.7f, 0.0f, 1.0f,
      0.0f, 0.0f
    },
    {
      -0.7f, 0.7f, 0.0f, 1.0f,
      0.0f, 1.0f
    },
    {
      0.7f, -0.7f, 0.0f, 1.0f,
      1.0f, 0.0f
    },
    {
      0.7f, -0.7f, 0.0f, 1.0f,
      1.0f, 0.0f
    },
    {
      -0.7f, 0.7f, 0.0f, 1.0f,
      0.0f, 1.0f
    },
    {
      0.7f, 0.7f, 0.0f, 1.0f,
      1.0f, 1.0f
    }
  };

  struct BufferInfo
  {
    VkDeviceMemory memory;
    uint32_t       count;
    uint64_t       size;
  };
  VkBuffer   m_vertex_buffer;
  BufferInfo m_vertex_buffer_info;
  VkBuffer   m_staging_buffer;
  BufferInfo m_staging_buffer_info;

  constexpr static size_t m_virtual_frames_count = 3;
  struct VirtualFrame
  {
    VkSemaphore     image_available_semaphore;
    VkSemaphore     finished_rendering_semaphore;
    VkFence         fence;
    VkCommandBuffer command_buffer;
    VkFramebuffer   frame_buffer;
  } m_virtual_frames[m_virtual_frames_count];
  size_t m_current_virtual_frame_index = 0;

  VkImage        m_texture;
  VkDeviceMemory m_texture_memory;
  VkImageView    m_texture_view;
  VkSampler      m_texture_sampler;

  VkDescriptorSet       m_descriptor_set;
  VkDescriptorSetLayout m_descriptor_set_layout;
  VkDescriptorPool      m_descriptor_pool;

  uint32_t      m_graphics_queue_family_index;
  uint32_t      m_present_queue_family_index;
  VkQueue       m_graphics_queue;
  VkQueue       m_present_queue;
  VkCommandPool m_graphics_command_pool;

  struct SwapChainInfo
  {
    VkFormat                 format;
    VkExtent2D               extent;
    std::vector<VkImage>     images;
    std::vector<VkImageView> image_views;
  } m_swap_chain_info;
  VkSwapchainKHR   m_swap_chain;
  VkRenderPass     m_render_pass;
  VkPipeline       m_pipeline;
  VkPipelineLayout m_pipeline_layout;

  virtual bool Initialize() override;
  virtual bool CreateDevice() override;
  bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device,
    uint32_t& selected_graphics_queue_family_index,
    uint32_t &selected_present_queue_family_index);
  bool GetDeviceQueue();
  bool CreateVertexBuffer();
  bool CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memoryProperty, VkBuffer& buffer, BufferInfo& buffer_info);
  bool AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlagBits property, VkDeviceMemory* memory);
  bool CreateStagingBuffer();
  bool CreateCommandBuffers();
  bool CreateCommandPool(uint32_t queue_family_index, VkCommandPool* pool);
  bool AllocateCommandBuffers(VkCommandPool pool, uint32_t count, VkCommandBuffer* command_buffers);
  bool CreateSemaphores();
  bool CreateFences();
  bool CreateTexture();
  bool CreateImage(uint32_t width, uint32_t height, VkImage* image);
  bool AllocateImageMemory(VkImage image, VkMemoryPropertyFlagBits property, VkDeviceMemory* memory);
  bool CreateImageView(VkImage image, VkImageView* image_view);
  bool CreateSampler(VkSampler* sampler);
  bool CopyTextureData(char* texture_data, uint32_t data_size, uint32_t width, uint32_t height);
  bool CreateDescriptorSetLayout();
  bool CreateDescriptorPool();
  bool AllocateDescriptorSet();
  bool UpdateDescriptorSet();
  bool CopyVertexData();
  bool OnWindowSizeChanged();
  void Clear();
  bool CreateSwapChain();
  uint32_t GetSwapChainNumImages(VkSurfaceCapabilitiesKHR &surface_capabilities);
  VkSurfaceFormatKHR GetSwapChainFormat(std::vector<VkSurfaceFormatKHR> &surface_formats);
  VkExtent2D GetSwapChainExtent(VkSurfaceCapabilitiesKHR &surface_capabilities);
  VkImageUsageFlags GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR& surface_capabilities);
  VkSurfaceTransformFlagBitsKHR GetSwapChainTransform(VkSurfaceCapabilitiesKHR& surface_capabilities);
  VkPresentModeKHR GetSwapChainPresentMode(std::vector<VkPresentModeKHR>& present_modes);
  bool CreateSwapChainImageViews();
  bool CreateRenderPass();
  bool CreatePipeline();
  VkShaderModule CreateShaderModule(const char* shader_code);
  bool CreatePipelineLayout();
  virtual bool Update() override;
  bool PrepareFrame(VkCommandBuffer command_buffer, const size_t& image_index, VkFramebuffer& frame_buffer);
  bool CreateFrameBuffer(VkFramebuffer& frame_buffer, const VkImageView image_view);
  virtual void Terminate() override;

public:
  friend bool Initialize();
  friend bool Update();
  friend void Terminate();
};