#pragma once
#include "camera.h"

#include <graphics.h>

#pragma comment(lib, "WINMM.lib")
#pragma comment(lib, "MSIMG32.lib")

struct Rect
{
    int x, y;
    int w, h;
};

inline void putimageEx(const Camera &camera, IMAGE *img, const Rect *rectDst, const Rect *rectSrc = nullptr)
{
    static BLENDFUNCTION blendFunc = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    const Vector2 &posCamera = camera.getPosition();
    AlphaBlend(GetImageHDC(GetWorkingImage()), (int)(rectDst->x - posCamera.x), (int)(rectDst->y - posCamera.y), rectDst->w, rectDst->h,
               GetImageHDC(img), rectSrc ? rectSrc->x : 0, rectSrc ? rectSrc->y : 0, rectSrc ? rectSrc->w : img->getwidth(), rectSrc ? rectSrc->h : img->getheight(), blendFunc);
}

inline void loadAudio(LPCTSTR path, LPCTSTR id, bool isLoop = false)
{
    static TCHAR strCmd[512];
    _stprintf_s(strCmd, L"open %s alias %s", path, id);
    mciSendString(strCmd, NULL, 0, NULL);
}

inline void playAudio(LPCTSTR id, bool isLoop = false)
{
    static TCHAR strCmd[512];
    _stprintf_s(strCmd, L"play %s %s from 0", id, isLoop ? L"repeat" : L"");
    mciSendString(strCmd, NULL, 0, NULL);
}

inline void stopAudio(LPCTSTR id)
{
    static TCHAR strCmd[512];
    _stprintf_s(strCmd, L"stop %s", id);
    mciSendString(strCmd, NULL, 0, NULL);
}