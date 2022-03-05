#define NEW
#ifdef NEW
#include "graph.h"
#include <mutex>
#include "thread_pool.h"
#include "SafeMultiset.h"
#include "TopSet.h"
#ifdef LINUX
#include "memoryshow.h"
#endif
#include <chrono>
double SetupSlack = 0;
double HoldSlack = 0;
double CombinalSlack=0;
mutex out_mutex;
thread_pool task(1);
std::set<int>* ClockTranceSet;
ST1* allClockPath;
struct AnsLine
{
	vector<PathNode> pathRecord;
	vector<PathNode> clockRecord;
	double time;
	bool operator<(const AnsLine& B)const{
		return time<B.time;
	}
};
TopSet<AnsLine> SetupPath;
TopSet<AnsLine> HoldPath;
TopSet<AnsLine> CombinalPath;
template <typename ADVANCER>
struct Functor {
	int parameter2;
	ADVANCER* parameter1;
	vector<Functor> after;
	Functor(ADVANCER* parameter1, int parameter2) {
		this->parameter1 = parameter1;
		this->parameter2 = parameter2;
	}
	inline static thread_pool& subflow=task;
	void operator()();
};//仿函数
template<typename ST,typename Container > //参数一为路径保存的数据结构，参数二为保存该数据结构的数据结构
struct Advancer {//任务执行的策略
	Container status;//所有到达该结点的路径
	mutex _mutex;
	Graph* g;
	Advancer<ST,Container>* allAdvancer;
	int myindex;
	int first; //标识当前状态
	int father;
	int ClockTranceFather;
	bool hasProcessed=0;
	ST1* clockPathRecord;
	Advancer(){}
	template<typename Subflow>
	void startExtend(Subflow& subflow) {
		auto& N=*(g->N);
		auto  G=g->G;
		if (!N.at(myindex).IsPin && N.at(myindex).IsFilpFlop && !N.at(myindex).hasPre) {
			if(N.at(myindex).ClockName!="") return;
			for (auto& x : G[myindex]) {
				(allAdvancer + x.to)->_mutex.lock();
				//ClockSync(myindex,x.to);
				(allAdvancer + x.to)->father--; //下一任务的前驱减少
				if ((allAdvancer + x.to)->father <= 0) subflow.emplace(Functor(allAdvancer + x.to, 0));
				(allAdvancer + x.to)->_mutex.unlock();
			}
			return;
		}
		ST temp = ST{ 0,myindex };
		if(clockPathRecord->arrive_time!=-1){
				temp.arrive_time += clockPathRecord->arrive_time;//添加时钟延迟。
		}
		for (auto& x : G[myindex]) {//遍历该结点所有出边
			temp.arrive_time += x.delay + g->N->at(myindex).delay; //拓展到下一个结点的延迟=该节点的延迟+连接他们的连线的延迟
			if (g->N->at(myindex).delay != 0||1) temp.pathRecord.push_back
			(PathNode{ myindex,temp.arrive_time - x.delay,g->N->at(myindex).delay,"@FPGA" });
			if (x.delay != 0) temp.pathRecord.push_back(PathNode{ myindex,temp.arrive_time,x.delay, x.isTDM ? "@TDM" : "@cable" }); //添加路径记录
			(allAdvancer + x.to)->_mutex.lock();
			allAdvancer[x.to].status.insertElem(temp);//将路径插入下一结点的路径记录中
			//ClockSync(myindex,x.to);
			(allAdvancer + x.to)->father--; //下一任务的前驱减少
			if ((allAdvancer + x.to)->father <= 0) {
				subflow.emplace(Functor(allAdvancer + x.to, 0));
			}
			(allAdvancer + x.to)->_mutex.unlock();
			if (x.delay != 0) temp.pathRecord.pop_back();
			if (g->N->at(myindex).delay != 0) temp.pathRecord.pop_back();
			temp.arrive_time -= x.delay + g->N->at(myindex).delay;
		}
	}
	template<typename Subflow>
	void extend(Subflow& subflow) {
		auto& N=*(g->N);
		auto  G=g->G;
		if (g->N->at(myindex).IsFilpFlop) {  dump(subflow); return; }
		ST temp;
		bool flag=0;
		if (!status.isEmpty()) {
			temp = status.getTop();
			if(temp.arrive_time!=-1) flag=1;
		}
		if(!flag) {
			for (auto& x : g->G[myindex]) {
				(allAdvancer + x.to)->_mutex.lock();
				//ClockSync(myindex,x.to);
				(allAdvancer + x.to)->father--; //下一任务的前驱减少
				if((allAdvancer + x.to)->father<=0) subflow.emplace(Functor(allAdvancer + x.to, 0));
				(allAdvancer + x.to)->_mutex.unlock();
			}
			return;
		}
		for (auto& x : g->G[myindex]) {//遍历该结点所有出边
			bool skip = 0;
			for (int i = 1; i < temp.pathRecord.size(); i++) {
				if (x.to == temp.pathRecord.at(i).index && temp.pathRecord.at(i).discribe == "@FPGA") { skip = 1; break; }
			}
			if (skip) continue;//可能有回路
			temp.arrive_time += x.delay + g->N->at(myindex).delay; //拓展到下一个结点的延迟=该节点的延迟+连接他们的连线的延迟
			if (g->N->at(myindex).delay != 0||1) temp.pathRecord.push_back
			(PathNode{ myindex,temp.arrive_time - x.delay,g->N->at(myindex).delay,"@FPGA" }); 
			if (x.delay != 0) temp.pathRecord.push_back(PathNode{ myindex,temp.arrive_time,x.delay, x.isTDM ? "@TDM" : "@cable" }); //添加路径记录
			(allAdvancer + x.to)->_mutex.lock();
			allAdvancer[x.to].status.insertElem(temp);//将路径插入下一结点的路径记录中
			//ClockSync(myindex,x.to);
			(allAdvancer + x.to)->father--; //下一任务的前驱减少
			if ((allAdvancer + x.to)->father <= 0) { 
				subflow.emplace(Functor(allAdvancer + x.to, 0));
			}
			(allAdvancer + x.to)->_mutex.unlock();
			if (x.delay != 0) temp.pathRecord.pop_back();
			if (g->N->at(myindex).delay != 0) temp.pathRecord.pop_back();
			temp.arrive_time -= x.delay + g->N->at(myindex).delay;
		}
	}
	template<typename Subflow>
	void onFail(vector<PathNode>& pathRecord, Subflow& subflow) {//如果不满足条件，则第二路径也有可能不满足。
		bool is_add = 0;
		vector<Functor<Advancer<ST,Container>>> chain;
		for (int i = 1; i < pathRecord.size(); i++) { //路径上的所有结点，拓展第二路径
			auto& x = pathRecord.at(i);
			if (x.discribe == "@FPGA") {
				chain.push_back(Functor((allAdvancer + x.index), 5));
				//Fail_extend(allAdvancer + x.index, subflow);
			}
		}
		chain.push_back(Functor(this, 2));
		Functor myTask(this,999);
		myTask.after = move(chain);
		subflow.emplace(myTask);
		//subflow.push({ this,2 });
		//dump(g->N->at(myindex).clock - Tsu, subflow);
	}
	template<typename Subflow>
	void Fail_extend(Subflow& subflow) {
		auto now = this;
		if (!now->status.isEmpty()) { now->status.eraseTop();}
		else { return; }
		if (now->status.isEmpty()) return;
		ST temp;
		if (!now->status.isEmpty()) temp = (now->status.getTop());
		else return;
		if (temp.arrive_time == -1) return;
		for (auto& x : g->G[now->myindex]) {
			bool skip = 0;
			for (int i = 1; i < temp.pathRecord.size(); i++) {
				if (x.to == temp.pathRecord.at(i).index && temp.pathRecord.at(i).discribe == "@FPGA") { skip = 1; break; }
			}
			if (skip) continue;
			temp.arrive_time += x.delay + g->N->at(myindex).delay;
			if (g->N->at(myindex).delay != 0||1) temp.pathRecord.push_back
			(PathNode{ myindex,temp.arrive_time - x.delay,g->N->at(myindex).delay,"@FPGA" });
			if (x.delay != 0) temp.pathRecord.push_back(PathNode{ myindex,temp.arrive_time,x.delay, x.isTDM ? "@TDM" : "@cable" });
			allAdvancer[x.to]._mutex.lock();
			allAdvancer[x.to].status.insertElem(temp);
			//ClockSync(myindex,x.to);
			allAdvancer[x.to]._mutex.unlock();
			if (x.delay != 0) temp.pathRecord.pop_back();
			if (g->N->at(myindex).delay != 0)temp.pathRecord.pop_back();
			temp.arrive_time -= x.delay + g->N->at(myindex).delay;
		}
	}
	template<typename Subflow>
	void dump(Subflow& subflow) {//忽略时钟偏斜，按end寄存器时钟来处理
		if(g->N->at(myindex).IsPin&&is_same<ST, ST1>::value){
			if (status.isEmpty()) return;
			auto temp = status.getTop();
			if (temp.arrive_time == -1) return;
			int beginIndex;
			if(temp.pathRecord.size()>0) beginIndex=temp.pathRecord[0].index;
			if(g->N->at(beginIndex).IsPin&&!g->N->at(beginIndex).IsCLK){
				status.eraseTop();
				temp.pathRecord.push_back(PathNode{myindex,0,0,"@end"});
				CombinalSlack+=temp.arrive_time-1;
				AnsLine ok;
				onFail(temp.pathRecord,subflow);
				ok.pathRecord=move(temp.pathRecord);
				ok.time=temp.arrive_time;
				CombinalPath.insert(ok);
				return;
			}
		}
		auto temp = status.getTop();
		if(g->N->at(myindex).IsPin){
			int first=temp.pathRecord.begin()->index;
			g->N->at(myindex).clock=g->N->at(first).clock;
			g->N->at(myindex).ClockName=g->N->at(first).ClockName;
			clockPathRecord->arrive_time = 0;
		}
		if(g->N->at(myindex).IsCLK) return;
		if(g->N->at(myindex).clock==0) return;
		if (clockPathRecord->arrive_time == -1) return;
		bool couldAnalyse=1;
		do{
			if(status.isEmpty()) return;
			auto temp = status.getTop();
			couldAnalyse=1;
			if (temp.arrive_time == -1) {status.eraseTop();couldAnalyse=0;continue;}
			if(clockPathRecord->pathRecord.size()&&clockPathRecord->pathRecord.begin()->index==temp.pathRecord.begin()->index) {
				status.eraseTop();
				couldAnalyse=0;
				continue;
			}
			if(g->N->at(temp.pathRecord.begin()->index).IsCLK) {
				status.eraseTop();
				couldAnalyse=0;
				continue;
			}
		}while(couldAnalyse==0);
		if (is_same<ST, ST1>::value) {
			double clockDelay=0;
			if(clockPathRecord->arrive_time!=-1)  clockDelay=clockPathRecord->arrive_time;
			double limit = g->N->at(myindex).clock-Tsu+clockDelay;
			if (status.isEmpty()) return;
			auto temp = status.getTop();
			if (temp.arrive_time == -1) return;
			status.eraseTop();
			temp.pathRecord.push_back(PathNode{myindex,0,0,"@end"});
			if (temp.arrive_time > limit) {
				SetupSlack += limit - temp.arrive_time;
				AnsLine ok;
				static int okk=1;
				ok.time=limit-temp.arrive_time;
				onFail(temp.pathRecord,subflow);
				ok.pathRecord=move(temp.pathRecord);
				ok.clockRecord=clockPathRecord->pathRecord;
				SetupPath.insert(ok);
			}
		}
		if (is_same<ST, ST2>::value) {
			double limit = Thd;
			if (status.isEmpty()) return;
			auto temp = status.getTop();
			if (temp.arrive_time == -1) return;
			status.eraseTop();
			if (temp.arrive_time < limit) {
				cout<<g->N->at(myindex).ID<<endl;
				for(auto& X:clockPathRecord->pathRecord){
					cout<<g->N->at(X.index).ID<<" "<<X.time<<" "<<X.discribe<<endl;
				}
				cout<<"--\n";
				for(auto& X:temp.pathRecord){
					cout<<g->N->at(X.index).ID<<" "<<X.time<<" "<<X.discribe<<endl;
				}
				cout<<temp.arrive_time<<" "<<limit<<endl;
				cout<<"###\n";
				HoldSlack +=temp.arrive_time-limit;
				AnsLine ok;
				onFail(temp.pathRecord,subflow);
				ok.pathRecord=move(temp.pathRecord);
				ok.clockRecord=clockPathRecord->pathRecord;
				ok.time=temp.arrive_time-limit;
				HoldPath.insert(ok);
			}
		}
	}
	void output(ostream& os, vector<PathNode>& pathRecord) {
		for (auto& x : pathRecord) {
			os << g->N->at(x.index).ID << " " << x.add << " " << x.time << " " << x.discribe << "\n";
		}
		os << g->N->at(myindex).ID << " end\n\n";
	}
	template<typename Subflow>
	void startClockExtend(Subflow& subflow) {
		if(hasProcessed) return;
		hasProcessed=1;
		g->N->at(myindex).IsCLK=1;
		ST1 temp;
		temp = ST1{ 0,myindex };
		for (auto& x : g->G[myindex]) {
			temp.arrive_time += x.delay + g->N->at(myindex).delay;
			if (g->N->at(myindex).delay != 0||1) temp.pathRecord.push_back
			(PathNode{ myindex,temp.arrive_time - x.delay,g->N->at(myindex).delay,"@FPGA" });
			if (x.delay != 0) temp.pathRecord.push_back(PathNode{ myindex,temp.arrive_time,x.delay, x.isTDM ? "@TDM" : "@cable" });
			allAdvancer[x.to]._mutex.lock();
			*(allAdvancer[x.to].clockPathRecord) = temp;
			//allAdvancer[x.to].father--;
			if(g->N->at(x.to).clock==0) {
				g->N->at(x.to).clock=g->N->at(myindex).clock;
				g->N->at(x.to).ClockName=g->N->at(myindex).ClockName;
			}
			//cout<<"clock ok:"<<g->N->at(x.to).ID<<endl;
			if(!g->N->at(x.to).IsFilpFlop) subflow.emplace(Functor(allAdvancer + x.to, 4));
			allAdvancer[x.to]._mutex.unlock();
			if (x.delay != 0) temp.pathRecord.pop_back();
			if (g->N->at(myindex).delay != 0) temp.pathRecord.pop_back();
			temp.arrive_time -= x.delay + g->N->at(myindex).delay;
		}
	}
	template<typename Subflow>
	void clockExtend(Subflow& subflow) {
		if(hasProcessed) return;
		hasProcessed=1;
		if (g->N->at(myindex).IsFilpFlop) { return; }
		if (g->N->at(myindex).ID.substr(0, 2)=="gp") { return; }
		ST1 temp;
		temp = *clockPathRecord;
		for (auto& x : g->G[myindex]) {
			bool skip = 0;
			for (int i = 1; i < temp.pathRecord.size(); i++) {
				if (x.to == temp.pathRecord.at(i).index && temp.pathRecord.at(i).discribe == "@FPGA") { skip = 1; break; }
			}
			if (skip) continue;
			temp.arrive_time += x.delay + g->N->at(myindex).delay;
			if (g->N->at(myindex).delay != 0||1) temp.pathRecord.push_back
			(PathNode{ myindex,temp.arrive_time - x.delay,g->N->at(myindex).delay,"@FPGA" });
			if (x.delay != 0) temp.pathRecord.push_back(PathNode{ myindex,temp.arrive_time,x.delay, x.isTDM ? "@TDM" : "@cable" });
			allAdvancer[x.to]._mutex.lock();
			*(allAdvancer[x.to].clockPathRecord) = temp;
			//allAdvancer[x.to].father--;
			if(g->N->at(x.to).clock==0) g->N->at(x.to).clock=g->N->at(myindex).clock;
			//cout<<"clock ok:"<<g->N->at(x.to).ID<<endl;
			if (g->N->at(x.to).clock == 0) g->N->at(x.to).clock = g->N->at(myindex).clock;
			if (!g->N->at(x.to).IsFilpFlop) subflow.emplace(Functor(allAdvancer + x.to, 4));
			allAdvancer[x.to]._mutex.unlock();
			if (x.delay != 0) temp.pathRecord.pop_back();
			if (g->N->at(myindex).delay != 0) temp.pathRecord.pop_back();
			temp.arrive_time -= x.delay + g->N->at(myindex).delay;
		}
	}
	template<typename Subflow>
	void ProcessClock(Subflow& subflow){
		auto& N=*(g->N);
		auto G=g->G;
		auto RG=g->G;
		for(auto& x:G[myindex]){
			bool success=0;
			if( N.at(x.to).clock==0 && N.at(x.from).clock != 0) success=1;
			if(success) subflow.emplace(Functor(allAdvancer + x.to,6));
		}
		for(auto& x:RG[myindex]){
			bool success=0;
			if( N.at(x.from).clock==0 && N.at(x.to).clock != 0) success=1;
			if(success) subflow.emplace(Functor(allAdvancer+x.from,6));
		}
	}
	template<typename Subflow>
	void startClockAnlyse(Subflow& subflow){
		auto& N=*(g->N);
		auto G=g->G;
		for(auto& x:G[myindex]){
			ClockTranceSet[x.to].insert(myindex);
			//cout<<N.at(x.from).ID<<"---->"<<N.at(x.to).ID<<endl;
			allAdvancer[x.to].ClockTranceFather--;
			if(allAdvancer[x.to].ClockTranceFather<=0){
				subflow.emplace(Functor(allAdvancer+x.to,7));
			}
		}
	}
    template<typename Subflow>
	void ClockAnlyse(Subflow& subflow){
		auto& N=*(g->N);
		auto G=g->G;
		if(N.at(myindex).IsFilpFlop) return;
		if(N.at(myindex).IsPin) return;
		for(auto& x:G[myindex]){
			for(auto y:ClockTranceSet[myindex]){
				ClockTranceSet[x.to].insert(y);
			}
			//cout<<N.at(x.from).ID<<"---->"<<N.at(x.to).ID<<endl;
			allAdvancer[x.to].ClockTranceFather--;
			if(allAdvancer[x.to].ClockTranceFather<=0){
				subflow.emplace(Functor(allAdvancer+x.to,7));
			}
		}
	}
	public:
	set<int> getSet(){
		set<int> ans;
		set<int> book;
		dfs(ans,book);
		return ans;
	}
	void dfs(set<int>& se,set<int>& se2){
		auto* RG=g->RG;
		auto& N=*(g->N);
		if(N.at(myindex).IsFilpFlop&&!se.empty()) return;
		for(auto x:RG[myindex]){
			if(!se2.count(x.to)&&(N.at(x.to).hasPre||N.at(x.to).ClockName!="")){
				se2.insert(x.to);
				if(N.at(x.to).IsFilpFlop) se.insert(x.to);
				allAdvancer[x.to].dfs(se,se2);
			}
		}
	}
};
template<typename ADVANCER>
void Functor<ADVANCER>::operator()() {
	//TODO
	auto Owner = parameter1;
	if (parameter2 == 0) Owner->extend(subflow);
	if (parameter2 == 1) Owner->startExtend(subflow);
	if (parameter2 == 2) { Owner->dump(subflow); }   //TODO
	if (parameter2 == 4 ) { Owner->clockExtend(subflow); }
	if (parameter2 == 3) { Owner->startClockExtend(subflow); }
	if (parameter2 == 5) { Owner->Fail_extend(subflow); }
	if (parameter2 == 6) {Owner->ProcessClock(subflow);}
	if (parameter2 == 7) {Owner->ClockAnlyse(subflow);}
	if (parameter2 == 8) {Owner->startClockAnlyse(subflow);}
	if (!after.empty()) {
		Functor next = Functor(after[0].parameter1, after[0].parameter2);
		next.after = move(after);
		next.after.erase(next.after.begin());
		subflow.emplace(next);
	}
}






////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ST>
void Find(Graph& g, ST* x) {
	if(is_same<ST,ST1>::value){
	Advancer<ST,SafeMultiset<ST>>* advancer = new Advancer<ST,SafeMultiset<ST>>[1000000];
	for (int i = 0; i < 1000000 ; i++) {
		advancer[i].g = &g;
		advancer[i].allAdvancer = advancer;
		advancer[i].myindex = i;
		advancer[i].clockPathRecord = (allClockPath+i);
		if(i<g.N->size()){
			advancer[i].father = g.N->at(i).father;
			advancer[i].ClockTranceFather = g.N->at(i).father;
			if(g.N->at(i).ClockName!="") g.N->at(i).clock = 1000.0 / g.MapOfClock[g.N->at(i).ClockName];
		}
	}
	//ClockPreProcess
	for(int i=1;i<g.N->size();i++){
		if(g.N->at(i).IsCLK){
			cout<<g.N->at(i).ID<<endl;
			Functor(advancer+i,3);
		}
    }
	task.wait_for_tasks();
	/*for(int i=1;i<g.N->size();i++){
		if(g.N->at(i).IsFilpFlop)
		task.emplace(Functor((advancer+i),8));
    }
	task.wait_for_tasks();
	for(int i=1;i<g.N->size();i++){
		ClockTranceSet[i].erase(i);
	}*/
	for(int i=1;i<g.N->size();i++){
		if(advancer[i].clockPathRecord->arrive_time!=-1) continue;
		if(g.N->at(i).IsCLK) continue;
		if(g.N->at(i).IsPin) continue;
		if(g.N->at(i).IsFilpFlop){
			auto C=advancer[i].getSet();
			C.erase(i);
			for(auto x:C){
				//cout<<g.N->at(i).ID<<"---"<<g.N->at(x).ID<<endl;
				/*auto& A=ClockTranceSet[i];
				auto& B=ClockTranceSet[x];*/
				auto A=C;
				A.erase(i);
				auto B=advancer[x].getSet();
				B.erase(x);
				std::set<int> C;
				set_intersection(A.begin(),A.end(),B.begin(),B.end(),inserter(C,C.begin()));
				if(C.size()!=0){
					//cout<<g.N->at(i).ID<<" and "<<g.N->at(x).ID<<endl;
					int clockIndex=-1;
					int MaybeClock=0;
					for(auto x:C){
						//cout<<g.N->at(x).ID<<"!!\n";
						if(g.N->at(x).ClockName!="") {
							clockIndex=x;
							MaybeClock++;
						}
					}
					if(clockIndex!=-1){
						cout<<g.N->at(clockIndex).ID<<"========"<<endl;
						task.emplace(Functor(advancer+clockIndex,3));
						task.wait_for_tasks();
						break;
					}
				}
			}
		}
	}
	//
	for(int i=1;i<g.N->size();i++){
		if(advancer[i].ClockTranceFather<0) cout<<g.N->at(i).ID<<"$$$"<<advancer[i].ClockTranceFather<<"\n";
	}
	/*for(int i=1;i<g.N->size();i++){
		if(g.N->at(i).IsFilpFlop&&advancer[i].clockPathRecord->arrive_time!=-1){
			g.N->at(i).delay+=advancer[i].clockPathRecord->arrive_time; //TODO
		}
	}*/
	for (int i = 1; i < g.N->size(); i++) {
		//if (g.N->at(i).IsCLK) continue;
		if (!(g.N->at(i).IsFilpFlop || g.N->at(i).ID.substr(0, 2) == "gp")) continue;
		if (!g.N->at(i).hasPre && g.N->at(i).ID.substr(0, 2) != "gp") {
			task.emplace(Functor(&advancer[i],1));
			continue;
		}
		task.emplace(Functor(&advancer[i],1));
	}
	task.wait_for_tasks();
	int akkcdb=0;
	for (int i = 1; i < g.N->size(); i++) {
		if(advancer[i].father>=0) continue;
		cout <<g.N->at(i).ID<<" " << advancer[i].father << endl;
		akkcdb++;
	}
	cout<<akkcdb<<endl;
	int cntt=0;
	int cnttt=0;
	for (int i = 1; i < g.N->size(); i++) {
		if(g.N->at(i).IsFilpFlop){cnttt++;}
		if(g.N->at(i).IsPin) continue;
		if(!g.N->at(i).hasPre) continue;
		if(g.N->at(i).IsFilpFlop) if(g.N->at(i).clock==0||advancer[i].clockPathRecord->arrive_time==-1) {
			//cout<<g.N->at(i).ID<<endl;
			cntt++;}
	}
	cout<<cntt<<"!"<<cnttt<<"!!\n";
	delete[] advancer;
	}
	else{
	Advancer<ST,SafeMultiset<ST>>* advancer = new Advancer<ST,SafeMultiset<ST>>[1000000];
	for (int i = 0; i < 1000000 ; i++) {
		advancer[i].g = &g;
		advancer[i].allAdvancer = advancer;
		advancer[i].myindex = i;
		advancer[i].clockPathRecord = (allClockPath+i);
		if(i<g.N->size()){
			advancer[i].father = g.N->at(i).father;
			advancer[i].ClockTranceFather = g.N->at(i).father;
			if(g.N->at(i).ClockName!="") g.N->at(i).clock = 1000.0 / g.MapOfClock[g.N->at(i).ClockName];
		}
	}
	for (int i = 1; i < g.N->size(); i++) {
		//if (g.N->at(i).IsCLK) continue;
		if (!(g.N->at(i).IsFilpFlop || g.N->at(i).ID.substr(0, 2) == "gp")) continue;
		if (!g.N->at(i).hasPre && g.N->at(i).ID.substr(0, 2) != "gp") {
			task.emplace(Functor(&advancer[i],1));
			continue;
		}
		task.emplace(Functor(&advancer[i],1));
	}
	task.wait_for_tasks();
	delete[] advancer;
	}
}
void solve(string FirePath) {
	chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
	Graph* gg = new Graph(1e6);
	Graph& g = *gg;
	string path = "./testcase_update20211029/testdata_10/";
	path=FirePath;
	fstream node_file(path + "design.node", ios::in);
	fstream tdm_file(path + "design.tdm", ios::in);
	fstream info_file(path + "design.are", ios::in);
	fstream clock_file(path + "design.clk", ios::in);
	fstream net_file(path + "design.net", ios::in);
	g.getNodeFromFile(node_file);
	g.getTDMFromFile(tdm_file);
	g.getNodeInfoFromFile(info_file);
	g.getClockFromFile(clock_file);
	g.getNetFromFile(net_file);
	node_file.close();
	info_file.close();
	clock_file.close();
	net_file.close();
	tdm_file.close();
	chrono::steady_clock::time_point t3 = chrono::steady_clock::now();
	chrono::duration<double> time_used0 = chrono::duration_cast<chrono::duration<double>>(t3 - t1);
    cout<<"read file time cost = "<<time_used0.count()<<" seconds."<<endl;
	allClockPath=new ST1[1000000];
	cout<<"file read ready begin to count time\n";
	Find(g, (ST1*)0);
	cout<<"-------------------------\n";
	//Find(g,(ST2*)0);
	fstream out(path+"sta.rpt",ios::out);
	cout<<path+"sta.rpt"<<endl;
	out<<"Setup Slack: "<<SetupSlack<<endl;
	out<<"Hold Slack: "<<HoldSlack<<endl;
	out<<"Combinal: " << CombinalSlack << endl;
	int pathcnt=0;
	for(const auto& x:SetupPath.container){
		out<<"path:"<<++pathcnt<<endl;
		out<<"\tdata arrival time:\n";
		int index;
		int beginIndex=x.pathRecord.begin()->index;
		for(const auto& y:x.pathRecord){
			if(y.discribe=="@end"){
				index=y.index;
				//break;
			}
			out<<"\t"<<g.N->at(y.index).ID<<" "<<y.add<<" "<<y.time<<" "<<y.discribe;
			if(y.discribe=="@FPGA"){
				out<<g.N->at(y.index).FPGA_ID;
			}
			out<<endl;
		}
		out<<"\tTC:\n";
		for(const auto& y:allClockPath[beginIndex].pathRecord){
			out<<"\t"<<g.N->at(y.index).ID<<" "<<y.add<<" "<<y.time<<" "<<y.discribe;
			if(y.discribe=="@FPGA"){
				out<<g.N->at(y.index).FPGA_ID;
			}
			out<<endl;
		}
		out<<"\tdata expected time:\n";
		out<<"\t"<<g.N->at(index).ClockName<<" rise edge "<<g.N->at(index).clock<<endl;
		double clocknow=g.N->at(index).clock;
		for(const auto& y:x.clockRecord){
			out<<"\t"<<g.N->at(y.index).ID<<" "<<y.add<<" "<<y.time<<" "<<y.discribe;
			if(y.discribe=="@FPGA"){
				out<<g.N->at(y.index).FPGA_ID;
			}
			out<<endl;
			clocknow+=y.add;
		}
		out<<"\t"<<g.N->at(index).ID<<" "<<"Tsu -1.0 "<<clocknow-1.0<<endl;
		out<<"--------------------------------\n";
		out<<"setup slack "<<x.time;
		out<<endl;
		out<<"============================================================================================\n";
	}
	for(const auto& x:HoldPath.container){
		out<<"path:"<<++pathcnt<<endl;
		out<<"\tdata arrival time:\n";
		int index;
		for(const auto& y:x.pathRecord){
			if(y.discribe=="@end"){
				index=y.index;
				break;
			}
			out<<"\t"<<g.N->at(y.index).ID<<" "<<y.add<<" "<<y.time<<" "<<y.discribe;
			if(y.discribe=="@FPGA"){
				out<<g.N->at(y.index).FPGA_ID;
			}
			out<<endl;
		}
		out<<"\tdata expected time:\n";
		double clocknow=0;
		for(const auto& y:x.clockRecord){
			out<<"\t"<<g.N->at(y.index).ID<<" "<<y.add<<" "<<y.time<<" "<<y.discribe;
			if(y.discribe=="@FPGA"){
				out<<g.N->at(y.index).FPGA_ID;
			}
			out<<endl;
			clocknow+=y.add;
		}
		out<<"\t"<<g.N->at(index).ID<<" "<<"Thd +1.0 "<<clocknow+1.0<<endl;
		out<<"--------------------------------\n";
		out<<"hold slack "<<x.time;
		out<<endl;
		out<<"============================================================================================\n";
	}
	for(const auto& x:CombinalPath.container){
		double clocknow=-1.0;
		bool first=1;
		for(const auto& y:x.pathRecord){
			out<<"\t"<<g.N->at(y.index).ID<<" "<<y.add<<" "<<y.time<<" "<<y.discribe;
			if(y.discribe=="@FPGA"){
				out<<g.N->at(y.index).FPGA_ID;
			}
			out<<endl;
			first=0;
			clocknow+=y.add;
		}
		out<<"--------------------------------\n";
		out<<"Combinal Port Delay: "<<clocknow;
		out<<endl;
		out<<"============================================================================================\n";
	}
	delete[] allClockPath;
	delete gg;
	chrono::steady_clock::time_point t2 = chrono::steady_clock::now();
	chrono::duration<double> time_used = chrono::duration_cast<chrono::duration<double>>(t2 - t3);
    cout<<"sta time cost = "<<time_used.count()<<" seconds."<<endl;
	#ifdef LINUX
	auto memMax=get_memory_by_pid(getpid());
	cout<<"memory cost = "<<memMax<<" KB.\n";
	#endif
}
int main(int argc, char** argv) {
	string filePath=argv[1];
	solve(filePath);
}
#endif // NEW