#include <iostream>
#include <thread>
#include <semaphore.h>
#include <chrono>
#include <fstream>
#include <vector>
#include <memory>

using namespace std;

// For Writer Preference Solution
shared_ptr<sem_t> mutex = make_shared<sem_t>();
shared_ptr<sem_t> writeblock = make_shared<sem_t>();
int rC = 0, wC = 0; // Number of readers and writers
shared_ptr<sem_t> rMutex = make_shared<sem_t>(); // Mutex for rC
shared_ptr<sem_t> wMutex = make_shared<sem_t>(); // Mutex for wC
shared_ptr<sem_t> rTry = make_shared<sem_t>(); // Semaphore for writer preference

// Critical section variables
int shared_data = 0;

void writer(int id, int kw, int randCSTime, int randRemTime, ofstream& logFile, vector<double>& entryTimes) {
    for(int i = 0; i <= kw; i++) {
        auto reqTime = chrono::system_clock::now();
        logFile << i << "th CS request by Writer Thread " << id << " at " << reqTime.time_since_epoch().count() << endl;

        sem_wait(rTry.get()); // Lock for writer preference
        sem_wait(writeblock.get()); // Acquire resource lock for writing
        sem_post(rTry.get()); // Release reader try lock

        auto enterTime = chrono::system_clock::now();
        logFile << i << "th CS Entry by Writer Thread " << id << " at " << enterTime.time_since_epoch().count() << endl;
        entryTimes.push_back(chrono::duration_cast<chrono::milliseconds>(enterTime - reqTime).count());

        shared_data++; // Modify shared data

        this_thread::sleep_for(chrono::milliseconds(randCSTime)); // Simulate writing

        sem_post(writeblock.get()); // Release resource lock for writing

        auto exitTime = chrono::system_clock::now();
        logFile << i << "th CS Exit by Writer Thread " << id << " at " << exitTime.time_since_epoch().count() << endl;

        this_thread::sleep_for(chrono::milliseconds(randRemTime)); // Simulate remainder section
    }
}

void reader(int id, int kr, int randCSTime, int randRemTime, ofstream& logFile, vector<double>& entryTimes) {
    for(int i = 0; i < kr; i++) {
        auto reqTime = chrono::system_clock::now();
        logFile << i << "th CS request by Reader Thread " << id << " at " << reqTime.time_since_epoch().count() << endl;

        sem_wait(mutex.get()); // Lock for readers
        sem_wait(rMutex.get()); // Lock for rC
        rC++;
        if (rC == 1) sem_wait(writeblock.get()); // First reader acquires resource lock
        sem_post(mutex.get()); // Unlock for readers
        sem_post(rMutex.get()); // Unlock for rC

        auto enterTime = chrono::system_clock::now();
        logFile << i << "th CS Entry by Reader Thread " << id << " at " << enterTime.time_since_epoch().count() << endl;
        entryTimes.push_back(chrono::duration_cast<chrono::milliseconds>(enterTime - reqTime).count());

        cout << "Data read by the reader " << id << " is " << shared_data << endl; // Read shared data

        this_thread::sleep_for(chrono::milliseconds(randCSTime)); // Simulate reading

        sem_wait(rMutex.get()); // Lock for rC
        rC--;
        if (rC == 0) sem_post(writeblock.get()); // Last reader releases resource lock
        sem_post(rMutex.get()); // Unlock for rC

        auto exitTime = chrono::system_clock::now();
        logFile << i << "th CS Exit by Reader Thread " << id << " at " << exitTime.time_since_epoch().count() << endl;

        this_thread::sleep_for(chrono::milliseconds(randRemTime)); // Simulate remainder section
    }
}

double calculateAverage(const vector<double>& times, const string& algorithm) {
    if (times.empty()) {
        return 0.0; // Return 0 if the vector is empty
    }

    double sum = 0;
    for(auto time : times) {
        sum += time;
    }
    double average = sum / times.size();

    // Output the average time with the algorithm name
    cout << "Average time for a thread to gain entry to the Critical Section (Algorithm: " << algorithm << "): " << average << " milliseconds" << endl;

    return average;
}

int main() {
    ofstream logFile("RW-log.txt");
    ofstream avgTimeFile("Average time.txt");

    int nw = 2, nr = 3, kw = 3, kr = 3, muCS = 100, muRem = 100;

    sem_init(mutex.get(), 0, 1);
    sem_init(writeblock.get(), 0, 1);
    sem_init(rMutex.get(), 0, 1);
    sem_init(wMutex.get(), 0, 1);
    sem_init(rTry.get(), 0, 1);

    vector<thread> writers(nw);
    vector<thread> readers(nr);

    vector<double> writerEntryTimes, readerEntryTimes;

    // Create all writer threads
    for(int i = 0; i < nw; i++)
        writers[i] = thread(writer, i, kw, muCS, muRem, ref(logFile), ref(writerEntryTimes));

    // Create all reader threads
    for(int i = 0; i < nr; i++)
        readers[i] = thread(reader, i, kr, muCS, muRem, ref(logFile), ref(readerEntryTimes));

    // Join all writer threads
    for(int i = 0; i < nw; i++)
        writers[i].join();

    // Join all reader threads
    for(int i = 0; i < nr; i++)
        readers[i].join();

    avgTimeFile << "Average time for a writer thread to gain entry to the Critical Section (Algorithm: W): " << calculateAverage(writerEntryTimes, "W") << " milliseconds" << endl;
    avgTimeFile << "Average time for a reader thread to gain entry to the Critical Section (Algorithm: R): " << calculateAverage(readerEntryTimes, "R") << " milliseconds" << endl;

    logFile.close();
    avgTimeFile.close();

    return 0;
}
