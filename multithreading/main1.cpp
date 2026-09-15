#include <iostream>
#include <thread>

void print(int arr[], int n);
void sort(int arr[], int start, int end, int depth);
void merge(int arr[], int left, int mid, int right);

int main() {
  srand((unsigned int) time(nullptr));
  int n = 0;
  std::cin >> n;
  int arr[n];
  for (int i = 0; i < n; ++i)
    arr[i] = rand() % 100000;
  print(arr, n);
  sort(arr, 0, n, 0);
  print(arr, n);
  return 0;
}

void print(int arr[], int n) {
  std::string delim;
  std::cout << "[";
  for (int i = 0; i < n; ++i) {
    std::cout << delim << arr[i];
    delim = ", ";
  }
  std::cout << "]\n";
}
void sort(int arr[], int start, int end, int depth) {
  if (start + 1 >= end)
    return;
  int mid = start + (end - start) / 2;
  if (depth < 2) {
    std::thread left(sort, arr, start, mid, depth + 1);
    std::thread right(sort, arr, mid, end, depth + 1);
    left.join();
    right.join();
    merge(arr, start, mid, end);
  }
  else {
    sort(arr, start, mid, depth + 1);
    sort(arr, mid, end, depth + 1);
    merge(arr, start, mid, end);
  }
}
void merge(int arr[], int left, int mid, int right) {
  int temp[right-left];
  int k = 0;
  int L = left;
  int R = mid;
  while (L < mid && R < right) {
    if (arr[L] < arr[R]) {
      temp[k] = arr[L];
      ++L;
    }
    else {
      temp[k] = arr[R];
      ++R;
    }
    ++k;
  }
  while (L < mid) {
    temp[k] = arr[L];
    ++L;
    ++k;
  }
  while (R < right) {
    temp[k] = arr[R];
    ++R;
    ++k;
  }
  k = 0;
  for (int i = left; i < right; ++i) {
    arr[i] = temp[k];
    ++k;
  }
}
