#include "SVD.h"
#include <fstream>
#include <ctime>
#include <random>
#include <assert.h>
#include <algorithm>
#include <map>
#include <chrono>
#include <iostream>

static inline ld product(ld *a, ld *b, int len)
{
    // 后期可以尝试并行化？特征如果不大，感觉没必要
    ld res = 0;
    for (int i = 0; i < len; i++)
    {
        res += a[i] * b[i];
    }
    return res;
}

static inline ld sim(ld x1, ld x2, ld y1, ld y2)
{
    // 皮尔逊相关系数，不能用这个
    // ld avg1, avg2;
    // avg1 = (x1 + x2) / 2.0;
    // avg2 = (y1 + y2) / 2.0;
    // x1 -= avg1;
    // x2 -= avg1;
    // y1 -= avg2;
    // y2 -= avg2;
    // ld res = 0;
    // ld sum, sq1, sq2;
    // sum = sq1 = sq2 = 0;
    // sum += x1 * y1 + x2 * y2;
    // sq1 += x1 * x1 + x2 * x2;
    // sq2 += y1 * y1 + y2 * y2;
    // sq1 = sqrt(sq1);
    // sq2 = sqrt(sq2);
    // res = sum / sq1 / sq2;
    // std::cout << x1 << " " << x2 << " " << y1 << " " << y2 << " " << res << std::endl;
    // 用欧式距离吧
    ld res = sqrt((x1 - y1) * (x1 - y1) + (x2 - y2) * (x2 - y2));
    return res;
}

void SVD::splitData(std::string dataPath, float rate)
{
    srand(time(0));
    int trainNum = 0; // 用于求平均值
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
        int exceptTestNum = (int)(rateNum * rate);
        int trueTestNum = 0;
        int itemId, score;
        while (rateNum)
        {
            readStream >> itemId >> score;

            if (abs(rand()) % 100 / 100.0 < rate && trueTestNum < exceptTestNum)
            {
                testMatrix[userId].push_back(std::make_pair(itemId, score));
                trueTestNum++;
            }
            else
            {
                trainMatrix[userId].push_back(std::make_pair(itemId, score));
                mean += score;
                trainNum++;
                clickPerItem[itemId]++;
            }
            rateNum--;
        }
        while (trueTestNum < exceptTestNum)
        {
            auto it = trainMatrix[userId].begin();
            while (it != trainMatrix[userId].end())
            {
                if (abs(rand()) % 100 / 100.0 < rate)
                {
                    mean -= (*it).second;
                    trainNum--;
                    testMatrix[userId].push_back(*it);
                    clickPerItem[(*it).first]--;
                    it = trainMatrix[userId].erase(it);
                    trueTestNum++;
                    if (trueTestNum >= exceptTestNum)
                        break;
                }
                else
                {
                    it++;
                }
            }
        }
    } while (!readStream.eof());
    readStream.close();

    mean /= trainNum;
    // std::ofstream output("debug.txt");
    // int trainSum = 0;
    // int testSum = 0;
    // for (int i = 0; i < userNum; i++)
    // {
    //     trainSum += trainMatrix[i].size();
    //     testSum += testMatrix[i].size();
    //     output << trainMatrix[i].size() << " " << testMatrix[i].size() << std::endl;
    // }
    // output << trainSum << " " << testSum;
    // output.close();
}

void SVD::processData(std::string dataPath, float rate)
{
    int maxUserId = 0;
    int minUserId = 0x3f3f3f3f;
    int maxItemId = 0;
    int minItemId = 0x3f3f3f3f;
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
        minUserId = std::min(userId, minUserId);
        maxUserId = std::max(userId, maxUserId);
        int itemId, score;
        while (rateNum)
        {
            readStream >> itemId >> score;
            minItemId = std::min(itemId, minItemId);
            maxItemId = std::max(itemId, maxItemId);
            rateNum--;
        }
    } while (!readStream.eof());
    readStream.close();

    // 其实俩min都是0，这里图个心里安慰，留下扩展的可能性，虽然数据集是固定给出的
    userNum = maxUserId - minUserId + 1;
    itemNum = maxItemId - minItemId + 1;

    bu = new ld[userNum];
    bi = new ld[itemNum];
    pu = new ld *[userNum];
    qi = new ld *[itemNum];
    for (int i = 0; i < userNum; i++)
        pu[i] = new ld[dim];
    for (int i = 0; i < itemNum; i++)
        qi[i] = new ld[dim];

    trainMatrix.resize(userNum);
    testMatrix.resize(userNum);
    clickPerItem.resize(itemNum);

    splitData(dataPath, rate);
}

void SVD::initModelArg()
{
    srand(time(0));
    for (int i = 0; i < userNum; i++)
    {
        bu[i] = 0;
        for (int j = 0; j < dim; j++)
            pu[i][j] = abs(rand()) % dim / (1.0 * dim);
    }
    for (int i = 0; i < itemNum; i++)
    {
        bi[i] = 0;
        for (int j = 0; j < dim; j++)
            qi[i][j] = abs(rand()) % dim / (1.0 * dim);
    }
}

void SVD::calRMSE()
{
    int trainNum = 0;
    ld trainRMSE = 0;
    int testNum = 0;
    ld testRMSE = 0;
    for (int userId = 0; userId < userNum; userId++)
    {
        trainNum += trainMatrix[userId].size();
        for (auto node : trainMatrix[userId])
        {
            int itemId = node.first;
            ld score = node.second;
            ld rui = predictUI(userId, itemId);
            // std::cout << mean << " " << bu[userId] << " " << bi[itemId] << " " << product(pu[userId], qi[itemId], dim) << std::endl;
            ld eui = score - rui;
            // std::cout << score << " " << rui << std::endl;
            trainRMSE += eui * eui;
        }
    }

    for (int userId = 0; userId < userNum; userId++)
    {
        testNum += testMatrix[userId].size();
        for (auto node : testMatrix[userId])
        {
            int itemId = node.first;
            ld score = node.second;
            // ld rui = predictUI(userId, itemId);
            ld rui = predictEXT(userId, itemId);
            ld eui = score - rui;
            testRMSE += eui * eui;
        }
    }
    // std::cout << trainRMSE << " " << testRMSE << std::endl;
    trainRMSE = sqrt(trainRMSE / trainNum);
    testRMSE = sqrt(testRMSE / testNum);

    if (testNum == 0)
        std::cout << "train RMSE: " << trainRMSE << std::endl;
    else
        std::cout << "train RMSE: " << trainRMSE << "\ttest RMSE: " << testRMSE << std::endl;
}

void SVD::calRMSEPerItem()
{
    for (int i = 0; i < userNum; i++)
    {
        for (auto node : testMatrix[i])
            trainMatrix[i].push_back(node);
    }
    std::map<int, std::pair<ld, int>> item2RMSE;
    for (int userId = 0; userId < userNum; userId++)
    {
        for (auto node : testMatrix[userId])
        {
            int itemId = node.first;
            ld score = node.second;
            ld rui = predictEXT(userId, itemId);
            ld eui = score - rui;
            item2RMSE[itemId].first += eui * eui;
            item2RMSE[itemId].second++;
        }
    }
    std::ofstream output;
    output.open("./rmsePerItem.txt");
    for (int itemId = 0; itemId < itemNum; itemId++)
    {
        ld rmse = 0;
        if (item2RMSE[itemId].second > 0)
            rmse = sqrt(item2RMSE[itemId].first / item2RMSE[itemId].second);
        output << itemId << " " << rmse << std::endl;
    }
    output.close();
}

ld SVD::predictUI(int uid, int iid)
{
    ld rui = mean + bu[uid] + bi[iid] + product(pu[uid], qi[iid], dim);
    if (rui > 100)
        rui = 100;
    else if (rui < 0)
        rui = 0;
    return rui;
}

ld SVD::predictEXT(int uid, int iid)
{
    ld res = predictUI(uid, iid);
    if (hasKNN && trainMatrix[uid].size() < itemNum / 10000 && clickPerItem[iid] < userNum / 1000)
    {
        ld knnRes = 0;
        ld simSum = 0;
        auto items = knn(uid, iid);
        for (int i = 0; i < items.size(); i++)
        {
            // knnRes += items[i].first * items[i].second;
            knnRes += predictUI(uid, items[i].first) * items[i].second;
            simSum += items[i].second;
            // std::cout << items[i].second << " ";
        }
        if (simSum != 0)
        {
            // std::cout << knnRes << " " << simSum << std::endl;
            ld knnWeight = 0.3;
            knnRes /= simSum;
            res = (1 - knnWeight) * res + knnWeight * knnRes;
            if (res > 100)
                res = 100;
            if (res < 0)
                res = 0;
        }
    }
    return res;
}

std::vector<std::pair<int, ld>> SVD::knn(int uid, int iid)
{
    std::vector<std::pair<int, ld>> res;
    // 这里先用train吧
    ld y1, y2;
    y1 = item2Attri[iid].first;
    y2 = item2Attri[iid].second;
    if (y1 == -1 || y2 == -1)
        return res;
    for (int i = 0; i < trainMatrix[uid].size(); i++)
    {
        int itemId = trainMatrix[uid][i].first;
        int score = trainMatrix[uid][i].second;
        ld x1, x2;
        x1 = item2Attri[itemId].first;
        x2 = item2Attri[itemId].second;
        if (iid == itemId || x1 == -1 || x2 == -1)
            continue;
        res.push_back(std::make_pair(itemId, sim(x1, x2, y1, y2)));
    }
    auto cmp = [&](std::pair<int, ld> &a, std::pair<int, ld> &b)
    {
        return a.second > b.second;
    };
    std::sort(res.begin(), res.end(), cmp);
    if (res.size() > k)
        res.resize(k);
    return res;
}

void SVD::openKNN()
{
    item2Attri.resize(itemNum);
    std::ifstream input;
    input.open(knnFile);
    std::string line, attri1, attri2;
    while (!input.eof())
    {
        input >> line;
        int itemId;
        int sepPos = line.find_first_of("|");
        itemId = atoi(line.substr(0, sepPos).c_str());
        line = line.substr(sepPos + 1, line.size() - sepPos - 1);
        sepPos = line.find_first_of("|");
        attri1 = line.substr(0, sepPos);
        attri2 = line.substr(sepPos + 1, line.size() - sepPos - 1);
        int at1, at2;
        at1 = at2 = -1;
        if (attri1 != "None")
            at1 = atoi(attri1.c_str());
        if (attri2 != "None")
            at2 = atoi(attri2.c_str());
        item2Attri[itemId] = std::make_pair(at1, at2);
    }
    input.close();
    for (int i = 0; i < itemNum; i++)
        if (item2Attri[i].first == 0 || item2Attri[i].second == 0)
            item2Attri[i].first = item2Attri[i].second = -1;
}

void SVD::train(std::string trainFile, ld alpha, ld lambda, float rate, int iterNum, bool saveModel)
{
    std::cout << "------------------preprocess start----------------" << std::endl;
    processData(trainFile, rate);
    initModelArg();
    if (hasKNN)
        openKNN();
    std::cout << "------------------preprocess over----------------" << std::endl;
    std::cout << "------------------train start----------------" << std::endl;
    std::cout << "epochs: " << iterNum << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int iter = 1; iter <= iterNum; iter++)
    {
        for (int userId = 0; userId < userNum; userId++)
        {
            for (auto node : trainMatrix[userId])
            {
                int itemId = node.first;
                ld score = node.second;
                ld rui = predictUI(userId, itemId);
                ld eui = score - rui;

                // update arg
                bu[userId] = bu[userId] + alpha * (eui - lambda * bu[userId]);
                bi[itemId] = bi[itemId] + alpha * (eui - lambda * bi[itemId]);

                for (int k = 0; k < dim; k++)
                {
                    ld tmp_pu = pu[userId][k];
                    pu[userId][k] = pu[userId][k] + alpha * (eui * qi[itemId][k] - lambda * pu[userId][k]);
                    qi[itemId][k] = qi[itemId][k] + alpha * (eui * tmp_pu - lambda * qi[itemId][k]);
                    // qi[itemId][k] = qi[itemId][k] + alpha * (eui * pu[userId][k] - lambda * qi[itemId][k]);
                }
            }
            if (userId % 5000 == 0)
            {
                std::cout << "epoch " << iter << " process " << userId << "user" << std::endl;
            }
        }
        // 打印RMSE信息
        std::cout << "epoch" << iter << ": \t";
        calRMSE();
        // 越往后，学习率应该越小，越趋于稳定。
        alpha *= 0.99;
        // lambda *= 1.05;
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
    std::cout << "train time: " << fp_ms.count() / 1000.0 << "s" << std::endl;
    // 画图用
    // calRMSEPerItem();
}

void SVD::predict(std::string predictFile)
{
    std::cout << "------------------predict start----------------" << std::endl;
    std::ofstream output;
    output.open("./res.txt");
    std::ifstream readStream;
    readStream.open(predictFile);
    do
    {
        std::string header;
        readStream >> header;
        output << header << std::endl;
        int userId, rateNum;
        int sepPos = header.find_first_of("|");
        userId = atoi(header.substr(0, sepPos).c_str());
        rateNum = atoi(header.substr(sepPos + 1, header.size() - sepPos - 1).c_str());
        int itemId;
        while (rateNum)
        {
            readStream >> itemId;
            ld rui = predictUI(userId, itemId);
            output << itemId << " " << int(rui) << std::endl;
            rateNum--;
        }
    } while (!readStream.eof());
    readStream.close();
    output.close();
    std::cout << "------------------predict over----------------" << std::endl;
}
