#ifndef SVD_H_
#define SVD_H_

#include <iostream>
#include <vector>
#include <map>
#include <string>

typedef double ld;

class SVD
{
private:
    bool hasKNN;
    int k;
    std::string knnFile;
    int userNum;
    int itemNum;
    // 特征的维度
    int dim;
    // rui = mean + bu + bi + pu*qi
    ld mean;
    ld *bu;
    ld *bi;
    ld **pu;
    ld **qi;
    // 把测试集划分为两个矩阵，一个用于训练，一个用于测试
    std::vector<std::vector<std::pair<int, int>>> trainMatrix;
    std::vector<std::vector<std::pair<int, int>>> testMatrix;
    std::vector<std::pair<int, int>> item2Attri;
    std::vector<int> clickPerItem;

    void splitData(std::string dataPath, float rate);
    void processData(std::string dataPath, float rate);
    void initModelArg();
    void calRMSE();
    void calRMSEPerItem();
    ld predictUI(int uid, int iid);
    ld predictEXT(int uid, int iid);
    void openKNN();
    std::vector<std::pair<int, ld>> knn(int uid, int iid);
    // void saveModel();

public:
    SVD(int dim = 100, bool hasKNN = false, std::string attriFile = "", int k = 5) : dim(dim), hasKNN(hasKNN), knnFile(attriFile), k(k){};
    void train(std::string trainFile, ld alpha, ld lambda, float rate, int iterNum, bool saveModel);
    void predict(std::string predictFile);
};

#endif