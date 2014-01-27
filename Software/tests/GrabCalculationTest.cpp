#include "gtest/gtest.h"
#include "calculations.hpp"

TEST(GrabCalculationTest, calculateAvgColorTest)
{
    QRgb result;
    unsigned char buf[16];
    memset(buf, 0xfa, 16);
    EXPECT_EQ(
        Grab::Calculations::calculateAvgColor(&result, buf, BufferFormatArgb, 16, QRect(0,0,4,1)),
        0) << "Failure. calculateAvgColor returned wrong errorcode";
    EXPECT_EQ(result, qRgb(0xfa,0xfa,0xfa));
}
