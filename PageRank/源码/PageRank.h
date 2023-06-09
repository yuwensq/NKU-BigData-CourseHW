#ifndef __PAGERANK_H_
#define __PAGERANK_H_

#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>

typedef long double rank_type;

class PageRank
{
private:
    std::string input_file;
    std::string output_file;
    double beta;
    int iteration;
    int block_num;
    int topn;
    int thread_num;

    /* data structure */
    std::unordered_map<int, std::unordered_set<int>> graph;
    std::unordered_map<int, rank_type> node_rank;
    /**
     * 处理数据
     */
    void process_data();
    /**
     * 迭代更新rank值
     */
    void iter(std::unordered_map<int, rank_type> &);
    /**
     * page rank过程
     */
    void page_rank();
    /**
     * 保存结果
     */
    void save_result();

public:
    PageRank(double beta, int block_num, int iteration,
             int topn = 100, int thread_num = 1,
             std::string input_file = "Data.txt",
             std::string output_file = "Result.txt");
    void process();
};

#endif