//struct RecyclingCommandPool
//{
//	VkCommandPool CommandPool;
//	VkCommandBuffer* Buffers;
//	VkFence* BufferFences;
//	u32 BufferCount;
//	u32 NextBufferIndex;
//};
//
//static void WaitForNextBuffer(VkDevice Device, RecyclingCommandPool* Pool, VkCommandBuffer* OutBuffer, VkFence* OutFence);
//
//void WaitForNextBuffer(VkDevice Device, RecyclingCommandPool* Pool, VkCommandBuffer* OutBuffer, VkFence* OutFence)
//{
//	const VkFence* Fence = Pool->BufferFences + Pool->NextBufferIndex;
//
//	vkWaitForFences(Device, 1, Fence, VK_TRUE, UINT64_MAX);
//	vkResetFences(Device, 1, Fence);
//
//	Pool->NextBufferIndex = (Pool->NextBufferIndex + 1) % Pool->BufferCount;
//	*OutBuffer = Pool->Buffers[Pool->NextBufferIndex];
//	*OutFence = *Fence;
//}
//
//void InitTransferCommandPool(VkDevice Device, u32 QueueFamilyIndex, u32 BufferCount)
//{
//	TransferCommandPool.BufferCount = BufferCount;
//	TransferCommandPool.NextBufferIndex = 0;
//	TransferCommandPool.Buffers = Memory::MemoryManagementSystem::Allocate<VkCommandBuffer>(TransferCommandPool.BufferCount);
//	TransferCommandPool.BufferFences = Memory::MemoryManagementSystem::Allocate<VkFence>(TransferCommandPool.BufferCount);
//
//	VkCommandPoolCreateInfo PoolInfo = { };
//	PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//	PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//	PoolInfo.queueFamilyIndex = QueueFamilyIndex;
//
//	VkFenceCreateInfo FenceCreateInfo = { };
//	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
//
//	VkResult Result = vkCreateCommandPool(Device, &PoolInfo, nullptr, &TransferCommandPool.CommandPool);
//	if (Result != VK_SUCCESS)
//	{
//		Util::RenderLog(Util::BMRVkLogType_Error, "vkCreateCommandPool result is %d", Result);
//	}
//
//	VkCommandBufferAllocateInfo TransferCommandBufferAllocateInfo = { };
//	TransferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//	TransferCommandBufferAllocateInfo.commandPool = TransferCommandPool.CommandPool;
//	TransferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//	TransferCommandBufferAllocateInfo.commandBufferCount = TransferCommandPool.BufferCount;
//
//	for (u32 i = 0; i < TransferCommandPool.BufferCount; ++i)
//	{
//		if (vkCreateFence(Device, &FenceCreateInfo, nullptr, TransferCommandPool.BufferFences + i) != VK_SUCCESS)
//		{
//			Util::RenderLog(Util::BMRVkLogType_Error, "TransferFences creation error");
//			assert(false);
//		}
//	}
//
//	if (vkAllocateCommandBuffers(Device, &TransferCommandBufferAllocateInfo, TransferCommandPool.Buffers) != VK_SUCCESS)
//	{
//		Util::RenderLog(Util::BMRVkLogType_Error, "vkAllocateCommandBuffers error");
//		assert(false);
//	}
//}

//struct ResourceHandle
	//{
	//	u32 Index;
	//	u32 Generation;
	//};

	//ResourceHandle CreateTexture(VkImage image, VkImageView view)
	//{
	//	u32 LogicalIndex;

	//	if (!freeList.empty())
	//	{
	//		LogicalIndex = freeList.back();
	//		freeList.pop_back();
	//	}
	//	else
	//	{
	//		LogicalIndex = static_cast<u32>(handleEntries.size());
	//		handleEntries.emplace_back();
	//	}

	//	const u32 PhysicalIndex = TextureIndex++;

	//	Textures[PhysicalIndex] = image;
	//	TextureViews[PhysicalIndex] = view;

	//	handleEntries[LogicalIndex].Generation++;
	//	handleEntries[LogicalIndex].Alive = true;
	//	handleEntries[LogicalIndex].PhysicalIndex = PhysicalIndex;

	//	reverseMapping[PhysicalIndex] = LogicalIndex;

	//	return ResourceHandle{ LogicalIndex, handleEntries[LogicalIndex].Generation };
	//}

	//void DestroyTexture(ResourceHandle h)
	//{
	//	if (!IsValid(h)) return;

	//	u32 removedPhys = handleEntries[h.Index].PhysicalIndex;
	//	u32 lastPhys = TextureIndex - 1;

	//	if (removedPhys != lastPhys)
	//	{
	//		Textures[removedPhys] = Textures[lastPhys];
	//		TextureViews[removedPhys] = TextureViews[lastPhys];

	//		u32 movedLogical = reverseMapping[lastPhys];
	//		handleEntries[movedLogical].PhysicalIndex = removedPhys;
	//		reverseMapping[removedPhys] = movedLogical;
	//	}

	//	handleEntries[h.Index].Alive = false;
	//	freeList.push_back(h.Index);
	//	--TextureIndex;
	//}

	//bool IsValid(ResourceHandle h)
	//{
	//	return h.Index < handleEntries.size() &&
	//		handleEntries[h.Index].Alive &&
	//		handleEntries[h.Index].Generation == h.Generation;
	//}

	//VkImage GetImage(ResourceHandle h)
	//{
	//	assert(IsValid(h));
	//	const u32 PhysicalIndex = handleEntries[h.Index].PhysicalIndex;
	//	return Textures[PhysicalIndex];
	//}

	//VkImageView GetView(ResourceHandle h)
	//{
	//	assert(IsValid(h));
	//	const u32 PhysicalIndex = handleEntries[h.Index].PhysicalIndex;
	//	return TextureViews[PhysicalIndex];
	//}