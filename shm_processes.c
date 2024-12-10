#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

#define NUM_ITERATIONS 25

void DearOldDad(sem_t *mutex, int *BankAccount);
void PoorStudent(sem_t *mutex, int *BankAccount, int ID);
void LovableMom(sem_t *mutex, int *BankAccount);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Number of Parents> <Number of Children>\n", argv[0]);
        exit(1);
    }

    int numParents = atoi(argv[1]);
    int numPoorStudents = atoi(argv[2]);
    int ShmID;
    int *ShmPTR;
    pid_t pid;

    // Create shared memory for the bank account
    ShmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** Error creating shared memory ***\n");
        exit(1);
    }

    ShmPTR = (int *)shmat(ShmID, NULL, 0);
    if (ShmPTR == (int *)-1) {
        printf("*** Error attaching shared memory ***\n");
        exit(1);
    }

    ShmPTR[0] = 0; // Initialize BankAccount

    // Create semaphore
    sem_t *mutex = sem_open("/mutex", O_CREAT, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("Semaphore creation failed");
        exit(1);
    }

    // Fork Dear Old Dad process
    pid = fork();
    if (pid < 0) {
        printf("*** Error creating Dear Old Dad process ***\n");
        exit(1);
    } else if (pid == 0) {
        DearOldDad(mutex, ShmPTR);
        exit(0);
    }

    // Fork Poor Student processes
    for (int i = 0; i < numPoorStudents; i++) {
        pid = fork();
        if (pid < 0) {
            printf("*** Error creating Poor Student process ***\n");
            exit(1);
        } else if (pid == 0) {
            PoorStudent(mutex, ShmPTR, i + 1);
            exit(0);
        }
    }

    // Fork Lovable Mom process
    if (numParents >= 1) {
        pid = fork();
        if (pid < 0) {
            printf("*** Error creating Lovable Mom process ***\n");
            exit(1);
        } else if (pid == 0) {
            LovableMom(mutex, ShmPTR);
            exit(0);
        }
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0);

    // Clean up shared memory and semaphore
    shmdt((void *)ShmPTR);
    shmctl(ShmID, IPC_RMID, NULL);
    sem_close(mutex);
    sem_unlink("/mutex");

    printf("All processes have completed successfully.\n");
    return 0;
}

void LovableMom(sem_t *mutex, int *BankAccount) {
    srand(time(NULL) + getpid());
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        sleep(rand() % 11); // Sleep for 0-10 seconds
        printf("Lovable Mom: Checking balance...\n");

        sem_wait(mutex); // Lock semaphore
        int localBalance = *BankAccount;

        if (localBalance <= 100) {
            int deposit = rand() % 126; // Random deposit amount (0-125)
            localBalance += deposit;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", deposit, localBalance);
            *BankAccount = localBalance; // Update shared BankAccount
        }
        sem_post(mutex); // Unlock semaphore
    }
}

void DearOldDad(sem_t *mutex, int *BankAccount) {
    srand(time(NULL) + getpid());
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        sleep(rand() % 6); // Sleep for 0-5 seconds
        printf("Dear Old Dad: Checking balance...\n");

        sem_wait(mutex); // Lock semaphore
        int account = *BankAccount;

        if (account < 100) {
            int deposit = rand() % 101; // Random deposit amount (0-100)
            if (deposit % 2 == 0) { // Even number deposit
                account += deposit;
                printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", deposit, account);
            } else {
                printf("Dear Old Dad: Doesn't have any money to give.\n");
            }
        } else {
            printf("Dear Old Dad: Thinks student has enough cash ($%d).\n", account);
        }

        *BankAccount = account; // Update shared BankAccount
        sem_post(mutex); // Unlock semaphore
    }
}

void PoorStudent(sem_t *mutex, int *BankAccount, int ID) {
    srand(time(NULL) + getpid());
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        sleep(rand() % 6); // Sleep for 0-5 seconds

        sem_wait(mutex); // Lock semaphore
        int account = *BankAccount;

        printf("Poor Student %d: Checking balance...\n", ID);
        int need = rand() % 51; // Random withdrawal amount (0-50)

        if (need % 2 == 0) { // Even number means attempt to withdraw
            printf("Poor Student %d needs $%d\n", ID, need);

            if (need <= account) {
                account -= need; // Perform withdrawal
                printf("Poor Student %d: Withdraws $%d / Balance = $%d\n", ID, need, account);
            } else {
                printf("Poor Student %d: Not enough cash (Balance = $%d)\n", ID, account);
            }
        } else {
            printf("Poor Student %d: Just checking balance = $%d\n", ID, account);
        }

        *BankAccount = account; // Update shared BankAccount
        sem_post(mutex); // Unlock semaphore
    }
    printf("Poor Student %d is exiting.\n", ID);
}
