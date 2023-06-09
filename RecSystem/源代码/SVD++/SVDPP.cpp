#include "SVDPP.h"
#include <fstream>
#include <ctime>
#include <random>
#include <assert.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <chrono>
#include <iostream>

static inline ld stdRui(ld rui)
{
    if (rui > 100)
        rui = 100;
    else if (rui < 0)
        rui = 0;
    return rui;
}

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

void SVDPP::splitData(std::string dataPath, float rate)
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

void SVDPP::processData(std::string dataPath, float rate)
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
    yj = new ld *[itemNum];
    for (int i = 0; i < userNum; i++)
        pu[i] = new ld[dim];
    for (int i = 0; i < itemNum; i++)
    {
        qi[i] = new ld[dim];
        yj[i] = new ld[dim];
    }

    trainMatrix.resize(userNum);
    testMatrix.resize(userNum);
    clickPerItem.resize(itemNum);

    splitData(dataPath, rate);
}

void SVDPP::initModelArg()
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
        {
            qi[i][j] = abs(rand()) % dim / (1.0 * dim);
            yj[i][j] = 0;
        }
    }
}

void SVDPP::calRMSE()
{
    int trainNum = 0;
    ld trainRMSE = 0;
    int testNum = 0;
    ld testRMSE = 0;
    ld *z = new ld[dim];

    for (int userId = 0; userId < userNum; userId++)
    {
        ld yjSum = trainMatrix[userId].size();
        yjSum = sqrt(yjSum);
        memset(z, 0, sizeof(z));
        for (auto [j, s] : trainMatrix[userId])
            for (int k = 0; k < dim; k++)
                z[k] += yj[j][k];
        for (int k = 0; k < dim; k++)
        {
            z[k] /= yjSum;
            z[k] += pu[userId][k];
        }
        trainNum += trainMatrix[userId].size();
        for (auto node : trainMatrix[userId])
        {
            int itemId = node.first;
            ld score = node.second;
            ld rui = mean + bu[userId] + bi[itemId] + product(qi[itemId], z, dim);
            rui = stdRui(rui);
            // std::cout << mean << " " << bu[userId] << " " << bi[itemId] << " " << product(pu[userId], z, dim) << std::endl;
            ld eui = score - rui;
            // std::cout << score << " " << rui << std::endl;
            trainRMSE += eui * eui;
        }
    }

    for (int userId = 0; userId < userNum; userId++)
    {
        ld yjSum = trainMatrix[userId].size();
        yjSum = sqrt(yjSum);
        memset(z, 0, sizeof(z));
        for (auto [j, s] : trainMatrix[userId])
            for (int k = 0; k < dim; k++)
                z[k] += yj[j][k];
        for (int k = 0; k < dim; k++)
        {
            z[k] /= yjSum;
            z[k] += pu[userId][k];
        }
        testNum += testMatrix[userId].size();
        for (auto node : testMatrix[userId])
        {
            int itemId = node.first;
            ld score = node.second;
            ld rui = mean + bu[userId] + bi[itemId] + product(qi[itemId], z, dim);
            rui = stdRui(rui);
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

void SVDPP::train(std::string trainFile, ld alpha, ld lambda, float rate, int iterNum, bool saveModel)
{
    std::cout << "------------------preprocess start----------------" << std::endl;
    processData(trainFile, rate);
    initModelArg();
    std::cout << "------------------preprocess over----------------" << std::endl;
    std::cout << "------------------train start----------------" << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    ld *z = new ld[dim];
    // ld *sum = new ld[dim];
    for (int iter = 1; iter <= iterNum; iter++)
    {
        for (int userId = 0; userId < userNum; userId++)
        {
            assert(trainMatrix[userId].size() != 0);
            if (trainMatrix[userId].size() == 0)
                continue;
            ld yjSum = trainMatrix[userId].size();
            yjSum = sqrt(yjSum);
            memset(z, 0, sizeof(z));
            // memset(sum, 0, sizeof(sum));
            for (auto [j, s] : trainMatrix[userId])
                for (int k = 0; k < dim; k++)
                    z[k] += yj[j][k];
            for (int k = 0; k < dim; k++)
            {
                z[k] /= yjSum;
                z[k] += pu[userId][k];
            }
            // std::cout << trainMatrix[userId].size() << std::endl;
            for (auto node : trainMatrix[userId])
            {
                int itemId = node.first;
                ld score = node.second;

                ld rui = mean + bu[userId] + bi[itemId] + product(qi[itemId], z, dim);
                rui = stdRui(rui);
                ld eui = score - rui;
                // std::cout << mean << " " << bu[userId] << " " << bi[itemId] << " " << product(pu[userId], z, dim) << std::endl;

                // update arg
                bu[userId] = bu[userId] + alpha * (eui - lambda * bu[userId]);
                bi[itemId] = bi[itemId] + alpha * (eui - lambda * bi[itemId]);

                // #pragma omp parallel for num_threads(4)
                for (int k = 0; k < dim; k++)
                {
                    // sum[k] += eui * qi[itemId][k] / yjSum;
                    // ld tmp_pu = pu[userId][k];
                    pu[userId][k] = pu[userId][k] + alpha * (eui * qi[itemId][k] - lambda * pu[userId][k]);
                    // qi[itemId][k] = qi[itemId][k] + alpha * (eui * pu[userId][k] - lambda * qi[itemId][k]);
                    // for (auto [j, s] : trainMatrix[userId])
                    //     yj[j][k] = yj[j][k] + alpha * (euiqi[itemId][k] / yjSum - lambda * yj[j][k]);
                    // qi[itemId][k] = qi[itemId][k] + alpha * (eui * z[k] - lambda * qi[itemId][k]);
                }
                // 这里为了充分利用cache，把他们三个的更新分开
                for (auto [j, s] : trainMatrix[userId])
                    // #pragma omp parallel for num_threads(4)
                    for (int k = 0; k < dim; k++)
                        yj[j][k] = yj[j][k] + alpha * (eui * qi[itemId][k] / yjSum - lambda * yj[j][k]);
                // #pragma omp parallel for num_threads(4)
                for (int k = 0; k < dim; k++)
                    qi[itemId][k] = qi[itemId][k] + alpha * (eui * z[k] - lambda * qi[itemId][k]);
            }
            if (userId % 1000 == 0)
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
}

void SVDPP::predict(std::string predictFile)
{
    // svd++ rmse比较大，就不拿他做预测了
    std::cout << "------------------predict start----------------" << std::endl;
    std::ofstream output;
    output.open("./res.txt");
    std::ifstream readStream;
    readStream.open(predictFile);
    ld *z = new ld[dim];
    do
    {
        std::string header;
        readStream >> header;
        output << header << std::endl;
        int userId, rateNum;
        int sepPos = header.find_first_of("|");
        userId = atoi(header.substr(0, sepPos).c_str());
        rateNum = atoi(header.substr(sepPos + 1, header.size() - sepPos - 1).c_str());
        ld yjSum = trainMatrix[userId].size();
        yjSum = sqrt(yjSum);
        memset(z, 0, sizeof(z));
        for (auto [j, s] : trainMatrix[userId])
            for (int k = 0; k < dim; k++)
                z[k] += yj[j][k];
        for (int k = 0; k < dim; k++)
        {
            z[k] /= yjSum;
            z[k] += pu[userId][k];
        }
        int itemId;
        while (rateNum)
        {
            readStream >> itemId;
            ld rui = mean + bu[userId] + bi[itemId] + product(qi[itemId], z, dim);
            rui = stdRui(rui);
            output << itemId << " " << int(rui) << std::endl;
            rateNum--;
        }
    } while (!readStream.eof());
    readStream.close();
    output.close();
    std::cout << "------------------predict over----------------" << std::endl;
}
