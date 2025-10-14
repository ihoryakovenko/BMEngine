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
    HandleManager::HandleManagerData manager;
    HandleManager::InitHandleManagerData(&manager, 10, sizeof(TestData));
    
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
    
    HandleManager::Handle handle1 = HandleManager::CreateHandle(&manager, &data1);
    HandleManager::Handle handle2 = HandleManager::CreateHandle(&manager, &data2);
    HandleManager::Handle handle3 = HandleManager::CreateHandle(&manager, &data3);
    
    assert(handle1.Index == 0);
    assert(handle1.Generation == 1);
    assert(handle2.Index == 1);
    assert(handle2.Generation == 1);
    assert(handle3.Index == 2);
    assert(handle3.Generation == 1);
    std::cout << "✓ Handle creation successful" << std::endl;
    
    // Test 3: Retrieve data
    std::cout << "Test 3: Retrieve data..." << std::endl;
    TestData* retrieved1 = static_cast<TestData*>(HandleManager::GetHandleData(&manager, handle1));
    TestData* retrieved2 = static_cast<TestData*>(HandleManager::GetHandleData(&manager, handle2));
    TestData* retrieved3 = static_cast<TestData*>(HandleManager::GetHandleData(&manager, handle3));
    
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
    assert(HandleManager::IsHandleValid(&manager, handle1) == true);
    assert(HandleManager::IsHandleValid(&manager, handle2) == true);
    assert(HandleManager::IsHandleValid(&manager, handle3) == true);
    
    // Test invalid handle
    HandleManager::Handle invalidHandle = {999, 1};
    assert(HandleManager::IsHandleValid(&manager, invalidHandle) == false);
    std::cout << "✓ Handle validation successful" << std::endl;
    
    // Test 5: Destroy handle and reuse
    std::cout << "Test 5: Destroy handle and reuse..." << std::endl;
    HandleManager::DestroyHandle(&manager, handle2);
    
    // Handle2 should be invalid now
    assert(HandleManager::IsHandleValid(&manager, handle2) == false);
    assert(HandleManager::GetHandleData(&manager, handle2) == nullptr);
    
    // Create new handle - should reuse index 1
    TestData data4 = {300, 4.5f, "Test4"};
    HandleManager::Handle handle4 = HandleManager::CreateHandle(&manager, &data4);
    
    assert(handle4.Index == 1); // Should reuse index 1
    assert(handle4.Generation == 2); // Generation should be incremented
    
    TestData* retrieved4 = static_cast<TestData*>(HandleManager::GetHandleData(&manager, handle4));
    assert(retrieved4 != nullptr);
    assert(retrieved4->value == 300);
    assert(retrieved4->data == 4.5f);
    assert(strcmp(retrieved4->name, "Test4") == 0);
    std::cout << "✓ Handle reuse successful" << std::endl;
    
    // Test 6: Multiple destroy and reuse
    std::cout << "Test 6: Multiple destroy and reuse..." << std::endl;
    HandleManager::DestroyHandle(&manager, handle1);
    HandleManager::DestroyHandle(&manager, handle3);
    
    // Create two new handles
    TestData data5 = {400, 5.5f, "Test5"};
    TestData data6 = {500, 6.5f, "Test6"};
    
    HandleManager::Handle handle5 = HandleManager::CreateHandle(&manager, &data5);
    HandleManager::Handle handle5_2 = HandleManager::CreateHandle(&manager, &data6);
    
    // Should reuse indices 0 and 2
    assert(handle5.Index == 0 || handle5.Index == 2);
    assert(handle5_2.Index == 0 || handle5_2.Index == 2);
    assert(handle5.Index != handle5_2.Index);
    assert(handle5.Generation == 2); // Reused slot
    assert(handle5_2.Generation == 2); // Reused slot
    std::cout << "✓ Multiple handle reuse successful" << std::endl;
    
    // Test 7: Cleanup
    std::cout << "Test 7: Cleanup..." << std::endl;
    HandleManager::ClearHandleManager(&manager);
    assert(manager.Entries == nullptr);
    assert(manager.StorageData == nullptr);
    std::cout << "✓ Cleanup successful" << std::endl;
    
    std::cout << "=== All HandleManager tests passed! ===" << std::endl;
}

int main()
{
    TestHandleManager();
    Engine::Main();	
}