#include "attendance.h"
#include <fstream>
#include <iostream>

#ifndef _ENABLE_GTEST

int main() {
    AttendanceSystem sys;
    sys.loadFromFile("attendance_weekday_500.txt");
    sys.compute();
    sys.printSummary(std::cout);
    return 0;
}

#else

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#endif
