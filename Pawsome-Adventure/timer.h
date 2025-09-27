#pragma once
#include <functional>

class Timer
{
public:
    Timer() = default;
    ~Timer() = default;

    void restart()
    {
        passTime = 0;
        shotted = false;
    }

    void setWaitTime(float val)
    {
        waitTime = val;
    }

    void setOneShot(bool flag)
    {
        oneShot = flag;
    }

    void setOnTimeout(std::function<void()> onTimeout)
    {
        this->onTimeout = onTimeout;
    }

    void pause()
    {
        paused = true;
    }

    void resume()
    {
        paused = false;
    }

    void onUpdate(float delta)
    {
        if (paused)
            return;

        passTime += delta;
        if (passTime >= waitTime)
        {
            bool canShot = (!oneShot || (oneShot && !shotted));
            shotted = true;
            if (canShot && onTimeout)
                onTimeout();
            passTime -= waitTime;
        }
    }

private:
    float passTime = 0;
    float waitTime = 0;
    bool paused = false;
    bool shotted = false;
    bool oneShot = false;
    std::function<void()> onTimeout;
};