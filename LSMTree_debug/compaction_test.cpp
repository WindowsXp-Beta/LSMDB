//
// Created by 魏新鹏 on 2021/6/7.
//

//
// Created by 魏新鹏 on 2021/6/7.
//

#include <iostream>
#include <cstdint>
#include <string>
#include <ctime>
#include <cstdlib>
#include "test.h"
int testArray[400000];
#define TIME
class CorrectnessTest : public Test {
private:
    const uint64_t SIMPLE_TEST_MAX = 1000;
    const uint64_t LARGE_TEST_MAX = 1024 * 64;

    void regular_test(uint64_t max)
    {
        uint64_t i;

        // Test a single key
//        EXPECT(not_found, store.get(1));
//        store.put(1, "SE");
//        EXPECT("SE", store.get(1));
//        EXPECT(true, store.del(1));
//        EXPECT(not_found, store.get(1));
//        EXPECT(false, store.del(1));
//
//        phase();
//        std::cout << "generate rand test sequence " << std::endl;
//        srand((unsigned int)time(nullptr));
//        for(int j = 0; j < 400000; j++) {
//            testArray[j] = rand()%50000;
//        }
        // Test multiple key-value pairs
        clock_t startTime, endTime;
        for(int j = 0; j < 400000;) {
            int target = j + 100;
            startTime = std::clock();
            for(;j < target; j++) {
                store.put(j, std::string(500, 's'));
            }
            endTime = std::clock();
            std::cout << (double)(endTime - startTime) / CLOCKS_PER_SEC << std::endl;
        }
        report();
    }

public:
    CorrectnessTest(const std::string &dir, bool v=true) : Test(dir, v)
    {
    }

    void start_test(void *args = NULL) override
    {
        std::cout << "KVStore Correctness Test" << std::endl;
        store.reset();
        std::cout << "[Simple Test]" << std::endl;
        regular_test(SIMPLE_TEST_MAX);

//        std::cout << "[Large Test]" << std::endl;
//        regular_test(LARGE_TEST_MAX);
    }
};

int main(int argc, char *argv[])
{
    bool verbose = (argc == 2 && std::string(argv[1]) == "-v");

    std::cout << "Usage: " << argv[0] << " [-v]" << std::endl;
    std::cout << "  -v: print extra info for failed tests [currently ";
    std::cout << (verbose ? "ON" : "OFF")<< "]" << std::endl;
    std::cout << std::endl;
    std::cout.flush();

    CorrectnessTest test("../data", verbose);
//    CorrectnessTest test("../../../../../../../../Volumes/其它/data", 1);
    test.start_test();

    return 0;
}
