#include <iostream>
#include <thread>

void foo() { // try i < 999
  std::cout << "in foo() ..." << std::endl;
  for (int i = 0; i < 500000; ++i) {}
  std::cout << "out foo() ..." << std::endl;
}

int main() {
  unsigned int amnt = std::thread::hardware_concurrency();
  std::cout << 
    "amount of concurrent logical processor\
 threads my hardware can run: " 
    << amnt << std::endl;
  std::cout << "t0 thread begun ..." << std::endl;
  std::thread t0(foo);
  std::cout << "t1 thread begun ..." << std::endl;
  std::thread t1(foo);
  std::cout << "t2 thread begun ..." << std::endl;
  std::thread t2(foo);
  std::cout << "t3 thread begun ..." << std::endl;
  std::thread t3(foo);

  t0.join();
  std::cout << "t0 thread closed ..." << std::endl;
  t1.join();
  std::cout << "t1 thread closed ..." << std::endl;
  t2.join();
  std::cout << "t2 thread closed ..." << std::endl;
  t3.join();
  std::cout << "t3 thread closed ..." << std::endl;

  return 0;
}
