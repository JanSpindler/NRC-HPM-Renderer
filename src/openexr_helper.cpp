/*#include <engine/util/openexr_helper.hpp>

#include <cstdio>
#include <cstdlib>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfRgbaFile.h>
#include <half.h>
#include <cassert>

namespace en
{
    bool ReadEXR(const char* name, float*& rgba, int& xRes, int& yRes, bool& hasAlpha)
    {
        try {
            Imf::InputFile file(name);
            Imath::Box2i dw = file.header().dataWindow();
            xRes = dw.max.x - dw.min.x + 1;
            yRes = dw.max.y - dw.min.y + 1;

            half* hrgba = new half[4 * xRes * yRes];

            // for now...
            hasAlpha = true;
            int nChannels = 4;

            hrgba -= 4 * (dw.min.x + dw.min.y * xRes);
            Imf::FrameBuffer frameBuffer;
            frameBuffer.insert("R", Imf::Slice(Imf::HALF, (char*)hrgba,
                4 * sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
            frameBuffer.insert("G", Imf::Slice(Imf::HALF, (char*)hrgba + sizeof(half),
                4 * sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
            frameBuffer.insert("B", Imf::Slice(Imf::HALF, (char*)hrgba + 2 * sizeof(half),
                4 * sizeof(half), xRes * 4 * sizeof(half), 1, 1, 0.0));
            frameBuffer.insert("A", Imf::Slice(Imf::HALF, (char*)hrgba + 3 * sizeof(half),
                4 * sizeof(half), xRes * 4 * sizeof(half), 1, 1, 1.0));

            file.setFrameBuffer(frameBuffer);
            file.readPixels(dw.min.y, dw.max.y);

            hrgba += 4 * (dw.min.x + dw.min.y * xRes);
            rgba = new float[nChannels * xRes * yRes];
            for (int i = 0; i < nChannels * xRes * yRes; ++i)
                rgba[i] = hrgba[i];
            delete[] hrgba;
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Unable to read image file \"%s\": %s", name, e.what());
            return NULL;
        }

        return rgba;
    }

    void WriteEXR(const char* name, float* pixels, int xRes, int yRes) {
        Imf::Rgba* hrgba = new Imf::Rgba[xRes * yRes];
        for (int i = 0; i < xRes * yRes; ++i)
            hrgba[i] = Imf::Rgba(pixels[4 * i], pixels[4 * i + 1], pixels[4 * i + 2], pixels[4 * i + 3]);

        Imath::Box2i displayWindow(Imath::V2i(0, 0), Imath::V2i(xRes - 1, yRes - 1));
        Imath::Box2i dataWindow = displayWindow;

        Imf::RgbaOutputFile file(name, displayWindow, dataWindow, Imf::WRITE_RGBA);
        file.setFrameBuffer(hrgba, 1, xRes);
        try {
            file.writePixels(yRes);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "Unable to write image file \"%s\": %s", name,
                e.what());
        }

        delete[] hrgba;
    }
}
*/