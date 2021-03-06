#include <gi_gbuffer.h>
#include <vk_utils.h>
#include <vk_initializers.h>
#include <vk_pipeline.h>

/*
* G BUFFER CONTENT
* R16G16B16A16: POS (XYZ) + MATERIAL_ID (A)
* R16G16B16A16: NORMAL (XYZ) + ...
* R16G16B16A16: TEX UV (XY) + LIGHTMAP UV (ZA)
*/

void GBuffer::init_render_pass(EngineData& engineData)
{
	VkAttachmentDescription attachmentDescs[5] = {};

	for (uint32_t i = 0; i < 5; ++i)
	{
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		if (i == 4)
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}

	attachmentDescs[0].format = engineData.color8Format;
	attachmentDescs[1].format = engineData.color16Format;
	attachmentDescs[2].format = engineData.color16Format;
	attachmentDescs[3].format = engineData.color16Format;
	attachmentDescs[4].format = engineData.depth32Format;

	VkAttachmentReference colorReferences[4] = {};
	colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	colorReferences[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 4;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorReferences;
	subpass.colorAttachmentCount = static_cast<uint32_t>(4);
	subpass.pDepthStencilAttachment = &depthReference;

	VkSubpassDependency dependencies[2] = {};

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs;
	renderPassInfo.attachmentCount = 5;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies;

	VK_CHECK(vkCreateRenderPass(engineData.device, &renderPassInfo, nullptr, &_gbufferRenderPass));
}

void GBuffer::init_images(EngineData& engineData, VkExtent2D imageSize)
{
	_imageSize = imageSize;
	VkExtent3D extent3D = {
				_imageSize.width,
				_imageSize.height,
				1
			};
	for(int i = 0; i < 2; i++)
	{
		{
			VkImageCreateInfo dimg_info = vkinit::image_create_info(engineData.color8Format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent3D);
			VmaAllocationCreateInfo dimg_allocinfo = {};
			dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vmaCreateImage(engineData.allocator, &dimg_info, &dimg_allocinfo, &_gbufferdata[i]._gbufferAlbedoMetallicImage._image, &_gbufferdata[i]._gbufferAlbedoMetallicImage._allocation, nullptr);

			VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(engineData.color8Format, _gbufferdata[i]._gbufferAlbedoMetallicImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
			VK_CHECK(vkCreateImageView(engineData.device, &imageViewInfo, nullptr, &_gbufferdata[i]._gbufferAlbedoMetallicImageView));
		}

		{
			VkImageCreateInfo dimg_info = vkinit::image_create_info(engineData.color16Format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent3D);
			VmaAllocationCreateInfo dimg_allocinfo = {};
			dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vmaCreateImage(engineData.allocator, &dimg_info, &dimg_allocinfo, &_gbufferdata[i]._gbufferNormalMotionImage._image, &_gbufferdata[i]._gbufferNormalMotionImage._allocation, nullptr);

			VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(engineData.color16Format, _gbufferdata[i]._gbufferNormalMotionImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
			VK_CHECK(vkCreateImageView(engineData.device, &imageViewInfo, nullptr, &_gbufferdata[i]._gbufferNormalMotionImageView));
		}

		{
			VkImageCreateInfo dimg_info = vkinit::image_create_info(engineData.color16Format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent3D);
			VmaAllocationCreateInfo dimg_allocinfo = {};
			dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vmaCreateImage(engineData.allocator, &dimg_info, &dimg_allocinfo, &_gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImage._image, &_gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImage._allocation, nullptr);

			VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(engineData.color16Format, _gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
			VK_CHECK(vkCreateImageView(engineData.device, &imageViewInfo, nullptr, &_gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImageView));
		}

		{
			VkImageCreateInfo dimg_info = vkinit::image_create_info(engineData.color16Format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent3D);
			VmaAllocationCreateInfo dimg_allocinfo = {};
			dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vmaCreateImage(engineData.allocator, &dimg_info, &dimg_allocinfo, &_gbufferdata[i]._gbufferUVImage._image, &_gbufferdata[i]._gbufferUVImage._allocation, nullptr);

			VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(engineData.color16Format, _gbufferdata[i]._gbufferUVImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
			VK_CHECK(vkCreateImageView(engineData.device, &imageViewInfo, nullptr, &_gbufferdata[i]._gbufferUVImageView));
		}

		{
			VkImageCreateInfo dimg_info = vkinit::image_create_info(engineData.depth32Format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent3D);
			VmaAllocationCreateInfo dimg_allocinfo = {};
			dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vmaCreateImage(engineData.allocator, &dimg_info, &dimg_allocinfo, &_gbufferdata[i]._gbufferDepthImage._image, &_gbufferdata[i]._gbufferDepthImage._allocation, nullptr);

			VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(engineData.depth32Format, _gbufferdata[i]._gbufferDepthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);
			VK_CHECK(vkCreateImageView(engineData.device, &imageViewInfo, nullptr, &_gbufferdata[i]._gbufferDepthImageView));
		}

		VkImageView attachments[5] = { _gbufferdata[i]._gbufferAlbedoMetallicImageView, _gbufferdata[i]._gbufferNormalMotionImageView, _gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImageView, _gbufferdata[i]._gbufferUVImageView, _gbufferdata[i]._gbufferDepthImageView };

		VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_gbufferRenderPass, imageSize);
		fb_info.pAttachments = attachments;
		fb_info.attachmentCount = 5;
		VK_CHECK(vkCreateFramebuffer(engineData.device, &fb_info, nullptr, &_gbufferdata[i]._gbufferFrameBuffer));
		
		vkutils::setObjectName(engineData.device, _gbufferdata[i]._gbufferFrameBuffer, "GBufferFrameBuffer");
	}
}

void GBuffer::init_descriptors(EngineData& engineData)
{
	VkDescriptorSetLayoutBinding bindings[5] = {
		vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT, 0),
		vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT, 1),
		vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT, 2),
		vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT, 3),
		vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT, 4)
	};
	
	VkDescriptorSetLayoutCreateInfo setinfo = vkinit::descriptorset_layout_create_info(bindings, 5);
	vkCreateDescriptorSetLayout(engineData.device, &setinfo, nullptr, &_gbufferDescriptorSetLayout);

	for(int i = 0; i < 2; i++)
	{
		VkDescriptorImageInfo albedoMetallicBufferInfo = { engineData.nearestSampler, _gbufferdata[i]._gbufferAlbedoMetallicImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkDescriptorImageInfo normalMotionBufferInfo = { engineData.nearestSampler, _gbufferdata[i]._gbufferNormalMotionImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkDescriptorImageInfo roughnessDepthCurvatureMaterialBufferInfo = { engineData.nearestSampler, _gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkDescriptorImageInfo uvBufferInfo = { engineData.nearestSampler, _gbufferdata[i]._gbufferUVImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkDescriptorImageInfo depthBufferInfo = { engineData.nearestSampler, _gbufferdata[i]._gbufferDepthImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorset_allocate_info(engineData.descriptorPool, &_gbufferDescriptorSetLayout, 1);
		vkAllocateDescriptorSets(engineData.device, &allocInfo, &_gbufferdata[i]._gbufferDescriptorSet);

		VkWriteDescriptorSet textures[5] = {
			vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _gbufferdata[i]._gbufferDescriptorSet, &albedoMetallicBufferInfo, 0, 1),
			vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _gbufferdata[i]._gbufferDescriptorSet, &normalMotionBufferInfo, 1, 1),
			vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _gbufferdata[i]._gbufferDescriptorSet, &roughnessDepthCurvatureMaterialBufferInfo, 2, 1),
			vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _gbufferdata[i]._gbufferDescriptorSet, &uvBufferInfo, 3, 1),
			vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _gbufferdata[i]._gbufferDescriptorSet, &depthBufferInfo, 4, 1),
		};

		vkUpdateDescriptorSets(engineData.device, 5, textures, 0, nullptr);
	}
}

void GBuffer::init_pipelines(EngineData& engineData, SceneDescriptors& sceneDescriptors, bool rebuild)
{
	VkShaderModule gbufferVertShader;
	if (!vkutils::load_shader_module(engineData.device, "../../shaders/gbuffer.vert.spv", &gbufferVertShader))
	{
		assert("G Buffer Vertex Shader Loading Issue");
	}

	VkShaderModule gbufferFragShader;
	if (!vkutils::load_shader_module(engineData.device, "../../shaders/gbuffer.frag.spv", &gbufferFragShader))
	{
		assert("F Buffer Fragment Shader Loading Issue");
	}

	if (!rebuild)
	{
		VkDescriptorSetLayout setLayouts[] = { sceneDescriptors.globalSetLayout, sceneDescriptors.objectSetLayout, sceneDescriptors.textureSetLayout, sceneDescriptors.materialSetLayout };
		VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info(setLayouts, 4);
		VK_CHECK(vkCreatePipelineLayout(engineData.device, &pipeline_layout_info, nullptr, &_gbufferPipelineLayout));
	}
	else {
		vkDestroyPipeline(engineData.device, _gbufferPipeline, nullptr);
	}

	//build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;

	//vertex input controls how to read vertices from vertex buffers. We aren't using it yet
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();

	//input assembly is the configuration for drawing triangle lists, strips, or individual points.
	//we are just going to draw triangle list
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	//configure the rasterizer to draw filled triangles
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

	//we don't use multisampling, so just run the default one
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();

	//a single blend attachment with no blending and writing to RGBA

	VkPipelineColorBlendAttachmentState blendAttachmentState[4] = {
		vkinit::color_blend_attachment_state(),
		vkinit::color_blend_attachment_state(),
		vkinit::color_blend_attachment_state(),
		vkinit::color_blend_attachment_state()
	};

	pipelineBuilder._colorBlending = vkinit::color_blend_state_create_info(4, blendAttachmentState);

	//build the mesh pipeline
	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, gbufferVertShader));

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, gbufferFragShader));

	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineBuilder._pipelineLayout = _gbufferPipelineLayout;

	//build the mesh triangle pipeline
	_gbufferPipeline = pipelineBuilder.build_pipeline(engineData.device, _gbufferRenderPass);

	vkDestroyShaderModule(engineData.device, gbufferVertShader, nullptr);
	vkDestroyShaderModule(engineData.device, gbufferFragShader, nullptr);
}

void GBuffer::render(VkCommandBuffer cmd, EngineData& engineData, SceneDescriptors& sceneDescriptors, std::function<void(VkCommandBuffer cmd)>&& function)
{
	_currFrame++;

	VkClearValue clearValues[5];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, -1.0f } };
	clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[4].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(_gbufferRenderPass, _imageSize, _gbufferdata[_currFrame % 2]._gbufferFrameBuffer);
	rpInfo.clearValueCount = 5;
	rpInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkutils::cmd_viewport_scissor(cmd, _imageSize);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipelineLayout, 0, 1, &sceneDescriptors.globalDescriptor, 0, nullptr);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipelineLayout, 1, 1, &sceneDescriptors.objectDescriptor, 0, nullptr);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipelineLayout, 2, 1, &sceneDescriptors.textureDescriptor, 0, nullptr);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPipelineLayout, 3, 1, &sceneDescriptors.materialDescriptor, 0, nullptr);

	function(cmd);

	vkCmdEndRenderPass(cmd);
}

void GBuffer::cleanup(EngineData& engineData)
{
	vkDestroyPipeline(engineData.device, _gbufferPipeline, nullptr);
	vkDestroyPipelineLayout(engineData.device, _gbufferPipelineLayout, nullptr);

	vkDestroyDescriptorSetLayout(engineData.device, _gbufferDescriptorSetLayout, nullptr);


	for (int i = 0; i < 2; i++) {
		vkDestroyFramebuffer(engineData.device, _gbufferdata[i]._gbufferFrameBuffer, nullptr);

		vkDestroyImageView(engineData.device, _gbufferdata[i]._gbufferAlbedoMetallicImageView, nullptr);
		vkDestroyImageView(engineData.device, _gbufferdata[i]._gbufferNormalMotionImageView, nullptr);
		vkDestroyImageView(engineData.device, _gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImageView, nullptr);
		vkDestroyImageView(engineData.device, _gbufferdata[i]._gbufferUVImageView, nullptr);
		vkDestroyImageView(engineData.device, _gbufferdata[i]._gbufferDepthImageView, nullptr);

		vmaDestroyImage(engineData.allocator, _gbufferdata[i]._gbufferAlbedoMetallicImage._image, _gbufferdata[i]._gbufferAlbedoMetallicImage._allocation);
		vmaDestroyImage(engineData.allocator, _gbufferdata[i]._gbufferNormalMotionImage._image, _gbufferdata[i]._gbufferNormalMotionImage._allocation);
		vmaDestroyImage(engineData.allocator, _gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImage._image, _gbufferdata[i]._gbufferRoughnessDepthCurvatureMaterialImage._allocation);
		vmaDestroyImage(engineData.allocator, _gbufferdata[i]._gbufferUVImage._image, _gbufferdata[i]._gbufferUVImage._allocation);
		vmaDestroyImage(engineData.allocator, _gbufferdata[i]._gbufferDepthImage._image, _gbufferdata[i]._gbufferDepthImage._allocation);
	}
	
	vkDestroyRenderPass(engineData.device, _gbufferRenderPass, nullptr);
}

VkDescriptorSet GBuffer::getGbufferCurrentDescriptorSet()
{
	return _gbufferdata[_currFrame % 2]._gbufferDescriptorSet;
}

VkDescriptorSet GBuffer::getGbufferPreviousFrameDescriptorSet()
{
	return _gbufferdata[(_currFrame - 1) % 2]._gbufferDescriptorSet;
}
