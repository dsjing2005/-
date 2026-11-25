#include "threadpool.h"

#include <chrono>
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

#if 1
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
    return 0;
}