/*
How are sockets made non-blocking?
  sockets are made non-blocking using the fcntl functions and passing in the
O_NONBLOCK flag.

What sockets are made non-blocking?
  the server socket and each client socket that connects

Why are these sockets made non-blocking? What purpose does it serve?
  the server socket being nonblocking means that accept() does not pause in
waiting for a client. that means if 1 client isnt ready, it will move on to the
next, resulting in a possible discrepancy beetween expected messages and actual
messages. the non blocking client sockets for read() serves this same purpose
except with checking if there is a new message instead of a new client.

Questions to answer at top of server.c:
(You should not need to change client.c)
Understanding the Client:
1. How is the client sending data to the server? What protocol?

Client is sending data to the server using Internet Protocol (IP) addresses

2. What data is the client sending to the server?

each element in the messages array

Understanding the Server:
1. Explain the argument that the `run_acceptor` thread is passed as an argument.

the argument that is passed to the run_acceptor thread is a struct that contains
3 fields:
1. atomic boolean variable <run>: atomic means that this variable can be safely
accessed and modified by multiple threads without explicit locking. this
variable tells the thread whether to keep accepting new client connections. the
variable is initially set to true and then false in the main thread after the
expected amount of client messages are received.

2. struct list_handle *handle is a representation of the access point of the
linkedlist storing all received messages from clients. this struct contains a
pointer to the last (or "current") node in the linkedlist, giving the client
thread a pointer to add the next message node to. for each client that joins,
the run_acceptor thread passes the initialized handle to its respective client
thread where it may be updated with thread safety in mind.

3. list_lock is a mutex created also with the purpose of thread safety to be
passed as an argument to each client thread to use, so they can safely add a new
message node to the current end of the linked list for each message received.

these structs and arguments highlight a modular design philosophy which avoids
the use of global variables, resulting in a cleaner, more readable and
maintanable code structure. although different parts of the code require access
to the linked list of messages, its declaration is contained within the main
function and passed as a parameter to others.

2. How are received messages stored?
  in the linkedlist created in the main function, with the run_client function
adding a new message node to the list using the list_handle struct containing
the current endpoint of the list.

3. What does `main()` do with the received messages?
  calls a function to print them all out, checks if the amount of messages
received is the expected amount. if so, print success, if not, exit with an
error code and print an error (not stderr)

  the head of the linkedlist is passed to the function.

4. How are threads used in this sample code?
  there is a separate thread for accepting clients, and each client that joins
is given its own thread.

  the main thread initializes all the variables and mutexes (linked_list,
list_lock) and the argument to be passed to  the accepter thread. it then
creates the accepter thread which runs parallel to the main thread and waits in
a busy loop until the expected number of messages from all the clients is
reached.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUF_SIZE 1024
#define PORT 8001
#define LISTEN_BACKLOG 32
#define MAX_CLIENTS 4
#define NUM_MSG_PER_CLIENT 5

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

struct list_node {
  struct list_node *next;
  void *data;
};

struct list_handle {
  struct list_node *last;
  volatile uint32_t count;
};

struct client_args {
  atomic_bool run;

  int cfd;
  struct list_handle *list_handle;
  pthread_mutex_t *list_lock;
};

struct acceptor_args {
  atomic_bool run;

  struct list_handle *list_handle;
  pthread_mutex_t *list_lock;
};

int init_server_socket() {
  struct sockaddr_in addr;

  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    handle_error("bind");
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) {
    handle_error("listen");
  }

  return sfd;
}

// Set a file descriptor to non-blocking mode
void set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl F_GETFL");
    exit(EXIT_FAILURE);
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl F_SETFL");
    exit(EXIT_FAILURE);
  }
}

void add_to_list(struct list_handle *list_handle, struct list_node *new_node) {
  struct list_node *last_node = list_handle->last;
  last_node->next = new_node;
  list_handle->last = last_node->next;
  list_handle->count++;
}

int collect_all(struct list_node head) {
  struct list_node *node = head.next; // get first node after head
  uint32_t total = 0;

  while (node != NULL) {
    printf("Collected: %s\n", (char *)node->data);
    total++;

    // Free node and advance to next item
    struct list_node *next = node->next;
    free(node->data);
    free(node);
    node = next;
  }

  return total;
}

static void *run_client(void *args) {
  struct client_args *cargs = (struct client_args *)args;
  int cfd = cargs->cfd;
  set_non_blocking(cfd);

  char msg_buf[BUF_SIZE];

  while (cargs->run) {
    ssize_t bytes_read = read(cfd, &msg_buf, BUF_SIZE);
    if (bytes_read == -1) {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
        perror("Problem reading from socket!\n");
        break;
      }
    } else if (bytes_read > 0) {
      // Create node with data
      struct list_node *new_node = malloc(sizeof(struct list_node));
      new_node->next = NULL;
      new_node->data = malloc(BUF_SIZE);
      memcpy(new_node->data, msg_buf, BUF_SIZE);

      struct list_handle *list_handle = cargs->list_handle;
      // TODO: Safely use add_to_list to add new_node to the list
      pthread_mutex_lock(cargs->list_lock);
      add_to_list(list_handle, new_node);
      pthread_mutex_unlock(cargs->list_lock);
    }
  }

  if (close(cfd) == -1) {
    perror("client thread close");
  }
  return NULL;
}

static void *run_acceptor(void *args) {
  int sfd = init_server_socket();
  set_non_blocking(sfd);

  struct acceptor_args *aargs = (struct acceptor_args *)args;
  pthread_t threads[MAX_CLIENTS];
  struct client_args client_args[MAX_CLIENTS];

  printf("Accepting clients...\n");

  uint16_t num_clients = 0;
  while (aargs->run) {
    if (num_clients < MAX_CLIENTS) {
      int cfd = accept(sfd, NULL, NULL);
      if (cfd == -1) {
        if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
          handle_error("accept");
        }
      } else {
        printf("Client connected!\n");

        client_args[num_clients].cfd = cfd;
        client_args[num_clients].run = true;
        client_args[num_clients].list_handle = aargs->list_handle;
        client_args[num_clients].list_lock = aargs->list_lock;
        num_clients++;

        // TODO: Create a new thread to handle the client
        pthread_create(&threads[num_clients - 1], NULL, run_client,
                       &client_args[num_clients - 1]);
      }
    }
  }

  printf("Not accepting any more clients!\n");

  // Shutdown and cleanup
  for (int i = 0; i < num_clients; i++) {
    // TODO: Set flag to stop the client thread
    client_args[i].run = false;
    // TODO: Wait for the client thread and close its socket
    pthread_join(threads[i], NULL);
    close(client_args[i].cfd);
  }

  if (close(sfd) == -1) {
    perror("closing server socket");
  }
  return NULL;
}

int main() {
  pthread_mutex_t list_mutex;
  pthread_mutex_init(&list_mutex, NULL);

  // List to store received messages
  // - Do not free list head (not dynamically allocated)
  struct list_node head = {NULL, NULL};
  struct list_node *last = &head;
  struct list_handle list_handle = {
      .last = &head,
      .count = 0,
  };

  pthread_t acceptor_thread;
  struct acceptor_args aargs = {
      .run = true,
      .list_handle = &list_handle,
      .list_lock = &list_mutex,
  };
  pthread_create(&acceptor_thread, NULL, run_acceptor, &aargs);

  // TODO: Wait until enough messages are received
  bool full = false;
  while (!full) {
    pthread_mutex_lock(&list_mutex);
    if (list_handle.count >= MAX_CLIENTS * NUM_MSG_PER_CLIENT)
      full = true;
    pthread_mutex_unlock(&list_mutex);
  }

  aargs.run = false;
  pthread_join(acceptor_thread, NULL);

  if (list_handle.count != MAX_CLIENTS * NUM_MSG_PER_CLIENT) {
    printf("Not enough messages were received!\n");
    return 1;
  }

  int collected = collect_all(head);
  printf("Collected: %d\n", collected);
  if (collected != list_handle.count) {
    printf("Not all messages were collected!\n");
    return 1;
  } else {
    printf("All messages were collected!\n");
  }

  pthread_mutex_destroy(&list_mutex);

  return 0;
}
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8001
#define BUF_SIZE 1024
#define ADDR "127.0.0.1"

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define NUM_MSG 5

static const char *messages[NUM_MSG] = {"Hello", "Apple", "Car", "Green",
                                        "Dog"};

int main() {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
    handle_error("inet_pton");
  }

  if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    handle_error("connect");
  }

  char buf[BUF_SIZE];
  for (int i = 0; i < NUM_MSG; i++) {
    sleep(1);
    // prepare message
    // this pads the desination with NULL
    strncpy(buf, messages[i], BUF_SIZE);

    if (write(sfd, buf, BUF_SIZE) == -1) {
      handle_error("write");
    } else {
      printf("Sent: %s\n", messages[i]);
    }
  }

  exit(EXIT_SUCCESS);
}
