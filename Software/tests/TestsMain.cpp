#include <iostream>
#include <QApplication>
#include "gtest/gtest.h"

using namespace std;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    cout << "Run LightPack tests:" << endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
