// File: main2.cpp
// Author: Ryan Harvey
//
// Demonstration of the differences between 
// parallelism and concurrency
//
// Concurrent: multiple threads of control
// (AKA independent sequences of work)
//
// Parallel: multiple pieces of work
// executing simultaneously
#include <iostream>
#include <thread>

int f(int n) {
  return n;
}

int g(int n) {
  return 2 * n;
}


void map(int (*func)(int), int arr[], int start, int end) {
  for (int i = start; i < end; ++i) {
    arr[i] = func(arr[i]);
  }
}

void not_concurrent_not_parallel(int arr[], int n) {
  map(f, arr, 0, n);
}
void parallel_not_concurrent(int arr[], int n) {
  int mid = n / 2;
  std::thread t0(map, f, arr, 0, mid);
  std::thread t1(map, f, arr, mid, n);
  t0.join();
  t1.join();
}
void concurrent_not_parallel(int arr[], int n) {
  int mid = n / 2;
  map(f, arr, 0, mid);
  map(g, arr, mid, n);
}
void concurrent_and_parallel(int arr[], int n) {
  int mid = n / 2;
  std::thread t0(map, f, arr, 0, mid);
  std::thread t1(map, g, arr, mid, n);
  t0.join();
  t1.join();
}

int main() {
  int n = 500;
  int arr[n];

  // not_concurrent_not_parallel(arr, n);
  // parallel_not_concurrent(arr, n);
  // concurrent_not_parallel(arr, n);
  // concurrent_and_parallel(arr, n);

  return 0;
}
