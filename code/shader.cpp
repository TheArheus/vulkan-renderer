#include <spirv-headers/spirv.h>

enum resource_type {ResourceType_Unknown, ResourceType_Variable};

struct resource
{
    uint32_t Binding;
    uint32_t Set;
    uint32_t Type;
    uint32_t Class;

    resource_type Kind = ResourceType_Unknown;
};

static VkShaderStageFlagBits
GetExecutionModel(SpvExecutionModel ExecutionModel)
{
    switch(ExecutionModel)
    {
        case SpvExecutionModelVertex:
        {
            return VK_SHADER_STAGE_VERTEX_BIT;
        } break;
        case SpvExecutionModelFragment:
        {
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        } break;
        case SpvExecutionModelTaskNV:
        {
            return VK_SHADER_STAGE_TASK_BIT_NV;
        } break;
        case SpvExecutionModelMeshNV:
        {
            return VK_SHADER_STAGE_MESH_BIT_NV;
        } break;
        default: 
        {
            assert("Unsupported Execution Model");
            return (VkShaderStageFlagBits)0;
        } break;
    }
}

static void
ParseShader(shader& Shader, const uint32_t* Code, uint32_t CodeSize)
{
    assert(Code[0] == SpvMagicNumber);
    uint32_t IdBound = Code[3];
    const uint32_t* Insn = Code + 5;

    std::vector<resource> Resources(IdBound);

    while(Insn != Code + CodeSize)
    {
        uint16_t OpCode = (uint16_t)(Insn[0]);
        uint16_t WordCount = (uint16_t)(Insn[0] >> 16);

        switch(OpCode)
        {
            case SpvOpEntryPoint:
            {
                assert(WordCount >= 2);
                Shader.Stage = GetExecutionModel(SpvExecutionModel(Insn[1]));
            } break;

            case SpvOpDecorate:
            {
                assert(WordCount >= 3);
                uint32_t ID = Insn[1];

                assert(ID < IdBound);

                switch(Insn[2])
                {
                    case SpvDecorationDescriptorSet:
                    {
                        assert(WordCount == 4);
                        Resources[ID].Set = Insn[3];
                    } break;
                    case SpvDecorationBinding:
                    {
                        assert(WordCount == 4);
                        Resources[ID].Binding = Insn[3];
                    } break;
                }
            } break;

            case SpvOpVariable:
            {
                assert(WordCount >= 4);

                uint32_t ID = Insn[2];

                assert(ID < IdBound);
                assert(Resources[ID].Kind == ResourceType_Unknown);

                Resources[ID].Kind = ResourceType_Variable;
                Resources[ID].Type = Insn[1];
                Resources[ID].Class = Insn[3];
            } break;
        }

        assert(Insn + WordCount <= Code + CodeSize);
        if(Insn + WordCount >= Code + CodeSize) break;
        Insn += WordCount;
    }

    for(auto& Id : Resources)
    {
        if(Id.Kind == ResourceType_Variable && Id.Class == SpvStorageClassUniform)
        {
            assert(Id.Set == 0);
            assert(Id.Binding < 32);
            assert((Shader.StorageBufferMask & (1 << Id.Binding)) == 0);

            Shader.StorageBufferMask |= 1 << Id.Binding;
        }
    }
}

bool
LoadShader(shader& Shader, VkDevice Device, const char* Path)
{
    FILE* File = fopen(Path, "rb");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        long Length = ftell(File);
        assert(Length >= 0);
        fseek(File, 0, SEEK_SET);

        char* Buffer = (char*)malloc(Length);
        assert(Buffer);

        fread(Buffer, 1, Length, File);
        fclose(File);

        VkShaderModuleCreateInfo ShaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ShaderModuleCreateInfo.codeSize = Length;
        ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(Buffer);

        VkShaderModule ShaderModule = 0;
        VK_CHECK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, 0, &ShaderModule));

        assert(Length % 4 == 0);
        ParseShader(Shader, reinterpret_cast<const uint32_t*>(Buffer), Length / 4);

        free(Buffer);

        Shader.Module = ShaderModule;

        return true;
    }
    return false;
}

VkDescriptorSetLayout CreateSetLayout(VkDevice Device, const shader& MeshVert, const shader& MeshFrag)
{
    std::vector<VkDescriptorSetLayoutBinding> BindingsSet;

    uint32_t StorageBufferMask = MeshVert.StorageBufferMask | MeshFrag.StorageBufferMask;
    for(int i = 0; i < 32; ++i)
    {
        if(StorageBufferMask & (1 << i))
        {
            VkDescriptorSetLayoutBinding Binding = {};

            Binding.binding = i;
            Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            Binding.descriptorCount = 1;
            Binding.stageFlags = 0;

            if(MeshVert.StorageBufferMask & (1 << i)) Binding.stageFlags |= MeshVert.Stage;
            if(MeshFrag.StorageBufferMask & (1 << i)) Binding.stageFlags |= MeshFrag.Stage;

            BindingsSet.push_back(Binding);
        }
    }

    VkDescriptorSetLayoutCreateInfo SetCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    SetCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    SetCreateInfo.bindingCount = (uint32_t)BindingsSet.size();
    SetCreateInfo.pBindings = BindingsSet.data();

    VkDescriptorSetLayout SetLayout = 0;
    VK_CHECK(vkCreateDescriptorSetLayout(Device, &SetCreateInfo, 0, &SetLayout));

    return SetLayout;
}

VkPipelineLayout CreatePipelineLayout(VkDevice Device, const shader& MeshVert, const shader& MeshFrag)
{
    VkDescriptorSetLayout SetLayout = CreateSetLayout(Device, MeshVert, MeshFrag);

    VkPipelineLayoutCreateInfo PipelineCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    PipelineCreateInfo.setLayoutCount = 1;
    PipelineCreateInfo.pSetLayouts = &SetLayout;

    VkPipelineLayout PipelineLayout = 0;
    VK_CHECK(vkCreatePipelineLayout(Device, &PipelineCreateInfo, 0, &PipelineLayout));

    vkDestroyDescriptorSetLayout(Device, SetLayout, 0);

    return PipelineLayout;
}

VkDescriptorUpdateTemplate 
CreateUpdateTemplate(VkDevice Device, VkPipelineBindPoint BindPoint, VkPipelineLayout PipelineLayout, const shader& MeshVert, const shader& MeshFrag)
{
    std::vector<VkDescriptorUpdateTemplateEntry> Entries;
    uint32_t StorageBufferMask = MeshVert.StorageBufferMask | MeshFrag.StorageBufferMask;

    for(int i = 0; i < 32; ++i)
    {
        if(StorageBufferMask & (1 << i))
        {
            VkDescriptorUpdateTemplateEntry Entry = {};

            Entry.dstBinding = i;
            Entry.dstArrayElement = 0;
            Entry.descriptorCount = 1;
            Entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            Entry.offset = sizeof(descriptor_info)*i;
            Entry.stride = sizeof(descriptor_info);

            Entries.push_back(Entry);
        }
    }

    VkDescriptorUpdateTemplateCreateInfo CreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO};

    CreateInfo.descriptorUpdateEntryCount = (uint32_t)Entries.size();
    CreateInfo.pDescriptorUpdateEntries = Entries.data();

    CreateInfo.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR;
    CreateInfo.pipelineBindPoint = BindPoint;
    CreateInfo.pipelineLayout = PipelineLayout;

    VkDescriptorUpdateTemplate UpdateTemplate = 0;
    VK_CHECK(vkCreateDescriptorUpdateTemplate(Device, &CreateInfo, 0, &UpdateTemplate));

    return UpdateTemplate;
}

VkPipeline
CreateGraphicsPipeline(VkDevice Device, VkRenderPass RenderPass, VkPipelineCache PipelineCache, const shader& MeshVert, const shader& MeshFrag, VkPipelineLayout PipelineLayout)
{
    assert(MeshVert.Stage == VK_SHADER_STAGE_VERTEX_BIT || MeshVert.Stage == VK_SHADER_STAGE_MESH_BIT_NV);
    assert(MeshFrag.Stage == VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineShaderStageCreateInfo Stages[2] = {};
    Stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    Stages[0].stage  = MeshVert.Stage;
    Stages[0].module = MeshVert.Module;
    Stages[0].pName  = "main";

    Stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    Stages[1].stage  = MeshFrag.Stage;
    Stages[1].module = MeshFrag.Module;
    Stages[1].pName  = "main";

    VkPipelineVertexInputStateCreateInfo VertexInput = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    VkPipelineInputAssemblyStateCreateInfo InputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo ViewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    ViewportState.viewportCount = 1;
    ViewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo RasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    RasterizationState.lineWidth = 1.0f;
    RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

    VkPipelineMultisampleStateCreateInfo MultisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo DepthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

    VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
    ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo ColorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    ColorBlendState.pAttachments = &ColorBlendAttachmentState;
    ColorBlendState.attachmentCount = 1;

    VkDynamicState DynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo DynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    DynamicState.dynamicStateCount = ArraySize(DynamicStates);
    DynamicState.pDynamicStates = DynamicStates;

    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    PipelineCreateInfo.stageCount          = ArraySize(Stages);
    PipelineCreateInfo.pStages             = Stages;
    PipelineCreateInfo.pVertexInputState   = &VertexInput;
    PipelineCreateInfo.pInputAssemblyState = &InputAssembly;
    PipelineCreateInfo.pViewportState      = &ViewportState;
    PipelineCreateInfo.pRasterizationState = &RasterizationState;
    PipelineCreateInfo.pMultisampleState   = &MultisampleState;
    PipelineCreateInfo.pDepthStencilState  = &DepthStencilState;
    PipelineCreateInfo.pColorBlendState    = &ColorBlendState;
    PipelineCreateInfo.pDynamicState       = &DynamicState;
    PipelineCreateInfo.layout              = PipelineLayout;
    PipelineCreateInfo.renderPass          = RenderPass;

    VkPipeline Pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(Device, PipelineCache, 1, &PipelineCreateInfo, 0, &Pipeline));

    return Pipeline;
}
