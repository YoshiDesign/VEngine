#include "aveng_descriptors.h"

#include <iostream>
#include <cassert>
#include <stdexcept>

namespace aveng {

    // *************** Descriptor Set Layout Builder *********************

    AvengDescriptorSetLayout::Builder::Builder(EngineDevice& device) : engineDevice{ device } {}

    /*
     * Add an individual descriptor definition to 
     * be added to a descriptor set for the layout being constructed
     */
    AvengDescriptorSetLayout::Builder& 
    AvengDescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count) 
    {
     
        std::cout << "Adding Binding (1 & 6): " << descriptorType << " at binding: " << binding << std::endl;
        assert(bindings.count(binding) == 0 && "Binding already in use");

        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;                // Binding location, 0, 1, 2, etc
        layoutBinding.descriptorType = descriptorType;  // Ex. VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER(_DYNAMIC) or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        layoutBinding.descriptorCount = count;          // Number of descriptors this layout will use
        layoutBinding.stageFlags = stageFlags;          // Default: 1 (VK_SHADER_STAGE_VERTEX_BIT) A VkShaderStageFlagBits determining which pipeline shader stages can access this layout binding. 

        layout_bindings[binding] = layoutBinding;              // Add the binding to the map

        return *this;
    }

    /*
    * Create a unique pointer to a Descriptor Set Layout using the
    */
    std::unique_ptr<AvengDescriptorSetLayout> AvengDescriptorSetLayout::Builder::build() const 
    {
        // Descriptor Set Layout Builder initializes its parent class
        return std::make_unique<AvengDescriptorSetLayout>(engineDevice, layout_bindings);
    }

    // *************** Descriptor Set Layout *********************
    AvengDescriptorSetLayout::AvengDescriptorSetLayout(EngineDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> layout_bindings)
        : engineDevice{ device }, layout_bindings{ layout_bindings } // Note that this delivers layout bindings from the Builder to the AvengDescriptorSetLayout
    {

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : layout_bindings) 
        {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(engineDevice.device(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

    }

    AvengDescriptorSetLayout::~AvengDescriptorSetLayout() 
    {
        vkDestroyDescriptorSetLayout(engineDevice.device(), descriptorSetLayout, nullptr);
    }

    // *************** Descriptor Pool Builder *********************
    AvengDescriptorPool::Builder::Builder(EngineDevice& device) : engineDevice{ device } {}

    AvengDescriptorPool::Builder& AvengDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count) 
    {
        /*
        * Reference
        * typedef struct VkDescriptorPoolSize {
            VkDescriptorType    type;                   // This matches the type of the descriptors it is meant to allocate
            uint32_t            descriptorCount;        // The number of descriptors of that type to allocate
}           VkDescriptorPoolSize;
        */
        poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    AvengDescriptorPool::Builder& AvengDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) 
    {
        // See: VkDescriptorPoolCreateFlagBits - This is where we can specify things like updating after binding
        // Pro Tip: Descriptor pools don't guarantee thread safe
        poolFlags = flags;
        return *this;
    }

    AvengDescriptorPool::Builder& AvengDescriptorPool::Builder::setMaxSets(uint32_t count) 
    {
        // Sync'd to SwapChain::MAX_FRAMES_IN_FLIGHT
        maxSets = count;
        return *this;
    }

    /*
    * Create a unique pointer to a Descriptor Pool using the data gathered throughout binding of descriptors
    */
    std::unique_ptr<AvengDescriptorPool> AvengDescriptorPool::Builder::build() const 
    {
        return std::make_unique<AvengDescriptorPool>(engineDevice, maxSets, poolFlags, poolSizes);
    }

    // *************** Descriptor Pool *********************

    AvengDescriptorPool::AvengDescriptorPool(
        EngineDevice& device,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes)
        : engineDevice{ device } 
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(engineDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) 
            != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    AvengDescriptorPool::~AvengDescriptorPool() 
    {
        vkDestroyDescriptorPool(engineDevice.device(), descriptorPool, nullptr);
    }

    bool AvengDescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const 
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up. But this is beyond our current scope
        if (vkAllocateDescriptorSets(engineDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
            return false;
        }

        return true;
    }

    void AvengDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const 
    {
        vkFreeDescriptorSets(
            engineDevice.device(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
    }

    void AvengDescriptorPool::resetPool() 
    {
        vkResetDescriptorPool(engineDevice.device(), descriptorPool, 0);
    }

    // *************** Descriptor Writer *********************

    AvengDescriptorSetWriter::AvengDescriptorSetWriter(AvengDescriptorSetLayout& setLayout, AvengDescriptorPool& pool)
        : setLayout{ setLayout }, pool{ pool } {}

    AvengDescriptorSetWriter& AvengDescriptorSetWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) 
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.layout_bindings[binding];
        std::cout << "Binding Descriptor type for Buffer: " << bindingDescription.descriptorType << std::endl;

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;       // The type of this structure.
        write.descriptorType = bindingDescription.descriptorType;   // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_SAMPLER etc...
        write.dstBinding = binding;                                 // Binding index within this descriptor set
        write.pBufferInfo = bufferInfo;                             // A pointer to an array of VkDescriptorBufferInfo structures or is ignored
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    AvengDescriptorSetWriter& AvengDescriptorSetWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, int nImages) 
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.layout_bindings[binding];

        std::cout << "Binding Descriptor type for Image: " << bindingDescription.descriptorType << std::endl;

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;       // The type of this structure.
        write.descriptorType = bindingDescription.descriptorType;   // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_SAMPLER etc...
        write.dstBinding = binding;                                 // Binding index within this descriptor set
        write.pImageInfo = imageInfo;                               // A pointer to an array of VkDescriptorImageInfo structures or is ignored
        //write.descriptorCount = nImages;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    bool AvengDescriptorSetWriter::build(VkDescriptorSet& set) 
    {
        bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
        if (!success) 
        {
            return false;
        }
        overwrite(set);
        return true;
    }

    void AvengDescriptorSetWriter::overwrite(VkDescriptorSet& set) 
    {
        std::cout << "Updating final descriptor sets!" << std::endl;
        for (auto& write : writes) 
        {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(pool.engineDevice.device(), writes.size(), writes.data(), 0, nullptr);
    }

}  // namespace aveng