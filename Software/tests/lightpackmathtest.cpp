#include "gtest/gtest.h"
#include "PrismatikMath.hpp"

TEST(LightpackMathTest, ChromaHSVTest)
{
    EXPECT_EQ( PrismatikMath::getValueHSV(qRgb(215,122,0)), 215) << "getValueHSV() is incorrect";

    const QRgb testRgb = qRgb(200, 100, 0);
    EXPECT_EQ( PrismatikMath::withChromaHSV(testRgb, 100), qRgb(200, 150, 100) ) << "getChromaHSV() is incorrect";
    EXPECT_EQ( PrismatikMath::withChromaHSV(testRgb, 250), qRgb(200, 75, 0)) << "getChromaHSV() is incorrect";
    EXPECT_EQ( PrismatikMath::withChromaHSV(testRgb, PrismatikMath::getChromaHSV(testRgb)), testRgb) << "getChromaHSV() is incorrect";
}
