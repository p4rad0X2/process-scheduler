/*
Pratul Rajagopalan
55858290
CS3103 - Assignment 3 
*/

#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

using namespace std;

//declaring a function to split the input strings
vector <string> split(const string& str) 
{
	vector<string> strings;
	istringstream iss(str);
	for (string s; iss >> s; )
		strings.push_back(s);
	return strings;
}

//declaring the service class to perform user defined functions
class Service
{
public:
    //initialising number of ticks, running time, mutex and command strings
    string mutex = "";
    int ti;
    int runningtime = 0;
    string cmd;

    //service class constructor to set values based on user input
    Service(string inp)
    {
        vector <string> input = split(inp);
        string cmd = input[0];
        int ticks;
        if (cmd == "L" || cmd == "U")
        {
            mutex = input[1];
            ti = 0;
        }
        else
        {
            ti = stoi(input[1]);
        }
        this->cmd = cmd;
        this->ti = ti;
    }

    //function to decrement number of ticks remaining
    void run()
    {
        this->ti--;
    }
};

//creating a boolean variable to track the status of the mutex
bool mutexstat = false;
//function to lock the mutex
void LockMutex()
{
    mutexstat = true;
}
//function to unlock the mutex
void UnlockMutex()
{
    mutexstat = false;
}

//declaring the process class
class Process
{
public:
    //declaring two vectors to keep track of timestamps and the output
    vector <int> timest;
    vector <int> opnum;
    //declaring processid, service count, arrival, and completion of service
    int pid, servct, arrival, servcomp = 0;
    //declaring a dequeue of services
    deque <Service> servicelist;
    int qprio = 0;
    //constructor to set values for initialised variables
    Process(string input, ifstream &in)
    {
        vector <string> inputs = split(input);
        int pid = stoi(inputs[1]), arrival = stoi(inputs[2]), servct = stoi(inputs[3]);
        this->pid = pid;
        this->arrival = arrival;
        this->servct = servct;
        for (int i = 0; i < servct; i++)
        {
            getline(in, input);
            servicelist.emplace_back(Service(input));
        }
    }
    //run process at the front of the queue
    void runproc()
    {
        this->servicelist.front().run();
    }
    //add a timestamp 
    void timestadd(int num)
    {
        timest.emplace_back(num);
    }
};


//initializing deques to keep track of the blocked queues for keyboard block, diskblock,
// mutexqueue, readyqueue, process queue and the queue to keep track of completed processes
int startTick = 0;

deque <Process> mutexq;
deque <Process> processq;
int counter = 0;
int tick = 0;
deque <Process> readyq;
deque <Process> completeq;
deque <Process> kbblock;
deque <Process> diskbl;
bool lucmd = false;
//value to keep track of number of ticks spent running
int runningtime = 0;

//function to pop the topmost process from one queue and place it at the back of another queue
void changeOrder(deque <Process> &l1, deque <Process> &l2)
{
    l2.emplace_back(l1.front());
    l1.pop_front();
}

//flag to check running or not
bool runchk = true;

//function to check for the next service
void nextservchk()
{
    //checking if the ready queue is empty
    if (readyq.empty())
    {
        return;
    }
    //checking if it is a disk I/O
    if (readyq.front().servicelist.front().cmd == "D")
    {
        readyq.front().timestadd(startTick);
        readyq.front().timestadd(tick + 1);
        startTick = tick + 1;
        runningtime = 0;
        runchk = false;
        changeOrder(readyq, diskbl);
    }
    //checking if it is a keyboard input
    else if (readyq.front().servicelist.front().cmd == "K")
    {
        readyq.front().timestadd(startTick);
        readyq.front().timestadd(tick + 1);
        startTick = tick + 1;
        runningtime = 0;
        runchk = false;
        changeOrder(readyq, kbblock);
    }
    //checking if it is a mutex lock
    else if (readyq.front().servicelist.front().cmd == "L")
    {
        readyq.front().servicelist.pop_front();
        if (mutexstat){
            readyq.front().timestadd(startTick);
            readyq.front().timestadd(tick + 1);
            startTick = tick + 1;
            runningtime = 0;
            runchk = false;
            changeOrder(readyq, mutexq);
        }
        else
        {
            LockMutex();
        }
        nextservchk();
    }
    //checking if it is a mutex unlock
    else if (readyq.front().servicelist.front().cmd == "U")
    {
        readyq.front().servicelist.pop_front();
        if (readyq.front().servicelist.size() == 0)
        {
            readyq.front().timestadd(startTick);
            startTick = tick + 1;
            runningtime=0;
            readyq.front().timestadd(tick + 1 + counter);
            changeOrder(readyq, completeq);
        }
        if (mutexq.empty())
        {
            UnlockMutex();
        }
        else
        {
            changeOrder(mutexq, readyq);
        }
        nextservchk();
    }
    //checking the flag for runtime check
    if (runchk)
    {
        //if the process has been running for 5 ticks then changes order
        if (runningtime == 5)
        {
            readyq.front().timestadd(startTick);
            readyq.front().timestadd(tick + 1);
            startTick = tick + 1;
            runningtime = 0;
            changeOrder(readyq, readyq);
        }
    }
}

//function to compare process ids
bool comparePid(Process pro1, Process pro2)
{
    return pro1.pid < pro2.pid;
}

//function to print the details of processes that have been completed
void printfile(const char* outp)
{
    //ofstream file;
    //file.open(outp);
    int size = completeq.size();
    for (int i = 0; i < size; i++)
    {
        cout << "process " << completeq.front().pid << endl;
        for (int j = 0; j < completeq.front().timest.size(); j++)
        {
            cout << completeq.front().timest[j] << " ";
        }
        cout << endl;
        completeq.pop_front();
    }
    //file.close();
}


//fucntion to perform First Come First Served Scheduling
void FCFS(const char* outp)
{
    int proCt = processq.size();
    int runningtime = 0;
    bool empchk = false;

    int currPID = -1, prevPID = -1;

    for (int i = 0; i < 1000; i++)
    {
        //checking for new processes and then adding them to the ready queue
        vector <Process> newPr;
        while (processq.front().arrival == tick)
        {
            newPr.emplace_back(processq.front());
            processq.pop_front();
        }
        sort(newPr.begin(), newPr.end(), comparePid);
        for (int i = 0; i < newPr.size(); i++)
        {
            readyq.emplace_back(newPr[i]);
        }
        newPr.clear();


        //checking keyboardblocked queue
        if (!kbblock.empty())
        {
            kbblock.front().runproc();
        }

        //checking disk blocked queue
        if (!diskbl.empty())
        {
            diskbl.front().runproc();
        }

        //checking if ready queue is empty
        if (readyq.empty())
        {
            currPID = -1;
            startTick = tick + 1;
            empchk = true;
        }
        else
        {
            currPID = readyq.front().pid;
            if (currPID != prevPID)
            {
                startTick = tick;
            }
            if (empchk)
            {
                startTick = tick + 1;
                counter++;
                empchk = false;
            }
            readyq.front().runproc();

            //checking for completion of the service and if it is completed it is added to the completed queue
            if (readyq.front().servicelist.front().ti == 0)
            {
                readyq.front().servicelist.pop_front();
                if (readyq.front().servicelist.size() == 0)
                {
                    readyq.front().timestadd(startTick);
                    startTick = tick + 1;
                    readyq.front().timestadd(tick + 1 + counter);
                    changeOrder(readyq, completeq);
                }
                else 
                {
                    nextservchk();
                }
            }

            if (!diskbl.empty())
            {
                    //disk blocking scheduling
                    if (diskbl.front().servicelist.front().ti == 0)
                    {
                        diskbl.front().servicelist.pop_front();
                        diskbl.front().servcomp++;
                        changeOrder(diskbl, readyq);
                    }
            }

            //keyboard blocking scheduling
            if (!kbblock.empty())
            {
                    if (kbblock.front().servicelist.front().ti == 0)
                    {
                        kbblock.front().servicelist.pop_front();
                        kbblock.front().servcomp++;
                        changeOrder(kbblock, readyq);
                    }
            }
            prevPID = currPID;
        }
        if (completeq.size() == proCt)
        {
            break;
        }
        tick++;
    }
    printfile(outp);
}

//Function to perform Round Robin Scheduling
void RR(const char* outp){

    int proCt = processq.size();
    bool firstpr = true;
    bool empchk = false;

    int currPID = -1, prevPID = -1;

    for (int i = 0; i < 1000; i++)
    {
        //checking for new processes then adding them to the ready queue
        vector <Process> newPr;
        while (processq.front().arrival == tick)
        {
            newPr.emplace_back(processq.front());
            processq.pop_front();
        }
        sort(newPr.begin(), newPr.end(), comparePid);
        for (int i = 0; i < newPr.size(); i++)
        {
            readyq.emplace_back(newPr[i]);
        }
        newPr.clear();

        //checking keyboard blocked queue
        if (!kbblock.empty())
        {
            kbblock.front().runproc();
        }

        //checking disk blocked queue
        if (!diskbl.empty())
        {
            diskbl.front().runproc();
        }

        //checking if ready queue is empty
        if (readyq.empty())
        {
            currPID = -1;
            startTick = tick + 1;
            empchk = true;
        }
        else
        {
            currPID = readyq.front().pid;
            if (firstpr)
            {
                prevPID = currPID;
                firstpr = false;
            }
            if (currPID != prevPID)
            {
                startTick = tick;
                runningtime = 0;
            }
            if (empchk)
            {
                startTick = tick + 1;
                counter++;
                empchk = false;
            }
            runningtime++;
            readyq.front().runproc();
            
            //checking for completion of the service and if yes it is added to the completed queue
            if (readyq.front().servicelist.front().ti == 0)
            {
                readyq.front().servicelist.pop_front();
                if (readyq.front().servicelist.size() == 0)
                {
                    readyq.front().timestadd(startTick);
                    startTick = tick + 1;
                    readyq.front().timestadd(tick + 1 + counter);
                    changeOrder(readyq, completeq);
                }
                else 
                {
                    nextservchk();
                }
            }

            if (!diskbl.empty())
            {
                    //disk blocking scheduling
                    if (diskbl.front().servicelist.front().ti == 0)
                    {
                        diskbl.front().servicelist.pop_front();
                        diskbl.front().servcomp++;
                        changeOrder(diskbl, readyq);
                    }
            }
            //keyboard blocking scheduling
            if (!kbblock.empty())
            {
                    if (kbblock.front().servicelist.front().ti == 0)
                    {
                        kbblock.front().servicelist.pop_front();
                        kbblock.front().servcomp++;
                        changeOrder(kbblock, readyq);
                    }
            }
            prevPID = currPID;
        }
        if (completeq.size() == proCt)
        {
            break;
        }
        tick++;
    }
    printfile(outp);
}

deque <Process> hpque; //RQ0 high priority
deque <Process> mpque; //RQ1 medium priority
deque <Process> lpque; //RQ2 low priority
deque <Process> runningproc; //deque of running processes

//adding processes to the running queue from the priority queue levels
void addreadyqproc()
{
    if (!hpque.empty())
    {
        changeOrder(hpque, runningproc);
    }
    else if (!mpque.empty())
    {
        changeOrder(mpque, runningproc);
    }
    else if (!lpque.empty())
    {
        changeOrder(lpque, runningproc);
    }
}

//checking the next service for the different possible services
void FBnextser()
{
    if (runningproc.empty())
    {
        return;
    }
    //Keyboard I/O
    if (runningproc.front().servicelist.front().cmd== "K")
    {
        runningproc.front().timestadd(startTick);
        runningproc.front().timestadd(tick + 1);
        startTick = tick + 1;
        runningtime = 0;
        runchk = false;
        changeOrder(runningproc, kbblock);
    }

    //disk I/O
    else if (runningproc.front().servicelist.front().cmd == "D")
    {
        runningproc.front().timestadd(startTick);
        runningproc.front().timestadd(tick + 1);
        startTick = tick + 1;
        runningtime = 0;
        runchk = false;
        changeOrder(runningproc, diskbl);
    }

    //Mutex lock
    else if (runningproc.front().servicelist.front().cmd == "L"){
        runningproc.front().servicelist.pop_front();
        if (mutexstat){
            runningproc.front().timestadd(startTick);
            runningproc.front().timestadd(tick + 1);
            startTick = tick + 1;
            runningtime = 0;
            runchk = false;
            changeOrder(runningproc, mutexq);
        }
        else{
            LockMutex();
        }
        FBnextser();
    }

    //mutex unlock
    else if (runningproc.front().servicelist.front().cmd == "U"){
        UnlockMutex();
        lucmd = true;
        runningproc.front().servicelist.pop_front();
        if (runningproc.front().servicelist.size() == 0){
            runningproc.front().timestadd(startTick);
            runningproc.front().timestadd(tick + 1 + counter);
            startTick = tick + 1;
            runningtime = 0;
            changeOrder(runningproc, completeq);
            addreadyqproc();
        }
        FBnextser();
    }

    //checking if the process has been running for 5 ticks or more
    if (runchk)
    {
        if (runningtime == 5)
        {
            runningproc.front().timestadd(startTick);
            runningproc.front().timestadd(tick + 1);
            startTick = tick + 1;
            runningtime = 0;
            if (runningproc.front().qprio == 0)
            {
                runningproc.front().qprio++;
                changeOrder(runningproc, mpque);
            }
            else
            {
                runningproc.front().qprio++;
                changeOrder(runningproc, lpque);
            }
        }    
    }
}



//Function to perform Feedback Scheduling
void FBscheduler(const char* outp)
{

//initialising variables
int proCt = processq.size();
bool firstpr = true;
bool empchk = false;
int currPID = -1, prevPID = -1;

for(int i=0; i<1000; i++)
{

    //checking for new processes in the tick
    vector <Process> newPr;
    while (processq.front().arrival == tick)
    {
        newPr.emplace_back(processq.front());
        processq.pop_front();
    }
    sort(newPr.begin(), newPr.end(), comparePid);
    for(int j = 0; j<newPr.size(); j++)
    {
        hpque.push_back(newPr[j]);
    }
    newPr.clear();

    //checking if first process
    if(firstpr)
    {
        addreadyqproc();
    }

    //checking if the running queue is empty
    if (runningproc.empty())
    {
        currPID = -1;
        empchk = true;
        startTick = tick + 1;
        addreadyqproc();
    }

    //checking if the keyboard block queue is not empty
    if (!kbblock.empty())
    {
        //runinng the first process from the keyboard queue
        kbblock.front().runproc();
        if (kbblock.front().servicelist.front().ti == 0)
        {
            kbblock.front().servicelist.pop_front();
            int num = kbblock.front().qprio;
            if (num == 0)
            {
                changeOrder(kbblock, hpque);
            }
            else if (num == 1)
            {
                changeOrder(kbblock, mpque);
            }   
            else
            {
                changeOrder(kbblock, lpque);
            }

        }   
    }
    //checking if the disk block queue is not empty
    if (!diskbl.empty())
    {
        //running the first process from the disk blocked queue
        diskbl.front().runproc();
        if (diskbl.front().servicelist.front().ti == 0)
        {
            diskbl.front().servicelist.pop_front();
            if (diskbl.front().servicelist.front().ti == 0)
            {
                diskbl.front().servicelist.pop_front();
                int num = diskbl.front().qprio;
                if (num == 0)
                {
                    changeOrder(diskbl, hpque);
                }
                else if (num == 1)
                {
                    changeOrder(diskbl, mpque);
                }   
                else
                {
                    changeOrder(diskbl, lpque);
                }

            }  
        }
    }

    //checking status of mutex
    if(!mutexstat)
    {
        //lock mutex if mutexqueue is not empty
        if(!mutexq.empty())
        {
            LockMutex();
        
            int num = mutexq.front().qprio;
            if (num == 0)
            {
                changeOrder(mutexq, hpque);
            }
            else if (num == 1)
            {
                changeOrder(mutexq, mpque);
            }
            else
            {
                changeOrder(mutexq, lpque);
            }
        }

        if(runningproc.empty())
        {
            addreadyqproc();
        }
    }
    //checking if running processqueue is empty
    if (runningproc.empty())
    {
        currPID = -1;
        startTick = tick + 1;
        empchk = true;
    }


    //to accomodate for other conditions
    else
    {
        if(!runningproc.empty())
        {

        
        currPID = runningproc.front().pid;
        if (firstpr)
        {
            prevPID = currPID;
            firstpr = false;
        }
            if (currPID != prevPID)
            {
                startTick = tick;
                runningtime = 0;
            }
            runningtime ++;
            runningproc.front().runproc();
            
                if (runningproc.front().servicelist.front().ti == 0)
                {
                    runningproc.front().servicelist.pop_front();
                    if (runningproc.front().servicelist.size() == 0)
                    {
                        runningproc.front().timestadd(startTick);
                        startTick = tick + 1;
                        runningproc.front().timestadd(tick + 1 + counter);
                        changeOrder(runningproc, completeq);
                        addreadyqproc();
                    }
                
                    else
                    {
                        FBnextser();
                    }       
                }
                else
                {
                    //checking if the process has been running for 5 ticks and if it has then reducing its priority
                        if (runningtime == 5)
                        {
                            runningproc.front().timestadd(startTick);
                            runningproc.front().timestadd(tick + 1);
                            startTick = tick + 1;
                            runningtime = 0;
                            if (runningproc.front().qprio == 0)
                            {
                                runningproc.front().qprio++;
                                changeOrder(runningproc, mpque);
                            }
                            else
                            {
                                runningproc.front().qprio++;
                                changeOrder(runningproc, lpque);
                            }
                        }
                }

        prevPID = currPID;
    }
    }
        if (completeq.size() == proCt)
        {
            break;
        }
        tick++;

}

printfile(outp);

}

int main(int argc, char* argv[])
{
    string line;
    const char* sctype = argv[1];
    const char* inpath = argv[2];
    const char* outpath = argv[3];
    inpath = "sample_0.txt";
    sctype = "fb";

    ifstream file (inpath);

    while (getline(file, line))
    {
        processq.emplace_back(Process(line, file));
    }

    if (strcmp(sctype, "fcfs") == 0)
    {
        FCFS(outpath);
    }
    else if (strcmp(sctype, "rr") == 0)
    {
        RR(outpath);
    }
    else if (strcmp(sctype, "fb") == 0)
    {
        FBscheduler(outpath);
    }
}