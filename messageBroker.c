#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define SHM_SIZE 1024
#define MAX_MESSAGES 5

struct message_info {
  char message[SHM_SIZE - sizeof(int)]; 
  int has_message; // Flag indicating presence of a new message
};

int main(int argc, const char *argv[]) {

    key_t key = 1234;

  // Create shared memory segment
  int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
  if (shmid < 0) {
    perror("shmget");
    exit(1);
  }

  // Attach shared memory to the process
  void *shared_memory = shmat(shmid, NULL, 0);
  if (shared_memory == (char *) -1) {
    perror("shmat");
    exit(1);
  }

  struct message_info *message_data = (struct message_info *)shared_memory;

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  } else if (pid == 0) {
    // Child process (receiver)

    for (int i = 0; i < MAX_MESSAGES; i++) {
      // Check for new message flag
      while (message_data->has_message == 0) {
        // No new message, wait for a short duration
        usleep(10000);
      }

      // Read the message from shared memory
      char message[SHM_SIZE - sizeof(int)]; // Buffer to hold the message
      strncpy(message, message_data->message, SHM_SIZE - sizeof(int) - 1);
      message[SHM_SIZE - sizeof(int) - 1] = '\0'; // Ensure null termination

      // Check if message is empty or end signal
      if (strcmp(message, "") == 0) {
        break;
      }

      printf("Received message: %s\n", message);

      // Clear only the message content, but keep the flag and null terminator
      memset(message_data->message, 0, strlen(message));

      // Clear the flag (optional, can be done by sender after reading)
      message_data->has_message = 0;
    }

    // Detach shared memory segment from the process
    if (shmdt(shared_memory) == -1) {
      perror("shmdt");
      exit(1);
    }
  } else {
    char message[100];

    for (int i = 0; i < MAX_MESSAGES; i++) {
      // Prepare message
      sprintf(message, "Message %d\n", i + 1);

      // Set flag to indicate new message
      message_data->has_message = 1;

      // Write the message to shared memory
      strcpy(message_data->message, message);
      printf("Sent message: %s\n", message);

      // Wait for a short duration to simulate message processing
      sleep(2);
    }

    // Send an empty message to signal end of messages
    strcpy(message_data->message, "");
    message_data->has_message = 1;

    // Wait for child process to finish
    wait(NULL);

    // Detach shared memory segment from the process (redundant here)
    if (shmdt(shared_memory) == -1) {
      perror("shmdt");
      exit(1);
    }

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
      perror("shmctl");
      exit(1);
    }
  }

  return 0;
}
