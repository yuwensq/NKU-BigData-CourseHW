#include "DataAnalyser.h"
#include "SVD.h"
#include "SVDPP.h"
#include <iostream>

using namespace std;

int main()
{
    // DataAnalyser *trainDa = new DataAnalyser("./train.txt");
    // auto dataInfo = trainDa->analyse();
    // cout << "------------------train data statistics----------------" << endl;
    // cout << "userNum: " << dataInfo["userNum"] << endl;
    // cout << "itemNum: " << dataInfo["itemNum"] << endl;
    // cout << "rateNum: " << dataInfo["rateNum"] << endl;
    // cout << "minUserId: " << dataInfo["minUserId"] << endl;
    // cout << "maxUserId: " << dataInfo["maxUserId"] << endl;
    // cout << "minItemId: " << dataInfo["minItemId"] << endl;
    // cout << "maxItemId: " << dataInfo["maxItemId"] << endl;

    // DataAnalyser *testDa = new DataAnalyser("./test.txt", true);
    // dataInfo = testDa->analyse();
    // cout << "------------------test data statistics----------------" << endl;
    // cout << "userNum: " << dataInfo["userNum"] << endl;
    // cout << "itemNum: " << dataInfo["itemNum"] << endl;
    // cout << "rAskNum: " << dataInfo["rateNum"] << endl;
    // cout << "minUserId: " << dataInfo["minUserId"] << endl;
    // cout << "maxUserId: " << dataInfo["maxUserId"] << endl;
    // cout << "minItemId: " << dataInfo["minItemId"] << endl;
    // cout << "maxItemId: " << dataInfo["maxItemId"] << endl;
    // delete trainDa;
    // delete testDa;

    SVDPP *svd = new SVDPP(10);
    svd->train("./train.txt", 0.001, 1.5, 0, 15, false);
    svd->predict("./test.txt");

    // SVD *svd = new SVD(300, true, "./itemAttribute.txt", 20);
    // svd->train("./train.txt", 0.001, 1.5, 0, 100, false);
    // svd->predict("./test.txt");

    delete svd;
    return 0;
}