#pragma once
#include "vector2.h"

#include <vector>

class Path
{
public:
    Path(const std::vector<Vector2> &pointList)
    {
        this->pointList = pointList;

        for (size_t i = 1; i < pointList.size(); i++)
        {
            float segmentLen = (pointList[i] - pointList[i - 1]).length();
            segmentLengthList.push_back(segmentLen);
            totalLength += segmentLen;
        }
    }
    ~Path() = default;

    Vector2 getPostitionAtProgress(float progress) const
    {
        if (progress <= 0)
            return pointList.front();
        if (progress >= 1)
            return pointList.back();

        float targetDistance = totalLength * progress;
        float accumulatedLen = 0.0f;

        for (size_t i = 1; i < pointList.size(); ++i)
        {
            accumulatedLen += segmentLengthList[i - 1];
            if (accumulatedLen >= targetDistance)
            {
                float segmentProgress = (targetDistance - (accumulatedLen - segmentLengthList[i - 1])) / segmentLengthList[i - 1];
                return pointList[i - 1] + (pointList[i] - pointList[i - 1]) * segmentProgress;
            }
        }
        return pointList.back();
    }

private:
    float totalLength = 0;

    std::vector<Vector2> pointList;
    std::vector<float> segmentLengthList;
};
