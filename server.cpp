// Example code: A simple server side code, which echos back the received
// message. Handle multiple socket connections with select and fd_set on Linux
#include <arpa/inet.h> //close
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //strlen
#include <string>
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <unistd.h> //close

#define TRUE 1
#define FALSE 0
#define PORT 8888

int msg_ok(char *message) {
  std::string input(message);
  std::regex r(
      "[[:digit:]]+\\s-?[[:digit:]]+\\s-?[[:digit:]]+\\s-?[[:digit:]]+\\s[[:"
      "digit:]"
      "]+"
      "\\s[[:digit:]]+\\s[[:digit:]]+\\s[[:digit:]]+\\s[[:digit:]]+\\s");
  if (std::regex_match(input, r)) {
    return 1;
  }
  printf("Wrong message format\n");
  return 0;
}

int main(int argc, char *argv[]) {
  int opt = TRUE;
  std::string input_str, test_str;
  int master_socket, addrlen, new_socket, client_socket[30],
      max_clients = 30, activity, i, valread, sd, sd_transmit;
  int max_sd;
  struct sockaddr_in address;
  char buffer[1025]; // data buffer of 1K

  // set of socket descriptors
  fd_set readfds;
  setvbuf(stdout, NULL, _IOLBF, 0);

  // Initial message
  char const *message = "ECHO Daemon v1.0 \r\n";

  // initialise all client_socket[] to 0 so not checked
  for (i = 0; i < max_clients; i++) {
    client_socket[i] = 0;
  }

  // create a master socket
  if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // set master socket to allow multiple connections ,
  // this is just a good habit, it will work without this
  if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // type of socket created
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // bind the socket to localhost port 8888
  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  printf("Listener on port %d \n", PORT);

  // try to specify maximum of 3 pending connections for the master socket
  if (listen(master_socket, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // accept the incoming connection
  addrlen = sizeof(address);
  puts("Waiting for connections ...");

  while (TRUE) {
    // clear the socket set
    FD_ZERO(&readfds);

    // add master socket to set
    FD_SET(master_socket, &readfds);
    FD_SET(STDIN_FILENO, &readfds);
    max_sd = master_socket;

    // add child sockets to set
    for (i = 0; i < max_clients; i++) {
      // socket descriptor
      sd = client_socket[i];

      // if valid socket descriptor then add to read list
      if (sd > 0)
        FD_SET(sd, &readfds);

      // highest file descriptor number, need it for the select function
      if (sd > max_sd)
        max_sd = sd;
    }

    // wait for an activity on one of the sockets , timeout is NULL ,
    // so wait indefinitely
    activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

    if ((activity < 0) && (errno != EINTR)) {
      printf("select error");
    }

    // If something happened on the master socket ,
    // then its an incoming connection
    if (FD_ISSET(master_socket, &readfds)) {
      if ((new_socket = accept(master_socket, (struct sockaddr *)&address,
                               (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
      }

      // inform user of socket number - used in send and receive commands
      printf("New connection , socket fd is %d , ip is : %s , port : %d \n ",
             new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

      // send new connection greeting message
      if (send(new_socket, message, strlen(message), 0) != strlen(message)) {
        perror("send");
      }

      puts("Welcome message sent successfully");

      // add new socket to array of sockets
      for (i = 0; i < max_clients; i++) {
        // if position is empty
        if (client_socket[i] == 0) {
          client_socket[i] = new_socket;
          printf("Adding to list of sockets as %d\n", i);
          break;
        }
      }
    }

    // Test implementation
    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      std::getline(std::cin, input_str);
      if (input_str == "test1") {
        for (i = 0; i < max_clients; i++) {
          sd_transmit = client_socket[i];
          if (sd_transmit > 0) {
            // Message with universal id for tests
            test_str = "200 0 0 0 0 0 0 0 0";
            send(sd_transmit, test_str.c_str(), strlen(test_str.c_str()), 0);
          }
        }
      }
    } else {
      // else its some IO operation on some other socket
      for (i = 0; i < max_clients; i++) {
        sd = client_socket[i];

        if (FD_ISSET(sd, &readfds)) {
          // Check if it was for closing , and also read the
          // incoming message
          if ((valread = read(sd, buffer, 1024)) == 0) {
            // Somebody disconnected , get his details and print
            getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            printf("Host disconnected , ip %s , port %d \n",
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Close the socket and mark as 0 in list for reuse
            close(sd);
            client_socket[i] = 0;
          }

          // Echo back the message that came in
          else {
            // set the string terminating NULL byte on the end
            // of the data read
            buffer[valread] = '\n';
            buffer[valread + 1] = '\0';
            if (msg_ok(buffer)) {
              for (i = 0; i < max_clients; i++) {
                // socket descriptor
                sd_transmit = client_socket[i];

                // if valid socket descriptor then add to read list
                if ((sd_transmit > 0) && (sd_transmit != sd))
                  send(sd_transmit, buffer, strlen(buffer), 0);
              }
              send(sd, buffer, strlen(buffer), 0);
              printf("Received and transmited: %s\n", buffer);
              // send(sd, hello, strlen(hello), 0);
            }
          }
        }
      }
    }
  }

  return 0;
}
