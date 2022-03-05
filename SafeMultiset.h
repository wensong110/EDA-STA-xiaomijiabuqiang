#include<set>
#include<mutex>
struct PathNode
{
	int index;
	double time;
	double add;
	string discribe;
};//·����¼
struct ST1
{
	double arrive_time = -1;
	int root;
	vector<PathNode> pathRecord;
	bool operator <(const ST1& other) const {
		if (arrive_time != other.arrive_time) return arrive_time > other.arrive_time;
		return root > other.root;
	}
};//����ӳ�·���洢�ṹ
struct ST2
{
	double arrive_time = -1;
	int root;
	vector<PathNode> pathRecord;
	bool operator <(const ST2& other) const {
		if (arrive_time != other.arrive_time) return arrive_time < other.arrive_time;
		return root < other.root;
	}
};//��С�ӳ�·���洢�ṹ
template<typename T>
struct SafeMultiset {
    mutex mymutex;
    multiset<T> Set;
    bool isEmpty() {
        mymutex.lock();
        bool ans;
        ans = Set.empty();
        mymutex.unlock();
        return ans;
    }
    void eraseTop() {
        mymutex.lock();
        if (!Set.empty()) Set.erase(Set.begin());
        mymutex.unlock();
    }
    void insertElem(const T& elem) {
        mymutex.lock();
        Set.insert(elem);
        mymutex.unlock();
    }
    T getTop() {
        mymutex.lock();
        if (!Set.empty()) { 
            auto ans = *(Set.begin());
            mymutex.unlock();
            return ans;
        }
        else { mymutex.unlock();  return T(); }
    }
};