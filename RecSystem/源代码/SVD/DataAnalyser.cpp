#include "DataAnalyser.h"

#include <fstream>
#include <set>
#include <iostream>
#include <vector>
#include <algorithm>

static bool cmp(std::pair<int, int> &a, std::pair<int, int> &b)
{
    return a.second > b.second;
}

std::map<std::string, int> DataAnalyser::analyse()
{
    std::map<int, int> rateTimes;
    std::set<int> users;
    std::set<int> items;
    int ratings = 0;
    int minUserId = 0x3f3f3f3f;
    int maxUserId = 0;
    int minItemId = 0x3f3f3f3f;
    int maxItemId = 0;
    std::map<std::string, int> dataInfo;
    std::ifstream readStream;
    readStream.open(dataPath);
    do
    {
        std::string header;
        readStream >> header;
        int userId, rateNum;
        int sepPos = header.find_first_of("|");
        userId = atoi(header.substr(0, sepPos).c_str());
        rateNum = atoi(header.substr(sepPos + 1, header.size() - sepPos - 1).c_str());
        users.insert(userId);
        ratings += rateNum;
        minUserId = std::min(userId, minUserId);
        maxUserId = std::max(userId, maxUserId);
        int itemId, score;
        while (rateNum)
        {
            if (!isTest)
            {
                readStream >> itemId >> score;
                rateTimes[itemId]++;
            }
            else
                readStream >> itemId;
            items.insert(itemId);
            minItemId = std::min(itemId, minItemId);
            maxItemId = std::max(itemId, maxItemId);
            rateNum--;
        }
    } while (!readStream.eof());
    readStream.close();
    dataInfo["userNum"] = users.size();
    dataInfo["itemNum"] = items.size();
    dataInfo["rateNum"] = ratings;
    dataInfo["minUserId"] = minUserId;
    dataInfo["maxUserId"] = maxUserId;
    dataInfo["minItemId"] = minItemId;
    dataInfo["maxItemId"] = maxItemId;

    if (!isTest)
    {
        std::ofstream longTailData;
        longTailData.open("./longTailData.txt");
        std::vector<std::pair<int, int>> allItemRatedTimes;
        for (int i = minItemId; i <= maxItemId; i++)
            allItemRatedTimes.push_back(std::make_pair(i, rateTimes[i]));
        sort(allItemRatedTimes.begin(), allItemRatedTimes.end(), cmp);
        for (auto pa : allItemRatedTimes)
            longTailData << pa.first << " " << pa.second << "\n";
        longTailData.close();
    }

    return dataInfo;
}