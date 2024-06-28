#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// GLOBAL VARIABLES
const int ARG_SIZE = 5;

// PROCESS STRUCTURE
typedef struct Process {

    // default fields
    int pid;
    int arrival;
    int burst;
    int priority;
    int quantum;

    // engineered fields
    int remaining;
    int waiting;
    int turnaround;
    int start;
    int complete;   // completion time
    bool finished;  // has been fully processed (preemption)
    bool visited;
} Process;

// for ready list
typedef struct Node {
    Process* process; // a pointer to a process
    struct Node* next;
} Node;

// ready list (linked list)
typedef struct RL {
    Node* head;
} RL;

// FUNCTION PROTOTYPES
void fcfs(Process** processes, int num_processes);
void sjf(RL* rl, Process** processes, int num_processes);
void ps(RL* rl, Process** processes, int num_processes);
void rr(RL* rl, Process** processes, int num_processes, int quantum);

int processDiff(const void *p1, const void *p2);
void printProcesses(Process** processes, int num_processes);
Process* findProcess(Process** processes, int num_processes, int pid);

void initRL(RL* rl);
void addNode(RL* rl, Process* p);
void addNodeRR(RL* rl, Process* p);
void addAllNodesRR(RL* rl, Process** processes, int num_processes, int time);
Process* removeNode(RL* rl);
int isEmpty(RL* rl);
void printList(RL* rl);
void wipeProcessTimes(Process** processes, int num_processes);

// SCHEDULE FUNCTION DEFINITIONS
void fcfs(Process** processes, int num_processes) {

    // sort processes by arrival times (asc)
    // uses processDiff as comparison function
    qsort(processes, num_processes, sizeof(Process*), processDiff);

    // array to handle gantt timeline (finite)
    int gantt_size = num_processes * 2;
    int gantt_pid[gantt_size];  // track pids in timeline
    int gantt_start[gantt_size];    // track pid local start
    int gantt_end[gantt_size];  // track pid local end
    int gantt_i = 0;

    // variable to represent schedule's current time
    int time = 0;

    // loop thru all processes
    for (int i = 0; i < num_processes; i++) {
        
        // handle idle time
        if (processes[i]->arrival > time) {

            // add idle time to gantt
            gantt_pid[gantt_i] = -1;    // idle pid = -1
            gantt_start[gantt_i] = time;

            // move time to nearest arrival
            time = processes[i]->arrival;

            // update gantt end time
            gantt_end[gantt_i++] = time;
        }

        // add process to gantt order
        gantt_pid[gantt_i] = processes[i]->pid;
        gantt_start[gantt_i] = time;

        // update process's start time
        processes[i]->start = time;

        // move current time to termination of current process
        time += processes[i]->burst;

        // update gantt end time
        gantt_end[gantt_i++] = time;

        // compute turnaround time (completion - arrival)
        processes[i]->turnaround = time - processes[i]->arrival;

        // compute waiting time (turnaround - burst)
        processes[i]->waiting = processes[i]->turnaround - processes[i]->burst;

        // update process's completion time
        processes[i]->complete = time;

    }

    // print FCFS stats
    printf("\n---------------------------- FCFS ----------------------------\n");
    printf("\tPID\t|\tWaiting \t|\tTurnaround\n");
    for (int i = 0; i < num_processes; i++) {

        // print waiting/turnaround times
        printf("\t %d\t|\t   %d\t\t|\t   %d\n", processes[i]->pid, processes[i]->waiting, processes[i]->turnaround);
    }
    printf("\n");

    // print gantt chart
    printf("Gantt Chart:\n");
    for (int i = 0; i < gantt_i; i++) {

        // print idle time or process lifecycle
        if (gantt_pid[i] == -1) {
            printf("[  %d  ]-----\tIDLE\t-----[  %d  ]\n", gantt_start[i], gantt_end[i]);    
        } else {
            printf("[  %d  ]-----\t%d\t-----[  %d  ]\n", gantt_start[i], gantt_pid[i], gantt_end[i]);
        }
    }
    printf("\n");

    // vars for time stats
    double avg_turnaround = 0.0;
    double avg_waiting = 0.0;
    double throughput = (double) num_processes / time;

    // calculate average turnaround & waiting times
    for (int i = 0; i < num_processes; i++) {
        avg_turnaround += processes[i]->turnaround;
        avg_waiting += processes[i]->waiting;
    }

    avg_turnaround /= num_processes;
    avg_waiting /= num_processes;

    // display overall schedule stats
    printf("Avg. Waiting Time: %f\n", avg_waiting);
    printf("Avg. Turnaround: %f\n", avg_turnaround);
    printf("Throughput: %f\n\n", throughput);

    time = 0;

}

// Shortest-Remaining-Time
void sjf(RL* rl, Process** processes, int num_processes) {

    // reset all process fields to default
    wipeProcessTimes(processes, num_processes);

    // sort processes by arrival times (asc)
    // uses processDiff as comparison function
    qsort(processes, num_processes, sizeof(Process*), processDiff);

    // array to handle gantt timeline (finite)
    int gantt_size = num_processes * 5;
    int gantt_pid[gantt_size];  // track pids in timeline
    int gantt_start[gantt_size];    // track pid local start
    int gantt_end[gantt_size];  // track pid local end
    int gantt_i = 0;
    bool gantt_idle = false;    // flag to detect if idle occurs

    // variables to manage processes and time
    int time = 0;
    int completed = 0;
    int curr_pid = -1;
    Process* p = NULL;

    // loop while RL isn't empty
    while (completed < num_processes) {

        // add processes to RL
        for (int i = 0; i < num_processes; i++) {

            // check if process not finished and arrival reached 
            if (processes[i]->arrival <= time && !processes[i]->finished && !processes[i]->visited) {

                // add process to ready list
                addNode(rl, processes[i]);

                // mark process as visited (for gantt)
                processes[i]->visited = true;
            }
        }

        // handle process if RL not empty
        if (curr_pid == -1 && !isEmpty(rl)) {

            // wrap up gantt idle time if previously idle
            if (gantt_idle) {
                gantt_idle = false;
                gantt_end[gantt_i++] = time;
            }

            // remove next node from ready list
            p = removeNode(rl);

            // toggle current pid
            curr_pid = p->pid;
            // printf("Process %d started at time %d\n", p->pid, time);

            // update process start time
            if (!p->visited) {
                p->start = time;
            }

            // add process pid/start to gantt chart (only inc. when preempted/finished)
            gantt_pid[gantt_i] = p->pid;
            gantt_start[gantt_i] = time;

        }

        // run current pid for 1 time unit
        if (curr_pid != -1) {

            // increment time by 1
            time++;

            // update remaining time of current process
            p->remaining -= 1;

            // check if process is done
            if (p->remaining == 0) {

                // mark process as finished
                p->finished = true;

                // remove current process (new process from RL next iter.)
                curr_pid = -1;
                // printf("Process %d finished at time %d\n", p->pid, time);

                // update process completion time
                p->complete = time;

                // compute process turnaround time (completion - arrival)
                p->turnaround = time - p->arrival;

                // compute process waiting time (turnaround - burst)
                p->waiting = p->turnaround - p->burst;

                // add end time to gantt and increment to next gantt event
                gantt_end[gantt_i++] = time;

                // update completed processes count
                completed++;

            // process not done, check preemption
            } else {

                // check if preemption available
                for (int i = 0; i < num_processes; i++) {

                    // see if any other processes can trigger preemption
                    if (processes[i]->arrival <= time && !processes[i]->finished && !processes[i]->visited && \
                    processes[i]->remaining < p->remaining)
                    {
                        // preempt current process ----

                        // add current process back into ready list
                        addNode(rl, p);
                        // printf("Process %d preempted at time %d\n", p->pid, time);

                        // add local end time to gantt and increment to next gantt event
                        gantt_end[gantt_i++] = time;

                        // empty current process for rescheduling (next increment)
                        curr_pid = -1;

                        break;
                    }
                }
            }

        // idle
        } else {

            // update gantt start/pid (idle = -1)
            if (!gantt_idle) {
                gantt_start[gantt_i] = time;
                gantt_pid[gantt_i] = -1;
            }

            // toggle gantt idle flag
            gantt_idle = true;

            time++;

        }

    }

    // print SJF stats
    printf("\n---------------------------- SJF ----------------------------\n");
    printf("\tPID\t|\tWaiting \t|\tTurnaround\n");
    for (int i = 0; i < num_processes; i++) {

        // print waiting/turnaround times
        printf("\t %d\t|\t   %d\t\t|\t   %d\n", processes[i]->pid, processes[i]->waiting, processes[i]->turnaround);
    }
    printf("\n");

    // print gantt chart
    printf("Gantt Chart:\n");
    for (int i = 0; i < gantt_i; i++) {

        // print idle time or process lifecycle
        if (gantt_pid[i] == -1) {
            printf("[  %d  ]-----\tIDLE\t-----[  %d  ]\n", gantt_start[i], gantt_end[i]);    
        } else {
            printf("[  %d  ]-----\t%d\t-----[  %d  ]\n", gantt_start[i], gantt_pid[i], gantt_end[i]);
        }
    }
    printf("\n");


    // vars for time stats
    double avg_turnaround = 0.0;
    double avg_waiting = 0.0;
    double throughput = (double) num_processes / time;

    // calculate average turnaround & waiting times
    for (int i = 0; i < num_processes; i++) {
        avg_turnaround += processes[i]->turnaround;
        avg_waiting += processes[i]->waiting;
    }

    avg_turnaround /= num_processes;
    avg_waiting /= num_processes;

    // display overall schedule stats
    printf("Avg. Waiting Time: %f\n", avg_waiting);
    printf("Avg. Turnaround: %f\n", avg_turnaround);
    printf("Throughput: %f\n\n", throughput);

}

// Priority Scheduling (w/o preemption)
void ps(RL* rl, Process** processes, int num_processes) {

    // reset all process fields to default
    wipeProcessTimes(processes, num_processes);

    // sort processes by arrival times (asc)
    // uses processDiff as comparison function
    qsort(processes, num_processes, sizeof(Process*), processDiff);

    // array to handle gantt timeline (finite)
    int gantt_size = num_processes * 2;
    int gantt_pid[gantt_size];  // track pids in timeline
    int gantt_start[gantt_size];    // track pid local start
    int gantt_end[gantt_size];  // track pid local end
    int gantt_i = 0;
    bool gantt_idle = false;    // flag to detect if idle occurs

    // var to track time
    int time = 0;

    // var to track num finished processes
    int completed = 0;

    // var to hold running process
    Process* p = NULL;

    // loop until all processes are done
    while (completed < num_processes) {

        // vars to track process with the highest priority
        int top_priority = 100000;
        int top_i = -1;

        // loop thru all processes
        for (int i = 0; i < num_processes; i++) {

            // check if process can be added to RL
            if (processes[i]->arrival <= time && !processes[i]->finished) {

                // compare priority
                if (processes[i]->priority < top_priority) {

                    // set new top priority
                    top_priority = processes[i]->priority;
                    top_i = i;

                }

            }

        }

        // if top priority process exists, handle it
        if (top_i != -1) {

            // wrap up idle
            if (gantt_idle) {

                gantt_end[gantt_i++] = time;
                gantt_idle = false;

            }

            // get top priority process
            Process* p = processes[top_i];

            // check if memory allocated properly
            if (p == NULL) {
                printf("ERROR allocating memory for process in PS\n");
                exit(1);
            }

            // update gantt
            gantt_pid[gantt_i] = p->pid;
            gantt_start[gantt_i] = time;

            // update number of completed processes
            completed++;

            // update process start time
            p->start = time;

            // update time by process burst
            time += p->burst;

            // update process completion time
            p->complete = time;

            // calculate process turnaround and waiting times
            p->turnaround = time - p->arrival;
            p->waiting = p->turnaround - p->burst;

            // update process finished flag
            p->finished = true;

            // update gantt chart
            gantt_end[gantt_i++] = time;

        // if no process, idle
        } else {

            // update gantt chart (idle pid = -1)
            if(!gantt_idle) {
                gantt_start[gantt_i] = time;
            }
            gantt_pid[gantt_i] = -1;
            gantt_idle = true;

            // update time
            time++;

        }

    }

    // print PS stats
    printf("\n---------------------------- PS ----------------------------\n");
    printf("\tPID\t|\tWaiting \t|\tTurnaround\n");
    for (int i = 0; i < num_processes; i++) {

        // print waiting/turnaround times
        printf("\t %d\t|\t   %d\t\t|\t   %d\n", processes[i]->pid, processes[i]->waiting, processes[i]->turnaround);
    }
    printf("\n");

    // print gantt chart
    printf("Gantt Chart:\n");
    for (int i = 0; i < gantt_i; i++) {

        // print idle time or process lifecycle
        if (gantt_pid[i] == -1) {
            printf("[  %d  ]-----\tIDLE\t-----[  %d  ]\n", gantt_start[i], gantt_end[i]);    
        } else {
            printf("[  %d  ]-----\t%d\t-----[  %d  ]\n", gantt_start[i], gantt_pid[i], gantt_end[i]);
        }
    }
    printf("\n");

    // vars for time stats
    double avg_turnaround = 0.0;
    double avg_waiting = 0.0;
    double throughput = (double) num_processes / time;

    // calculate average turnaround & waiting times
    for (int i = 0; i < num_processes; i++) {
        avg_turnaround += processes[i]->turnaround;
        avg_waiting += processes[i]->waiting;
    }

    avg_turnaround /= num_processes;
    avg_waiting /= num_processes;

    // display overall schedule stats
    printf("Avg. Waiting Time: %f\n", avg_waiting);
    printf("Avg. Turnaround: %f\n", avg_turnaround);
    printf("Throughput: %f\n\n", throughput);


}

// Round Robin
void rr(RL* rl, Process** processes, int num_processes, int quantum) {

    // exit if quantum is invalid
    if (quantum <= 0) {
        printf("Error invalid quantum: %d\n", quantum);
        exit(1);
    }

    // reset all process info to default
    wipeProcessTimes(processes, num_processes);

    // sort processes by arrival times (asc)
    // uses processDiff as comparison function
    qsort(processes, num_processes, sizeof(Process*), processDiff);

    // vars for scheduling
    int time = 0;
    int completed = 0;

    // vars for gantt
    int gantt_size = num_processes * 8;
    int gantt_pid[gantt_size];  // track pids in timeline
    int gantt_start[gantt_size];    // track pid local start
    int gantt_end[gantt_size];  // track pid local end
    int gantt_i = 0;
    bool gantt_idle = false;    // flag to detect if idle occurs

    // loop until all processes finished
    while (completed < num_processes) {

        for (int i = 0; i < num_processes; i++) {

            // add new processes to ready list
            if (processes[i]->arrival <= time && !processes[i]->finished && !processes[i]->visited) {
                
                // add process to ready list TODO: add nodes by arrival during quantum (before completion)
                addNodeRR(rl, processes[i]);

                // mark process as visited
                processes[i]->visited = true;
            }
        }

        // handle process if RL not empty
        if (!isEmpty(rl)) {

            // wrap update gantt idle
            if (gantt_idle) {
                gantt_end[gantt_i++] = time;
                gantt_idle = false;
            }

            // get next node from ready list
            Process* p = removeNode(rl);

            // check if process can run through quantum length
            if (p->remaining > quantum) {

                // printf("Process %d ran from %d to %d\n", p->pid, time, time + quantum);

                // update gantt start/pid
                gantt_start[gantt_i] = time;
                gantt_pid[gantt_i] = p->pid;

                // update time
                time += quantum;

                // update gantt end
                gantt_end[gantt_i++] = time;

                // // aggregate process time fragment (redacted)
                // if (gantt_i != 0 && gantt_pid[gantt_i - 1] == p->pid) {

                //     // update prev gantt entry's end time to now
                //     gantt_end[gantt_i - 1] = time;

                // } else {
                //     gantt_end[gantt_i++] = time;   
                // }

                // update process's remaining time
                p->remaining -= quantum;

                // add all nodes that became available between quantum change
                addAllNodesRR(rl, processes, num_processes, time);

                // put process back into ready list
                addNodeRR(rl, p);

            // process has less remaining time than quantum
            } else {

                // update gantt start/pid
                gantt_start[gantt_i] = time;
                gantt_pid[gantt_i] = p->pid;

                // update time by remaining time
                time += p->remaining;

                // update gantt end
                gantt_end[gantt_i++] = time;

                // // aggregate process time fragment (redacted)
                // if (gantt_i != 0 && gantt_pid[gantt_i - 1] == p->pid) {

                //     // update prev gantt entry's end time to now
                //     gantt_end[gantt_i - 1] = time;
                    
                // } else {
                //     gantt_end[gantt_i++] = time;   
                // }

                // update process's remaining time
                p->remaining = 0;

                completed++;

                p->complete = time;

                // compute turnaround and waiting times
                p->turnaround = p->complete - p->arrival;
                p->waiting = p->turnaround - p->burst;

                // printf("Process %d completed at time %d\n", p->pid, time);

            }

        // run idle
        } else {

            // update gantt start/pid (idle = -1)
            if (!gantt_idle) {
                gantt_start[gantt_i] = time;
                gantt_pid[gantt_i] = -1;
            }

            // toggle gantt idle flag
            gantt_idle = true;

            time++;

            // printf("Idle\n");

        }
    }

    // print RR stats
    printf("\n---------------------------- RR ----------------------------\n");
    printf("\tPID\t|\tWaiting \t|\tTurnaround\n");
    for (int i = 0; i < num_processes; i++) {

        // print waiting/turnaround times
        printf("\t %d\t|\t   %d\t\t|\t   %d\n", processes[i]->pid, processes[i]->waiting, processes[i]->turnaround);
    }
    printf("\n");

    // print gantt chart
    printf("Gantt Chart:\n");
    for (int i = 0; i < gantt_i; i++) {

        // print idle time or process lifecycle
        if (gantt_pid[i] == -1) {
            printf("[  %d  ]-----\tIDLE\t-----[  %d  ]\n", gantt_start[i], gantt_end[i]);    
        } else {
            printf("[  %d  ]-----\t%d\t-----[  %d  ]\n", gantt_start[i], gantt_pid[i], gantt_end[i]);
        }
    }
    printf("\n");

    // vars for time stats
    double avg_turnaround = 0.0;
    double avg_waiting = 0.0;
    double throughput = (double) num_processes / time;

    // calculate average turnaround & waiting times
    for (int i = 0; i < num_processes; i++) {
        avg_turnaround += processes[i]->turnaround;
        avg_waiting += processes[i]->waiting;
    }

    avg_turnaround /= num_processes;
    avg_waiting /= num_processes;

    // display overall schedule stats
    printf("Avg. Waiting Time: %f\n", avg_waiting);
    printf("Avg. Turnaround: %f\n", avg_turnaround);
    printf("Throughput: %f\n\n", throughput);


}

// HELPER FUNCTION DEFINITIONS

// helper to compare process arrival/pid for sorting
int processDiff(const void *p1, const void *p2) {

    // convert process ptrs to processes
    Process* process1 = *(Process**)p1;
    Process* process2 = *(Process**)p2;

    // return pid difference if arrival times are the same
    if (process1->arrival == process2->arrival) {
        return process1->pid - process2->pid;
    }

    // return the arrival time difference btw processes
    return process1->arrival - process2->arrival;
}

// print general info about processes
void printProcesses(Process** processes, int num_processes) {

    printf("\n\tPID \t|\tARRIV \t|\tBURST \t|\tPRIOR \t|\tQUANT \t|\tREMAIN \t|\tWAIT \t|\tTURN\n");
    printf("--------------------------------------------------------------------------------------------------------------------------------\n");

    for (int j = 0; j < num_processes; j++) {
        printf("\t%d\t|\t%d\t|\t%d\t|\t%d\t|\t%d\t|\t%d\t|\t%d\t|\t%d\n", \
            processes[j]->pid,
            processes[j]->arrival,
            processes[j]->burst,
            processes[j]->priority,
            processes[j]->quantum,
            processes[j]->remaining,
            processes[j]->waiting,
            processes[j]->turnaround
        );
    }
    printf("\n");

}
 
// find process object reference by pid
Process* findProcess(Process** processes, int num_processes, int pid) {
    for (int i = 0; i < num_processes; i++) {
        if (processes[i]->pid == pid) {
            return processes[i];
        }
    }
    
    // display error if process not found
    printf("No process with PID: %d exists", pid);
    exit(1);
}

// READY LIST FUNCTIONS

// initialize head of a new ready list (linked list)
void initRL(RL* rl) {
    rl->head = NULL;
}

// add a node to the ready list (node->process)
void addNode(RL* rl, Process* p) {

    // alloc mem for new node
    Node* node = (Node*) malloc (sizeof(Node));

    // set new node's info
    node->process = p; // node->process is a pointer to a process
    node->next = NULL;

    if (isEmpty(rl) || p->remaining < rl->head->process->remaining) {
        node->next = rl->head;
        rl->head = node;
    } else {

        Node* curr = rl->head;

        // find end of list TODO: consider fixing this to be simpler
        if (!p->visited) {
            while (curr->next != NULL && curr->next->process->remaining < p->remaining) {
                curr = curr->next;
            }
        } else {
            while (curr->next != NULL) {
                curr = curr->next;
            }
        }
        // while (curr->next != NULL && curr->next->process->remaining < p->remaining) {
        //     curr = curr->next;
        // }

        // add node to end of list
        node->next = curr->next;
        curr->next = node;
    }
}

// add a node to the ready list (node->process)
void addNodeRR(RL* rl, Process* p) {

    // alloc mem for new node
    Node* node = (Node*) malloc (sizeof(Node));

    // set new node's info
    node->process = p; // node->process is a pointer to a process
    node->next = NULL;

    // add to head of list of list is empty
    if (isEmpty(rl)) {
        node->next = rl->head;
        rl->head = node;
    } else {

        // set tracker node
        Node* curr = rl->head;

        // find end of ready list
        while (curr->next != NULL) {
            curr = curr->next;
        }        

        // add node to end of list
        node->next = curr->next;
        curr->next = node;
    }
}

// add all available nodes to ready list
void addAllNodesRR(RL* rl, Process** processes, int num_processes, int time) {

    for (int i = 0; i < num_processes; i++) {

        // add new processes to ready list
        if (processes[i]->arrival <= time && !processes[i]->finished && \
        !processes[i]->visited) {
            
            // add process to ready list TODO: add nodes by arrival during quantum (before completion)
            addNodeRR(rl, processes[i]);

            // mark process as visited
            processes[i]->visited = true;
        }

    }
}

// check if ready list is empty
int isEmpty(RL* rl) {

    // return if ready list's head points to nothing
    return (rl->head == NULL);
}

// remove node from ready list (FIFO)
Process* removeNode(RL* rl) {
    
    // check if RL has any processes
    if (isEmpty(rl)) {
        printf("RL is empty. Can't remove");
        exit(1);
    }

    // get node to remove
    Node* node = rl->head;

    // set new head
    rl->head = rl->head->next;

    // save process associated with node
    Process* p = node->process;

    // deallocate node from mem
    free(node);

    // return process of deleted node
    return p;
}

// print all nodes currently in ready list
void printList(RL* rl) {

    // check if list is empty
    if (isEmpty(rl)) {
        printf("List is Empty\n");
        return;
    }

    Node* node = rl->head;
    printf("\nRL: ");
    
    // print PIDs currently in ready list
    while (node != NULL) {
        printf("%d ", node->process->pid);
        node = node->next;
    }
    printf("\n");
}

// wipe time info for all processes
void wipeProcessTimes(Process** processes, int num_processes) {

    // reset all but pid, arrival, burst, priority, and quantum 
    for (int i = 0; i < num_processes; i++) {
        processes[i]->remaining = processes[i]->burst;
        processes[i]->start = 0;
        processes[i]->complete = 0;
        processes[i]->finished = false;
        processes[i]->visited = false;
    }
}

// MAIN CALL
int main(int argc, char* argv[]) {

    // check if num args is valid
    if (argc != 2) {
        printf("INVALID CALL -- Usage ... ./schedule test1.txt\n");
        exit(1);
    }

    // declare file i/o vars
    FILE* file_ptr;
    char ln[16];
    const char comma[] = ",";

    // declare process vars
    Process** processes;
    int num_processes = 0;

    // open file for reading
    file_ptr = fopen(argv[1], "r");

    // check if error occurred when opening the file
    if (file_ptr == NULL) {
        printf("ERROR reading file\n");
        exit(1);
    }

    // count the number of processes in file
    while (fgets(ln, sizeof(ln), file_ptr)) {
        num_processes++;
    }

    // reset file ptr to beginning of file
    fseek(file_ptr, 0, SEEK_SET);

    // dynamically allocate memory for list of processes
    processes = (Process**) malloc(num_processes * sizeof(Process*));

    // check if error occurred while allocating mem
    if (processes == NULL) {
        printf("ERROR allocating memory for processes\n");
        exit(1);
    }

    // read file line-by-line
    int j = 0;
    while (fgets(ln, sizeof(ln), file_ptr) && j < num_processes) {

        // allocate memory for new process
        processes[j] = (Process*) malloc (sizeof(Process));

        // check if memory was allocated correctly
        if (processes[j] == NULL) {
            printf("Error allocating memory for new process");
            exit(1);
        }

        // create process object with line info 
        sscanf(ln, "%d,%d,%d,%d,%d",
            &(processes[j]->pid),
            &(processes[j]->arrival),
            &(processes[j]->burst),
            &(processes[j]->priority),
            &(processes[j]->quantum)
        );

        // set engineered process info
        processes[j]->remaining = processes[j]->burst;
        processes[j]->waiting = 0;
        processes[j]->turnaround = 0;
        processes[j]->complete = 0;
        processes[j]->finished = false;
        processes[j]->visited = false;

        // increment to next process slot in process list
        j++;
    }

    // initialize ready list
    RL* rl = (RL*) malloc (sizeof(RL));
    if (rl == NULL) {
        printf("Error allocating mem for ready list\n");
        exit(1);
    }
    initRL(rl);    

    // CALL SCHEDULE FUNCTIONS

    // run First-Come-First-Serve on file info
    fcfs(processes, num_processes);

    // run Shortest-Remaining-Time on file info
    sjf(rl, processes, num_processes);

    // run Priority Scheduling
    ps(rl, processes, num_processes);

    // run Round-Robin on file info
    int quantum = processes[0]->quantum;
    rr(rl, processes, num_processes, quantum);

    // CLEAN MEMORY AND FILE I/O

    // close the file
    fclose(file_ptr);

    // free process memory
    free(processes);

    // free ready list memory
    free(rl);

    return 0;

}