#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<fstream>
#include<cctype>
#include<map>
#include<cstdlib>
#include<stack>
#include<set>
#include<sstream>
#include<thread>
#include<chrono>
#include<queue>
#include"Formula.h"
#include<unordered_set>
using namespace std;
#define endl "\n"
const double Tsu = 1.0;
const double Thd = 1.0;
const int THREAD_MAX = 8;
namespace util {
    int getNumOfString(const string& s) {//string 转 int
        int ans = 0;
        for (int i = s.length() - 1; i >= 0; i--) {
            if (!(s[i] >= '0' && s[i] <= '9')) return ans;
            ans *= 10;
            ans += s[i] - '0';
        }
        return ans;
    }
    string NumToString(int x) {
        string ans;
        stack<char> st;
        while (x) {
            st.push((char)('0' + x % 10));
            x /= 10;
        }
        while (!st.empty()) {
            ans += st.top();
            st.pop();
        }
        return ans;
    }//锟斤拷锟斤拷转锟街凤拷锟斤拷
};
struct Node
{
    int FPGA_ID;
    int Type;
    string ID;
    bool operator <(const Node& B)const {
        return ID < B.ID;
    }
    bool IsFilpFlop = 0;
    string ClockName;
    double clock=0;
    double delay;
    double clockDelay=0;
    bool IsCLK;
    bool hasPre;
    int father=0;
    bool IsPin=0;
};//Node锟斤拷
struct ClockInfo {
    string ClockName;
    double frequency;
    double getTime() {
        return 1000.0 / frequency;
    }
};//时锟斤拷锟斤拷
struct EDGE
{
    int from;
    int to;
    double delay=0;
    string TDM;
    bool isTDM;
    bool operator <(const EDGE& other)const {
        if (from != other.from) return from < other.from;
        if (to != other.to) return to < other.to;
        if (delay != other.delay) return delay < other.delay;
        if (TDM != other.TDM) return TDM < other.TDM;
        return 0;
    }
};//锟斤拷
class Timer;
struct Graph
{
    vector<EDGE>* G;
    vector<EDGE>* RG;
    vector<Node>* N;
    map<string, double> MapOfClock;//锟斤拷锟街碉拷锟斤拷锟斤拷锟斤拷映锟斤拷
    map<string, int> MapOfNode;//锟斤拷锟街碉拷锟斤拷锟斤拷锟斤拷映锟斤拷
    map<string, string> MapofTDM;
    Graph(int n) {
        G = new vector<EDGE>[n + 1];
        RG = new vector<EDGE>[n + 1];
        N = new vector<Node>();
        N->push_back(Node{});
    }
    ~Graph() {
        delete[] RG;
        delete[] G;
        delete N;
    }
    bool getNodeFromFile(istream& fs) {//锟斤拷锟斤拷诘锟?
        int cnt = 1;
        if (fs.fail()) return 0;
        string temp;
        int FPGA_ID;
        int TYPE;
        while (!fs.eof())
        {
            if (!(fs >> temp)) break;
            if (temp.length() >= 4 && temp.substr(0, 4) == "FPGA") {
                FPGA_ID = util::getNumOfString(temp);
            }
            else if (temp.length() >= 4 && temp.substr(0, 4) == "TYPE") {
                fs >> temp;
                TYPE = util::getNumOfString(temp);
            }
            else {
                if (temp[0] == ':'&&temp.length()==1) continue;
                if (temp[0] == ':') temp = temp.substr(1, temp.length() - 1);
                MapOfNode[temp] = cnt++;
                N->push_back(Node{ FPGA_ID,TYPE,temp });
            }
        }
        return 1;
    }
    bool getNodeInfoFromFile(istream& fs) {//锟斤拷锟斤拷诘锟斤拷锟较?
        if (fs.fail()) return 0;
        string node_id;
        string delay;
        string temp;
        while (!fs.eof()) {
            string line;
            if (!getline(fs, line)) break;
            stringstream ss(line);
            ss >> node_id;
            bool isConfig = 0;
            while (ss >> temp) {
                if (temp[0] == '{') {
                    isConfig = 1;
                    temp = temp.substr(1, temp.length() - 1);
                    if (temp[temp.length() - 1] == '}') temp = temp.substr(0, temp.length() - 1);
                    if (temp == "ff") { (*N)[MapOfNode[node_id]].IsFilpFlop = 1; }
                    else { 
                        (*N)[MapOfNode[node_id]].ClockName = temp; 
                        if (node_id.substr(0, 2) == "gp") (*N)[MapOfNode[node_id]].IsCLK = 1;
                    }
                }
                else if (!isConfig) {
                    delay = util::getNumOfString(temp);
                }
                else {
                    if (temp[temp.length() - 1] == '}') temp = temp.substr(0, temp.length() - 1);
                    if (temp == "ff") { (*N)[MapOfNode[node_id]].IsFilpFlop = 1; }
                    else if (temp == "clk") (*N)[MapOfNode[node_id]].IsCLK = 1;
                    else (*N)[MapOfNode[node_id]].ClockName = temp;
                }
            }
            if ((*N)[MapOfNode[node_id]].ID.substr(0, 2) == "gp") {
                (*N)[MapOfNode[node_id]].IsFilpFlop = 1;
                (*N)[MapOfNode[node_id]].IsPin=1;
            }
            if (!(*N)[MapOfNode[node_id]].IsFilpFlop) (*N)[MapOfNode[node_id]].delay = 0.1;
            else (*N)[MapOfNode[node_id]].delay = 1;
            if((*N)[MapOfNode[node_id]].IsCLK == 1) (*N)[MapOfNode[node_id]].delay = 0;
        }
        //DEBUG
        /*cout<<"-------------------NODE--------------------------\n";
        for(auto x:*N){
            cout<<x.FPGA_ID<<" "<<x.Type<<" "<<x.ID<<" "<<x.ClockName<<" "<<x.IsFilpFlop<<" "<<x.delay<<endl;
        }*/
        return 1;
    }
    bool getClockFromFile(istream& fs) {
        if (fs.fail()) return 0;
        while (!fs.eof())
        {
            string name;
            double fre;
            if (!(fs >> name)) break;
            fs >> fre;
            MapOfClock[name] = fre;
        }
        return 1;
    }
    bool getNetFromFile(istream& fs) {
        if (fs.fail()) return 0;
        string id;
        string temp;
        string pre;
        string tdm;
        bool isTDM=0;
        double delay;
        char ch;
        bool isclockPath=0;
        while (!fs.eof()) {
            tdm = "";
            string line;
            if (!getline(fs, line)) break;
            stringstream ss(line);
            delay = 0;
            ss >> id;
            while (ss >> temp) {
                if (temp == "s" || temp == "l") {
                    if (temp == "s") pre = id;
                }
                else if (temp[0] == '{'||temp[0]=='t') {
                    tdm = temp;
                    string TDMname;
                    string value;
                    isTDM = 1;
                    for (int i = 0; i < temp.size(); i++) {
                        if (temp[i] == 'r') {
                            TDMname = temp.substr(0, i);
                            value = temp.substr(i+1, temp.length() - i);
                            break;
                        }
                    }
                    delay = Formula(MapofTDM[TDMname], stod(value)).calc();
                }
                else {
                    delay = util::getNumOfString(temp);
                }
            }
            if (pre != id) {
                G[MapOfNode[pre]].push_back(EDGE{ MapOfNode[pre],MapOfNode[id],delay,tdm,isTDM });
                RG[MapOfNode[id]].push_back(EDGE{ MapOfNode[id],MapOfNode[pre],delay,tdm,isTDM });
                (*N)[MapOfNode[id]].hasPre = 1;
                (*N)[MapOfNode[id]].father++;
            }
        }
        //DEBUG
        /*cout<<"----------------------EDGE--------------------------\n";
        for(auto i:*N){
            if(G[MapOfNode[i.ID]].size()!=0) cout<<i.ID<<"->  ";
            for(auto j:G[MapOfNode[i.ID]]){
                cout<<(*N)[j.to].ID<<" ";
            }
            if(G[MapOfNode[i.ID]].size()!=0) cout<<endl;
        }*/
        return 1;
    }
    bool getTDMFromFile(istream& fs) {
        string line;
        while (getline(fs, line)) {
            stringstream ss(line);
            string name;
            string formula;
            ss >> name;
            string temp;
            while (ss >> temp) {
                formula += temp;
            }
            MapofTDM[name] = formula;
        }
        return 1;
    }
    friend Timer;//锟斤拷锟斤拷实锟街讹拷锟街凤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
};