#include "threadpool.h"

#include <chrono>

const int TASK_MAX_THRESHHOLD = 4;
const int THREAD_MAX_THRESHHOLD = 100;


// 线程池构造和析构
ThreadPool::ThreadPool()
    : initThreadSize_(0)
    , taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MODE_FIXED)
    , quit_(false)
    , idleThreadSize_(0)
    , threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
    , curThreadSize_(0)
{}

ThreadPool::~ThreadPool()
{
    quit_ = true;
    notEmpty_.notify_all();     // 唤醒所有消费线程

    // 现在线程池中的线程有两种状态： 1. 阻塞 && 2.正在执行中

    // threads_ 成员会在 ~ThreadPool 之后析构
    // ~Thread 里 join， 等待所有工作线程真正退出  
}

// 执行线程执行函数
void ThreadPool::threadHandler()
{
    while(1)
    {
        Task task;
        {
            // 获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);
            // cache模式下，有可能已经创建了很多的线程，但是空闲时间超过60s，应该把多余的线程
            // 结束回收掉（超过initThreadSize_数量的线程要进行回收）
            // 当前时间 = 上一次线程执行时间 > 60s  回收机制 =》 没任务
            if(poolMode_ == PoolMode::MODE_FIXED)
            {
                // 等待notEmpty调教
                notEmpty_.wait(lock, [this]()->bool{
                    return taskQue_.size() > 0 || quit_ == true;
                });

                // 线程池结束了
                if(quit_ == true && taskQue_.empty())
                {
                    return;     // 结束线程函数， 隔离了这个线程，还需要 join才结束这个线程
                }
            }
            else    // cached 模式
            {
                bool hasTask = notEmpty_.wait_for(lock, std::chrono::seconds(60), [this]()->bool{
                    return !taskQue_.empty() || quit_ == true;
                });

                // 1. 线程池要关闭 quit_ == true： 先处理 quit
                if(quit_ == true && taskQue_.empty())
                {
                    return;
                }

                // 2. 超时且没任务： taskQue_没有任务，因此 线程池在干等，考虑回收自己
                if(!hasTask)
                {
                    if(curThreadSize_ > initThreadSize_)    // 超出 初始线程数
                    {
                        // 释放threads_
                        --curThreadSize_;       // 线程总数 - 1
                        --idleThreadSize_;      // 空闲线程 - 1
                        return;
                    }
                    continue;       // 没有超出 initThreadSize 就是没任务，继续等任务
                }
            }

            // 该线程在规定时间拿到了任务
            // 空闲线程数量 --
            -- idleThreadSize_;

            // 取一个任务出来，消费 =》 notFull通知
            task = taskQue_.front();
            taskQue_.pop();
            std::cout << std::this_thread::get_id() << "获取任务成功" << std::endl;

            // 取出一个任务，进行通知
            if(taskQue_.size() > 0) notEmpty_.notify_all();

            // 取出一个任务，进行通知，通知可以继续提交生成任务
            notFull_.notify_all();

        }   // 就应该在这释放掉

        // 当前线程负责执行这个任务
        if(task != nullptr)
        {
            //task->run();        // 执行任务，把任务的返回值给到Result
            //task->exec();
            task();     // 执行function<void()>函数对象
        }

        // 线程执行完成，空闲线程数量 ++
        ++ idleThreadSize_;
    }
}

// 开启线程池
void ThreadPool::start(int initThreadSize)
{
    // 记录初始线程个数
    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;

    //创建线程对象
    for(int i = 0; i < initThreadSize_; ++i)
    {
        auto th = std::make_unique<Thread>(std::bind(&ThreadPool::threadHandler, this));
        threads_.emplace_back(std::move(th));
    }

    // 启动所有线程
    for(int i = 0; i < initThreadSize_; ++i)
    {
        threads_[i]->start();
        ++idleThreadSize_;
    }
}

// 设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
    // 线程池已创建线程，不应该修改 启动模式
    if(curThreadSize_ > 0) return;
    poolMode_ = mode;
}

// 设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
    taskQueMaxThreshHold_ = threshhold;
}

void ThreadPool::setThreadMaxThreshHold(int threadsize)
{
    if(curThreadSize_ > 0 || poolMode_ != PoolMode::MODE_CACHED) return;
    threadSizeThreshHold_ = threadsize;
}


/*
//-----------------------------Result-----------------------------
Result::Result(std::shared_ptr<Task> task, bool isValied)
    : task_(task)
    , isValied_(isValied)
{
    task_->setResult(this);     // 将自己地址 交给 Task，因此 Taks::result_
}
*/
