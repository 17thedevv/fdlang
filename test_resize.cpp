#include <vector>
#include <iostream>
int main() { std::vector<int> v = {1, 2, 3}; v.resize(3, 9); std::cout << v[0] << v[1] << v[2]; v.resize(5, 9); std::cout << v[3] << v[4]; return 0; }