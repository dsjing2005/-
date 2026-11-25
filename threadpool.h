#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <iostream>
#include <future>

  

/*
// Any类型，可以接收任意数据的类型
class Any
{
public:
    Any() = default;
    ~Any() = default;

    Any(const Any&) = delete;       // unique_ptr 左值引用 delete
    Any& operator=(const Any&) = delete;  // unique_ptr 左值引用 delete

    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    // 这个构造函数可以让Any类型接收任意其他类型
    template<typename T,
            typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Any>>>
    Any(T data) 
    : base_(std::make_unique<Derived<std::decay_t<T>>>(std::forward<T>(data))) 
    {}

    // 这个方法能把Any对象里面存储的data数据提取出来
    template<typename T>
    T cast_() const
    {
        // 我们怎么从base_找到它所指向的Derived对象，从它里面取出data成员变量
        // 使用dynamic_cast来完成 
        Derived<T>* pd = dynamic_cast<Derived<T>*>(base_.get());
        if(pd == nullptr)
        {
            throw "type id unmatch!";
        }
        else return pd->getData();
    }

private:
    // 基类类型
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    // 派生类类型
    template<typename T>
    class Derived : public Base
    {
    public:
        Derived(T&& data) : data_(std::forward<T>(data)) {}
        const T& getData() const { return data_; }
    private:
        T data_;
    };
private:
    std::unique_ptr<Base> base_;
};
*/

/*
// 实现一个信号量类
class Semaphore
{
public: 
    Semaphore(int limit = 0) : resLimit_(limit) {}
    ~Semaphore() = default;

    // 获取一个信号量资源
    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待幸信号量有资源，没有资源的话，会阻塞当前线程
        cond_.wait(lock, [this]()->bool{
            return resLimit_ > 0;
        });
        --resLimit_;
    }

    // 增加一个信号量资源
    void post()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        ++resLimit_;
        cond_.notify_all();
    }

private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};
*/

/*
class Task;     // Task类型的前置声明
*/


/*
Result 和 Taks  用“双向指针” 建立 Result 和 Task的关联 ：
正向： Result 持有 std::shared_ptr<Task> task_， 管理Task的生命周期（Result活着，Task就不会被销毁）
反向： Task持有 Result* result_（裸指针），用于回写
注册时机：  Result 构造函数里 task_->setResult(this)  把自己地址交给 Task。
            这就是“把回调对象注册到执行者”的经典手法，类似 promise  和  std::future
对象创建：  Result res = submitTask(...) 
            1. std::make_shared<MyTask> 创建Task对象，此时 Task::result_ = nullptr
            2. summitTaks 里 return Result(sp, true) 构造Result，在构造函数体里
                task_->setResult(this)把Result地址注册给Task
*/

/*
Result <=> future
// 实现接收提交到线程池的task任务执行完成后的返回值类型Result
class Result
{
public:
    Result(std::shared_ptr<Task> task, bool isValied = true);
    ~Result() = default;

    // setVal方法， 获取任务执行完的返回值
    void setVal(Any any)
    {
        // 存储task的返回值
        this->any_ = std::move(any);
        sem_.post();        // 已经获取了任务的返回值，增加信号量资源

    }

    // get方法， 用户调用这个方法获取task的返回值
    Any get()
    {
        if(!isValied_) 
        {
            return Any();
        }
        sem_.wait();        // task任务如果没有执行完，这里会阻塞用户线程
        return std::move(any_);
    }
private:
    Any any_;       // 存储任务的返回值 
    Semaphore sem_;  // 线程通信，信号量
    std::shared_ptr<Task> task_;       // 指向对应获取返回值的任务对象
    std::atomic_bool isValied_;     // 返回值是否有效
};
*/

/*
// 任务抽象基类
class Task
{
public:
    // 用户可以自定义任意任务类型，从Task继承，重写run方法，实现自定义任务处理
    Task() : result_(nullptr) {}
    virtual ~Task() = default;
    // 返回任务结束的 值
    void exec()
    {
        if(result_ != nullptr) result_->setVal(run());
    }
    void setResult(Result* res)
    {
        result_ = res;
    }
    virtual Any run() = 0;
private:
    Result* result_;        
};
*/

class Thread
{
public:
    using ThreadCallback = std::function<void()>;		// 线程启动需要执行的回调函数 <= ThreadPool指定的
    Thread(ThreadCallback threadCallback)
        : threadCallback_(threadCallback)
    {}
    ~Thread()
    {
        if(thread_.joinable()) thread_.join();
    }
    
    // 启动线程
    void start()
    {
        // 创建一个线程，来执行ThreadPool 需要 Thread执行的 回调函数
        thread_ = std::thread([this]()->void{
           	if(threadCallback_ != nullptr) threadCallback_(); 
        });
    }
private:
	std::thread thread_;	// 类成员变量的声明，本身不启动任何线程———— 默认构造的std::thread不关联任何线程
    ThreadCallback threadCallback_;
};

enum class PoolMode
{
	MODE_FIXED,		// 固定数量的线程
    MODE_CACHED		// 可动态增长、动态删除
};

class ThreadPool
{
public:
    // 构造和析构函数
    ThreadPool();
    ~ThreadPool();
    
    void start(int initThreadSize = std::thread::hardware_concurrency());	// 开启线程池
    
    void setMode(PoolMode poolMOde);		// 设置线程池工作模式
    
    void setTaskQueMaxThreshold(int threshold);		// 设置task任务队列上限阈值
    
    void setThreadMaxThreshold(int threadsize);		// 设置线程池线程数量上限
    
    // 给线程池提交任务 =》 使用可变惨模板 让 submitTask可以接收任意任务函数 和 任意数量的 参数
    template<typename Func, typename... Args>
    auto submitTask(Func func, Args&&... args) -> std::future<decltype(func(args...))>;
    
    // mutex atomic condition_variable  =》  不可拷贝、不可移动
    // 1.显示禁止拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    // 2.显示禁止移动
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
private:
    void threadHandler();		// 线程启动时线程池指定的 回调函数
private:
// 线程相关：
    std::vector<std::unique_ptr<Thread>> threads_;	// 线程列表
    int threadSizeThreshold_;		// 线程池的线程上限
    int initThreadSize_;		// 初始化线程数 （默认线程数） =》 作为Cached模式下的 底线
    // 用于Cached 模式下的 多余线程回收
    std::atomic_int curThreadSize_;		// 当前线程数量		
    std::atomic_int idleThreadSize_;		// 空闲线程数量		// 用于Cahced下的 线程扩充
    
// Task任务相关：
    using Task = std::function<void()>;		// 不明确用户提交任务的参数 和 返回值 类型，因此使用 void ( ) 来完成抽象
    std::queue<Task> taskQue_;		// 任务队列 =》 存放用户给的函数对象 最上层 抽象
    std::mutex taskQueMtx_;			// 互斥锁： 保证任务队列的线程安全  
   	// 互斥锁同样保证了该类 ThreadPool 不可拷贝 不可移动
    int taskQueMaxThreshold_;		// 任务队列最大长度
   
// 运行模式：
    PoolMode poolMode_;
 
// 线程间通信，生产者 <-> 消费者
    std::condition_variable notFull_;		// 任务队列 不满 信号
    std::condition_variable notEmpty_;		// 任务队列 不空 信号

// 线程池停止运行标志：
    std::atomic_bool quit_;		// quit_ == true 表示 线程池停止运行
};

//----------------------------------------全局变量--------------------------------------
const int TASK_MAX_THRESHOLD = 4;
const int THREAD_MAX_THRESHOLD = 100;

//----------------------------------------ThreadPool-----------------------------------
ThreadPool::ThreadPool()
    : initThreadSize_(0)
    , curThreadSize_(0)
    , idleThreadSize_(0)
    , taskQueMaxThreshold_(TASK_MAX_THRESHOLD)
    , poolMode_(PoolMode::MODE_FIXED)
    , threadSizeThreshold_(THREAD_MAX_THRESHOLD)
    , quit_(false)
{}

ThreadPool::~ThreadPool()
{
    quit_ = true;
    notEmpty_.notify_all();
}

void ThreadPool::setMode(PoolMode poolMode)		// 设置线程池工作模式
{
    if(curThreadSize_ > 0) return;		// 线程池开始运行，无法做出模式修改
    poolMode_ = poolMode;
}
void ThreadPool::setTaskQueMaxThreshold(int threshold)		// 设置task任务队列上限阈值
{
    taskQueMaxThreshold_ = threshold;
}
void ThreadPool::setThreadMaxThreshold(int threadsize)		// 设置线程池线程数量上限
{
    threadSizeThreshold_ = threadsize;
}

// 开启线程池
void ThreadPool::start(int initThreadSize)
{
    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;
    idleThreadSize_ = initThreadSize;
    
    for(int i = 0; i < initThreadSize; ++i)
    {
        // make_unique 返回一个临时变量 unique_ptr ，unique_ptr有 右值 拷贝和移动函数
        threads_.emplace_back(std::make_unique<Thread>(std::bind(&ThreadPool::threadHandler, this)));
        // 启动线程，线程启动运行线程池所指向的回调函数
        threads_[i]->start();			// 就是执行ThreadPool::threadHandle
    }
}

// ThreadPool::threadHandle		=> 线程池 指定 线程启动  运行的函数
void ThreadPool::threadHandler()
{
    while(1)
    {
        Task task;		// 获取锁内任务
        {
            // worker线程启动，应该在任务队列TaskQue_中 拿任务
            std::unique_lock<std::mutex> lock(taskQueMtx_);
            // 分为Cached，和FIXED 模式，因为，CACHED模式下：一个线程如果等待60s还没有任务时，应该删除
            if(poolMode_ == PoolMode::MODE_CACHED)
            {
                bool hasTask = notEmpty_.wait_for(lock, std::chrono::seconds(60), [this]()->bool{
                    return taskQue_.size() > 0 || quit_ == true; 		// 任务队列有任务 || 线程池要销毁了
                });
                
                // 任务队列没有任务，线程池要销毁了， 这个 worker线程 直接结束
                if(quit_ == true && taskQue_.size() == 0) return;
                
                // 2. 是超时60s且没任务，因此该worker线程在干等，因该删除释放 该 worker线程
                if(!hasTask)
                {
                    if(curThreadSize_ > initThreadSize_)	// 该线程可以被删除释放
                    {
#ifdef MY_DEBUG
                        std::cout<< std::this_thread::get_id() << " : 隔离线程成功" << std::endl;
#endif
                        --idleThreadSize_;
                        --curThreadSize_;
                        return;
                    }	
                    continue;	// 没有超出，就是没任务了，继续等待任务
                }
            }
            else
            {
                notEmpty_.wait(lock,[this]()->bool{
                    return taskQue_.size() > 0 || quit_ == true;	// 任务队列有任务 || 线程池要销毁了 
                });
                
                // 任务队列没有任务，线程池要销毁了， 这个 worker线程 直接结束
                if(quit_ == true && taskQue_.size() == 0) return;
            }

            // 任务队列有任务，可以通过.join来完成剩余工作，因此将任务 添加到 该worker线程执行
            task = taskQue_.front();	// 任务执行，不需要锁保护
            taskQue_.pop();
            --idleThreadSize_;
#ifdef MY_DEBUG
            std::cout<< std::this_thread::get_id() << " 获取任务成功" << std::endl;
#endif
            
            // 通知消费者，生产者
            if(taskQue_.size() > 0) notEmpty_.notify_all();		// 通知worker线程，还有任务
            notFull_.notify_all();		// 通知消费者生产任务
        }
        
        // 从任务队列taskQue_中获取的任务不为空
        if(task != nullptr)		// 执行任务，单独线程执行，不需要taskQueMtx_锁
        {
            task();		// 在该worker线程执行任务 task();
        }
        ++idleThreadSize_;		// worker线程运行任务结束，重新变成 空闲线程
    }
}

// 给线程池提交任务 =》 使用可变惨模板 让 submitTask可以接收任意任务函数 和 任意数量的 参数
// 我们需要将 func(args...) 打包为 packaged_task 类型
// auto  ...  -> decltype 这是返回值类型推导
template<typename Func, typename... Args>
auto ThreadPool::submitTask(Func func, Args&&... args) -> std::future<decltype(func(args...))>
{
    using RType = decltype(func(args...));		// func(args...)的返回类型
    /*
// 创建一个packaged_task包，包装函数func : 
这是用户原本的packaged_task，std::packaged_task<RType()> task(std::bind(func, args...));		
但是函数对象<void()>要获取 task，拷贝获取， 1. 使用指针 （选择shared_ptr 支持拷贝）  2.<RType()> 转换-> <void()> 
	auto task = std::make_shared<std::packated_task<RType()> >(std::bind(func, args...));
		这里 taks <=> std::shared_ptr<RType()> 这样的类型 =》  可以拷贝
    */
    
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    bool hasMoreTask = notFull_.wait_for(lock, std::chrono::seconds(1), [this]()->bool{
		return taskQue_.size() < taskQueMaxThreshold_;	// 任务队列未满 
    });
    
    // 表示 taskQue_.size() >= taskQueMaxThreshold_ 任务超级多，沾满了任务队列，现在还有任务提交
    if(!hasMoreTask)	// 说明这次提交任务失败
    {
        // 任务太多了，CACHED模式，应该新增线程
        if(poolMode_ == PoolMode::MODE_CACHED && curThreadSize_ < threadSizeThreshold_)
        {
            threads_.emplace_back(std::make_unique<Thread>(std::bind(&ThreadPool::threadHandler, this)));
            threads_.back()->start();		// 现在是工作线程很忙，因为taskQue_.size()严重 > 0，新线程也立刻work了
            ++idleThreadSize_;
            ++curThreadSize_;
#ifdef MY_DEBUG
            std::cout<< std::this_thread::get_id() << " => 触发新增线程" << std::endl;
#endif
        }
        
        // 返回一个"get()"时抛异常的future
#ifdef MY_DEBUG
        std::cerr << "task queue is full, submit task fail" << std::endl;
#endif
        std::packaged_task<RType()> task([]()->RType{
            throw std::runtime_error("task queue is full");
        });
        std::future<RType> fut = task.get_future();
        task();
        return fut;		// future是 move_only类型，return fut是移动   
    }
    
    // taskQue_.size() < taskQueMaxThreshold_  任务队列未满  提交任务成功
    auto task = std::make_shared<std::packaged_task<RType()>>(std::bind(func, args...));
    std::future<RType> fut = task->get_future();
    taskQue_.emplace([task]()->void { (*task)(); });    //temp() =》 (*task)() 就是用户需要执行的任务，这里只是函数对象
    notEmpty_.notify_all();
#ifdef MY_DEBUG
    std::cout<< std::this_thread::get_id() << " submit 任务" << std::endl;
#endif
    return fut;
}