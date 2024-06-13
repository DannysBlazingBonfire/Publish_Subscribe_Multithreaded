#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

const int rangeStart = 500;
const int rangeEnd = 1200;
std::uniform_int_distribution<> distr(rangeStart, rangeEnd); // random num gen

struct order
{
	int orderID;
	int cooktime;
};

std::mutex ordersMutex;
std::mutex printMutex;
std::condition_variable ordersCV;

std::vector<order> orders; // order queue
const int ordersCapacity = 5;

order createOrder() // return order with ID and random cooktime
{
	order newOrder;
	newOrder.orderID = orders.size();
	newOrder.cooktime = distr(gen);
	return newOrder;
}

void customerPrint(const int& customerID, const int& orderNr) // synchronized customer print function
{
	{
		std::lock_guard<std::mutex> printlock(printMutex);
		std::cout << "Customer: " << customerID << " placed Order: " << orderNr << std::endl;
	}
}

void customerTask(const int ID)
{
	order customerOrder;
	// Cs
	{
		std::unique_lock<std::mutex> lock(ordersMutex);
		ordersCV.wait(lock, [&]() { return orders.size() < ordersCapacity; }); // while orders are not maxed, place order or wait
		
		customerOrder = createOrder();
		orders.push_back(customerOrder); // add order to queue
	}
	customerPrint(ID, customerOrder.orderID);
	ordersCV.notify_one();
}

void printProgress(const int& baristaID, const order& orderTaken, const int& progress) // Baristas progress print
{
	{
		std::lock_guard<std::mutex> printlock(printMutex);
		if (progress != 100)
		{
			std::cout << "barista: " << baristaID << " - Order: " << orderTaken.orderID << ", progress: " << progress << "%" << std::endl;
		}
		else
		{
			std::cout << "barista: " << baristaID << " - Order: " << orderTaken.orderID << " is complete! " << std::endl;
		}
	}
}

void baristaTask(const int ID)
{
	int workTime = 0;
	bool orderDone = false;
	bool jobDone = false;
	int progressInc = 0;
	int progressPercent = 0;
	order takenOrder;

	while (!jobDone)
	{
		// Cs
		{
			std::unique_lock<std::mutex> orderLock(ordersMutex);
			ordersCV.wait(orderLock, [&]() { return orders.size() > 0; }); // while orders are not empty, get order else wait.
			takenOrder = orders.back();
			workTime = orders.back().cooktime;
			orders.pop_back();
		}
		ordersCV.notify_one();
		{
			std::lock_guard<std::mutex> printlock(printMutex);
			std::cout << "Barista: " << ID << " started Order: " << takenOrder.orderID << std::endl;
		}

		// simulate working on order
		progressInc = workTime / 10;

		while (!orderDone)
		{
			if (progressPercent == 100) { orderDone = true; }
			printProgress(ID, takenOrder, progressPercent);
			std::this_thread::sleep_for(std::chrono::milliseconds(progressInc));
			progressPercent += 10;
		}

		if (orders.empty()) { jobDone = true; } // job is done if no more orders in queue

		// reset
		workTime = 0;
		orderDone = false;
		progressInc = 0;
		progressPercent = 0;
	}

	{
		std::lock_guard<std::mutex> printlock(printMutex);
		std::cout << "Barista: " << ID << " is done working!" << std::endl;
	}
}

int main()
{

	std::thread customers[5];
	std::thread Baristas[2];

	for (int i = 0; i < 5; i++)
	{
		customers[i] = std::thread(customerTask, i);
	}

	for (int j = 0; j < 2; j++)
	{
		Baristas[j] = std::thread(baristaTask,j);
	}

	for (int i = 0; i < 5; i++)
	{
		customers[i].join();
	}

	for (int j = 0; j < 2; j++)
	{
		Baristas[j].join();
	}

	return 0;
}