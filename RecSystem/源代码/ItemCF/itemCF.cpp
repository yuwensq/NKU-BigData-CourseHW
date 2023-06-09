#include <iostream>
#include <fstream>
#include <Windows.h>
#include <assert.h>
#include <algorithm>
#include <algorithm>
#include <string>
#include <cstring>
#include <math.h>
#include <time.h>
#include <vector>
using namespace std;

const string testFile = "test.txt";
const string trainFile = "train.txt";
const string attributeFile = "itemAttribute.txt";
const int user = 19835;
const int max_item = 624961;
const int predict_num = 6;
const bool consider_attr = true;

double userAVG[user];
double itemAVG[max_item];
double avg;

struct rating {
    int id; // Data: user_id, SimMap: item_id
    double score; // Data: user's rating score, SimMap: similarity between items
    bool operator == (const rating& e) {
        return (this->id == e.id) && (this->score == e.score);
    }
    bool operator == (const int& user_id) {
        return (this->id == user_id);
    }
};

struct items {
    double attr1;
    double attr2;
    double norm;
};

items attrList[max_item];
int ratingNum_item[max_item];
int ratingNum_user[user];
vector<vector<rating>> SimMap(max_item);
vector<vector<rating>> Data(max_item);
int ReadBuffer[user][predict_num];

bool cmp_by_sim(rating a, rating b) {
    return a.score > b.score;
}

int binarySearch(vector<vector<rating>>& data, int item_id, int num, int aim) { // accelerate the speed when finding in vector
    int left = 0, right = num - 1, mid;
    while (left <= right) {
        if (data[item_id][left].id == aim) {
            return left;
        }
        if (data[item_id][right].id == aim) {
            return right;
        }
        mid = left + ((right - left) / 2);
        if (data[item_id][mid].id == aim) {
            return mid;
        }
        if (data[item_id][mid].id < aim) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }
    if (left > right) {
        return -1;
    }
}

void read_test_file() {
    ifstream file;
    file.open(testFile);
    assert(file.is_open());

    string s;
    int user_id = -1;
    int i = 0;

    while (getline(file, s)) {
        int mark_pos = s.find('|');
        if (s == "") {
            break;
        }
        if (mark_pos == -1) {
            ReadBuffer[user_id][i++] = stoi(s);
        }
        else {
            user_id++;
            i = 0;
        }
    }

    file.close();
    cout << "[info] read test file successfully." << endl;
}

void calculate_item_avg() { // get the average score of specific item
    for (int i = 0; i < max_item; i++) {
        if (ratingNum_item[i] != 0) {
            itemAVG[i] /= ratingNum_item[i];
        }
        else {
            rating blank; blank.id = -1; blank.score = 0;
            Data[i].push_back(blank);
            ratingNum_item[i]++;
        }
    }
}

void read_train_file() {
    ifstream file;
    file.open(trainFile);
    assert(file.is_open());

    string s;
    int user_id = -1;
    double total = 0, count = 0;

    while (getline(file, s)) {
        int mark_pos = s.find('|');
        if (s == "") {
            break;
        }
        if (mark_pos == -1) { // new rating information
            int blank_pos = s.find(" ");
            string id_str = s.substr(0, blank_pos);
            string score_str = s.substr(blank_pos + 2, s.length() - mark_pos);
            int id = stoi(id_str);
            double score = stof(score_str);

            // format in Data[item_id]: item_id - (user_id, score)
            rating rate_info; rate_info.id = user_id, rate_info.score = score;
            Data[id].push_back(rate_info);
            // update the number of people who have rated the item
            ratingNum_item[id]++;

            // calculate sum score
            userAVG[user_id] += score;
            itemAVG[id] += score;
            total += score;
        }
        else { // new user
            if (user_id != -1) // calculate the average rating score of specific user
                userAVG[user_id] /= ratingNum_user[user_id];
            // initialize new user
            user_id++;
            // calculate number
            string num_str = s.substr(mark_pos + 1, s.length() - mark_pos);
            int num = stoi(num_str);
            ratingNum_user[user_id] = num;
            count += num;
        }
    }
    // calculate the overall average score and average rating score of specific item
    avg = total / count;
    calculate_item_avg();

    file.close();
    cout << "[info] read train file successfully." << endl;
}

void process_attribute_file() {
    ifstream file;
    file.open(attributeFile);
    assert(file.is_open());

    string s;
    int id = 0;

    while (getline(file, s)) {
        int mark_pos1 = s.find('|');
        if (s == "") {
            break;
        }
        // data format: 0|224058|587636
        string attrs_str = s.substr(mark_pos1 + 1);
        int mark_pos2 = attrs_str.find('|');
        string attr1_str = attrs_str.substr(0, mark_pos2);
        string attr2_str = attrs_str.substr(mark_pos2 + 1);
        attrList[id].attr1 = (attr1_str != "None") ? stoi(attr1_str) : 0;
        attrList[id].attr2 = (attr2_str != "None") ? stoi(attr2_str) : 0;
        attrList[id].norm = sqrt(attrList[id].attr1 * attrList[id].attr1 + attrList[id].attr2 * attrList[id].attr2);
        id++;
    }
    file.close();
    cout << "[info] read attribute file successfully." << endl;
}

double calculate_PearsonSim(int item1, int item2) {
    int item_less = (ratingNum_item[item1] <= ratingNum_item[item2]) ? item1 : item2;
    int item_more = (ratingNum_item[item1] <= ratingNum_item[item2]) ? item2 : item1;
    int num = (ratingNum_item[item1] <= ratingNum_item[item2]) ? ratingNum_item[item1] : ratingNum_item[item2];
    int count = 0;

    if (Data[item_less][0].id == -1 || Data[item_more][0].id == -1) {
        return 0;
    }
    // calculate the numerator
    double numerator = 0;
    for (int i = 0; i < num; i++) {
        int iter = binarySearch(Data, item_more, ratingNum_item[item_more], Data[item_less][i].id);
        if (iter != -1) {
            count++;
            numerator += (Data[item_more][iter].score - itemAVG[item_more]) * (Data[item_less][i].score - itemAVG[item_less]);
        }
    }
    // only consider items rated by over 20 users to avoid too much bias
    if (count < 20) {
        return 0;
    }
    // calculate the denominator
    double sum1 = 0;
    for (int i = 0; i < ratingNum_item[item1]; i++) {
        sum1 += (Data[item1][i].score - itemAVG[item1]) * (Data[item1][i].score - itemAVG[item1]);
    }
    double sqrt1 = sqrt(sum1);

    double sum2 = 0;
    for (int i = 0; i < ratingNum_item[item2]; i++) {
        sum2 += (Data[item2][i].score - itemAVG[item2]) * (Data[item2][i].score - itemAVG[item2]);
    }
    double sqrt2 = sqrt(sum2);

    if (sqrt1 == 0 || sqrt2 == 0) {
        return 0;
    }

    double similarity = numerator / (sqrt1 * sqrt2);
    return similarity;
}

double calculate_AttributeSim(int item1, int item2) {
    if (attrList[item1].norm == 0 || attrList[item2].norm == 0) {
        return 0;
    }
    double a = (attrList[item1].attr1 * attrList[item2].attr1 + attrList[item1].attr2 * attrList[item2].attr2);
    double b = (attrList[item1].norm * attrList[item2].norm);
    double attribute_similarity = a / b;
    return attribute_similarity;
}

double calculate_Sim(int item1, int item2) {
    int item_min = min(item1, item2);
    int item_max = max(item1, item2);
    int iter = binarySearch(SimMap, item_min, SimMap[item_min].size(), item_max);
    if (iter != -1) {
        return SimMap[item_min][iter].score;
    }
    double Pearson, Similarity, Attribute = 0;
    Pearson = calculate_PearsonSim(item1, item2);
    if (consider_attr)
        Attribute = calculate_AttributeSim(item1, item2);
    Similarity = (consider_attr == true) ? (Pearson + Attribute) / 2 : Pearson;

    rating similarity1; similarity1.id = item_max; similarity1.score = Similarity;
    SimMap[item_min].push_back(similarity1);
    return Similarity;
}

double predict(int user_id, int item_id) {
    double sumSim = 0;
    double sumSim_item = 0;
    for (int i = 0; i < max_item; i++) {
        if (i == item_id)
            continue;
        int iter = binarySearch(Data, i, ratingNum_item[i], user_id);
        if (iter != -1) {
            double sim = calculate_Sim(item_id, i);
            if (sim > 0) {
                // consider deviations
                double baseline = avg + (userAVG[user_id] - avg) + (itemAVG[i] - avg);
                sumSim += sim;
                sumSim_item += sim * (Data[i][iter].score - baseline);
            }
        }
    }
    if (sumSim != 0 && sumSim_item != 0) {
        // consider deviations
        double baseline = avg + (userAVG[user_id] - avg) + (itemAVG[item_id] - avg);
        double predict_score = baseline + sumSim_item / sumSim;
        if (predict_score > 100) {
            return 100;
        }
        else if (predict_score < 0) {
            return 0;
        }
        return predict_score;
    }
    else {
        return userAVG[user_id];
    }
}

void write2text() {
    cout << "[info] making predictions..." << endl;
    fstream file;
    file.open("result.txt");
    for (int i = 0; i < user; i++) {
        for (int j = 0; j < predict_num; j++) {
            if (j == 0) {
                file << i << "|" << "6" << endl;
            }
            file << ReadBuffer[i][j] << " " << int(predict(i, ReadBuffer[i][j])) << endl;
            // cout << "[info] predict score between user" << i << " and item" << ReadBuffer[i][j] << endl;
        }
    }
    file.close();
    cout << "[info] done." << endl;
}

int main() {
    double start, end;
    start = clock();

    // create thread to read file at the same time
    HANDLE hThread; DWORD ThreadID;
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)read_test_file, NULL, 0, &ThreadID);
    process_attribute_file();

    // read train file and calculate all the deviation information
    read_train_file();

    // predict through model and write result to .txt
    write2text();

    end = clock();
    cout << "[info] overall running time: " << end - start << endl;
    return 0;
}
