#include <assert.h>
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

void mutex_thread(int32_t &count, std::binary_semaphore &count_mutex) {
  count_mutex.acquire();
  count++;
  count_mutex.release();
}

void mutex() {
  std::binary_semaphore count_mutex(1);
  int32_t count = 0;

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
  std::this_thread::sleep_for(std::chrono::seconds(2));
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

// Two phase barrier class
class Barrier {
public:
  Barrier(int32_t n)
      : m_nthreads(n), m_mutex(std::binary_semaphore(1)),
        m_turnstile(std::counting_semaphore<1>(0)),
        m_turnstile2(std::counting_semaphore<1>(0)){};

  void acquire() {
    this->phase_1();
    this->phase_2();
  }

private:
  const int32_t m_nthreads;
  int32_t m_count = 0;
  std::binary_semaphore m_mutex;
  std::counting_semaphore<1> m_turnstile;
  std::counting_semaphore<1> m_turnstile2;

  void phase_1() {
    m_mutex.acquire();
    m_count++;
    if (m_count == m_nthreads) {
      m_turnstile.release(m_nthreads);
    }
    m_mutex.release();
    m_turnstile.acquire();
  };

  void phase_2() {
    m_mutex.acquire();
    m_count--;
    if (!m_count) {
      m_turnstile2.release(m_nthreads);
    }
    m_mutex.release();
    m_turnstile2.acquire();
  }
};

// Barrier (generalised Rendezvous)
void barrier_thread(Barrier &barrier) {
  for (int32_t i = 0; i < 5; i++) {
    barrier.acquire();
    // Do work
    printf("Barrier: Doing work on iteration %d\n", i);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void barrier() {
  const int32_t nthreads = 10;
  std::thread threads[nthreads];
  Barrier barrier(nthreads);

  for (int i = 0; i < nthreads; i++) {
    threads[i] = std::thread(barrier_thread, std::ref(barrier));
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
  barrier();
  return 0;
}
