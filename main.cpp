#include<bits/stdc++.h>
#include<fstream>
using namespace std;

int main() {
	string path = "D:\\Desktop\\tests.txt";
	ofstream out(path, ios::app);
	if (!out) {
		cerr << "文件打开失败";
		return -1;
	}
	out << "写入第一行" << '\n';
	out << "second line" << '\t' << "hi\n" ;
	cout << "文件写入成功";
	out.close();

	ifstream in(path);
	if (!in) {
		cout << "读入文件打开失败";
	}
	string line;
	while (getline(in, line)) {
		cout << line << endl;
	}
	

	return 0;
}