/**
 * @file test_object_pool.cpp
 * @brief Tests for ObjectPool utility
 */

#include <gtest/gtest.h>
#include <fluxeem/base/utility/object_pool/object_pool.h>

using namespace fluxeem;

// Simple test class for object pool testing
class TestObject {
public:
    TestObject() : value_(0) { ++instance_count; }
    explicit TestObject(int val) : value_(val) { ++instance_count; }
    ~TestObject() { --instance_count; }

    void setValue(int val) { value_ = val; }
    int getValue() const { return value_; }

    static int getInstanceCount() { return instance_count; }

private:
    int value_;
    static int instance_count;
};

int TestObject::instance_count = 0;

class ObjectPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset instance count before each test
        // Note: This is a simplification; in real scenarios, we'd need proper isolation
    }

    void TearDown() override {
        // Ensure all objects are cleaned up
    }
};

TEST_F(ObjectPoolTest, CreateUnboundedPool) {
    auto pool = ObjectPool<TestObject>::make_unbounded(10);
    EXPECT_EQ(pool.size(), 10);
    EXPECT_FALSE(pool.is_bounded());
}

TEST_F(ObjectPoolTest, CreateBoundedPool) {
    auto pool = ObjectPool<TestObject>::make_bounded(5);
    EXPECT_EQ(pool.size(), 5);
    EXPECT_TRUE(pool.is_bounded());
}

TEST_F(ObjectPoolTest, AcquireFromPool) {
    auto pool = ObjectPool<TestObject>::make_unbounded(5);
    EXPECT_EQ(pool.size(), 5);

    {
        auto obj = pool.acquire();
        EXPECT_EQ(pool.size(), 4);
        EXPECT_NE(obj, nullptr);
    }

    // Object should be returned to pool after going out of scope
    EXPECT_EQ(pool.size(), 5);
}

TEST_F(ObjectPoolTest, AcquireMultipleObjects) {
    auto pool = ObjectPool<TestObject>::make_unbounded(10);
    std::vector<ObjectPool<TestObject>::ptr_type> objects;

    // Acquire 5 objects
    for (int i = 0; i < 5; ++i) {
        auto obj = pool.acquire();
        obj->setValue(i * 10);
        objects.push_back(std::move(obj));
    }

    EXPECT_EQ(pool.size(), 5);

    // Verify values
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(objects[i]->getValue(), i * 10);
    }

    // Release objects back to pool
    objects.clear();
    EXPECT_EQ(pool.size(), 10);
}

TEST_F(ObjectPoolTest, UnboundedPoolExpands) {
    auto pool = ObjectPool<TestObject>::make_unbounded(2);
    EXPECT_EQ(pool.size(), 2);

    // Acquire more than initial capacity
    auto obj1 = pool.acquire();
    auto obj2 = pool.acquire();
    auto obj3 = pool.acquire();  // Should create new object

    EXPECT_EQ(pool.size(), 0);

    // Release all
    obj1.reset();
    obj2.reset();
    obj3.reset();

    EXPECT_EQ(pool.size(), 3);  // Pool now has 3 objects
}

TEST_F(ObjectPoolTest, AcquireWithConstructorArgs) {
    auto pool = ObjectPool<TestObject>::make_unbounded(3, 42);

    auto obj = pool.acquire();
    EXPECT_EQ(obj->getValue(), 42);

    obj->setValue(100);
    obj.reset();

    // After returning to pool and re-acquiring, object retains its previous state
    // (object pool does not reset object values)
    auto obj2 = pool.acquire();
    EXPECT_EQ(obj2->getValue(), 100);  // Retains previous value
}

TEST_F(ObjectPoolTest, EmptyPool) {
    auto pool = ObjectPool<TestObject>::make_unbounded(0);
    EXPECT_TRUE(pool.empty());

    // Acquire creates a new object in unbounded pool (when empty, it allocates)
    auto obj = pool.acquire();
    // After acquiring from empty unbounded pool, the pool is still empty
    // because the newly created object is immediately taken out
    EXPECT_TRUE(pool.empty());

    // Return object to pool
    obj.reset();
    EXPECT_FALSE(pool.empty());
}

TEST_F(ObjectPoolTest, ArrangeUnboundedPool) {
    auto pool = ObjectPool<TestObject>::make_unbounded(2);
    EXPECT_EQ(pool.size(), 2);

    size_t added = pool.arrange(10);
    EXPECT_EQ(added, 8);
    EXPECT_EQ(pool.size(), 10);
}

TEST_F(ObjectPoolTest, BoundedPoolCannotArrange) {
    auto pool = ObjectPool<TestObject>::make_bounded(2);
    EXPECT_EQ(pool.size(), 2);

    // Arrange should not add objects to bounded pool
    size_t added = pool.arrange(10);
    EXPECT_EQ(added, 0);
    EXPECT_EQ(pool.size(), 2);
}

TEST_F(ObjectPoolTest, DefaultConstructorCreatesUnbounded) {
    ObjectPool<TestObject> pool;  // Default constructor
    EXPECT_FALSE(pool.is_bounded());
    EXPECT_EQ(pool.size(), 64);  // Default initial size
}
