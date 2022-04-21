#include <iostream>
#include <thread>
#include <string>
#include <ctime>
#include <vector>
#include <grpc++/grpc++.h>

inline std::mutex mtx1;
inline std::mutex mtx2;
inline std::mutex mtx3;