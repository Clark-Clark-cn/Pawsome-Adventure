#pragma once
#include <vector>
#include <graphics.h>

class Atlas
{
public:
    Atlas() = default;
    ~Atlas() = default;

    void load(LPCTSTR pathTemplate, int num)
    {
        imgList.clear();
        imgList.resize(num);
        TCHAR pathFile[256];
        for (int i = 0; i < num; i++)
        {
            _stprintf_s(pathFile, pathTemplate, i + 1);
            loadimage(&imgList[i], pathFile);
            if(GetImageBuffer(&imgList[i])==nullptr)
            {
                MessageBox(nullptr, pathFile, L"加载图片失败", MB_OK | MB_ICONERROR);
			}
        }
    }

    void clear()
    {
        imgList.clear();
    }

    int getSize() const
    {
        return (int)imgList.size();
    }

    IMAGE *getImage(int idx)
    {
        if (idx < 0 || idx >= imgList.size())
            return nullptr;

        return &imgList[idx];
    }

    void addImage(const IMAGE &img)
    {
        imgList.push_back(img);
    }

private:
    std::vector<IMAGE> imgList;
};
