//
// Created by stowy on 10/03/2023.
//

#include "job_system.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "thread_safe_ring_buffer.hpp"

namespace Stw::JobSystem
{
std::uint32_t numThreads = 0;
ThreadSafeRingBuffer<std::function<void()>, 256> jobPool;
std::condition_variable wakeCondition;
std::mutex wakeMutex;
std::uint64_t currentLabel = 0;
std::atomic<std::uint64_t> finishedLabel;

void Initialize()
{
	finishedLabel.store(0);

	auto numCores = std::thread::hardware_concurrency();
	numThreads = std::max(1u, numCores);

	for (std::uint32_t threadId = 0; threadId < numThreads; ++threadId)
	{
		std::thread worker([]()
		{
			while (true)
			{
				if (auto jobOpt = jobPool.PopFront())
				{
					auto& job = jobOpt.value();
					job();
					finishedLabel.fetch_add(1);
				}
				else
				{
					std::unique_lock<std::mutex> lock(wakeMutex);
					wakeCondition.wait(lock);
				}
			}
		});

		worker.detach();
	}
}

void Poll()
{
	wakeCondition.notify_one();
	std::this_thread::yield();
}

void Execute(const std::function<void()>& job)
{
	currentLabel += 1;

	while (!jobPool.PushBack(job))
	{
		Poll();
	}

	wakeCondition.notify_one();
}

bool IsBusy()
{
	return finishedLabel.load() < currentLabel;
}

void Wait()
{
	while (IsBusy())
	{
		Poll();
	}
}

void Dispatch(std::uint32_t jobCount, std::uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
{
	if (jobCount == 0 || groupSize == 0) return;

	const auto groupCount = (jobCount + groupSize - 1) / groupSize;
	currentLabel += groupCount;

	for (std::uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
	{
		auto jobGroup = [jobCount, groupSize, job, groupIndex]()
		{
			const auto groupJobOffset = groupIndex * groupSize;
			const auto groupJobEnd = std::min(groupJobOffset + groupSize, jobCount);

			JobDispatchArgs args{};
			args.groupIndex = groupIndex;

			for (std::uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
			{
				args.jobIndex = i;
				job(args);
			}
		};

		while (!jobPool.PushBack(jobGroup))
		{
			Poll();
		}

		wakeCondition.notify_one();
	}
}
}
