#if !defined(SHADER_H_)
#define SHADER_H_

struct shader
{
    VkShaderModule Module;
    VkShaderStageFlagBits Stage;

    uint32_t StorageBufferMask;
};

bool LoadShader(const shader& Shader, VkDevice Device, const char* Path);

VkPipelineLayout CreatePipelineLayout(VkDevice Device, const shader& VertexBuffer, const shader& FragmentShader);
VkDescriptorUpdateTemplate CreateUpdateTemplate(VkDevice Device, VkPipelineBindPoint BindPoint, VkPipelineLayout PipelineLayout, const shader& VertexBuffer, const shader& FragmentShader);
VkPipeline CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineCache PipelineCache, const shader& MeshVert, const shader& MeshFrag, VkPipelineLayout PipelineLayout);

struct descriptor_info
{
    union
    {
        VkDescriptorImageInfo Image;
        VkDescriptorBufferInfo Buffer;
    };

    descriptor_info()
    {
    }

    descriptor_info(VkSampler Sampler_, VkImageView ImageView_, VkImageLayout ImageLayout_)
    {
        Image.sampler = Sampler_;
        Image.imageView = ImageView_;
        Image.imageLayout = ImageLayout_;
    }

    descriptor_info(VkBuffer Buffer_, VkDeviceSize Offset_, VkDeviceSize Size_)
    {
        Buffer.buffer = Buffer_;
        Buffer.offset = Offset_;
        Buffer.range = Size_;
    }

    descriptor_info(VkBuffer Buffer_)
    {
        Buffer.buffer = Buffer_;
        Buffer.offset = 0;
        Buffer.range = VK_WHOLE_SIZE;
    }
};

#endif
