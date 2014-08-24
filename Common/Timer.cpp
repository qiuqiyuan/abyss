#include "Timer.h"
#include "Log.h"
#include <iomanip>
#include <iostream>

using namespace std;

// Constructor starts the timer
Timer::Timer(string funcString)
	: m_funcStr(funcString), m_start(clock())
{
}

// Destructor stops it and prints
Timer::~Timer()
{
	logger(2) << m_funcStr << ": " << setprecision(3)
		<< (double)(clock() - m_start) / CLOCKS_PER_SEC << " s\n";
}

/************* Rahul Nihalani *************/

// Constructor starts the timer
RTimer::RTimer(string funcString)
        : m_funcStr(funcString), m_start(clock())
{
}

// Destructor stops it and prints
RTimer::~RTimer()
{
        std::cout << m_funcStr << ": " << setprecision(3)
                << (double)(clock() - m_start) / CLOCKS_PER_SEC << " s\n";
}

/**********End Rahul Nihalani *************/

