OS Assignment 2 Submission - Spring '18 Habib University


Group Members:

Osama Yousuf (oy02945)
Gohar Ayoub Wassan (gw02370)

Sample usage through terminal:

(Terminal 1 ->) make
(Terminal 1 ->) ./se 5050 (Start server)
(Terminal 2 ->) ./cl 127.0.0.1 5050 cl1 (Create unique client)
(Terminal 3 ->) ./cl 127.0.0.1 5050 cl2 (Create unique client)
(Terminal 4 ->) ./cl 127.0.0.1 5050 cl2 (Create duplicate client, isn't created)
(Terminal 3 ->) ./cl 127.0.0.1 5050 cl3 (Create unique client)
(Terminal 3 ->) /list (Displays list of clients)
(Terminal 2 ->) /msg cl5 hello (Send message to inexistent client from cl1)
(Terminal 2 ->) /msg cl3 hello (Send message to cl3 from cl1)
(Terminal 3 ->) /quit (Exit cl2)
(Terminal 2 ->) /list (Check updated list from server)


Adopted from:

https://github.com/codophobia/Multi-Client-Server-Chat/
