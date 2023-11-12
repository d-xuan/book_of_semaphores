#include <assert.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <semaphore>
#include <stdio.h>
#include <thread>

#define PROJECT_NAME "book_of_semaphores.cpp"

// Rendezvous
// Given threads A, B and instructions a1, a2, b1, b2.
// Ensure a2 < b1 and b2 < a1.
//
void rendezvous_a(std::binary_semaphore &a_done,
                  std::binary_semaphore &b_done) {
  std::cout << "Rendezvous: Executing A1\n";
  a_done.release();
  b_done.acquire();
  std::cout << "Rendezvous: Executing A2\n";
}

void rendezvous_b(std::binary_semaphore &a_done,
                  std::binary_semaphore &b_done) {
  std::cout << "Rendezvous: Executing B1\n";
  b_done.release();
  a_done.acquire();
  std::cout << "Rendezvous: Executing B2\n";
}

void rendezvous() {
  std::binary_semaphore a_done(0);
  std::binary_semaphore b_done(0);
  std::thread thread_a(rendezvous_a, std::ref(a_done), std::ref(b_done));
  std::thread thread_b(rendezvous_b, std::ref(a_done), std::ref(b_done));

  thread_a.join();
  thread_b.join();
}

// Mutex
// Given threads A, B and shared variable `count`, enforce mutual exclusion on
// `count`

void mutex_thread(std::atomic_int32_t &count,
                  std::binary_semaphore &count_mutex) {
  count_mutex.acquire();
  count += 1;
  count_mutex.release();
}

void mutex() {
  std::binary_semaphore count_mutex(1);
  std::atomic_int32_t count = 0;

  std::thread thread_a(mutex_thread, std::ref(count), std::ref(count_mutex));
  std::thread thread_b(mutex_thread, std::ref(count), std::ref(count_mutex));

  thread_a.join();
  thread_b.join();

  assert(count == 2);
}

// Multiplex Given threads T_1, T_2, .. T_n, allow up to but no more than k <= n
// threads to enter a critical section simultaneously.

template <int64_t N>
void multiplex_thread(std::counting_semaphore<N> &multiplex_semaphore) {
  multiplex_semaphore.acquire();
  std::cout << "Multiplex: Entering Critical Region\n";
  std::this_thread::sleep_for(std::chrono::seconds(5));
  std::cout << "Multiplex: Leaving Critical Region\n";
  multiplex_semaphore.release();
}

void multiplex() {
  const int32_t nthreads = 10;
  const int64_t multiplex_capacity = 5;

  std::counting_semaphore<multiplex_capacity> multiplex_semaphore(
      multiplex_capacity);

  std::thread threads[nthreads];

  for (int i = 0; i < nthreads; i++) {
    threads[i] = std::thread(multiplex_thread<multiplex_capacity>,
                             std::ref(multiplex_semaphore));
  }

  for (int i = 0; i < nthreads; i++) {
    threads[i].join();
  }
}

int main(int argc, char **argv) {
  if (argc != 1) {
    printf("%s takes no arguments.\n", argv[0]);
    return 1;
  }
  rendezvous();
  mutex();
  multiplex();
  return 0;
}
