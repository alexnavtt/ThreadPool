#include "threadpool/threadpool.hpp"
#include <iostream>

using namespace std::chrono_literals;
volatile int j;
volatile bool done = false;
volatile int test_idx = 0;

void test1(){
    std::this_thread::sleep_for(3s);
    printf("test1\n");
}

void test2(){
    while (not done){
        std::this_thread::sleep_for(1s);
    }
    printf("test2\n");
}

void test3_fcn(int i){
    std::this_thread::sleep_for(1s);
    printf("test%d\n", i);
}

struct test6_struct
{
    void operator() (){
        std::this_thread::sleep_for(1s);
        printf("test6\n");
    }
};

void (*test7)();

void test7_fcn(){
    printf("test7\n");
}

void test8(){
    int idx = test_idx++;
    printf("test8 - 1 : %d\n", idx);
    for (int i = 0; i < 1e9; i++){
        j = i;
    }
    printf("test8 - 2 : %d\n", idx);
}


int main(){
    ThreadPool pool;

    // Test with an object created by std::bind
    auto test3 = std::bind(&test3_fcn, 3);

    // Test with a lambda
    auto test4 = [](){
    std::this_thread::sleep_for(1s);
        printf("test4\n");
    };

    // Test with a capture lambda
    int i = 5;
    auto test5 = [i](){
    std::this_thread::sleep_for(1s);
        printf("test%d\n", i);
    };

    // Test with a functor
    auto test6 = test6_struct();

    // Test with a function pointer
    test7 = &test7_fcn;

    // Make sure that join is valid even with no tasks
    pool.joinAll();

    auto idx1 = pool.queueTask(test1);
    auto idx2 = pool.queueTask(test2);
    auto idx3 = pool.queueTask(test3);

    // Test waiting for a single task to finish
    printf("Waiting for task one to finish\n");
    pool.joinOne(idx1);
    printf("Task one is done\n");

    // Test waiting for all tasks to finish
    done = true;
    pool.joinAll();
    printf("--- Tasks 1, 2, and 3 done ---\n");
    printf("--- Starting 4, 5, 6, and 7 ---\n");

    pool.queueTask(test4);
    pool.queueTask(test5);
    pool.queueTask(test6);
    pool.queueTask(test7);

    // Test that the destructor only allows the currently active task to finish
    {
        ThreadPool new_pool(2);
        auto test_idx = new_pool.queueTask(test8);
        new_pool.queueTask(test8);
        new_pool.queueTask(test8);
        new_pool.queueTask(test8);
        std::this_thread::sleep_for(10ms);
    }


    printf("Waiting for all the threads to finish\n");
    pool.joinAll();

    return 0;
}