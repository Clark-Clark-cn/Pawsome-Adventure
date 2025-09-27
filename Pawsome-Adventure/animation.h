#pragma once
#include "util.h"
#include "timer.h"
#include "atlas.h"
#include "vector2.h"

#include <vector>
#include <functional>

class Animation
{
public:
    Animation()
    {
        timer.setOneShot(false);
        timer.setOnTimeout([&]()
                           {
            idxFrame++;
            if(idxFrame>=frameList.size())
            {
                idxFrame=isLoop?0:frameList.size()-1;
                if (!isLoop && onFinished)
                    onFinished();
            } });
    }

    ~Animation() = default;

    void reset()
    {
        timer.restart();
        idxFrame = 0;
    }

    void setPosition(const Vector2 &position)
    {
        this->position = position;
    }

    void setLoop(bool isLoop)
    {
        this->isLoop = isLoop;
    }

    void setInterval(float interval)
    {
        timer.setWaitTime(interval);
    }
    void setOnFinished(std::function<void()> onFinished)
    {
        this->onFinished = onFinished;
    }

    void addFrame(IMAGE *image, int num_h)
    {
        int width = image->getwidth();
        int height = image->getheight();
        int widthFrame = width / num_h;

        for (int i = 0; i < num_h; i++)
        {
            Rect rectSrc;
            rectSrc.x = i * widthFrame, rectSrc.y = 0;
            rectSrc.w = widthFrame, rectSrc.h = height;

            frameList.emplace_back(image, rectSrc);
        }
    }

    void addFrame(Atlas *atlas)
    {
        for (int i = 0; i < atlas->getSize(); i++)
        {
            IMAGE *image = atlas->getImage(i);
            int width = image->getwidth();
            int height = image->getheight();
            Rect rectSrc;
            rectSrc.x = 0, rectSrc.y = 0;
            rectSrc.w = width, rectSrc.h = height;
            frameList.emplace_back(image, rectSrc);
        }
    }

    void onUpdate(float delta)
    {
        timer.onUpdate(delta);
    }

    void onRender(const Camera &camera)
    {
        const Frame &frame = frameList[idxFrame];

        Rect rectDst;
        rectDst.x = (int)position.x - frame.rectSrc.w / 2;
        rectDst.y = (int)position.y - frame.rectSrc.h / 2;
        rectDst.w = frame.rectSrc.w;
        rectDst.h = frame.rectSrc.h;

        putimageEx(camera, frame.image, &rectDst, &frame.rectSrc);
    }

private:
    struct Frame
    {
        Rect rectSrc;
        IMAGE *image = nullptr;

        Frame() = default;
        Frame(IMAGE *image, const Rect &rectSrc) : image(image), rectSrc(rectSrc) {}

        ~Frame() = default;
    };

private:
    Timer timer;
    Vector2 position;
    bool isLoop = true;
    size_t idxFrame = 0;
    std::vector<Frame> frameList;
    std::function<void()> onFinished;
};