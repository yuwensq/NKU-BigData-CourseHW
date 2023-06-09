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
using namespace std;

const string testFile = "test.txt";
const string trainFile = "train.txt";
const int user = 19835;
const int max_item = 624961;
const int predict_num = 6;

double userAVG[user];
double itemAVG[max_item];
double avg;

struct rating {
    int id;
    double score;
};

int ratingNum[user];
rating** Data = new rating * [user];
int ReadBuffer[user][predict_num];


bool cmp_by_id(rating a, rating b) {
    return a.id < b.id;
}

int binarySearch(rating** data, int user_id, int num, int aim) {
    int left = 0, right = num - 1, mid;
    while (left <= right) {
        if (data[user_id][left].id == aim) {
            return left;
        }
        if (data[user_id][right].id == aim) {
            return right;
        }
        mid = left + ((right - left) / 2);
        if (data[user_id][mid].id == aim) {
            return mid;
        }
        if (data[user_id][mid].id < aim) {
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

void reorganize_data(int user_id) {
    // calculate the average rating score of specific user
    userAVG[user_id] /= ratingNum[user_id];
    // sort by item id
    sort(Data[user_id], Data[user_id] + ratingNum[user_id], cmp_by_id);
}

void read_train_file() {
    ifstream file;
    file.open(trainFile);
    assert(file.is_open());

    string s;
    int user_id = -1;
    int i = 0;
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
            rating rate_info; rate_info.id = id, rate_info.score = score;
            Data[user_id][i++] = rate_info;
            userAVG[user_id] += score;
            total += score;
        }
        else { // new user
            // reoganize sequence
            if (user_id != -1) {
                reorganize_data(user_id);
            }
            // initialize new user
            user_id++;
            i = 0;
            string num_str = s.substr(mark_pos + 1, s.length() - mark_pos);
            int num = stoi(num_str);
            ratingNum[user_id] = num;
            Data[user_id] = new rating[num];

            count += num;
        }
    }
    // calculate the overall average score
    avg = total / count;

    file.close();
    cout << "[info] read train file successfully." << endl;
}

double get_item_avg(int id) { // get the average score of specific item
    if (itemAVG[id] != 0)
        return itemAVG[id];
    double sum = 0, num = 0;
    for (int i = 0; i < user; i++) {
        int j = binarySearch(Data, i, ratingNum[i], id);
        if (j != -1) {
            sum += Data[i][j].score;
            num++;
        }
    }
    itemAVG[id] = sum / num;
    return itemAVG[id];
}

double calculate_PearsonSim(int user1, int user2) {
    int user_less = (ratingNum[user1] <= ratingNum[user2]) ? user1 : user2;
    int user_more = (ratingNum[user1] <= ratingNum[user2]) ? user2 : user1;
    int num = (ratingNum[user1] <= ratingNum[user2]) ? ratingNum[user1] : ratingNum[user2];

    // calculate the numerator
    double numerator = 0;
    for (int i = 0; i < num; i++) {
        int j = binarySearch(Data, user_more, ratingNum[user_more], Data[user_less][i].id);
        if (j != -1) {
            numerator += (Data[user_less][i].score - userAVG[user_less]) * (Data[user_more][j].score - userAVG[user_more]);
        }
    }

    // calculate the denominator
    double sum1 = 0;
    for (int i = 0; i < ratingNum[user1]; i++) {
        sum1 += (Data[user1][i].score - userAVG[user1]) * (Data[user1][i].score - userAVG[user1]);
    }
    double sqrt1 = sqrt(sum1);

    double sum2 = 0;
    for (int i = 0; i < ratingNum[user2]; i++) {
        sum2 += (Data[user2][i].score - userAVG[user2]) * (Data[user2][i].score - userAVG[user2]);
    }
    double sqrt2 = sqrt(sum2);

    if (sqrt1 == 0 || sqrt2 == 0) {
        return 0;
    }

    double similarity = numerator / (sqrt1 * sqrt2);
    return similarity;
}

double predict(int user_id, int item_id) {
    double sumSim = 0;
    double sumSim_item = 0;
    for (int i = 0; i < user; i++) {
        if (i == user_id)
            continue;
        int j = binarySearch(Data, i, ratingNum[i], item_id);
        if (j != -1) {
            double sim = calculate_PearsonSim(user_id, i);
            if (sim > 0) {
                double baseline = avg + (userAVG[i] - avg) + (get_item_avg(Data[i][j].id) - avg);
                sumSim += sim;
                sumSim_item += sim * (Data[i][j].score - baseline);
            }
        }
    }
    if (sumSim != 0 && sumSim_item != 0) {
        double baseline = avg + (userAVG[user_id] - avg) + (get_item_avg(item_id) - avg);
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
    // read train file and train the model
    read_train_file();
    // predict through model and write result to .txt
    write2text();

    end = clock();
    cout << "[info] overall running time: " << end - start << endl;
    return 0;
}
