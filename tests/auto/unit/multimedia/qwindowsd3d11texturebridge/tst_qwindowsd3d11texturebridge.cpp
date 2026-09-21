// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>

#include <QtTest/QtTest>
#include <QtMultimedia/private/qwindowsd3d11texturebridge_p.h>
#include <QtGui/qcolor.h>
#include <QtCore/qobject.h>
#include <QtCore/private/qsystemerror_p.h>

using namespace std::chrono_literals;
using QWindowsD3D11TextureBridge = QtMultimediaPrivate::QWindowsD3D11TextureBridge;

// Helper macro to verify q23::expected<T, HRESULT>
#define QVERIFYCOMRESULT(comresult) \
    QVERIFY2(comresult,             \
             comresult ? "no error" \
                       : qPrintable(QSystemError::windowsComString((comresult).error())))

QSize getTextureSize(const ComPtr<ID3D11Texture2D> &tex)
{
    D3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);

    return QSize{ static_cast<int>(desc.Width), static_cast<int>(desc.Height) };
}

class tst_qwindowsd3d11texturebridge : public QObject
{
    Q_OBJECT

public slots:
    void init()
    {
        ComResult<QWindowsD3D11TestDeviceContext> src = createD3D11TestDeviceContext();
        QVERIFYCOMRESULT(src);
        m_src = *src;

        ComResult<QWindowsD3D11TestDeviceContext> dst = createD3D11TestDeviceContext();
        QVERIFYCOMRESULT(dst);
        m_dst = *dst;
    }

private slots:
    void copyToSharedTex_copiesCorrectPlaneBetweenDevices_whenCalledWithTextureArray()
    {
        constexpr QSize frameSize{ 128, 64 };

        const std::vector<QColor> testColors{ Qt::red, Qt::blue, Qt::green };

        const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                m_src.createTextureArray(frameSize, testColors);

        QVERIFYCOMRESULT(srcTex);

        QWindowsD3D11TextureBridge bridge{};

        for (UINT plane = 0; plane < static_cast<UINT>(testColors.size()); ++plane) {

            const bool copySuccess =
                    bridge.copyToSharedTex(m_src.device, m_src.context, *srcTex, frameSize, plane);
            QVERIFY(copySuccess);

            const ComPtr<ID3D11Texture2D> copy =
                    bridge.copyFromSharedTex(m_dst.device, m_dst.context);

            const ComResult<QColor> actualColor = m_dst.getFirstPixelColor(copy);
            QVERIFYCOMRESULT(actualColor);

            QCOMPARE_EQ(getTextureSize(copy), frameSize);
            QCOMPARE_EQ(*actualColor, testColors[plane]);
        }
    }

    void copyToSharedTex_copiesBetweenDevices_whenWritingAndReadingMultipleTimes()
    {
        constexpr QSize frameSize{ 128, 64 };
        const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                m_src.createTextureArray(frameSize, { Qt::yellow });

        QVERIFYCOMRESULT(srcTex);

        QWindowsD3D11TextureBridge bridge{};

        for (UINT iteration = 0; iteration < 3; ++iteration) {

            const bool copySuccess =
                    bridge.copyToSharedTex(m_src.device, m_src.context, *srcTex, frameSize);
            QVERIFY(copySuccess);

            const ComPtr<ID3D11Texture2D> copy =
                    bridge.copyFromSharedTex(m_dst.device, m_dst.context);

            const ComResult<QColor> actualColor = m_dst.getFirstPixelColor(copy);
            QVERIFYCOMRESULT(actualColor);

            QCOMPARE_EQ(getTextureSize(copy), frameSize);
            QCOMPARE_EQ(*actualColor, Qt::yellow);
        }
    }

    void copyToSharedTex_copiesBetweenDevices_whenDestinationDeviceChanges()
    {
        constexpr QSize frameSize{ 128, 64 };

        QWindowsD3D11TextureBridge bridge{};

        { // Arrange bridge such that a texture was already copied to a primary destination device
            const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                    m_src.createTextureArray(frameSize, { Qt::yellow });

            QVERIFYCOMRESULT(srcTex);

            bool copySuccess =
                    bridge.copyToSharedTex(m_src.device, m_src.context, *srcTex, frameSize);
            QVERIFY(copySuccess);

            ComPtr<ID3D11Texture2D> copy = bridge.copyFromSharedTex(m_dst.device, m_dst.context);
            QVERIFY(copy);

            copySuccess = bridge.copyToSharedTex(m_src.device, m_src.context, *srcTex, frameSize);
            QVERIFY(copySuccess);
        }

        const ComResult<QWindowsD3D11TestDeviceContext> secondDeviceContext =
                createD3D11TestDeviceContext();
        QVERIFYCOMRESULT(secondDeviceContext);

        // Act
        const ComPtr<ID3D11Texture2D> copy =
                bridge.copyFromSharedTex(secondDeviceContext->device, secondDeviceContext->context);

        QVERIFY(copy);

        const ComResult<QColor> actualColor = secondDeviceContext->getFirstPixelColor(copy);
        QVERIFYCOMRESULT(actualColor);

        // Assert
        QCOMPARE_EQ(getTextureSize(copy), frameSize);
        QCOMPARE_EQ(*actualColor, Qt::yellow);
    }

    void copyToSharedTex_copiesBetweenDevices_whenFrameSizeChanges()
    {
        const std::vector frameSizes{
            QSize{ 128, 64 }, QSize{ 129, 64 }, // grow
            QSize{ 128, 55 }, // shrink
            QSize{ 500, 600 } // grow
        };

        QWindowsD3D11TextureBridge bridge{};

        constexpr QSize padding{
            64, 32
        }; // Source texture may have padding (e.g. decoder surface alignment), so test with that.

        for (const QSize &frameSize : frameSizes) {
            const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                    m_src.createTextureArray(frameSize + padding, { Qt::magenta, Qt::darkBlue });

            QVERIFYCOMRESULT(srcTex);

            const bool copySuccess =
                    bridge.copyToSharedTex(m_src.device, m_src.context, *srcTex, frameSize, 1);
            QVERIFY(copySuccess);

            const ComPtr<ID3D11Texture2D> copy =
                    bridge.copyFromSharedTex(m_dst.device, m_dst.context);

            const ComResult<QColor> actualColor = m_dst.getFirstPixelColor(copy);
            QVERIFYCOMRESULT(actualColor);

            QCOMPARE_EQ(getTextureSize(copy), frameSize);
            QCOMPARE_EQ(*actualColor, Qt::darkBlue);
        }
    }

private:
    QWindowsD3D11TestDeviceContext m_src;
    QWindowsD3D11TestDeviceContext m_dst;
};

QTEST_GUILESS_MAIN(tst_qwindowsd3d11texturebridge)

#include "tst_qwindowsd3d11texturebridge.moc"
