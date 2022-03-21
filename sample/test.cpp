#include <iostream>
using namespace std;

#define CONFIG_COVERAGE_GCOV
#include "../coverage/gcov.h"

class Singleton
{
private:
    Singleton();

    static class Inst
    {
    public:
        int a;

        Inst()
        {
            cout << "init Inst" << endl;
            a = 11;
        }
    } _inst;

public:
    static void test_method()
    {
        cout << "a = " << _inst.a << endl;
    }
};

Singleton::Inst Singleton::_inst;

int main()
{
    cout << "main" << endl;

    Singleton::test_method();

    gcov_coverage_dump();

    return 0;
}