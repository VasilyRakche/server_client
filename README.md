# server_client

run make 
open multiple terminals, and in each run:  
./server  
./client 100 //for every one of the clients  
./client 101  
./client 102  
./client 103  


Write messages to the clients, example:  
101 1 1 1 0 0 0 0 0  

Keep in mind that client 100(drive-by-wire), if gets a message, sends message to everybody else, based on his message content


Tests:  
You can run test1 by typing it in the terminal of server  
Every client should print zero, and send message with its own id back  


Also, messages are beeing tested, please use right form of messages!  
