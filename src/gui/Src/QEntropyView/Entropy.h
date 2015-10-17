#ifndef ENTROPY_H
#define ENTROPY_H

#include <cmath>
#include <vector>

class Entropy
{
public:
    static double MeasureData(const unsigned char* data, int dataSize)
    {
        int occurrences[256] = {};
        for(int i = 0; i < dataSize; i++)
            occurrences[data[i]]++;
        double entropy = 0.0;
        double logBase = log(256);
        for(int i = 0; i < 256; i++)
        {
            if(occurrences[i] == 0)
                continue;
            double p = (double)occurrences[i] / (double)dataSize;
            entropy += p * log(p) / logBase;
        }
        return -entropy;
    }

    static double MeasureByte(const unsigned char* data, int dataSize, int index, unsigned char* block, int blockSize)
    {
        if(dataSize < blockSize)
            return -1;
        int start = index - blockSize / 2;
        int end = index + blockSize / 2;
        if(start < 0)
        {
            end += -start;
            start = 0;
        }
        else if(end > dataSize)
        {
            start -= end - dataSize;
            end = dataSize;
        }
        for(int i = start; i < end; i++)
            block[i - start] = data[i];
        return MeasureData(block, blockSize);
    }

    static void MeasurePoints(const unsigned char* data, int dataSize, int blockSize, std::vector<double> & points, int pointCount)
    {
        points.clear();
        if(dataSize < pointCount)
            return;
        if(dataSize % pointCount != 0)
            pointCount += dataSize % pointCount;

        unsigned char* block = new unsigned char[blockSize];
        int interval = dataSize / pointCount;
        points.reserve(pointCount);
        for(int i = 0; i < dataSize; i += interval)
            points.push_back(MeasureByte(data, dataSize, i, block, blockSize));
        delete[] block;
    }
};

#endif // ENTROPY_H

