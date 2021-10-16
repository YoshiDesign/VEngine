#pragma once
#include "../../CoreVK/EngineDevice.h"
#include "Renderer.h"
#include "../../stb/stb_image.h"

#include <iostream>
#include <unordered_map>
#include <vector>

/*
	All of the helper functions that submit commands so far have been set up to execute synchronously
	by waiting for the queue to become idle. For practical applications it is recommended to combine
	these operations in a single command buffer and execute them asynchronously for higher throughput,
	especially the transitions and copy in the createTextureImage function. Try to experiment with this
	by creating a setupCommandBuffer that the helper functions record commands into, and add a
	flushSetupCommands to execute the commands that have been recorded so far
*/
namespace aveng {

	class ImageSystem {

		const char* textures[8] = {
			"textures/grid2.png", "textures/theme1.png", "textures/theme3.png", "textures/theme4.png",
			"textures/theme2.png", "textures/sm2.png", "textures/sm3.png", "textures/sm4.png"
		};

	public:

		const int MAX_TEXTURES{ 8 };

		ImageSystem(EngineDevice& device);
		~ImageSystem();

		void createTextureImage(const char* filepath, size_t i);
		VkImageView createImageView(VkImage image, VkFormat format, uint32_t mipLevels);
		void createTextureImageView(VkImage image, size_t i);
		void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
		void createTextureSampler();

		void createImageDescriptors(std::vector<VkImageView> views);
		VkDescriptorImageInfo getImageInfoAtIndex(int index) { return imageInfosArray[index]; };
		std::vector<VkDescriptorImageInfo> descriptorInfoForAllImages()
		{ 
			return imageInfosArray; 
		}

	private:

		// mipLevels is one dependency keeping us from merging some of these functions with SwapChain's impelmentations of them

		EngineDevice& engineDevice;

		VkSampler textureSampler;
		std::vector<VkImage> images;
		std::vector<uint32_t> mipLevels;
		std::vector<VkImageView> textureImageViews;
		std::vector<VkDeviceMemory> allImageMemory;
		std::vector<VkDescriptorImageInfo> imageInfosArray;
		
		//std::unordered_map<std::string, Texture> textures;

	};

}