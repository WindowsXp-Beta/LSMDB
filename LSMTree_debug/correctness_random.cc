#include <iostream>
#include <cstdint>
#include <string>
#include <ctime>
#include <cstdlib>
#include "test.h"
int testArray[32768];
#define TIMEx
class CorrectnessTest : public Test {
private:
	const uint64_t SIMPLE_TEST_MAX = 5000;
	const uint64_t LARGE_TEST_MAX = 1024 * 32;

	void regular_test(uint64_t max)
	{
		uint64_t i;

		// Test a single key
		EXPECT(not_found, store.get(1));
		store.put(1, "SE");
		EXPECT("SE", store.get(1));
		EXPECT(true, store.del(1));
		EXPECT(not_found, store.get(1));
		EXPECT(false, store.del(1));

		phase();
		std::cout << "generate rand test sequence " << std::endl;
		srand((unsigned int)time(nullptr));
		for(int j = 0; j < max; j++) {
		    testArray[j] = rand()%30000;
		}

		// Test multiple key-value pairs
        clock_t startTime, endTime;
#ifdef TIME
        startTime = std::clock();
#endif
        for (i = 0; i < max; ++i) {
            store.put(testArray[i], std::string(testArray[i] + 1, 's'));
            EXPECT(std::string(testArray[i] + 1, 's'), store.get(testArray[i]));
        }
#ifdef TIME
        endTime = std::clock();
        std::cout << "Test put input number " << SIMPLE_TEST_MAX << " and time is " << (double)(endTime - startTime) / CLOCKS_PER_SEC << std::endl;
#endif
        phase();

		// Test after all insertions
//        std::cout << "Now the memtable size is " << store.getSize() << std::endl;
//        std::cout << "generate rand test sequence " << std::endl;
//        srand((unsigned int)time(nullptr));
//        for(int j = 0; j < max; j++) {
//            testArray[j] = rand()%30000;
//        }
#ifdef TIME
        startTime = std::clock();
#endif
		for (i = 0; i < max; ++i)
        {
		    EXPECT(std::string(testArray[i] + 1, 's'), store.get(testArray[i]));
//            store.get(testArray[i]);
        }
#ifdef TIME
        endTime = std::clock();
        std::cout << "Test get input number " << SIMPLE_TEST_MAX << " and time is " << (double)(endTime - startTime) / CLOCKS_PER_SEC << std::endl;
#endif
        phase();

		// Test deletions
//        std::cout << "generate rand test sequence " << std::endl;
//        srand((unsigned int)time(nullptr));
//        for(int j = 0; j < max; j++) {
//            testArray[j] = rand()%30000;
//        }
#ifdef TIME
        startTime = std::clock();
#endif
		for (i = 0; i < max; i+=1)
			EXPECT(true, store.del(testArray[i]));
//		    store.del(testArray[i]);
#ifdef TIME
        endTime = std::clock();
        std::cout << "Test delete input number " << SIMPLE_TEST_MAX << " and time is " << (double)(endTime - startTime) / CLOCKS_PER_SEC << std::endl;
#endif

//		for (i = 0; i < max; ++i)
//			EXPECT((i & 1) ? std::string(i+1, 's') : not_found,
//			       store.get(i));

//		for (i = 1; i < max; ++i)
//			EXPECT(i & 1, store.del(i));

		phase();

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

//		std::cout << "[Large Test]" << std::endl;
//		regular_test(LARGE_TEST_MAX);
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

	CorrectnessTest test("../../../../../../../../Volumes/其它/data", 1);
//    CorrectnessTest test("../data", 1);
	test.start_test();

	return 0;
}
