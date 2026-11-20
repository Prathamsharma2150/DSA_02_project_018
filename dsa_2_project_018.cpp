#include <bits/stdc++.h>
using namespace std;
struct Block {
    int start;
    int size;
    bool isFree;
    int pid;          
    int partitionId;  
    Block* prev;
    Block* next;
};
struct Memory {
    Block* head = nullptr;
    int policy = 1;           
    int allocated = 0;         
    int failed = 0;            
    long long comparisons = 0; 
    vector<int> failedPids;
    vector<Block*> owner;     
};
struct Metrics {
    int allocated = 0;
    int failed = 0;
    vector<int> failedPids;
    long long comparisons = 0;
    long long totalFree = 0;
    long long allocatedMem = 0;
    int holes = 0;
    int largestHole = 0;
    double externalFrag = 0.0;  
    double utilization = 0.0;   
};
string policyName(int policy) {
    if (policy == 1) return "First Fit";
    if (policy == 2) return "Best Fit";
    return "Worst Fit";
}
Block* firstFit(Block* head, int size, long long& comps) {
    for (Block* b = head; b; b = b->next) {
        comps++;
        if (b->isFree && b->size >= size) return b;
    }
    return nullptr;
}
Block* bestFit(Block* head, int size, long long& comps) {
    Block* best = nullptr;
    for (Block* b = head; b; b = b->next) {
        comps++;
        if (b->isFree && b->size >= size && (!best || b->size < best->size)) best = b;
    }
    return best;
}
Block* worstFit(Block* head, int size, long long& comps) {
    Block* worst = nullptr;
    for (Block* b = head; b; b = b->next) {
        comps++;
        if (b->isFree && b->size >= size && (!worst || b->size > worst->size)) worst = b;
    }
    return worst;
}
Block* findHole(Block* head, int size, int policy, long long& comps) {
    if (policy == 1) return firstFit(head, size, comps);
    if (policy == 2) return bestFit(head, size, comps);
    return worstFit(head, size, comps);
}
void freeMemory(Memory& mem) {
    Block* b = mem.head;
    while (b) {
        Block* n = b->next;
        delete b;
        b = n;
    }
    mem.head = nullptr;
}
void initMemory(Memory& mem, const vector<int>& blocks, int policy) {
    freeMemory(mem);
    mem.policy = policy;
    mem.allocated = 0;
    mem.failed = 0;
    mem.comparisons = 0;
    mem.failedPids.clear();
    mem.owner.clear();
    Block* tail = nullptr;
    int addr = 0;
    int count = blocks.size();
    for (int i = 0; i < count; i++) {
        Block* b = new Block{addr, blocks[i], true, -1, i, tail, nullptr};
        if (tail) tail->next = b; else mem.head = b;
        tail = b;
        addr += blocks[i];
    }
}
bool allocateProcess(Memory& mem, int pid, int size) {
    if (size <= 0 || pid <= 0) return false;
    int cap = mem.owner.size();
    if (pid < cap && mem.owner[pid] != nullptr) return false;
    Block* hole = findHole(mem.head, size, mem.policy, mem.comparisons);
    if (!hole) {
        mem.failed++;
        mem.failedPids.push_back(pid);
        return false;
    }
    if (hole->size > size) {  
        Block* rest = new Block{hole->start + size, hole->size - size, true, -1,
                                hole->partitionId, hole, hole->next};
        if (hole->next) hole->next->prev = rest;
        hole->next = rest;
        hole->size = size;
    }
    hole->isFree = false;
    hole->pid = pid;
    if (pid >= cap) mem.owner.resize(pid + 1, nullptr);
    mem.owner[pid] = hole;
    mem.allocated++;
    return true;
}
bool deallocateProcess(Memory& mem, int pid) {
    int cap = mem.owner.size();
    if (pid <= 0 || pid >= cap || mem.owner[pid] == nullptr) return false;
    Block* b = mem.owner[pid];
    mem.owner[pid] = nullptr;
    b->isFree = true;
    b->pid = -1;
    mem.allocated--;
    if (b->next && b->next->isFree && b->next->partitionId == b->partitionId) {
        Block* n = b->next;
        b->size += n->size;
        b->next = n->next;
        if (n->next) n->next->prev = b;
        delete n;
    }
    if (b->prev && b->prev->isFree && b->prev->partitionId == b->partitionId) {
        Block* p = b->prev;
        p->size += b->size;
        p->next = b->next;
        if (b->next) b->next->prev = p;
        delete b;
    }
    return true;
}
Metrics getMetrics(const Memory& mem) {
    Metrics m;
    m.allocated = mem.allocated;
    m.failed = mem.failed;
    m.failedPids = mem.failedPids;
    m.comparisons = mem.comparisons;
    for (Block* b = mem.head; b; b = b->next) {
        if (b->isFree) {
            m.totalFree += b->size;
            m.holes++;
            m.largestHole = max(m.largestHole, b->size);
        } else {
            m.allocatedMem += b->size;
        }
    }
    long long total = m.totalFree + m.allocatedMem;
    if (m.totalFree > 0) m.externalFrag = 1.0 - 1.0 * m.largestHole / m.totalFree;
    if (total > 0) m.utilization = 100.0 * m.allocatedMem / total;
    return m;
}
string pct(double v) {
    int tenths = v * 10 + 0.5;
    return to_string(tenths / 10) + "." + to_string(tenths % 10) + "%";
}
string pidList(const vector<int>& v) {
    string s;
    for (int p : v) s += "P" + to_string(p) + " ";
    if (s.empty()) s = "-";
    return s;
}
bool readPositive(const string& prompt, int& out) {
    cout << prompt;
    while (true) {
        if (!(cin >> out)) {
            if (cin.eof()) return false;
            cin.clear();
            cin.ignore(1000, '\n');
        } else if (out > 0) {
            return true;
        }
        cout << "Enter a positive whole number: ";
    }
}
void printTable(const Memory& mem, int maxRows = 60) {
    cout << "\n========== MEMORY MAP (" << policyName(mem.policy) << ") ==========\n";
    cout << left << setw(11) << "Partition" << setw(8) << "Start" << setw(8) << "End"
         << setw(8) << "Size" << "Status\n";
    int rows = 0, total = 0;
    for (Block* b = mem.head; b; b = b->next) total++;
    for (Block* b = mem.head; b; b = b->next) {
        if (rows == maxRows) {
            cout << "... " << (total - maxRows) << " more blocks not shown\n";
            break;
        }
        rows++;
        cout << left << setw(11) << (b->partitionId + 1) << setw(8) << b->start << setw(8)
             << (b->start + b->size - 1) << setw(8) << b->size;
        if (b->isFree) cout << "FREE\n"; else cout << "P" << b->pid << "\n";
    }
}
void printLine(const Memory& mem, const string& label, int maxBlocks = 40) {
    cout << left << setw(11) << label << ": ";
    int lastPart = -1, shown = 0;
    for (Block* b = mem.head; b; b = b->next) {
        if (shown == maxBlocks) { cout << " ..."; break; }
        shown++;
        if (lastPart != -1 && b->partitionId != lastPart) cout << " | ";
        lastPart = b->partitionId;
        if (b->isFree) cout << "[free:" << b->size << "]";
        else           cout << "[P" << b->pid << ":" << b->size << "]";
    }
    cout << "\n";
}
void printMetrics(const Metrics& m) {
    cout << "\nAllocated: " << m.allocated << "   Failed: " << m.failed << " (" << pidList(m.failedPids)
         << ")\n";
    cout << "Blocks examined (comparisons): " << m.comparisons << "\n";
    cout << "Total free: " << m.totalFree << "   Holes: " << m.holes << "   Largest hole: "
         << m.largestHole << "\n";
    cout << "External fragmentation: " << pct(m.externalFrag * 100) << "\n";
    cout << "Memory utilization:     " << pct(m.utilization) << "\n";
    cout << "Internal fragmentation: 0 (holes are split, so no space is wasted inside blocks)\n";
}
void rowNum(const string& name, long long a, long long b, long long c) {
    cout << left << setw(24) << name << setw(14) << a << setw(14) << b << setw(14) << c << "\n";
}
void rowText(const string& name, const string& a, const string& b, const string& c) {
    cout << left << setw(24) << name << setw(14) << a << setw(14) << b << setw(14) << c << "\n";
}
bool isBetter(const Metrics& a, const Metrics& b) {
    if (a.allocated != b.allocated) return a.allocated > b.allocated;
    if (a.externalFrag != b.externalFrag) return a.externalFrag < b.externalFrag;
    return a.comparisons < b.comparisons;
}
void runPolicy(Memory& mem, const vector<int>& blocks, const vector<int>& requests,
               int policy, int& nextPid) {
    initMemory(mem, blocks, policy);
    int count = requests.size();
    bool verbose = count <= 50;
    cout << "\n--- " << policyName(policy) << " ---\n";
    for (int i = 0; i < count; i++) {
        bool ok = allocateProcess(mem, i + 1, requests[i]);
        if (verbose) cout << "  P" << i + 1 << " (" << requests[i] << ") " << (ok ? "placed" : "FAILED") << "\n";
    }
    if (!verbose) cout << "  (" << count << " requests processed; per-request lines hidden)\n";
    nextPid = count + 1;
    printTable(mem);
    printMetrics(getMetrics(mem));
}
void compareAll(const vector<int>& blocks, const vector<int>& requests) {
    Memory mems[3];
    Metrics res[3];
    int count = requests.size();
    for (int p = 0; p < 3; p++) {
        initMemory(mems[p], blocks, p + 1);
        for (int i = 0; i < count; i++) allocateProcess(mems[p], i + 1, requests[i]);
        res[p] = getMetrics(mems[p]);
    }
    cout << "\n========== COMPARISON ==========\n";
    cout << left << setw(24) << "Metric";
    for (int p = 1; p <= 3; p++) cout << setw(14) << policyName(p);
    cout << "\n" << string(24 + 14 * 3, '-') << "\n";
    rowNum("Allocated", res[0].allocated, res[1].allocated, res[2].allocated);
    rowNum("Failed", res[0].failed, res[1].failed, res[2].failed);
    rowText("Failed requests", pidList(res[0].failedPids), pidList(res[1].failedPids), pidList(res[2].failedPids));
    rowNum("Blocks examined", res[0].comparisons, res[1].comparisons, res[2].comparisons);
    rowNum("Total free", res[0].totalFree, res[1].totalFree, res[2].totalFree);
    rowNum("Holes", res[0].holes, res[1].holes, res[2].holes);
    rowNum("Largest hole", res[0].largestHole, res[1].largestHole, res[2].largestHole);
    rowText("External frag.", pct(res[0].externalFrag * 100), pct(res[1].externalFrag * 100), pct(res[2].externalFrag * 100));
    rowText("Memory utilization", pct(res[0].utilization), pct(res[1].utilization), pct(res[2].utilization));
    cout << "(Internal fragmentation is 0 for all: holes are split.)\n";
    int best = 0;
    for (int p = 1; p < 3; p++)
        if (isBetter(res[p], res[best])) best = p;
    cout << "\nBest for this input: " << policyName(best + 1) << "\n\n";
    for (int p = 0; p < 3; p++) {
        printLine(mems[p], policyName(p + 1));
        freeMemory(mems[p]);
    }
}
int main() {
    cout << "===== MEMORY ALLOCATION SIMULATOR =====\n";
    int n, m;
    if (!readPositive("\nNumber of memory blocks: ", n)) return 0;
    vector<int> blocks(n);
    cout << "Memory block sizes (" << n << " values):\n";
    for (int& b : blocks)
        if (!readPositive("", b)) return 0;
    if (!readPositive("\nNumber of process requests: ", m)) return 0;
    vector<int> requests(m);
    cout << "Process memory requests (" << m << " values):\n";
    for (int& r : requests)
        if (!readPositive("", r)) return 0;
    int nextPid = m + 1;  
    int curPolicy = 1;
    Memory cur;
    initMemory(cur, blocks, curPolicy);
    while (true) {
        cout << "\n========== MENU ==========\n"
             << "1. Run First Fit\n"
             << "2. Run Best Fit\n"
             << "3. Run Worst Fit\n"
             << "4. Compare all three (does not change current memory)\n"
             << "5. Show memory map\n"
             << "6. Allocate a new process (current policy: " << policyName(curPolicy) << ")\n"
             << "7. Deallocate a process\n"
             << "8. Reset memory\n"
             << "9. Exit\n";
        int choice;
        if (!readPositive("\nEnter choice: ", choice)) break;
        if (choice == 9) {
            break;
        } else if (choice >= 1 && choice <= 3) {
            curPolicy = choice;
            runPolicy(cur, blocks, requests, curPolicy, nextPid);
        } else if (choice == 4) {
            compareAll(blocks, requests);
        } else if (choice == 5) {
            printTable(cur);
            printMetrics(getMetrics(cur));
        } else if (choice == 6) {
            int size;
            if (!readPositive("Request size: ", size)) break;
            int pid = nextPid++;
            bool ok = allocateProcess(cur, pid, size);
            cout << "P" << pid << " (" << size << ") " << (ok ? "placed" : "FAILED: no hole is large enough") << "\n";
            printTable(cur);
        } else if (choice == 7) {
            int pid;
            if (!readPositive("Process ID to deallocate: ", pid)) break;
            if (deallocateProcess(cur, pid)) {
                cout << "Process P" << pid << " deallocated.\n";
                printTable(cur);
            } else {
                cout << "Process P" << pid << " is not currently allocated.\n";
            }
        } else if (choice == 8) {
            initMemory(cur, blocks, curPolicy);
            nextPid = m + 1;
            cout << "Memory reset to the original blocks.\n";
        } else {
            cout << "Invalid choice.\n";
        }
    }
    freeMemory(cur);
    return 0;
}