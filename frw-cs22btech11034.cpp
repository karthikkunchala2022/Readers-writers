#include <iostream>
#include <thread>
#include <semaphore.h>
#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

// For Fair Solution
shared_ptr<sem_t> queue = make_shared<sem_t>();

// Function to generate a formatted timestamp string
string getTimestamp() {
    auto now = chrono::system_clock::now();
    auto now_time_t = chrono::system_clock::to_time_t(now);
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    string timestamp(30, '\0');
    strftime(&timestamp[0], timestamp.size(), "%Y-%m-%d %H:%M:%S", localtime(&now_time_t));
    return timestamp + "." + to_string(ms.count());
}

void writer(int id, int kw, int randCSTime, int randRemTime, ofstream& logFile, vector<double>& entryTimes) {
    for(int i = 0; i <= kw; i++) {
        auto reqTime = getTimestamp();
        logFile << i << "th CS request by Writer Thread " << id << " at " << reqTime << endl;

        sem_wait(queue.get()); // Wait in the queue

        auto enterTime = getTimestamp();
        logFile << i << "th CS Entry by Writer Thread " << id << " at " << enterTime << endl;
        entryTimes.push_back(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count());

        // Modify shared data
        // Simulate writing
        this_thread::sleep_for(chrono::milliseconds(randCSTime));

        auto exitTime = getTimestamp();
        logFile << i << "th CS Exit by Writer Thread " << id << " at " << exitTime << endl;

        sem_post(queue.get()); // Release queue lock

        // Simulate remainder section
        this_thread::sleep_for(chrono::milliseconds(randRemTime));
    }
}

void reader(int id, int kr, int randCSTime, int randRemTime, ofstream& logFile, vector<double>& entryTimes) {
    for(int i = 0; i < kr; i++) {
        auto reqTime = getTimestamp();
        logFile << i << "th CS request by Reader Thread " << id << " at " << reqTime << endl;

        sem_wait(queue.get()); // Wait in the queue

        auto enterTime = getTimestamp();
        logFile << i << "th CS Entry by Reader Thread " << id << " at " << enterTime << endl;
        entryTimes.push_back(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count());

        // Simulate reading
        this_thread::sleep_for(chrono::milliseconds(randCSTime));

        auto exitTime = getTimestamp();
        logFile << i << "th CS Exit by Reader Thread " << id << " at " << exitTime << endl;

        sem_post(queue.get()); // Release queue lock

        // Simulate remainder section
        this_thread::sleep_for(chrono::milliseconds(randRemTime));
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
    ifstream inputFile("inp-params.txt");
    ofstream logFile("FairRW-log.txt");
    ofstream avgTimeFile("Average time.txt");

    int nw, nr, kw, kr, muCS, muRem;
    inputFile >> nw >> nr >> kw >> kr >> muCS >> muRem;

    sem_init(queue.get(), 0, 1);

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

    avgTimeFile << "Average time for a writer thread to gain entry to the Critical Section (Algorithm: Fair-RW): " << calculateAverage(writerEntryTimes, "Fair-RW") << " milliseconds" << endl;
    avgTimeFile << "Average time for a reader thread to gain entry to the Critical Section (Algorithm: Fair-RW): " << calculateAverage(readerEntryTimes, "Fair-RW") << " milliseconds" << endl;

    inputFile.close();
    logFile.close();
    avgTimeFile.close();

    return 0;
}
