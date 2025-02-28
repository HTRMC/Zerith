// DebugRenderer.cpp
#include "DebugRenderer.hpp"

#include <array>
#include <stdexcept>

#include "ShaderManager.hpp"

DebugRenderer::DebugRenderer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkRenderPass renderPass,
        uint32_t windowWidth,
        uint32_t windowHeight
        )
        : device(device),
          physicalDevice(physicalDevice),
          renderPass(renderPass),
          width(windowWidth),
          height(windowHeight),
          needsUpdate(false),
          descriptorSetLayout(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE) {
    createVertexBuffer();
    createPipeline();
}

DebugRenderer::~DebugRenderer() {
    if (device) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }

        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
    }
}

void DebugRenderer::drawLine(const glm::vec3 &start, const glm::vec3 &end,
                             const glm::vec4 &color, float duration) {
    lines.push_back({start, end, color, duration, 0.0f});
    needsUpdate = true;
}

void DebugRenderer::drawBox(const AABB &box, const glm::vec4 &color, float duration) {
    if (boxes.size() >= MAX_BOXES) {
        return; // Skip if we've reached the maximum
    }
    boxes.push_back({box, color, duration, 0.0f});
    needsUpdate = true;
}

void DebugRenderer::update(float deltaTime) {
    // Update timers and remove expired debug elements
    auto removeExpired = [deltaTime](auto &container) {
        container.erase(
                std::remove_if(container.begin(), container.end(),
                               [deltaTime](auto &element) {
                                   if (element.duration <= 0.0f) return false;
                                   element.startTime += deltaTime;
                                   return element.startTime >= element.duration;
                               }
                ),
                container.end()
        );
    };

    removeExpired(lines);
    removeExpired(boxes);

    if (needsUpdate) {
        updateBuffers();
        needsUpdate = false;
    }
}

void DebugRenderer::updateBuffers() {
    vertices.clear();

    // Convert lines to vertices
    for (const auto &line: lines) {
        vertices.push_back({line.start, line.color});
        vertices.push_back({line.end, line.color});
    }

    // Convert boxes to vertices
    for (const auto &box: boxes) {
        auto boxVertices = generateBoxVertices(box.aabb, box.color);
        vertices.insert(vertices.end(), boxVertices.begin(), boxVertices.end());
    }

    // Update vertex buffer
    if (!vertices.empty()) {
        void *data;
        vkMapMemory(device, vertexBufferMemory, 0, vertices.size() * sizeof(DebugVertex), 0, &data);
        memcpy(data, vertices.data(), vertices.size() * sizeof(DebugVertex));
        vkUnmapMemory(device, vertexBufferMemory);
    }
}

std::vector<DebugVertex> DebugRenderer::generateBoxVertices(const AABB &box,
                                                            const glm::vec4 &color) {
    std::vector<DebugVertex> boxVertices;
    boxVertices.reserve(24); // 12 lines * 2 vertices per line

    // Define the 8 corners of the box
    std::vector<glm::vec3> corners = {
            {box.min.x, box.min.y, box.min.z},
            {box.max.x, box.min.y, box.min.z},
            {box.min.x, box.max.y, box.min.z},
            {box.max.x, box.max.y, box.min.z},
            {box.min.x, box.min.y, box.max.z},
            {box.max.x, box.min.y, box.max.z},
            {box.min.x, box.max.y, box.max.z},
            {box.max.x, box.max.y, box.max.z}
    };

    // Define the 12 lines that make up the box
    const std::vector<std::pair<int, int>> lines = {
            {0, 1},
            {1, 3},
            {3, 2},
            {2, 0}, // Bottom face
            {4, 5},
            {5, 7},
            {7, 6},
            {6, 4}, // Top face
            {0, 4},
            {1, 5},
            {2, 6},
            {3, 7}  // Vertical edges
    };

    // Generate vertices for each line
    for (const auto &[start, end]: lines) {
        boxVertices.push_back({corners[start], color});
        boxVertices.push_back({corners[end], color});
    }

    return boxVertices;
}

void DebugRenderer::createVertexBuffer() {
    const size_t MAX_VERTICES = MAX_BOXES * 24;
    VkDeviceSize bufferSize = sizeof(DebugVertex) * MAX_VERTICES;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create debug vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    // Find suitable memory type
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t memoryTypeIndex = uint32_t(-1);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags &
             (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == uint32_t(-1)) {
        throw std::runtime_error("failed to find suitable memory type for debug vertex buffer!");
    }

    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate debug vertex buffer memory!");
    }

    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
}

void DebugRenderer::createPipeline() {
    // Vertex input state
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(DebugVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(DebugVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(DebugVertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor states
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Dynamic states
    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth and stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Push constant for view-projection matrix
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4);

    // Create pipeline layout FIRST
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create debug pipeline layout!");
    }

    // THEN get shaders and create pipeline
    auto [vertShaderModule, fragShaderModule] = ShaderManager::getInstance().getShaderPair(
            "debug.vert.spv", "debug.frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create debug graphics pipeline!");
    }
}

void DebugRenderer::render(VkCommandBuffer commandBuffer, const glm::mat4 &viewProj) {
    if (vertices.empty()) return;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Set viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Set scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width, height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // Push constants for view-projection matrix
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(glm::mat4), &viewProj);

    // Draw all debug primitives
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
}