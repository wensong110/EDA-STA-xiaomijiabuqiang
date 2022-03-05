#pragma once
#include<string>
#include<vector>
#include<stack>
#include<iostream>
using namespace std;
struct ST
{
    string str;
    double num;
    int type;
    ST(string str1) {
        str = str1;
        type = 1;
    }
    ST(double num1) {
        num = num1;
        type = 2;
    }
};
double DIntToDouble(int a, int b) { //����intתdouble
    double ans = a;
    double afterDot = b;
    while (b) {
        b /= 10;
        afterDot /= 10.0;
    }
    ans += afterDot;
    return ans;
}
struct Formula {
    stack<double> shu;
    stack<string> fu;
    vector<ST> part;
    vector<ST> houzhui;
    string formula;
    Formula(string formula1, double n1);
    double calc();
    double n;
    static void trim(string& s){
        int index = 0;
        if( !s.empty()){
            while( (index = s.find(' ',index)) != string::npos){
                s.erase(index,1);
            }
        }
    }
};
enum STATUS {
    INIT = 0,
    FUHAO = 1,
    SHUZI1 = 2,
    SHUZI2 = 3,
};
Formula::Formula(string formula1, double n1) :formula(formula1), n(n1) {
    int status = INIT;
    int beforeDot = 0;
    int afterDot = 0;
    trim(formula);
    string fuhao = "";
    for (int i = 0; i < formula.length(); i++) {
        if (formula[i] == 'r') {
            if (status == FUHAO) {
                part.push_back(fuhao);
                fuhao = "";
            }//push the fuhao into stack.
            status = INIT;
            part.push_back(ST(n));
        }
        else if (formula[i] != '.' && !isdigit(formula[i])) {
            if (status == SHUZI1 || status == SHUZI2) {
                part.push_back(ST(DIntToDouble(beforeDot, afterDot)));
                beforeDot = 0;
                afterDot = 0;
            }//push the number to the stack.
            if (status == FUHAO) {
                part.push_back(fuhao);
                fuhao = "";
            }
            status = FUHAO;
            fuhao += formula[i];
        }
        else if (isdigit(formula[i])) {
            if (status == FUHAO || status == SHUZI1 || status == INIT) {
                if (status == FUHAO) {
                    part.push_back(fuhao);
                    fuhao = "";
                }//push the fuhao into stack.
                status = SHUZI1;
                beforeDot *= 10;
                beforeDot += formula[i] - '0';
            }
            else if (status == SHUZI2) {
                status = SHUZI2;
                afterDot *= 10;
                afterDot += formula[i] - '0';
            }
        }
        else if (formula[i] == '.') {
            status = SHUZI2;
        }
    }
    if (status == FUHAO) {
        part.push_back(ST(fuhao));
        fuhao = "";
    }
    else if (status == SHUZI1 || status == SHUZI2) {
        part.push_back(ST(DIntToDouble(beforeDot, afterDot)));
        beforeDot = 0;
        afterDot = 0;
    }
}
double Formula::calc() {
    /*for(auto x:part){
        if(x.type==2) cout<<x.num<<"-";
        else cout<<x.str<<"-";
    }*/
    //cout<<endl;
    for (auto x : part) {
        if (x.type == 2) houzhui.push_back(x);
        else {
            string ok = x.str;
            if (ok == "(") fu.push(ok);
            else if (fu.empty()) fu.push(ok);
            else if (ok == ")") {
                while (fu.top() != "(") {
                    houzhui.push_back(ST{ fu.top() });
                    fu.pop();
                }
                fu.pop();
            }
            else if ((ok == "*" || ok == "/") && (fu.top() == "+" || fu.top() == "-")) {
                fu.push(ok);
            }
            else {
                int level1 = -1;
                int level2 = -1;
                if (ok == "+" || ok == "-") level1 = 1;
                if (ok == "*" || ok == "/") level1 = 2;
                if (fu.top() == "+" || fu.top() == "-") level2 = 1;
                if (fu.top() == "*" || fu.top() == "/") level2 = 2;
                while (!fu.empty() && level1 != -1 && level2 != -1 && level1 <= level2) {
                    houzhui.push_back(ST{ fu.top() });
                    fu.pop();
                    if (fu.empty()) break;
                    if (fu.top() == "+" || fu.top() == "-") level2 = 1;
                    else if (fu.top() == "*" || fu.top() == "/") level2 = 2;
                    else level2 = -1;
                }
                fu.push(ok);
            }
        }
    }
    while (!fu.empty()) {
        houzhui.push_back(ST{ fu.top() });
        fu.pop();
    }
    for (auto x : houzhui) {
        /*if(x.type==2) cout<<x.num<<endl;
        else cout<<x.str<<endl;*/
        if (x.type == 2) shu.push(x.num);
        else {
            double v1, v2;
            v1 = shu.top();
            shu.pop();
            v2 = shu.top();
            shu.pop();
            string op = x.str;
            if (op == "+") shu.push(v2 + v1);
            if (op == "-") shu.push(v2 - v1);
            if (op == "*") shu.push(v2 * v1);
            if (op == "/") shu.push(v2 / v1);
        }
    }
    return shu.top();
};