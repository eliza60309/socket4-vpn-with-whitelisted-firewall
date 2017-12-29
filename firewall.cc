#include <bits/stdc++.h>

using namespace std;

string read_until(const char *arr, const char *terminal, int &cursor)
{
	char str[1024] = {};
	for(int i = 0; i < 1024; i++)
	{
		if(arr[cursor] == '\0')return string(str);
		for(int j = 0;terminal[j] != '\0'; j++)
		{
			if(arr[cursor] == terminal[j])
			{
				cursor++;
				return string(str);
			}
		}
		str[i] = arr[cursor];
		cursor++;
	}
}

int main()
{
	fstream f("socks.conf");
	vector<vector<int> >firewall;
	while(!f.eof())
	{
		string in;
		f >> in;
		if(in == "permit")
		{
			f >> in;
			if(in == "c")
			{
				f >> in;
				int cursor = 0;
				vector<int> tmp;
				for(int i = 0; i < 4; i++)
				{
					string s = read_until(in.c_str(), ".", cursor);
					if(s == "*")tmp.push_back(-1);
					else
					{
						int byte = 0;
						for(int i = 0; i < s.size(); i++)
						{
							byte *= 10;
							byte += s[i] - '0';
						}
						tmp.push_back(byte);
					}
				}
				firewall.push_back(tmp);
			}
		}
	}
	for(int i = 0; i < firewall.size(); i++)
	{
		for(int j = 0; j < 4; j++)
		{
			cout << firewall[i][j] << " ";
		}
		cout << endl;
	}
}
