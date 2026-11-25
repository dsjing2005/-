#include "threadpool.h"

#include <chrono>
#include <stdexcept>
#include <thread>

/*
有些场景，是希望能够获取线程执行任务得返回值的：
举例：
1 + 。。。 + 3000 
task1 : 1+ ... + 1000
task2 : 1001 + ... + 2000
....
*/
/*
class MyTask : public Task
{
public:
    // 问题一：怎么设计run函数的返回值，可以表示任意的类型
    // C++17 Any类型 《=  可以接收任意的其他类型 (int, double, string...)
    Any run() override
    {
        ulong sum = 0;
        for(ulong i = begin_; i <= end_; ++i)
        {
            sum += i;
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return sum;
    }
    MyTask(ulong begin, ulong end) : begin_(begin), end_(end) {}
private:
    ulong begin_;
    ulong end_;
};
*/

int sum1(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return a + b;
}
int sum2(int a, int b, int c)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return a + b + c;
}
int main()
{
#if 0
    ThreadPool pool;

    pool.setMode(PoolMode::MODE_CACHED);
    pool.start(4);

    // 用户提交任务的时候，应该更简单一点
    /*
        1. pool.submitTask能不能也实现成这样：
        int sum1(int a, int b) {...}
        int sum2(int a, int b, int c) {...}
        thread t1(sum1, 10, 20);
        thread 52(sum2, 1, 3, 6);

        =》 pool.submitTask(sum1, 1, 222000);   可变参模板

        2. 我们自己造了一个Result以及相关的类型，代码挺多
            C++11 线程库  thread   packaged_task(就是函数对象function)  async异步
            使用future来代替Result 节省 线程池代码
    */
   /*
    Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(10001, 20000));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));
    Result res4 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));

    Result res5 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));
    Result res6 = pool.submitTask(std::make_shared<MyTask>(20001, 30000));

    ulong sum1 = res1.get().cast_<ulong>();
    ulong sum2 = res2.get().cast_<ulong>();
    ulong sum3 = res3.get().cast_<ulong>();
    ulong sum4 = res4.get().cast_<ulong>();
    ulong sum5 = res5.get().cast_<ulong>();

    std::cout << (sum1 + sum2 + sum3) << std::endl;
    std::cout << (sum4 + sum5) << std::endl;

    getchar();
    */
#endif 

#if 0
    ThreadPool pool;
    pool.start();
    Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 10000));
    ulong sum1 = res1.get().cast_<ulong>();
    std::cout<< sum1 << std::endl;
#endif 

#if 0
    // DEMO_FIXED
    ThreadPool pool;
    pool.start(2);
    std::future<int> r1 = pool.submitTask(sum1, 1, 2);
    std::future<int> r3 = pool.submitTask(sum2, 3, 2, 1);
    std::future<int> r4 = pool.submitTask(sum1, 4, 2);
    std::future<int> r5 = pool.submitTask(sum1, 5, 2);
    std::future<int> r6 = pool.submitTask(sum1, 6, 2);
    std::future<int> r7 = pool.submitTask(sum1, 6, 2);
    std::future<int> r8 = pool.submitTask(sum1, 6, 2);

    std::cout << r1.get() << std::endl;
    std::cout << r3.get() << std::endl;
    std::cout << r8.get() << std::endl;

#endif

#if 1
    // REPRO_CACHED
    // Occupy the sole worker and fill the one-slot queue.
    // The next submission starts another worker but still rejects that task.
    // Compile with -DMY_DEBUG to see the growth and the idle worker's exit.
    {
        std::promise<void> started;
        auto startedFuture = started.get_future();
        std::promise<void> release;
        auto gate = release.get_future().share();

        ThreadPool pool;
        pool.setMode(PoolMode::MODE_CACHED);
        pool.setTaskQueMaxThreshold(1);
        pool.setThreadMaxThreshold(2);
        pool.start(1);
        auto running = pool.submitTask([&started, gate] {
            started.set_value();
            gate.wait();
        });
        startedFuture.wait();

        auto queued = pool.submitTask([] {});
        auto rejected = pool.submitTask([] {});
        try {
            rejected.get();
            std::cout << "Third submission accepted" << std::endl;
        } catch (const std::runtime_error& e) {
            std::cout << "Third submission: " << e.what() << std::endl;
        }

        release.set_value();
        running.get();
        queued.get();
        // Break on the next output to inspect curThreadSize_ and threads_.size().
        // In the current code they should be 1 and 2 after the idle timeout.
        std::cout << "Waiting for cached worker to exit" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(62));
        std::cout << "Idle timeout observation complete" << std::endl;
    }
#endif

#if 0
    // REPRO_SHUTDOWN
    // Leave scope while a task is still running. Use a shell timeout because
    // the current destructor can race with workers that still use pool members.
    std::cout << "Starting shutdown stress test" << std::endl;
    for (int i = 0; i < 100; ++i) {
        std::promise<void> started;
        auto startedFuture = started.get_future();
        ThreadPool pool;
        pool.start(2);
        auto done = pool.submitTask([&started] {
            started.set_value();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        });
        startedFuture.wait();
        // Deliberately skip done.get() to exercise destruction during the task.
    }
    std::cout << "Shutdown stress test completed" << std::endl;
#endif

#if 0
    // REPRO_BEFORE_START
    // Check readiness with a bounded wait, then start the worker for cleanup.
    {
        ThreadPool pool;
        auto result = pool.submitTask([] { return 42; });
        if (result.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout) {
            std::cout << "Task still pending before start()" << std::endl;
        }
        pool.start(1);
        std::cout << "Result after start(): " << result.get() << std::endl;
    }
#endif

    return 0;
}