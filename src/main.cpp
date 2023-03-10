#include <iostream>
#include <chrono>
#include <utility>

#include "job_system.hpp"

void Spin(float milli)
{
	milli /= 1000.0f;
	const auto t1 = std::chrono::high_resolution_clock::now();
	double ms = 0.0;
	while (ms < milli)
	{
		const auto t2 = std::chrono::high_resolution_clock::now();
		const auto timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);;
		ms = timeSpan.count();
	}
}

struct Timer
{
	std::string name;
	std::chrono::high_resolution_clock::time_point start;

	explicit Timer(std::string name)
		: name(std::move(name)), start(std::chrono::high_resolution_clock::now())
	{
	}

	~Timer()
	{
		const auto end = std::chrono::high_resolution_clock::now();
		const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		std::cout << name << ": " << time << " milliseconds\n";
	}
};

void ExecuteTest()
{
	{
		auto t = Timer("Serial Test");
		Spin(100);
		Spin(100);
		Spin(100);
		Spin(100);
	}

	{
		auto t = Timer("Job System Test");
		Stw::JobSystem::Execute([]()
		{ Spin(100); });
		Stw::JobSystem::Execute([]()
		{ Spin(100); });
		Stw::JobSystem::Execute([]()
		{ Spin(100); });
		Stw::JobSystem::Execute([]()
		{ Spin(100); });
		Stw::JobSystem::Wait();
	}
}

struct Data
{
	float m[16];

	void Compute(std::uint32_t value)
	{
		for (int i = 0; i < 16; ++i)
		{
			m[i] += static_cast<float>((value + i));
		}
	}
};

void DispatchTest()
{
	constexpr std::size_t dataCount = 1'000'000;
	{
		Data* dataSet = new Data[dataCount];
		{
			auto timer = Timer("Loop Test");
			for (std::uint32_t i = 0; i < dataCount; ++i)
			{
				dataSet[i].Compute(i);
			}
		}
		delete[] dataSet;
	}

	{
		Data* dataSet = new Data[dataCount];
		{
			auto timer = Timer("Dispatch() Test");
			const std::uint32_t groupSize = 1000;
			Stw::JobSystem::Dispatch(dataCount, groupSize, [&dataSet](Stw::JobDispatchArgs args)
			{
				dataSet[args.jobIndex].Compute(1);
			});
		}
		delete[] dataSet;
	}
}

int main()
{
	std::cout << "Testing the job system !\n";
	Stw::JobSystem::Initialize();
	std::cout << "Execute tests\n";
	ExecuteTest();
	std::cout << "\nDispatch tests\n";
	DispatchTest();
}
