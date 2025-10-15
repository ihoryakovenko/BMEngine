#include "Engine/Engine.h"
#include "Engine/Systems/HandleManager.h"
#include <iostream>
#include <cassert>

// Test data structure
struct TestData
{
	int value;
	float data;
	char name[32];
};

void TestHandleManager()
{
	std::cout << "=== HandleManager Tests ===" << std::endl;
	
	// Test 1: Basic initialization
	std::cout << "Test 1: Basic initialization..." << std::endl;
	Systems_HandleManager_Data manager;
	Systems_HandleManager_InitData(&manager, 10, sizeof(TestData), 1);
	
	assert(manager.Entries != nullptr);
	assert(manager.StorageData != nullptr);
	assert(manager.StorageCapacity == 10);
	assert(manager.DataSize == sizeof(TestData));
	std::cout << "✓ Initialization successful" << std::endl;
	
	// Test 2: Create handles
	std::cout << "Test 2: Create handles..." << std::endl;
	TestData data1 = {42, 3.14f, "Test1"};
	TestData data2 = {100, 2.71f, "Test2"};
	TestData data3 = {200, 1.41f, "Test3"};
	
	Systems_HandleManager_Handle handle1 = Systems_HandleManager_CreateHandle(&manager, &data1);
	Systems_HandleManager_Handle handle2 = Systems_HandleManager_CreateHandle(&manager, &data2);
	Systems_HandleManager_Handle handle3 = Systems_HandleManager_CreateHandle(&manager, &data3);
	
	assert(handle1.Index == 0);
	assert(handle1.Generation == 1);
	assert(handle1.Type == 1);
	assert(handle2.Index == 1);
	assert(handle2.Generation == 1);
	assert(handle2.Type == 1);
	assert(handle3.Index == 2);
	assert(handle3.Generation == 1);
	assert(handle3.Type == 1);
	std::cout << "✓ Handle creation successful" << std::endl;
	
	// Test 3: Retrieve data
	std::cout << "Test 3: Retrieve data..." << std::endl;
	TestData* retrieved1 = static_cast<TestData*>(Systems_HandleManager_GetHandleData(&manager, handle1));
	TestData* retrieved2 = static_cast<TestData*>(Systems_HandleManager_GetHandleData(&manager, handle2));
	TestData* retrieved3 = static_cast<TestData*>(Systems_HandleManager_GetHandleData(&manager, handle3));
	
	assert(retrieved1 != nullptr);
	assert(retrieved1->value == 42);
	assert(retrieved1->data == 3.14f);
	assert(strcmp(retrieved1->name, "Test1") == 0);
	
	assert(retrieved2 != nullptr);
	assert(retrieved2->value == 100);
	assert(retrieved2->data == 2.71f);
	assert(strcmp(retrieved2->name, "Test2") == 0);
	
	assert(retrieved3 != nullptr);
	assert(retrieved3->value == 200);
	assert(retrieved3->data == 1.41f);
	assert(strcmp(retrieved3->name, "Test3") == 0);
	std::cout << "✓ Data retrieval successful" << std::endl;
	
	// Test 4: Handle validation
	std::cout << "Test 4: Handle validation..." << std::endl;
	assert(Systems_HandleManager_IsHandleValid(&manager, handle1) == true);
	assert(Systems_HandleManager_IsHandleValid(&manager, handle2) == true);
	assert(Systems_HandleManager_IsHandleValid(&manager, handle3) == true);
	
	// Test 5: Destroy handle and reuse
	std::cout << "Test 5: Destroy handle and reuse..." << std::endl;
	Systems_HandleManager_DestroyHandle(&manager, handle2);
	
	// Handle2 should be invalid now
	assert(Systems_HandleManager_IsHandleValid(&manager, handle2) == false);
	
	// Create new handle - should reuse index 1
	TestData data4 = {300, 4.5f, "Test4"};
	Systems_HandleManager_Handle handle4 = Systems_HandleManager_CreateHandle(&manager, &data4);
	
	assert(handle4.Index == 1); // Should reuse index 1
	assert(handle4.Generation == 2); // Generation should be incremented
	assert(handle4.Type == 1); // Type should be preserved
	
	TestData* retrieved4 = static_cast<TestData*>(Systems_HandleManager_GetHandleData(&manager, handle4));
	assert(retrieved4 != nullptr);
	assert(retrieved4->value == 300);
	assert(retrieved4->data == 4.5f);
	assert(strcmp(retrieved4->name, "Test4") == 0);
	std::cout << "✓ Handle reuse successful" << std::endl;
	
	// Test 6: Multiple destroy and reuse
	std::cout << "Test 6: Multiple destroy and reuse..." << std::endl;
	Systems_HandleManager_DestroyHandle(&manager, handle1);
	Systems_HandleManager_DestroyHandle(&manager, handle3);
	
	// Create two new handles
	TestData data5 = {400, 5.5f, "Test5"};
	TestData data6 = {500, 6.5f, "Test6"};
	
	Systems_HandleManager_Handle handle5 = Systems_HandleManager_CreateHandle(&manager, &data5);
	Systems_HandleManager_Handle handle5_2 = Systems_HandleManager_CreateHandle(&manager, &data6);
	
	// Should reuse indices 0 and 2
	assert(handle5.Index == 0 || handle5.Index == 2);
	assert(handle5_2.Index == 0 || handle5_2.Index == 2);
	assert(handle5.Index != handle5_2.Index);
	assert(handle5.Generation == 2); // Reused slot
	assert(handle5.Type == 1); // Type should be preserved
	assert(handle5_2.Generation == 2); // Reused slot
	assert(handle5_2.Type == 1); // Type should be preserved
	std::cout << "✓ Multiple handle reuse successful" << std::endl;
	
	// Test 7: Capacity expansion
	std::cout << "Test 7: Capacity expansion..." << std::endl;
	
	// Create manager with capacity 4
	Systems_HandleManager_Data smallManager;
	Systems_HandleManager_InitData(&smallManager, 4, sizeof(TestData), 1);
	
	assert(smallManager.StorageCapacity == 4);
	assert(smallManager.FreeIndicesCapacity == 4);
	
	// Create 6 handles (should trigger storage expansion)
	TestData data[6];
	Systems_HandleManager_Handle handles[6];
	
	for (int i = 0; i < 6; i++)
	{
		data[i] = {i + 1, (float)(i + 1), "Test"};
		handles[i] = Systems_HandleManager_CreateHandle(&smallManager, &data[i]);
	}
	
	assert(smallManager.StorageCapacity == 8); // Should be doubled
	assert(smallManager.StorageCount == 6);
	
	// Destroy all 6 handles (should trigger FreeIndices expansion)
	for (int i = 0; i < 6; i++)
	{
		Systems_HandleManager_DestroyHandle(&smallManager, handles[i]);
	}
	
	assert(smallManager.FreeIndicesCapacity == 8); // Should be doubled
	assert(smallManager.FreeIndicesCount == 6);
	
	std::cout << "✓ Capacity expansion successful" << std::endl;
	
	// Test 9: Cleanup
	std::cout << "Test 9: Cleanup..." << std::endl;
	Systems_HandleManager_ClearData(&manager);
	Systems_HandleManager_ClearData(&smallManager);
	std::cout << "✓ Cleanup successful" << std::endl;
	
	std::cout << "=== All HandleManager tests passed! ===" << std::endl;
}

int main()
{
	TestHandleManager();
	Engine::Main();	
}