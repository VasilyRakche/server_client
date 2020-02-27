// Client side C/C++ program to demonstrate Socket programming
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8888
#define TRUE 1
#define DELAY 10000 // For multiple transition from drive-by-wire

class REECan {
public:
  // Messages for drive-by-wire
  std::string steering_msg, throttle_msg, brakes_msg;

  // recieved message should contain command and id
  REECan(char *message) {
    sscanf(message, "%d %d %d %d %d %d %d %d %d", &id, &values[0], &values[1],
           &values[2], &values[3], &values[4], &values[5], &values[6],
           &values[7]);
    if (id == 100) {
      steering_msg = fill_message(101, 440, 0);
      throttle_msg = fill_message(102, 100, 1);
      brakes_msg = fill_message(103, 100, 2);
    }
  }

  void print_values() {
    for (int i = 0; i < 8; i++) {
      printf("%d ", values[i]);
    }
    printf("\n");
  }

  // id of message is compared to one from socket
  int check_id(int socket_id) {
    if (id == socket_id)
      return 1;
    return 0;
  }

  // Is test message
  int is_test() {
    if (id == 200)
      return 1;
    return 0;
  }

private:
  int id;
  int values[8];

  // Fill values of messages for drive-by-wire
  std::string fill_message(int msg_id, int msg_value, int index) {
    std::stringstream ss;
    ss << msg_id << ' ' << msg_value * values[index] << ' ';
    for (int i = 1; i < 7; i++)
      ss << 0 << ' ';
    ss << 0; // For good formating without ' ' at the end
    return ss.str();
  }
};

int main(int argc, char const *argv[]) {
  int sock = 0, valread;
  std::string input_str, test_msg;
  struct sockaddr_in serv_addr;
  char buffer[1024] = {0};
  fd_set readfds;
  int max_sd;

  if (argc < 2) {
    fprintf(stderr, "ERROR, no ID provided\n");
    exit(0);
  }

  int id = atoi(argv[1]);

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if (argc < 2) {
    fprintf(stderr, "ERROR, no ID provided\n");
    exit(0);
  }
  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  valread = read(sock, buffer, 1024);
  buffer[valread] = '\0';
  printf("%s", buffer);
  printf("Connection established\n");

  while (TRUE) {
    // clear the socket set
    FD_ZERO(&readfds);
    // add socket and stdin to set
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    // STDIN is always 0, sock should be >0
    max_sd = sock;
    // Wait for some interupt
    select(max_sd + 1, &readfds, NULL, NULL, NULL);

    if (FD_ISSET(sock, &readfds)) {
      valread = read(sock, buffer, 1024);
      buffer[valread] = '\0';
      REECan message(buffer);
      if (message.check_id(id)) {
        message.print_values();
      }

      // For testing
      if (message.is_test()) {
        message.print_values();
        test_msg = std::to_string(id) + " 0 0 0 0 0 0 0 0";
        send(sock, test_msg.c_str(), strlen(test_msg.c_str()), 0);
      }
      if ((id == 100) && (!message.is_test())) {
        send(sock, message.throttle_msg.c_str(),
             strlen(message.throttle_msg.c_str()), 0);
        usleep(DELAY);
        send(sock, message.steering_msg.c_str(),
             strlen(message.steering_msg.c_str()), 0);
        usleep(DELAY);
        send(sock, message.brakes_msg.c_str(),
             strlen(message.brakes_msg.c_str()), 0);
      }

    } else {
      std::getline(std::cin, input_str);
      send(sock, input_str.c_str(), strlen(input_str.c_str()), 0);
    }
  }
  return 0;
}
