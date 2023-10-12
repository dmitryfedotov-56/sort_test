	/***********************************************************/

    #define _CRT_SECURE_NO_WARNINGS

	#include <queue>
	#include <future>
	#include <condition_variable>
	#include <vector>
    #include <iostream>

    using namespace std;

    /***********************************************************/
    /*                         Pool Class                      */
    /***********************************************************/

	typedef function<void()> task_type;     // functor for each task

    class ThreadPool {
    public:
 
        ThreadPool();           
        ~ThreadPool();          
 
        void push_task(function<void()>);    // method to submit a task for execution
       
    private:
       
        void threadFunc();                      // supplementary method

        vector<thread> m_threads;               // threads
       
        mutex m_locker;                         // synchronization
        
        queue<task_type> m_task_queue;          // task queue
        
        condition_variable m_event_holder;      // signal
  
        volatile bool m_work;                   // completion flag
    };

	/***********************************************************/

    ThreadPool::ThreadPool() {
        m_work = true;
        for (int i = 0; i < 4; i++) {
            m_threads.push_back(thread(&ThreadPool::threadFunc, this));
        }
    };

    /***********************************************************/

    ThreadPool::~ThreadPool()
    {
        m_work = false;
        m_event_holder.notify_all();
        for (auto& t : m_threads) {
            t.join();
        }
    };

    /***********************************************************/

    void ThreadPool::push_task(function<void()> f)
    {
        lock_guard<mutex> l(m_locker);       // lock pool
        task_type new_task([=] {f(); });     // create a functor
        m_task_queue.push(new_task);         // to the queue
        m_event_holder.notify_one();         // notify a thread waiting for m_event_holder
    };

    /***********************************************************/

    void ThreadPool::threadFunc() {
        while (true)
        {
            task_type task_to_do;
            {
                unique_lock<mutex> l(m_locker);

                if (m_task_queue.empty() && !m_work) // queue is empty and end of work
                    return;

                // not the end of work but queue is empty
                // wait for a signal, on receiving queue should be not empty

                if (m_task_queue.empty())
                {
                    m_event_holder.wait(l, [&]() {return !m_task_queue.empty() || !m_work; });
                };

                // Woke up, check that it's not the end of the job
                if (!m_task_queue.empty()) {
                    task_to_do = m_task_queue.front();
                    m_task_queue.pop();
                };
            };

            task_to_do();
        };
    };

     /***********************************************************/
    /*                            Sotrer                       */
    /***********************************************************/

    class Quick_Sorter 
    {
    public:
        void sort(int* array, long size);
        int num_Task();
    private:
        std::atomic<int> tasknum = 0;
        void quicksort(int* array, long left, long right, ThreadPool* pool_ptr, bool inpool);
    };

    /***********************************************************/

    int Quick_Sorter::num_Task() { return tasknum; };

    void Quick_Sorter::quicksort(int* array, long left, long right, ThreadPool* pool_ptr, bool inpool)
    {
        if (left >= right) return;

        if (inpool) tasknum++;

        long left_bound = left;
        long right_bound = right;

        long middle = array[(left_bound + right_bound) / 2];

        do {                                            // prepare partitions
            while (array[left_bound] < middle) {
                left_bound++;
            }
            while (array[right_bound] > middle) {
                right_bound--;
            };

            if (left_bound <= right_bound) {
                std::swap(array[left_bound], array[right_bound]);
                left_bound++;
                right_bound--;
            }
        } while (left_bound <= right_bound);

        if (right_bound - left > 10000)                   // sort partitions
        {
            pool_ptr->push_task([=]() { quicksort(array, left, right_bound, pool_ptr, true); });
            pool_ptr->push_task([=]() { quicksort(array, left_bound, right, pool_ptr, true); });
            return;
        }
        else
        {
            quicksort(array, left, right_bound, pool_ptr, false);
            quicksort(array, left_bound, right, pool_ptr, false);
        };
    };

    /***********************************************************/

    void Quick_Sorter::sort(int* array, long size)
    {
        ThreadPool pool;
        quicksort(array, 0, size - 1, &pool, false);
    };

    /***********************************************************/
   /*                            Sotrer                       */
   /***********************************************************/

    #include <conio.h>

    int main()
    {
        Quick_Sorter sorter;
        long size = 100000;
        int* array = new int[size];
/*
        for (long i = 0; i < size; i++) {
            array[i] = rand() % 500000;
        };
*/
        for (long i = 0; i < size; i++)     // reverse order
            array[i] = size - 1 - i;

        sorter.sort(array, size);
        cout << "number of tasks " << sorter.num_Task() << endl;
 
        for (long i = 0; i < size - 1; i++) {
            if (array[i] > array[i + 1]) {
                cout << "Unsorted" << endl;
                break;
            };
        };

        cout << "finished, press any key";
        char c = _getch();
        return 0;
    };

    /***********************************************************/

