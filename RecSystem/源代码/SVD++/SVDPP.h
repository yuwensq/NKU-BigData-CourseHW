#ifndef SVDPP_H_
#define SVDPP_H_

#include <vector>
#include <map>
#include <string>

typedef double ld;

class SVDPP
{
private:
    bool hasKNN;
    int k;
    std::string knnFile;
    int userNum;
    int itemNum;
    // 特征的维度
    int dim;
    ld mean;
    ld *bu;
    ld *bi;
    ld **pu;
    ld **qi;
    ld **yj;
    // 把测试集划分为两个矩阵，一个用于训练，一个用于测试
    std::vector<std::vector<std::pair<int, int>>> trainMatrix;
    std::vector<std::vector<std::pair<int, int>>> testMatrix;
    std::vector<std::pair<int, int>> item2Attri;
    std::vector<int> clickPerItem;

    void splitData(std::string dataPath, float rate);
    void processData(std::string dataPath, float rate);
    void initModelArg();
    void calRMSE();
    // void saveModel();

public:
    SVDPP(int dim = 100) : dim(dim){};
    void train(std::string trainFile, ld alpha, ld lambda, float rate, int iterNum, bool saveModel);
    void predict(std::string predictFile);
};

#endif