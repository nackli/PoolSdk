#include "thread/AdvancedThreadPool.h"
int main()
{
	AdvancedThreadPool pool(2, 4);
	
	auto high_priority_task = pool.enqueue(
        AdvancedThreadPool::Priority::Normal,
        [&pool, hPipe]() {
            while (1)
            {
                auto threadId = std::this_thread::get_id();
                const char* szTestPipe = "hello nack";
                //writePipe(hPipe[OPT_WRITE], szTestPipe,strlen(szTestPipe));
                LOG_TRACE("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_DEBUG("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_INFO("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_WARNING("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_ERROR("High priority task running %d writePipe=%s", threadId, szTestPipe);
                LOG_FATAL("High priority task running %d writePipe=%s", threadId, szTestPipe);
                //std::cout << " High priority task running " << threadId << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));;
            }
     

        }
    );

    auto high_priority_taskT = pool.enqueue(
        AdvancedThreadPool::Priority::Normal,
        [&pool, hPipe]() {
            while (1)
            {
                auto threadId = std::this_thread::get_id();
                char szData[256] = { 0 };
                //readPipe(hPipe[OPT_READ], szData, sizeof(szData));
                LOG_DEBUG("High priority task running %d readPipe=%s", threadId, szData);
                //std::cout << " High priority task running " << threadId << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));;
            }
        }
    );
}