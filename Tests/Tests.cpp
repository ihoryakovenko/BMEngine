#include <iostream>
#include <stdexcept>
#include <string>

#ifdef NDEBUG
#define assert(condition) ((void)0)
#define TEST_ASSERT(condition, message) ((void)0)
#else

#undef assert
#define assert(condition) \
		do { \
			if (!(condition)) { \
				throw std::runtime_error("Assertion failed: " #condition " - " __FILE__ ":" + std::to_string(__LINE__)); \
			} \
		} while (0)


#define TEST_ASSERT(condition, message) \
		do { \
			if (!(condition)) { \
				throw std::runtime_error("Test assertion failed: " message + std::string(" (") + #condition + ") - " __FILE__ ":" + std::to_string(__LINE__)); \
			} \
		} while (0)
#endif

#define CUSTOM_ASSERT
#include <Engine/Systems/Memory/MemoryManagmentSystem.h>

using namespace Memory;


template <typename T>
void PrintBufferState(const char* test_name, const RingBuffer<T>*buffer)
{
    std::cout << "  State (" << test_name << "): Cap=" << buffer->Capacity
        << ", Head=" << buffer->Head << ", Tail=" << buffer->Tail
        << ", Wrapped=" << (buffer->Wrapped ? "true" : "false") << "\n";
    std::cout << "  Data: [";
    if (buffer->DataArray && buffer->Capacity > 0)
    {
        for (u64 i = 0; i < buffer->Capacity; ++i)
        {
            std::cout << buffer->DataArray[i];
            if (i < buffer->Capacity - 1) std::cout << ", ";
        }
    }
    else
    {
        std::cout << "nullptr/empty";
    }
    std::cout << "]\n";
}

// --- Test Functions ---

bool TestAllocateRingBuffer()
{
    std::cout << "Running TestAllocateRingBuffer...\n";
    bool pass = true;

    // Test case 1: Positive capacity (happy path)
    {
        u64 test_capacity = 10;
        RingBuffer<int> buffer;

        try
        {
            buffer = AllocateRingBuffer<int>(test_capacity);
            TEST_ASSERT(buffer.Data != nullptr, "Data is nullptr for positive capacity.");
            TEST_ASSERT(buffer.Capacity == test_capacity, "Capacity mismatch.");
            TEST_ASSERT(buffer.Head == 0, "Head not initialized to 0.");
            TEST_ASSERT(buffer.Tail == 0, "Tail not initialized to 0.");
            TEST_ASSERT(buffer.Wrapped == false, "Wrapped not initialized to false.");
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not correctly reported as empty after allocation.");
            FreeRingBuffer(&buffer); // Free valid buffer
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for AllocateRingBuffer(positive capacity): " << e.what() << "\n";
            pass = false;
        }
    }

    // Test case 2: Zero capacity (should assert)
    {
        u64 test_capacity = 0;
        bool asserted = false;
        try
        {
            AllocateRingBuffer<int>(test_capacity); // This call is expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for AllocateRingBuffer(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "AllocateRingBuffer did not assert on zero capacity.");
    }

    std::cout << "TestAllocateRingBuffer: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestFreeRingBuffer()
{
    std::cout << "Running TestFreeRingBuffer...\n";
    bool pass = true;

    // Test case 1: Standard free (happy path)
    {
        u64 test_capacity = 5;
        auto buffer = AllocateRingBuffer<int>(test_capacity);
        int val = 10;
        PushToRingBuffer(&buffer, &val); // Add some data

        try
        {
            FreeRingBuffer(&buffer);
            TEST_ASSERT(buffer.Data == nullptr, "Data pointer not null after free.");
            TEST_ASSERT(buffer.Capacity == 0, "Capacity not zero after free.");
            TEST_ASSERT(buffer.Head == 0, "Head not zero after free.");
            TEST_ASSERT(buffer.Tail == 0, "Tail not zero after free.");
            TEST_ASSERT(buffer.Wrapped == false, "Wrapped not false after free.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for FreeRingBuffer(valid buffer): " << e.what() << "\n";
            pass = false;
        }
    }

    // Test case 2: Freeing a zero-capacity buffer (should assert)
    {
        // For this test, we need to manually construct the buffer to have 0 capacity
        // without triggering the assert in AllocateRingBuffer itself.
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Data = nullptr; // Ensure Data is null for a zero-cap buffer
        buffer_zero_cap.Capacity = 0;
        buffer_zero_cap.Head = 0;
        buffer_zero_cap.Tail = 0;
        buffer_zero_cap.Wrapped = false;

        bool asserted = false;
        try
        {
            FreeRingBuffer(&buffer_zero_cap); // This call is expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for FreeRingBuffer(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "FreeRingBuffer did not assert on zero capacity buffer.");
    }

    // Test case 3: Freeing an uninitialized buffer (which would have 0 capacity by default)
    {
        RingBuffer<int> buffer_uninit = { }; // Uninitialized state (Capacity will be 0)
        bool asserted = false;
        try
        {
            FreeRingBuffer(&buffer_uninit); // This will also assert due to Capacity != 0
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for FreeRingBuffer(uninitialized): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "FreeRingBuffer did not assert on uninitialized buffer.");
    }

    std::cout << "TestFreeRingBuffer: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestIsRingBufferEmptyAndFull()
{
    std::cout << "Running TestIsRingBufferEmptyAndFull...\n";
    bool pass = true;

    // Test case 1: Newly allocated buffer (empty) - happy path
    {
        auto buffer = AllocateRingBuffer<int>(3);
        try
        {
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Newly allocated buffer not empty.");
            TEST_ASSERT(!IsRingBufferFull(&buffer), "Newly allocated buffer reported as full.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for IsEmpty/IsFull(valid empty buffer): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 2: After some pushes (not empty, not full) - happy path
    {
        auto buffer = AllocateRingBuffer<int>(3);
        int val = 1;
        PushToRingBuffer(&buffer, &val);
        try
        {
            TEST_ASSERT(!IsRingBufferEmpty(&buffer), "Buffer with one item reported as empty.");
            TEST_ASSERT(!IsRingBufferFull(&buffer), "Buffer with one item reported as full.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for IsEmpty/IsFull(valid partial buffer): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 3: Full buffer - happy path
    {
        auto buffer = AllocateRingBuffer<int>(2);
        int v1 = 1, v2 = 2;
        PushToRingBuffer(&buffer, &v1);
        PushToRingBuffer(&buffer, &v2); // Now full
        try
        {
            TEST_ASSERT(!IsRingBufferEmpty(&buffer), "Full buffer reported as empty.");
            TEST_ASSERT(IsRingBufferFull(&buffer), "Full buffer not reported as full.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for IsEmpty/IsFull(valid full buffer): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 4: Pop until empty - happy path
    {
        auto buffer = AllocateRingBuffer<int>(2);
        int v1 = 1, v2 = 2;
        PushToRingBuffer(&buffer, &v1);
        PushToRingBuffer(&buffer, &v2);
        RingBufferPopFirst(&buffer);
        RingBufferPopFirst(&buffer); // Now empty again
        try
        {
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not empty after popping all items.");
            TEST_ASSERT(!IsRingBufferFull(&buffer), "Buffer reported as full after popping all items.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for IsEmpty/IsFull(valid popped-empty buffer): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 5: Zero-capacity buffer (should assert)
    {
        // Manually create a 0-capacity buffer to test assert
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Data = nullptr;
        buffer_zero_cap.Capacity = 0; // The target of the assert

        bool asserted_empty = false;
        try
        {
            IsRingBufferEmpty(&buffer_zero_cap); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for IsRingBufferEmpty(0-capacity): " << e.what() << "\n";
            asserted_empty = true;
        }
        TEST_ASSERT(asserted_empty, "IsRingBufferEmpty did not assert on zero capacity buffer.");

        bool asserted_full = false;
        try
        {
            IsRingBufferFull(&buffer_zero_cap); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for IsRingBufferFull(0-capacity): " << e.what() << "\n";
            asserted_full = true;
        }
        TEST_ASSERT(asserted_full, "IsRingBufferFull did not assert on zero capacity buffer.");
    }

    std::cout << "TestIsRingBufferEmptyAndFull: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestPushToRingBuffer()
{
    std::cout << "Running TestPushToRingBuffer...\n";
    bool pass = true;

    // Test case 1: Push to an empty buffer (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(3);
        int val = 10;
        try
        {
            PushToRingBuffer(&buffer, &val);
            TEST_ASSERT(buffer.Data[0] == 10, "Data not correct after first push.");
            TEST_ASSERT(buffer.Head == 1, "Head not correct after first push.");
            TEST_ASSERT(buffer.Tail == 0, "Tail not correct after first push.");
            TEST_ASSERT(buffer.Wrapped == false, "Wrapped not correct after first push.");
            TEST_ASSERT(!IsRingBufferEmpty(&buffer), "Buffer empty after first push.");
            TEST_ASSERT(!IsRingBufferFull(&buffer), "Buffer full after first push.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for PushToRingBuffer(valid empty buffer): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 2: Push until full (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(3);
        int v1 = 1, v2 = 2, v3 = 3;
        try
        {
            PushToRingBuffer(&buffer, &v1);
            PushToRingBuffer(&buffer, &v2);
            PushToRingBuffer(&buffer, &v3); // Now full
            TEST_ASSERT(buffer.Head == 0, "Head not correct after filling buffer.");
            TEST_ASSERT(buffer.Tail == 0, "Tail not correct after filling buffer.");
            TEST_ASSERT(buffer.Wrapped == true, "Wrapped not true after filling buffer.");
            TEST_ASSERT(IsRingBufferFull(&buffer), "Buffer not reported as full after filling.");
            TEST_ASSERT(!IsRingBufferEmpty(&buffer), "Buffer empty after filling.");
            TEST_ASSERT(buffer.Data[0] == 1 && buffer.Data[1] == 2 && buffer.Data[2] == 3, "Data not correct after filling buffer.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for PushToRingBuffer(filling buffer): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 3: Attempt to push when full (should assert)
    {
        auto buffer = AllocateRingBuffer<int>(2);
        int v1 = 1, v2 = 2, v3 = 3;
        PushToRingBuffer(&buffer, &v1);
        PushToRingBuffer(&buffer, &v2); // Buffer is now full

        bool asserted = false;
        try
        {
            PushToRingBuffer(&buffer, &v3); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for PushToRingBuffer(full): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "PushToRingBuffer did not assert on full buffer.");
        // Verify state remains unchanged after failed push
        TEST_ASSERT(buffer.Head == 0 && buffer.Tail == 0 && buffer.Wrapped == true, "Buffer state changed after failed push to full buffer.");
        TEST_ASSERT(IsRingBufferFull(&buffer), "Buffer not full after failed push.");

        FreeRingBuffer(&buffer);
    }

    // Test case 4: Push with zero capacity (should assert)
    {
        // Manually create a 0-capacity buffer to test assert
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Data = nullptr;
        buffer_zero_cap.Capacity = 0; // The target of the assert
        int val = 1;
        bool asserted = false;
        try
        {
            PushToRingBuffer(&buffer_zero_cap, &val); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for PushToRingBuffer(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "PushToRingBuffer did not assert on zero-capacity buffer.");
    }

    std::cout << "TestPushToRingBuffer: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestRingBufferGetFirst()
{
    std::cout << "Running TestRingBufferGetFirst...\n";
    bool pass = true;

    // Test case 1: Get from a buffer with one element (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(3);
        int val = 100;
        PushToRingBuffer(&buffer, &val);
        try
        {
            int* retrieved = RingBufferGetFirst(&buffer);
            TEST_ASSERT(retrieved != nullptr && *retrieved == 100, "GetFirst did not return correct element for single item.");
            TEST_ASSERT(buffer.Head == 1 && buffer.Tail == 0 && buffer.Wrapped == false, "Buffer state changed after GetFirst.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for GetFirst(valid single item): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 2: Get from a buffer with multiple elements and wrap-around (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(3);
        int v1 = 1, v2 = 2, v3 = 3;
        PushToRingBuffer(&buffer, &v1);
        PushToRingBuffer(&buffer, &v2);
        PushToRingBuffer(&buffer, &v3); // Now full
        try
        {
            int* retrieved = RingBufferGetFirst(&buffer);
            TEST_ASSERT(retrieved != nullptr && *retrieved == 1, "GetFirst did not return correct element after wrap-around.");
            TEST_ASSERT(buffer.Head == 0 && buffer.Tail == 0 && buffer.Wrapped == true, "Buffer state changed after GetFirst from full buffer.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for GetFirst(valid full buffer): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 3: Get from an empty buffer (should assert)
    {
        auto buffer = AllocateRingBuffer<int>(3); // Empty buffer
        bool asserted = false;
        try
        {
            RingBufferGetFirst(&buffer); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingBufferGetFirst(empty): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingBufferGetFirst did not assert on empty buffer.");
        FreeRingBuffer(&buffer);
    }

    // Test case 4: Get from zero-capacity buffer (should assert)
    {
        // Manually create a 0-capacity buffer to test assert
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Data = nullptr;
        buffer_zero_cap.Capacity = 0; // The target of the assert
        bool asserted = false;
        try
        {
            RingBufferGetFirst(&buffer_zero_cap); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingBufferGetFirst(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingBufferGetFirst did not assert on zero-capacity buffer.");
    }

    std::cout << "TestRingBufferGetFirst: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestRingBufferPopFirst()
{
    std::cout << "Running TestRingBufferPopFirst...\n";
    bool pass = true;

    // Test case 1: Pop from a buffer with one element (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(3);
        int val = 100;
        PushToRingBuffer(&buffer, &val);
        try
        {
            RingBufferPopFirst(&buffer);
            TEST_ASSERT(buffer.Head == 1 && buffer.Tail == 1 && buffer.Wrapped == false, "Buffer state incorrect after popping last item.");
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not empty after popping last item.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for PopFirst(valid single item): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 2: Pop multiple times with wrap-around, empty at the end (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(4);
        int v1 = 1, v2 = 2, v3 = 3, v4 = 4;
        PushToRingBuffer(&buffer, &v1);
        PushToRingBuffer(&buffer, &v2);
        PushToRingBuffer(&buffer, &v3);
        PushToRingBuffer(&buffer, &v4); // Full

        try
        {
            TEST_ASSERT(*RingBufferGetFirst(&buffer) == v1, "GetFirst did not return correct element before pop 1.");
            RingBufferPopFirst(&buffer); // T=1
            TEST_ASSERT(buffer.Tail == 1, "Tail incorrect after first pop.");
            TEST_ASSERT(*RingBufferGetFirst(&buffer) == v2, "GetFirst did not return correct element after pop 1.");

            RingBufferPopFirst(&buffer); // T=2
            TEST_ASSERT(buffer.Tail == 2, "Tail incorrect after second pop.");
            TEST_ASSERT(*RingBufferGetFirst(&buffer) == v3, "GetFirst did not return correct element after pop 2.");

            RingBufferPopFirst(&buffer); // T=3
            TEST_ASSERT(buffer.Tail == 3, "Tail incorrect after third pop.");
            TEST_ASSERT(*RingBufferGetFirst(&buffer) == v4, "GetFirst did not return correct element after pop 3.");

            RingBufferPopFirst(&buffer); // T=0. Now H=0, T=0, W=false (Empty)
            TEST_ASSERT(buffer.Head == 0 && buffer.Tail == 0 && buffer.Wrapped == false, "Buffer state incorrect after popping all items.");
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not empty after popping all items.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert during multiple valid pops: " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test case 3: Pop from an empty buffer (should assert)
    {
        auto buffer = AllocateRingBuffer<int>(3); // Empty buffer
        bool asserted = false;
        try
        {
            RingBufferPopFirst(&buffer); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingBufferPopFirst(empty): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingBufferPopFirst did not assert on empty buffer.");
        // State should not change after failed pop
        TEST_ASSERT(buffer.Head == 0 && buffer.Tail == 0 && buffer.Wrapped == false, "Buffer state changed after failed pop from empty buffer.");
        FreeRingBuffer(&buffer);
    }

    // Test case 4: Pop from zero-capacity buffer (should assert)
    {
        // Manually create a 0-capacity buffer to test assert
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Data = nullptr;
        buffer_zero_cap.Capacity = 0; // The target of the assert
        bool asserted = false;
        try
        {
            RingBufferPopFirst(&buffer_zero_cap); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingBufferPopFirst(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingBufferPopFirst did not assert on zero-capacity buffer.");
    }

    std::cout << "TestRingBufferPopFirst: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestPushPopWraparound()
{
    std::cout << "Running TestPushPopWraparound...\n";
    auto buffer = AllocateRingBuffer<int>(4); // Capacity 4
    bool pass = true;

    int v1 = 10, v2 = 20, v3 = 30, v4 = 40, v5 = 50;

    // Push 3 elements: H=3, T=0, W=false (happy path)
    try
    {
        PushToRingBuffer(&buffer, &v1);
        PushToRingBuffer(&buffer, &v2);
        PushToRingBuffer(&buffer, &v3);
        TEST_ASSERT(buffer.Head == 3 && buffer.Tail == 0 && buffer.Wrapped == false, "State incorrect after 3 pushes.");
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "  - FAILED: Unexpected assert during initial pushes: " << e.what() << "\n";
        pass = false;
    }

    // Push 4th element: H=0, T=0, W=true (Buffer is now full) (happy path)
    try
    {
        PushToRingBuffer(&buffer, &v4);
        TEST_ASSERT(buffer.Head == 0 && buffer.Tail == 0 && buffer.Wrapped == true, "State incorrect after 4 pushes (full).");
        TEST_ASSERT(IsRingBufferFull(&buffer), "Buffer not reported full after 4 pushes.");
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "  - FAILED: Unexpected assert during 4th push (filling): " << e.what() << "\n";
        pass = false;
    }

    // Attempt to push 5th element: Should assert
    bool asserted_push_full = false;
    try
    {
        PushToRingBuffer(&buffer, &v5); // Expected to assert
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "  - Caught expected assert for PushToRingBuffer(full): " << e.what() << "\n";
        asserted_push_full = true;
    }
    TEST_ASSERT(asserted_push_full, "PushToRingBuffer did not assert when buffer was full.");

    // Pop 1st element: T=1 (v1). H=0, T=1, W=true (happy path)
    try
    {
        TEST_ASSERT(*RingBufferGetFirst(&buffer) == v1, "GetFirst did not return correct element before pop 1.");
        RingBufferPopFirst(&buffer);
        TEST_ASSERT(buffer.Head == 0 && buffer.Tail == 1 && buffer.Wrapped == true, "State incorrect after 1 pop.");
        TEST_ASSERT(!IsRingBufferEmpty(&buffer), "Buffer empty too early after 1 pop.");
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "  - FAILED: Unexpected assert during 1st pop: " << e.what() << "\n";
        pass = false;
    }

    // Push new element: H=1, T=1, W=true (happy path)
    try
    {
        PushToRingBuffer(&buffer, &v5); // Data[0] gets v5
        TEST_ASSERT(buffer.Head == 1 && buffer.Tail == 1 && buffer.Wrapped == true, "State incorrect after new push.");
        TEST_ASSERT(IsRingBufferFull(&buffer), "Buffer not full after new push.");
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "  - FAILED: Unexpected assert during new push after pop: " << e.what() << "\n";
        pass = false;
    }

    // Verify order after mixed operations and pop all (happy path)
    try
    {
        TEST_ASSERT(*RingBufferGetFirst(&buffer) == v2, "GetFirst did not return correct element after pop 1, before pop 2.");
        RingBufferPopFirst(&buffer); // T=2
        TEST_ASSERT(*RingBufferGetFirst(&buffer) == v3, "GetFirst did not return correct element after pop 2.");
        RingBufferPopFirst(&buffer); // T=3
        TEST_ASSERT(*RingBufferGetFirst(&buffer) == v4, "GetFirst didData return correct element after pop 3.");
        RingBufferPopFirst(&buffer); // T=0
        TEST_ASSERT(*RingBufferGetFirst(&buffer) == v5, "GetFirst did not return correct element after pop 4.");
        RingBufferPopFirst(&buffer); // T=1. Now H=1, T=1, W=false (Empty)

        TEST_ASSERT(buffer.Head == 1 && buffer.Tail == 1 && buffer.Wrapped == false, "State incorrect after all pops (empty).");
        TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not empty after all elements were popped.");
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "  - FAILED: Unexpected assert during final pops: " << e.what() << "\n";
        pass = false;
    }

    // Attempt to get from empty buffer: Should assert
    bool asserted_get_empty = false;
    try
    {
        RingBufferGetFirst(&buffer); // Expected to assert
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "  - Caught expected assert for RingBufferGetFirst(empty) after all pops: " << e.what() << "\n";
        asserted_get_empty = true;
    }
    TEST_ASSERT(asserted_get_empty, "RingBufferGetFirst did not assert when buffer was empty after all pops.");

    FreeRingBuffer(&buffer);
    std::cout << "TestPushPopWraparound: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestRingIsFit()
{
    std::cout << "Running TestRingIsFit...\n";
    bool pass = true;

    // Test Case 1: Empty buffer (Head=0, Tail=0, Wrapped=false, Leftover=0)
    {
        auto buffer = AllocateRingBuffer<int>(10);
        try
        {
            TEST_ASSERT(RingIsFit(&buffer, 1), "RingIsFit(empty, 1) failed.");
            TEST_ASSERT(RingIsFit(&buffer, 10), "RingIsFit(empty, 10) failed.");
            TEST_ASSERT(!RingIsFit(&buffer, 11), "RingIsFit(empty, 11) unexpectedly true.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingIsFit(empty): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 2: Partially filled, contiguous space at end (Head=5, Tail=0, Wrapped=false, Leftover=0)
    {
        auto buffer = AllocateRingBuffer<int>(10);
        // Manually set state to simulate 5 elements used, 5 free at end
        int val = 0; // Dummy value for Data
        for (int i = 0; i < 5; ++i) buffer.Data[i] = val; // Simulate data
        buffer.Head = 5;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0;

        try
        {
            TEST_ASSERT(RingIsFit(&buffer, 5), "RingIsFit(partial, 5) failed."); // 5 free at end
            TEST_ASSERT(!RingIsFit(&buffer, 6), "RingIsFit(partial, 6) unexpectedly true.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingIsFit(partial, contiguous): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 3: Full buffer (Head=0, Tail=0, Wrapped=true, Leftover=0)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        // Manually set state to simulate full buffer
        int val = 0;
        for (int i = 0; i < 5; ++i) buffer.Data[i] = val;
        buffer.Head = 0;
        buffer.Tail = 0;
        buffer.Wrapped = true;
        buffer.Leftover = 0;

        try
        {
            TEST_ASSERT(!RingIsFit(&buffer, 1), "RingIsFit(full, 1) unexpectedly true.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingIsFit(full): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 4: Partially filled, wrapped, space at beginning (Head=4, Tail=1, Wrapped=true, Leftover=1)
    {
        auto buffer = AllocateRingBuffer<int>(5); // Capacity 5
        // Manually set state to simulate:
        // Used: indices 1,2,3 (3 elements)
        // Free: index 4 (1 element), index 0 (1 element)
        // Head=4, Tail=1, Wrapped=true, Leftover=1 (This Leftover would be set by RingAlloc's wrap logic)

        // Simulate data
        buffer.Data[1] = 1; buffer.Data[2] = 2; buffer.Data[3] = 3;
        buffer.Head = 4;
        buffer.Tail = 1;
        buffer.Wrapped = true; // Set wrapped to true to indicate Head has passed Tail
        buffer.Leftover = 1; // Simulate leftover from a previous wrap-around alloc

        try
        {
            // Check if a block of 1 fits at the end (index 4)
            TEST_ASSERT(RingIsFit(&buffer, 1), "RingIsFit(partial, 1, end) failed."); // Fits at index 4
            // Check if a block of 2 fits (it should not fit contiguously anywhere)
            TEST_ASSERT(!RingIsFit(&buffer, 2), "RingIsFit(partial, 2) unexpectedly true.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingIsFit(wrapped, space at beginning): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 5: Zero-capacity buffer (should assert)
    {
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Capacity = 0;
        bool asserted = false;
        try
        {
            RingIsFit(&buffer_zero_cap, 1); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingIsFit(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingIsFit did not assert on zero capacity buffer.");
    }

    // Test Case 6: Zero count (should assert)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        bool asserted = false;
        try
        {
            RingIsFit(&buffer, 0); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingIsFit(count 0): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingIsFit did not assert on zero count.");
        FreeRingBuffer(&buffer);
    }

    std::cout << "TestRingIsFit: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestRingUsed()
{
    std::cout << "Running TestRingUsed...\n";
    bool pass = true;

    // Test Case 1: Empty buffer (Head=0, Tail=0, Wrapped=false, Leftover=0)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        try
        {
            TEST_ASSERT(RingUsed(&buffer) == 0, "RingUsed(empty) failed.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingUsed(empty): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 2: Partially filled (no wrap) (Head=2, Tail=0, Wrapped=false, Leftover=0)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        // Manually set state
        buffer.Head = 2;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0;
        try
        {
            TEST_ASSERT(RingUsed(&buffer) == 2, "RingUsed(partial, no wrap) failed.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingUsed(partial, no wrap): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 3: Full buffer (Head=0, Tail=0, Wrapped=true, Leftover=0)
    {
        auto buffer = AllocateRingBuffer<int>(3);
        // Manually set state
        buffer.Head = 0;
        buffer.Tail = 0;
        buffer.Wrapped = true;
        buffer.Leftover = 0;
        try
        {
            TEST_ASSERT(RingUsed(&buffer) == 3, "RingUsed(full) failed.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingUsed(full): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 4: Partially filled (wrapped) (Head=0, Tail=2, Wrapped=true, Leftover=0)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        // Manually set state: 3 elements used (indices 2,3,4).
        buffer.Head = 0;
        buffer.Tail = 2;
        buffer.Wrapped = true;
        buffer.Leftover = 0;
        try
        {
            TEST_ASSERT(RingUsed(&buffer) == 3, "RingUsed(partial, wrapped) failed.");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingUsed(partial, wrapped): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 5: Zero-capacity buffer (should assert)
    {
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Capacity = 0;
        bool asserted = false;
        try
        {
            RingUsed(&buffer_zero_cap); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingUsed(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingUsed did not assert on zero capacity buffer.");
    }

    std::cout << "TestRingUsed: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestRingAlloc()
{
    std::cout << "Running TestRingAlloc...\n";
    bool pass = true;

    // Test Case 1: Allocate single element (happy path, empty)
    {
        auto buffer = AllocateRingBuffer<int>(5); // H=0, T=0, W=false, L=0
        try
        {
            int* ptr = RingAlloc(&buffer, 1);
            TEST_ASSERT(ptr == &buffer.Data[0], "RingAlloc(1) did not return correct pointer.");
            TEST_ASSERT(buffer.Head == 1, "Head incorrect after RingAlloc(1).");
            TEST_ASSERT(buffer.Tail == 0, "Tail should be 0 after RingAlloc(1).");
            TEST_ASSERT(buffer.Wrapped == false, "Wrapped should be false after RingAlloc(1).");
            TEST_ASSERT(buffer.Leftover == 0, "Leftover should be 0 after RingAlloc(1).");
            TEST_ASSERT(RingUsed(&buffer) == 1, "RingUsed incorrect after RingAlloc(1).");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingAlloc(1): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 2: Allocate multiple elements, fills buffer (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(3); // H=0, T=0, W=false, L=0
        try
        {
            int* ptr = RingAlloc(&buffer, 3);
            TEST_ASSERT(ptr == &buffer.Data[0], "RingAlloc(3) did not return correct pointer.");
            TEST_ASSERT(buffer.Head == 0, "Head incorrect after RingAlloc(3).");
            TEST_ASSERT(buffer.Tail == 0, "Tail incorrect after RingAlloc(3).");
            TEST_ASSERT(buffer.Wrapped == true, "Wrapped not true after RingAlloc(3).");
            TEST_ASSERT(buffer.Leftover == 0, "Leftover should be 0 after RingAlloc(3).");
            TEST_ASSERT(IsRingBufferFull(&buffer), "Buffer not full after RingAlloc(3).");
            TEST_ASSERT(RingUsed(&buffer) == 3, "RingUsed incorrect after RingAlloc(3).");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingAlloc(3): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 3: Allocate multiple elements, partial fill, no wrap (happy path)
    {
        auto buffer = AllocateRingBuffer<int>(10); // H=0, T=0, W=false, L=0
        // Manually set state: 1 element used at index 0.
        buffer.Head = 1;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0;

        try
        {
            int* ptr = RingAlloc(&buffer, 5); // Should allocate from index 1. New Head = 1 + 5 = 6.
            TEST_ASSERT(ptr == &buffer.Data[1], "RingAlloc(5) did not return correct pointer.");
            TEST_ASSERT(buffer.Head == 6, "Head incorrect after RingAlloc(5).");
            TEST_ASSERT(buffer.Tail == 0, "Tail incorrect after RingAlloc(5).");
            TEST_ASSERT(!buffer.Wrapped, "Wrapped true after RingAlloc(5).");
            TEST_ASSERT(buffer.Leftover == 0, "Leftover should be 0 after RingAlloc(5).");
            TEST_ASSERT(RingUsed(&buffer) == 6, "RingUsed incorrect after RingAlloc(5).");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingAlloc(5): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 4: Allocate requires wrap-around (Leftover logic - critical case as per your feedback)
    {
        auto buffer = AllocateRingBuffer<int>(5); // Cap 5
        buffer.Head = 4; // Used: Data[3]
        buffer.Tail = 3;
        buffer.Wrapped = false;
        buffer.Leftover = 0; // Start with no leftover

        try
        {
            int* ptr = RingAlloc(&buffer, 2); // Allocate 2 elements
            TEST_ASSERT(ptr == &buffer.Data[0], "RingAlloc(2, wrap) did not return correct pointer.");
            TEST_ASSERT(buffer.Head == 2, "Head incorrect after RingAlloc(2, wrap).");
            TEST_ASSERT(buffer.Tail == 3, "Tail incorrect after RingAlloc(2, wrap).");
            TEST_ASSERT(buffer.Wrapped == true, "Wrapped not true after RingAlloc(2, wrap).");
            TEST_ASSERT(buffer.Leftover == 1, "Leftover incorrect after RingAlloc(2, wrap)."); // Capacity - old Head = 5 - 4 = 1
            TEST_ASSERT(RingUsed(&buffer) == (5 - 3 + 2), "RingUsed incorrect after RingAlloc(2, wrap)."); // (Cap - Tail + Head) = 5 - 3 + 2 = 4
            TEST_ASSERT(RingUsed(&buffer) == 4, "RingUsed (calculated) incorrect after RingAlloc(2, wrap)."); // Explicitly check the calculated value
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingAlloc(wrap-around): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 5: Zero capacity (should assert)
    {
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Capacity = 0;
        bool asserted = false;
        try
        {
            RingAlloc(&buffer_zero_cap, 1); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingAlloc(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingAlloc did not assert on zero capacity buffer.");
    }

    // Test Case 6: Zero count (should assert)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        bool asserted = false;
        try
        {
            RingAlloc(&buffer, 0); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingAlloc(count 0): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingAlloc did not assert on zero count.");
        FreeRingBuffer(&buffer);
    }

    // Test Case 7: Not enough space (should assert due to RingIsFit check)
    {
        auto buffer = AllocateRingBuffer<int>(2); // Cap 2
        buffer.Head = 1;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0; // 1 element used at index 0. 1 free at index 1.

        bool asserted = false;
        try
        {
            RingAlloc(&buffer, 2); // Expected to assert because not enough contiguous space (only 1 at index 1)
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingAlloc(not enough contiguous space): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingAlloc did not assert when not enough space.");
        FreeRingBuffer(&buffer);
    }

    std::cout << "TestRingAlloc: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}

bool TestRingFree()
{
    std::cout << "Running TestRingFree...\n";
    bool pass = true;

    // Test Case 1: Free single element (happy path, no wrap, from tail)
    {
        auto buffer = AllocateRingBuffer<int>(5); // H=0, T=0, W=false, L=0
        buffer.Head = 1;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0; // 1 element used at index 0.

        try
        {
            RingFree(&buffer, 1);
            TEST_ASSERT(buffer.Tail == 0, "Tail incorrect after RingFree(0).");
            TEST_ASSERT(buffer.Head == 0, "Head should be 1 after RingFree(0).");
            TEST_ASSERT(buffer.Wrapped == false, "Wrapped should be false after RingFree(1).");
            TEST_ASSERT(buffer.Leftover == 0, "Leftover should be 0 after RingFree(1).");
            TEST_ASSERT(RingUsed(&buffer) == 0, "RingUsed incorrect after RingFree(1).");
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not empty after RingFree(1).");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingFree(1): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 2: Free multiple elements, fully empty (happy path, wrapped initially)
    {
        auto buffer = AllocateRingBuffer<int>(3); // H=0, T=0, W=false, L=0
        buffer.Head = 0;
        buffer.Tail = 0;
        buffer.Wrapped = true; // Buffer is full
        buffer.Leftover = 0;

        try
        {
            RingFree(&buffer, 3);
            TEST_ASSERT(buffer.Head == 0 && buffer.Tail == 0 && buffer.Wrapped == false, "State incorrect after RingFree(3).");
            TEST_ASSERT(buffer.Leftover == 0, "Leftover should be 0 after RingFree(3).");
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not empty after RingFree(3).");
            TEST_ASSERT(RingUsed(&buffer) == 0, "RingUsed incorrect after RingFree(3).");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingFree(3): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 3: Free multiple elements, partially empty (no wrap involved)
    {
        auto buffer = AllocateRingBuffer<int>(10); // H=0, T=0, W=false, L=0
        buffer.Head = 7;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0; // 7 elements used (indices 0-6).

        try
        {
            RingFree(&buffer, 3); // T=3
            TEST_ASSERT(buffer.Tail == 3, "Tail incorrect after RingFree(3).");
            TEST_ASSERT(buffer.Head == 7, "Head should be 7 after RingFree(3).");
            TEST_ASSERT(buffer.Wrapped == false, "Wrapped should be false after RingFree(3).");
            TEST_ASSERT(buffer.Leftover == 0, "Leftover should be 0 after RingFree(3).");
            TEST_ASSERT(RingUsed(&buffer) == 4, "RingUsed incorrect after RingFree(3).");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingFree(3, partial): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }

    // Test Case 4: Free with wrap-around, consuming leftover (Complex case)
    {
        auto buffer = AllocateRingBuffer<int>(5); // Cap 5
        // Setup buffer state as if a `RingAlloc` wrapped previously:
        // Original data in Data[3] (used by old Head).
        // New data in Data[0], Data[1] (allocated by RingAlloc).
        // So Tail=3, Head=2, Wrapped=true, Leftover=1.
        // Elements used: Data[3], Data[4], Data[0], Data[1], Data[2].
        // (Cap - Tail) = 5-3 = 2 elements (Data[3], Data[4])
        // (Head) = 2 elements (Data[0], Data[1])
        // Used = (5-3) + 2 = 4 (as per RingUsed formula).
        // This is a common situation for a full wrapped buffer with a leftover hole.
        buffer.Head = 2;
        buffer.Tail = 3;
        buffer.Wrapped = true;
        buffer.Leftover = 1; // This is the space from old Head (4) to Capacity-1 (4)

        try
        {
            // Free 3 elements. Current Tail is 3.
            // Tail will advance (3 + 3 = 6).
            // (Tail + count) > Capacity -> (3+3 > 5) -> True.
            // So, `Buffer->Tail = (Buffer->Tail + count + Buffer->Leftover) % Buffer->Capacity;`
            // `Buffer->Tail = (3 + 3 + 1) % 5 = 7 % 5 = 2`.
            RingFree(&buffer, 3);
            TEST_ASSERT(buffer.Tail == 0, "Tail incorrect after RingFree(3, consuming leftover).");
            TEST_ASSERT(buffer.Head == 0, "Head should match Tail after fully freeing all segments.");
            TEST_ASSERT(buffer.Wrapped == false, "Wrapped should be false as buffer becomes empty.");
            TEST_ASSERT(buffer.Leftover == 0, "Leftover should be 0 after consumption.");
            TEST_ASSERT(RingUsed(&buffer) == 0, "RingUsed incorrect after RingFree(3, consuming leftover).");
            TEST_ASSERT(IsRingBufferEmpty(&buffer), "Buffer not empty after RingFree(3, consuming leftover).");
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - FAILED: Unexpected assert for RingFree(wrap-around, consuming leftover): " << e.what() << "\n";
            pass = false;
        }
        FreeRingBuffer(&buffer);
    }


    // Test Case 5: Zero capacity (should assert)
    {
        RingBuffer<int> buffer_zero_cap = { };
        buffer_zero_cap.Capacity = 0;
        bool asserted = false;
        try
        {
            RingFree(&buffer_zero_cap, 1); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingFree(0-capacity): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingFree did not assert on zero capacity buffer.");
    }

    // Test Case 6: Zero count (should assert)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        buffer.Head = 1;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0; // 1 element used.

        bool asserted = false;
        try
        {
            RingFree(&buffer, 0); // Expected to assert
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingFree(count 0): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingFree did not assert on zero count.");
        FreeRingBuffer(&buffer);
    }

    // Test Case 7: Freeing more elements than available (should assert due to RingUsed check)
    {
        auto buffer = AllocateRingBuffer<int>(5);
        buffer.Head = 1;
        buffer.Tail = 0;
        buffer.Wrapped = false;
        buffer.Leftover = 0; // 1 element used.

        bool asserted = false;
        try
        {
            RingFree(&buffer, 2); // Expected to assert because count > RingUsed (1 currently)
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "  - Caught expected assert for RingFree(underflow): " << e.what() << "\n";
            asserted = true;
        }
        TEST_ASSERT(asserted, "RingFree did not assert on underflow.");
        FreeRingBuffer(&buffer);
    }

    std::cout << "TestRingFree: " << (pass ? "PASSED\n" : "FAILED\n");
    return pass;
}
// --- Main Test Runner ---
int main()
{
    bool allPassed = true;

    std::cout << "Starting all tests for RingBuffer implementation...\n\n";

    allPassed &= TestAllocateRingBuffer();
    std::cout << "\n";
    allPassed &= TestFreeRingBuffer();
    std::cout << "\n";
    allPassed &= TestIsRingBufferEmptyAndFull();
    std::cout << "\n";
    allPassed &= TestPushToRingBuffer();
    std::cout << "\n";
    allPassed &= TestRingBufferGetFirst();
    std::cout << "\n";
    allPassed &= TestRingBufferPopFirst();
    std::cout << "\n";
    allPassed &= TestPushPopWraparound();
    std::cout << "\n";
    allPassed &= TestRingIsFit();
    std::cout << "\n";
    allPassed &= TestRingUsed();
    std::cout << "\n";
    allPassed &= TestRingAlloc();
    std::cout << "\n";
    allPassed &= TestRingFree();
    std::cout << "\n";

    std::cout << (allPassed ? "All tests completed successfully.\n" : "Some tests FAILED.\n");
    return allPassed ? 0 : 1;
}