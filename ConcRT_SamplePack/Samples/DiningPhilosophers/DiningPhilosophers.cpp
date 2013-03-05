//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: DiningPhilosophers.cpp
//
//--------------------------------------------------------------------------
#include <string>
#include <agents.h>
#include <iostream>
#include <array>
#include <windows.h>
#include <ppl.h>
using namespace ::Concurrency;

#pragma optimize("", off)
void RandomSpin()
{
    for(int i=0;i<50*500000;++i);
}
#pragma optimize("",on)
class Chopstick{
    const std::string m_Id;
public:
    Chopstick(const std::string & Id):m_Id(Id){};
    const std::string GetID()
    {
        return m_Id;
    };
};

enum PhilosopherState {Thinking,Eating};

typedef Concurrency::unbounded_buffer<Chopstick*> ChopstickProvider;
class Philosopher : public Concurrency::agent
{
    ChopstickProvider* m_LeftChopstickProvider;
    ChopstickProvider* m_RightChopstickProvider;

public:
    const std::string m_Name;
    const size_t  m_Bites;
    Philosopher(const std::string& name, size_t bites=10):m_Name(name),m_Bites(bites){};
    Concurrency::unbounded_buffer<ChopstickProvider*> LeftChopstickProviderBuffer;
    Concurrency::unbounded_buffer<ChopstickProvider*> RightChopstickProviderBuffer;
    Concurrency::overwrite_buffer<PhilosopherState> CurrentState;
    void run()
    {
        //The first thing we need to do inside run is to initialize the ChopstickProviders. We do this by waiting for messages to be sent on the public methods with receive:

        //initialize the ChopstickProviders
        m_LeftChopstickProvider  = Concurrency::receive(LeftChopstickProviderBuffer);
        m_RightChopstickProvider = Concurrency::receive(RightChopstickProviderBuffer);

        //Now we need to transition between thinking and eating, to do this I’m going to use two additional methods that are as yet unmentioned (PickupChopsticks and PutDownChopsticks):

        for(size_t i = 0; i < m_Bites;++i)
        {
            Think();
            std::vector<Chopstick*> chopsticks(PickupChopsticks());
            Eat();
            PutDownChopsticks(chopsticks);
        }

        //The only thing remaining to implement in our run method is to clean up after ourselves by returning the ChopstickProviders so that they can be reused if needed and set our agents status to done.
        Concurrency::send(LeftChopstickProviderBuffer,  m_LeftChopstickProvider);
        Concurrency::send(RightChopstickProviderBuffer, m_RightChopstickProvider);

        this->done();
    }

    std::vector<Chopstick*> PickupChopsticks()
    {
        //create the join
        Concurrency::join<Chopstick*,Concurrency::non_greedy> j(2);
        m_LeftChopstickProvider->link_target(&j); 
        m_RightChopstickProvider->link_target(&j);

        //pickup the chopsticks
        return Concurrency::receive (j);
    }	
    void PutDownChopsticks(std::vector<Chopstick*>& v)
    {
        Concurrency::asend(m_LeftChopstickProvider,v[0]);
        Concurrency::asend(m_RightChopstickProvider,v[1]);
    }
private:
    void Eat()
    {
        send(&CurrentState,Eating);
        RandomSpin();
    };
    void Think()
    {
        send(&CurrentState,Thinking);
        RandomSpin();
    };
};
template<class PhilosopherList>
class Table
{
    PhilosopherList & m_Philosophers;
    std::vector<ChopstickProvider*> m_ChopstickProviders;
    std::vector<Chopstick*> m_Chopsticks;

    //The only public method in the Table class is the constructor which initializes the vectors and assigns to ChopstickProviders to each Philosopher:
public:
    Table(PhilosopherList& philosophers): m_Philosophers(philosophers)
    {
        //fill the chopstick and the chopstick holder vectors
        for(size_t i = 0; i < m_Philosophers.size();++i)
        {
            m_ChopstickProviders.push_back(new ChopstickProvider());
            m_Chopsticks.push_back(new Chopstick("chopstick"));
            //put the chopsticks in the chopstick providers
            send(m_ChopstickProviders[i],m_Chopsticks[i]);
        }
        //assign the philosophers to their spots
        for(size_t leftIndex = 0; leftIndex < m_Philosophers.size();++leftIndex)
        {
            //calculate the rightIndex
            size_t rightIndex = (leftIndex+1)% m_Philosophers.size();

            //send the left & right chopstick provider to the appropriate Philosopher
            Concurrency::asend(& m_Philosophers[leftIndex].LeftChopstickProviderBuffer, 
                m_ChopstickProviders[leftIndex]);
            Concurrency::asend(& m_Philosophers[leftIndex].RightChopstickProviderBuffer, 
                m_ChopstickProviders[rightIndex]);
        }
    }
    ~Table(){
        m_ChopstickProviders.clear();
        m_Chopsticks.clear();
    }

};
void jointest()
{
    //create two chopsticks
    Chopstick chopstick1("chopstick one");
    Chopstick chopstick2("chopstick two");


    //create a ChopstickProvider to store the chopstick
    ChopstickProvider chopstickBuffer1;
    ChopstickProvider chopstickBuffer2;


    //put a chopstick in each chopstick holder
    Concurrency::send(chopstickBuffer1, &chopstick1);
    Concurrency::send(chopstickBuffer2, &chopstick2);

    //declare a single-type non greedy join to acquire them.
    //the constructor parameter is the number of inputs
    Concurrency::join<Chopstick*,non_greedy> j(2);

    //connect the chopstick providers to the join so that messages 
    //sent will propagate forwards
    chopstickBuffer1.link_target(&j);
    chopstickBuffer2.link_target(&j);

    //the single type join message block produces a vector of Chopsticks
    std::vector<Chopstick*> result = Concurrency::receive(j);

}
void TestSingle()
{

    //create an instance of a Philosopher and have it take 5 bites
    Philosopher Descartres("Descartres",5);

    //create a pair of chopsticks 
    Chopstick chopstick1("chopstick1");
    Chopstick chopstick2("chopstick2");

    //create a pair of ChopstickProviders
    ChopstickProvider chopstickBuffer1;
    ChopstickProvider chopstickBuffer2;

    //associate our chopstick providers with Descartes
    Concurrency::send(&Descartres.LeftChopstickProviderBuffer, &chopstickBuffer1);
    Concurrency::send(&Descartres.RightChopstickProviderBuffer, &chopstickBuffer2);	

    //start the Philosopher asynchronously 
    Descartres.start();

    //declare a call that will be used to display the philosophers state
    //note this is using a C++0x lambda
    Concurrency::call<PhilosopherState> display([](PhilosopherState state){
        if (state ==  Eating)
            std::cout << "eating\n" ;
        else
            std::cout << "thinking\n" ;
    });
    //link the call to the status buffer on our Philosopher
    Descartres.CurrentState.link_target(&display);

    //start our Philosopher eating / thinking by sending chopsticks
    asend(&chopstickBuffer1,&chopstick1);
    asend(&chopstickBuffer2,&chopstick2);

    Sleep(5000);

}
int main()
{
    //create a set of Philosophers using the tr1 array
    std::tr1::array<Philosopher,5> philosophers = {"Socrates", "Descartes", "Nietzche", "Sartre", "Amdahl"};
    Table<std::tr1::array<Philosopher,5>> Table(philosophers);
    //create a list of call blocks to display
    std::vector<Concurrency::call<PhilosopherState>*> displays;
    //start the Philosophers and create display item
    std::for_each(philosophers.begin(),philosophers.end(),[&displays](Philosopher& cur)
    {
        //create a new call block to display the status
        Concurrency::call<PhilosopherState>* consoleDisplayBlock = new Concurrency::call<PhilosopherState>([&](PhilosopherState in){
            //cout isn’t threadsafe between each output
            if(in == Eating) 
                std::cout << cur.m_Name << " is eating\n";
            else
                std::cout << cur.m_Name << " is  thinking\n";
        });
        //link up the display and store it in a vector
        cur.CurrentState.link_target(consoleDisplayBlock);
        displays.push_back(consoleDisplayBlock);
        //and start the agent
        cur.start();
    });
    //wait for them to complete
    std::for_each(philosophers.begin(),philosophers.end(),[](Philosopher& cur)
    {
        cur.wait(&cur);
    });

    displays.clear();
};
