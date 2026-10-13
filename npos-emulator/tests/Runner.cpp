/**
 * @file Runner.cpp
 * @author Adrian Szczepanski
 * @date 2026-09-15
 */

#include <CppUTest/CommandLineTestRunner.h>

int main(int argc, char* argv[])
{
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
