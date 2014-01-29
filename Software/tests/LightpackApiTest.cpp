/*
 * LightpackApiTest.cpp
 *
 *  Created on: 07.09.2011
 *     Project: Lightpack
 *
 *  Copyright (c) 2011 Mike Shatohin, mikeshatohin [at] gmail.com
 *
 *  Lightpack a USB content-driving ambient lighting system
 *
 *  Lightpack is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Lightpack is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QString>
#include <QtWidgets/QApplication>
#include <QScopedPointer>
#include <QtNetwork>

#include "debug.h"
#include "ApiServer.hpp"
#include "LightpackPluginInterface.hpp"
#include "Settings.hpp"
#include "enums.hpp"
#include "SettingsWindowMockup.hpp"

#include <stdlib.h>
#include <iostream>
#include "gtest/gtest.h"

using namespace std;
using namespace SettingsScope;

#define VERSION_API_TESTS   "1.4"

unsigned g_debugLevel = Debug::LowLevel;

// Print helpers
// It's important that the << operator is defined in the SAME
// namespace that defines Bar.  C++'s look-up rules rely on that.
::std::ostream& operator<<(::std::ostream& os, const QString& str) {
  return os << str.constData();
}

::std::ostream& operator<<(::std::ostream& os, const QByteArray& bytes) {
  return os << bytes.constData();
}

void PrintTo(const QRgb& rgb, ::std::ostream* os) {
    *os << (int)rgb << "=" << qRed(rgb) << qGreen(rgb) << qBlue(rgb);
}

namespace
{
template <typename T, size_t N>
char ( &_ArraySizeHelper( T (&array)[N] ))[N];
#define ARRAY_SIZE( array ) (sizeof( _ArraySizeHelper( array ) ))

inline bool checkApiVersion(const QString& apiVersion)
{
    static const QString versionTests = "Lightpack API v" VERSION_API_TESTS " (type \"help\" for more info)";
    return (apiVersion.trimmed() == versionTests);
}

// get
// getstatus - on off
// getstatusapi - busy idle
// getprofiles - list name profiles
// getprofile - current name profile

// commands
// lock - begin work with api (disable capture,backlight)
// unlock - end work with api (enable capture,backlight)
// setcolor:1-r,g,b;5-r,g,b;   numbering starts with 1
// setgamma:2.00 - set gamma for setcolor
// setsmooth:100 - set smooth in device
// setprofile:<name> - set profile
// setstatus:on - set status (on, off)
class LightpackApiTest : public ::testing::Test
{
private:
    virtual void SetUp();
    virtual void TearDown();

protected:
    QByteArray readResult(QTcpSocket * socket);
    void writeCommand(QTcpSocket * socket, const char * cmd);
    bool writeCommandWithCheck(QTcpSocket * socket, const QByteArray & command, const QByteArray & result);

    QString getProfilesResultString();
    void processEventsFromLittle();

    bool checkVersion(QTcpSocket * socket);
    bool lock(QTcpSocket * socket);
    bool unlock(QTcpSocket * socket);
    bool setGamma(QTcpSocket * socket, QString gammaStr);

protected:
    QScopedPointer<ApiServer> m_apiServer;
    QScopedPointer<QThread> m_apiServerThread;
    QScopedPointer<LightpackPluginInterface> m_interfaceApi;
    QScopedPointer<SettingsWindowMockup> m_little;

    QScopedPointer<QTcpSocket> m_socket;
    bool m_sockReadLineOk;
    QByteArray m_apiVersion;
};

void LightpackApiTest::SetUp()
{
    static bool kOneTimeInit = false;
    if (!kOneTimeInit) {
        // Register QMetaType for Qt::QueuedConnection
        qRegisterMetaType< QList<QRgb> >("QList<QRgb>");
        qRegisterMetaType<Backlight::Status>("Backlight::Status");
        Settings::Initialize(QDir::currentPath(), true);
        kOneTimeInit = true;
    }

    // Start Api Server in separate thread for access by QTcpSocket-s
    m_apiServer.reset(new ApiServer(3636));
    m_interfaceApi.reset(new LightpackPluginInterface());
    m_apiServer->setInterface(m_interfaceApi.data());
    m_apiServerThread.reset(new QThread());

    m_apiServer->moveToThread(m_apiServerThread.data());

    QObject::connect(m_apiServer.data(), SIGNAL(finished()), m_apiServerThread.data(), SLOT(quit()));
    QObject::connect(m_apiServer.data(), SIGNAL(finished()), m_apiServer.data(), SLOT(deleteLater()));
    QObject::connect(m_apiServerThread.data(), SIGNAL(finished()), m_apiServerThread.data(), SLOT(deleteLater()));

    m_little.reset(new SettingsWindowMockup());

    QObject::connect(m_little.data(), SIGNAL(enableApiServer(bool)), m_apiServer.data(), SLOT(enableApiServer(bool)), Qt::DirectConnection);
    QObject::connect(m_little.data(), SIGNAL(updateApiKey(QString)), m_apiServer.data(), SLOT(updateApiKey(QString)), Qt::DirectConnection);
    QObject::connect(m_little.data(), SIGNAL(updateApiPort(int)), m_apiServer.data(), SLOT(updateApiPort(int)), Qt::DirectConnection);

    QObject::connect(m_interfaceApi.data(), SIGNAL(requestBacklightStatus()), m_little.data(), SLOT(requestBacklightStatus()), Qt::QueuedConnection);
    QObject::connect(m_little.data(), SIGNAL(resultBacklightStatus(Backlight::Status)), m_interfaceApi.data(), SLOT(resultBacklightStatus(Backlight::Status)));

    QObject::connect(m_interfaceApi.data(), SIGNAL(updateLedsColors(QList<QRgb>)), m_little.data(), SLOT(setLedColors(QList<QRgb>)), Qt::QueuedConnection);
    QObject::connect(m_interfaceApi.data(), SIGNAL(updateGamma(double)), m_little.data(), SLOT(setGamma(double)), Qt::QueuedConnection);
    QObject::connect(m_interfaceApi.data(), SIGNAL(updateBrightness(int)), m_little.data(), SLOT(setBrightness(int)), Qt::QueuedConnection);
    QObject::connect(m_interfaceApi.data(), SIGNAL(updateSmooth(int)), m_little.data(), SLOT(setSmooth(int)), Qt::QueuedConnection);
    QObject::connect(m_interfaceApi.data(), SIGNAL(updateProfile(QString)), m_little.data(), SLOT(setProfile(QString)), Qt::QueuedConnection);
    QObject::connect(m_interfaceApi.data(), SIGNAL(updateStatus(Backlight::Status)), m_little.data(), SLOT(setStatus(Backlight::Status)), Qt::QueuedConnection);

    m_little->setApiKey("");
    m_apiServerThread->start();

    m_socket.reset(new QTcpSocket());

    // Reconnect to host before each test case
    m_socket->connectToHost("127.0.0.1", 3636);

    // Wait 5 second for connected
    EXPECT_TRUE(m_socket->waitForConnected(5000));

    m_sockReadLineOk = false;
    checkVersion(m_socket.data());
}

void LightpackApiTest::TearDown()
{
    EXPECT_TRUE(m_socket.data() != NULL);

    m_socket->abort();
    m_socket.reset();

    emit m_apiServer->finished();
    m_apiServerThread->wait();
}

// Private help functions

QByteArray LightpackApiTest::readResult(QTcpSocket * socket)
{
    m_sockReadLineOk = socket->waitForReadyRead(1000) && socket->canReadLine();
    return socket->readLine();
}

void LightpackApiTest::writeCommand(QTcpSocket * socket, const char * cmd)
{
    socket->write(cmd);
    socket->write("\n");
}

bool LightpackApiTest::writeCommandWithCheck(QTcpSocket * socket, const QByteArray & command, const QByteArray & result)
{
    writeCommand(socket, command);
    QByteArray read = readResult(socket);

    return (m_sockReadLineOk && read == result);
}

QString LightpackApiTest::getProfilesResultString()
{
    QStringList profiles = Settings::findAllProfiles();

    QString cmdProfilesCheckResult = ApiServer::CmdResultProfiles;
    for (int i = 0; i < profiles.count(); i++)
        cmdProfilesCheckResult += profiles[i].toUtf8() + ";";

    return cmdProfilesCheckResult;
}

void LightpackApiTest::processEventsFromLittle()
{
    QTime time;
    time.restart();
    m_little->m_isDone = false;

    while (m_little->m_isDone == false && time.elapsed() < ApiServer::SignalWaitTimeoutMs)
    {
        QApplication::processEvents(QEventLoop::WaitForMoreEvents, ApiServer::SignalWaitTimeoutMs);
    }
}

bool LightpackApiTest::checkVersion(QTcpSocket * socket)
{
    // Check the version of the API and API Tests on match

    m_apiVersion = readResult(socket);

    return (m_sockReadLineOk && checkApiVersion(QString(m_apiVersion)));
}

bool LightpackApiTest::lock(QTcpSocket * socket)
{
    return writeCommandWithCheck(socket, ApiServer::CmdLock, ApiServer::CmdResultLock_Success);
}

bool LightpackApiTest::unlock(QTcpSocket * socket)
{
    // Must be locked before unlock, else unlock() return false,
    // because result will be ApiServer::CmdResultUnlock_NotLocked
    return writeCommandWithCheck(socket, ApiServer::CmdUnlock, ApiServer::CmdResultUnlock_Success);
}
}

//
// Test cases
//
TEST_F(LightpackApiTest, testApiVersion)
{
    // Check version of API and version of API Tests on match
    QString apiVersion(m_apiVersion);
    EXPECT_TRUE(checkApiVersion(apiVersion)) << apiVersion.constData();
}

TEST_F(LightpackApiTest, testGetStatus)
{       
    // Test Backlight Off state:
    m_little->setStatus(Backlight::StatusOff);
    writeCommand(m_socket.data(), ApiServer::CmdGetStatus);

    processEventsFromLittle();

    QByteArray result = readResult(m_socket.data());
    EXPECT_EQ(ApiServer::CmdResultStatus_Off, result);

    // Test Backlight On state:
    m_little->setStatus(Backlight::StatusOn);
    writeCommand(m_socket.data(), ApiServer::CmdGetStatus);

    processEventsFromLittle();
    processEventsFromLittle();

    result = readResult(m_socket.data());
    EXPECT_EQ(ApiServer::CmdResultStatus_On, result);

    // Test Backlight DeviceError state:
    m_little->setStatus(Backlight::StatusDeviceError);
    writeCommand(m_socket.data(), ApiServer::CmdGetStatus);

    processEventsFromLittle();
    processEventsFromLittle();

    result = readResult(m_socket.data());
    EXPECT_EQ(ApiServer::CmdResultStatus_DeviceError, result);
}

TEST_F(LightpackApiTest, testGetStatusAPI)
{
    // Test idle state:
    writeCommand(m_socket.data(), ApiServer::CmdGetStatusAPI);

    QByteArray result = readResult(m_socket.data());
    EXPECT_EQ(ApiServer::CmdResultStatusAPI_Idle, result) << result;

    // Test lock and busy state:
    QTcpSocket sockLock;
    sockLock.connectToHost("127.0.0.1", 3636);
    EXPECT_TRUE(checkVersion(&sockLock));

    writeCommand(&sockLock, ApiServer::CmdLock);
    EXPECT_EQ(ApiServer::CmdResultLock_Success, readResult(&sockLock));
    EXPECT_TRUE(m_sockReadLineOk);

    writeCommand(m_socket.data(), ApiServer::CmdGetStatusAPI);
    EXPECT_EQ(readResult(m_socket.data()), ApiServer::CmdResultStatusAPI_Busy);
    EXPECT_TRUE(m_sockReadLineOk);

    // Test unlock and return to idle state
    writeCommand(&sockLock, ApiServer::CmdUnlock);
    EXPECT_EQ(readResult(&sockLock), ApiServer::CmdResultUnlock_Success);
    EXPECT_TRUE(m_sockReadLineOk);

    writeCommand(m_socket.data(), ApiServer::CmdGetStatusAPI);
    EXPECT_EQ(readResult(m_socket.data()), ApiServer::CmdResultStatusAPI_Idle);
    EXPECT_TRUE(m_sockReadLineOk);
}

TEST_F(LightpackApiTest, testGetProfiles)
{
    QString cmdProfilesCheckResult = getProfilesResultString();

    // Test GetProfiles command:

    writeCommand(m_socket.data(), ApiServer::CmdGetProfiles);

    QByteArray result = readResult(m_socket.data()).trimmed();
    EXPECT_TRUE(m_sockReadLineOk);
    EXPECT_EQ(QString(result), cmdProfilesCheckResult);

    // Test UTF-8 in profile name

    QString utf8Check = QObject::trUtf8("\u041F\u0440\u043E\u0432\u0435\u0440\u043A\u0430"); // Russian word "Proverka"

    Settings::loadOrCreateProfile("ApiTestProfile");
    Settings::loadOrCreateProfile("ApiTestProfile-UTF-8-" + utf8Check);

    QString cmdProfilesCheckResultWithUtf8 = getProfilesResultString();

    writeCommand(m_socket.data(), ApiServer::CmdGetProfiles);

    result = readResult(m_socket.data()).trimmed();
    EXPECT_TRUE(m_sockReadLineOk);

    EXPECT_EQ(QString(result), cmdProfilesCheckResultWithUtf8);
}

TEST_F(LightpackApiTest, testGetProfile)
{
    QString cmdProfileCheckResult = ApiServer::CmdResultProfile +
            Settings::getCurrentProfileName().toUtf8();

    // Test GetProfile command:

    writeCommand(m_socket.data(), ApiServer::CmdGetProfile);

    QString result = readResult(m_socket.data()).trimmed();
    EXPECT_TRUE(m_sockReadLineOk);
    EXPECT_EQ(result, cmdProfileCheckResult);
}

TEST_F(LightpackApiTest, testLock)
{
    QTcpSocket sockTryLock;
    sockTryLock.connectToHost("127.0.0.1", 3636);
    EXPECT_TRUE(checkVersion(&sockTryLock));

    // Test lock success
    EXPECT_TRUE(lock(m_socket.data()));

    // Test lock busy
    writeCommand(&sockTryLock, ApiServer::CmdLock);
    QString result = readResult(&sockTryLock);
    EXPECT_TRUE(m_sockReadLineOk);
    EXPECT_EQ(result, ApiServer::CmdResultLock_Busy);

    // Test lock success after unlock
    EXPECT_TRUE(unlock(m_socket.data()));

    writeCommand(&sockTryLock, ApiServer::CmdLock);
    result = readResult(&sockTryLock);
    EXPECT_TRUE(m_sockReadLineOk);
    EXPECT_EQ(result, ApiServer::CmdResultLock_Success);
}

TEST_F(LightpackApiTest, testUnlock)
{
    // Test Unlock command:
    writeCommand(m_socket.data(), ApiServer::CmdUnlock);
    QString result = readResult(m_socket.data());
    EXPECT_TRUE(m_sockReadLineOk);
    EXPECT_EQ(result, ApiServer::CmdResultUnlock_NotLocked);
}

TEST_F(LightpackApiTest, testSetColor)
{    
    QTcpSocket sockLock;
    sockLock.connectToHost("127.0.0.1", 3636);
    EXPECT_TRUE(checkVersion(&sockLock));

    // Test lock state in SetColor command:
    QByteArray setColorCmd = ApiServer::CmdSetColor;

    setColorCmd += "1-23,2,65;";
    int led = 1;
    QRgb rgb = qRgba(23, 2, 65, 0xff);

    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setColorCmd, ApiServer::CmdSetResult_NotLocked));

    // Test busy state in SetColor command:
    EXPECT_TRUE(lock(&sockLock));

    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setColorCmd, ApiServer::CmdSetResult_Busy));

    // Test in SetColor change to unlock state:
    EXPECT_TRUE(unlock(&sockLock));

    EXPECT_TRUE(lock(m_socket.data()));

    // Test SetColor on valid command
    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setColorCmd, ApiServer::CmdSetResult_Ok));

    processEventsFromLittle();

    EXPECT_EQ(m_little->m_colors[led-1], rgb);

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetColorValid)
{
    EXPECT_TRUE(lock(m_socket.data()));

    struct ColorData {
        const char* cmd;
        int led;
        QRgb color;
    } colorData [] = {
        { "1-1,1,1",        1,  qRgb(1, 1, 1) }, // setcolor without semicolon is valid!
        { "1-1,1,1",        1,  qRgb(1, 1, 1) },
        { "2-1,1,1",        2,  qRgb(1, 1, 1) },
        { "10-1,1,1",       10, qRgb(1, 1, 1) },
        { "1-192,1,1",      1,  qRgb(192, 1, 1) },
        { "1-1,185,1",      1,  qRgb(1, 185, 1) },
        { "1-14,15,19",     1,  qRgb(14, 15, 19) },
        { "1-255,255,255",  1,  qRgb(255, 255, 255) },
        { "10-255,255,255", 10, qRgb(255, 255, 255) },
        { "10-0,0,1",       10, qRgb(0, 0, 1) },
        { "10-1,0,0",       10, qRgb(1, 0, 0) }
    };

    for (int ii = 0; ii < ARRAY_SIZE(colorData); ++ii)
    {
        QByteArray setColorCmd = ApiServer::CmdSetColor;
        setColorCmd += colorData[ii].cmd;

        // Test SetColor different valid strings
        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setColorCmd, ApiServer::CmdSetResult_Ok));

        processEventsFromLittle();

        EXPECT_EQ(m_little->m_colors[colorData[ii].led-1], colorData[ii].color)
                << "index =" << ii << ", cmd = " << setColorCmd;
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetColorValid2)
{
    EXPECT_TRUE(lock(m_socket.data()));

    const char* colorStrings[] = {
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;5-5,5,5;6-6,6,6;7-7,7,7;8-8,8,8;9-9,9,9;10-10,10,10;",
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;5-5,5,5;6-6,6,6;7-7,7,7;8-8,8,8;9-9,9,9;",
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;5-5,5,5;6-6,6,6;7-7,7,7;8-8,8,8;",
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;5-5,5,5;6-6,6,6;7-7,7,7;",
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;5-5,5,5;6-6,6,6;",
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;5-5,5,5;",
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;",
        "1-1,1,1;2-2,2,2;3-3,3,3;",
        "1-1,1,1;2-2,2,2;",
        "1-1,1,1;"
    };

    for (int ii = 0; ii < ARRAY_SIZE(colorStrings); ++ii)
    {
        QByteArray setColorCmd = ApiServer::CmdSetColor;
        setColorCmd += colorStrings[ii];

        // Test SetColor different valid strings

        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setColorCmd, ApiServer::CmdSetResult_Ok))
                << "index =" << ii << ", cmd = " << setColorCmd;

        // Just process all pending events from m_apiServer
        processEventsFromLittle();
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetColorInvalid)
{
    EXPECT_TRUE(lock(m_socket.data()));

    const char* colorStrings[] = {
        "1--1,1,1;",
        "1-1,,1,1;",
        "1-1,1,,1;",
        "1-1.1.1",
        "1-1,1,1;2-",
        "11-1,1,1;",
        "0-1,1,1;",
        "1-1,-1,1;",
        "1-1,1111,1;",
        "1-1,1,256;",
        "!-1,1,1;",
        "-1,1,1;",
        "1-1,1;",
        "1-1,100000000000000000000000;",
        "1-1,1,1;2-4,5,,;",
        "1-1,1,1;2-2,2,2;3-3,3,3;4-4,4,4;5-5,5,5;;6-6,6,6;7-7,7,7;8-8,8,8;9-9,9,9;"
    };

    for (int ii = 0; ii < ARRAY_SIZE(colorStrings); ++ii)
    {
        QByteArray setColorCmd = ApiServer::CmdSetColor;
        setColorCmd += colorStrings[ii];

        // Test SetColor different valid strings
        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setColorCmd, ApiServer::CmdSetResult_Error))
                << "index =" << ii << ", cmd = " << setColorCmd;
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetGammaValid)
{
    EXPECT_TRUE(lock(m_socket.data()));

    struct GammaData {
        const char* str;
        double value;
    } gammaData [] = {
        { ".01", 0.01 },
        { "1.0", 1.0 },
        { "2.0", 2.0 },
        { "3", 3.0 },
        { "10.00", 10.0 }
    };

    for (int ii = 0; ii < ARRAY_SIZE(gammaData); ++ii)
    {
        QByteArray setGammaCmd = ApiServer::CmdSetGamma;
        setGammaCmd += gammaData[ii].str;

        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setGammaCmd, ApiServer::CmdSetResult_Ok))
                << "index =" << ii << ", cmd = " << setGammaCmd;

        processEventsFromLittle();

        EXPECT_DOUBLE_EQ(m_little->m_gamma, gammaData[ii].value);
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetGammaInvalid)
{
    EXPECT_TRUE(lock(m_socket.data()));

    const char* gammaStrings[] = {
        "0.00",
        "0.0001",
        "12.0",
        ":12.0",
        "2.0;",
        "1.2.0",
        "-1.2",
        "10.01",
        "4.56857",
        "4.5685787384739473827432423489237985739487593745987349857938475",
        "+100500",
        "Galaxy in Danger!",
        "reinterpret_cast<CodeMonkey*>(m_pSelf);",
        "BSOD are coming soon!"
    };

    for (int ii = 0; ii < ARRAY_SIZE(gammaStrings); ++ii)
    {
        QByteArray setGammaCmd = ApiServer::CmdSetGamma;
        setGammaCmd += gammaStrings[ii];

        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setGammaCmd, ApiServer::CmdSetResult_Error))
                << "index =" << ii << ", cmd = " << setGammaCmd;
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetBrightnessValid)
{
    EXPECT_TRUE(lock(m_socket.data()));

    struct BrightnessData {
        const char* str;
        int value;
    } brightnessData [] = {
        { "0", 0 },
        { "1", 1 },
        { "55", 55 },
        { "100", 100 }
    };

    for (int ii = 0; ii < ARRAY_SIZE(brightnessData); ++ii)
    {
        QByteArray setBrightnessCmd = ApiServer::CmdSetBrightness;
        setBrightnessCmd += brightnessData[ii].str;

        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setBrightnessCmd, ApiServer::CmdSetResult_Ok))
                << "index =" << ii << ", cmd = " << setBrightnessCmd;

        processEventsFromLittle();

        EXPECT_EQ(m_little->m_brightness, brightnessData[ii].value);
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetBrightnessInvalid)
{
    EXPECT_TRUE(lock(m_socket.data()));

    const char* brightnessStrings[] = {
        "0.0",
        "0.0001",
        "-12",
        ":12.0",
        "2.;",
        ".2.",
        "120",
        "100500"
    };

    for (int ii = 0; ii < ARRAY_SIZE(brightnessStrings); ++ii)
    {
        QByteArray setBrightnessCmd = ApiServer::CmdSetBrightness;
        setBrightnessCmd += brightnessStrings[ii];

        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setBrightnessCmd, ApiServer::CmdSetResult_Error))
                << "index =" << ii << ", cmd = " << setBrightnessCmd;
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetSmoothValid)
{
    EXPECT_TRUE(lock(m_socket.data()));

    struct SmoothData {
        const char* str;
        int value;
    } smoothData [] = {
        { "0", 0 },
        { "255", 255 },
        { "43", 43 }
    };

    for (int ii = 0; ii < ARRAY_SIZE(smoothData); ++ii)
    {
        QByteArray setSmoothCmd = ApiServer::CmdSetSmooth;
        setSmoothCmd += smoothData[ii].str;

        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setSmoothCmd, ApiServer::CmdSetResult_Ok))
                << "index =" << ii << ", cmd = " << setSmoothCmd;

        processEventsFromLittle();

        EXPECT_EQ(m_little->m_smooth, smoothData[ii].value);
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetSmoothInvalid)
{
    const char* smoothStrings[] = {
        "-1",
        "256",
        "10.0",
        "1.0",
        ".0",
        "..0",
        "1."
    };
    EXPECT_TRUE(lock(m_socket.data()));

    for (int ii = 0; ii < ARRAY_SIZE(smoothStrings); ++ii)
    {
        QByteArray setSmoothCmd = ApiServer::CmdSetSmooth;
        setSmoothCmd += smoothStrings[ii];

        EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setSmoothCmd, ApiServer::CmdSetResult_Error))
                << "index =" << ii << ", cmd = " << setSmoothCmd;
    }

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetProfile)
{
    EXPECT_TRUE(lock(m_socket.data()));

    QStringList profiles = Settings::findAllProfiles();
    EXPECT_GT(profiles.count(), 0);

    QByteArray setProfileCmd = ApiServer::CmdSetProfile;
    setProfileCmd += profiles.at(0);

    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setProfileCmd, ApiServer::CmdSetResult_Ok))
            << "cmd = " << setProfileCmd;

    processEventsFromLittle();

    EXPECT_EQ(profiles.at(0), m_little->m_profile);

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testSetStatus)
{
    EXPECT_TRUE(lock(m_socket.data()));

    QByteArray setStatusCmd = ApiServer::CmdSetStatus;
    setStatusCmd += ApiServer::CmdSetStatus_On;

    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setStatusCmd, ApiServer::CmdSetResult_Ok))
            << "cmd = " << setStatusCmd;

    processEventsFromLittle();

    EXPECT_EQ(Backlight::StatusOn, m_little->m_status);

    setStatusCmd = ApiServer::CmdSetStatus;
    setStatusCmd += ApiServer::CmdSetStatus_Off;

    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), setStatusCmd, ApiServer::CmdSetResult_Ok))
            << "cmd = " << setStatusCmd;

    processEventsFromLittle();

    EXPECT_EQ(Backlight::StatusOff, m_little->m_status);

    EXPECT_TRUE(unlock(m_socket.data()));
}

TEST_F(LightpackApiTest, testApiAuthorization)
{
    QString testKey = "test-key";

    m_little->setApiKey(testKey);

    QByteArray cmdApiKey = ApiServer::CmdApiKey;
    cmdApiKey += testKey;

    // Authorization
    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), cmdApiKey, ApiServer::CmdApiKeyResult_Ok))
            << "cmd = " << cmdApiKey;

    // Try lock device after SUCCESS authorization
    EXPECT_TRUE(lock(m_socket.data()));

    // Authorization FAIL check
    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), cmdApiKey + "invalid", ApiServer::CmdApiKeyResult_Fail))
            << "cmd = " << cmdApiKey;

    // Try lock device after FAIL authorization
    EXPECT_TRUE(writeCommandWithCheck(m_socket.data(), ApiServer::CmdLock, ApiServer::CmdApiCheck_AuthRequired))
            << "cmd = " << ApiServer::CmdLock;
}
