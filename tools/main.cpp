#include <iostream>
//#define TEST_THREAD_POOL
#define TEST_LOGGER

#ifdef TEST_THREAD_POOL
#include "thread_pool.hpp"  // 引入线程池头文件
// 一个简单的函数，模拟一些工作
void exampleFunction(int id) {
    std::cout << "Start task in thread " << id << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));  // 模拟耗时操作
    std::cout << "Task completed in thread " << id << std::endl;
}

int main() {
    // 获取线程池的实例
    opensource::ctrlfrmb::ThreadPool& pool = opensource::ctrlfrmb::ThreadPool::Instance();
    std::cout << "test thread pool "<< std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 设置线程池参数
    pool.set(4, 2, 5); // 设置最大线程数为4，最小线程数为2，线程超时时间为600秒

    // 提交几个任务到线程池
    std::future<void> result1 = pool.submit(exampleFunction, 1);
    std::future<void> result2 = pool.submit(exampleFunction, 2);
    std::future<void> result3 = pool.submit(exampleFunction, 3);
    pool.submit(exampleFunction, 4);
    pool.submit(exampleFunction, 5);
    pool.submit(exampleFunction, 6);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    pool.submit(exampleFunction, 7);
    pool.submit(exampleFunction, 8);
    pool.submit(exampleFunction, 9);

    // 等待任务完成
    //result1.get();
    //result2.get();
    //result3.get();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    // 关闭线程池
    //pool.shutdown();

    return 0;
}
#elif defined(TEST_LOGGER)
#include "logger.hpp"

int main() {
    using namespace opensource::ctrlfrmb;
    Logger logger;

    // 设置日志等级为 DEBUG，这样所有等级的日志都将显示
    logger.setLevel(LogLevel::DEBUG);

    // 设置自定义前缀回调
    logger.setPrefixCallback([]() {
        time_t now = time(nullptr);
        return ctime(&now);
    });

    FileLoggerConfig config("logs", "mylog", ".txt", 1024 * 1024, 5, true); // 启用异步写入
    logger.enableFileWrite(config);

    int counter = 100000;
    while (--counter) {
        // 输出不同等级的日志
        logger.trace("This is a trace message.");
        logger.debug("This is a debug message.");
        logger.info("This is an info message.");
        logger.warn("This is a warning message.");
        logger.error("This is an error message.");
        logger.fatal("This is a fatal message.");
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    return 0;
}

#endif