#ifndef DATA_ANALYSER_H_
#define DATA_ANALYSER_H_

#include <string>
#include <map>

/**
 * 这个类主要用于数据分析啥的
 */
class DataAnalyser
{
private:
    std::string dataPath;
    bool isTest;

public:
    DataAnalyser(std::string dataPath = "train.txt", bool isTest = false) : dataPath(dataPath), isTest(isTest){};
    std::map<std::string, int> analyse();
};

#endif