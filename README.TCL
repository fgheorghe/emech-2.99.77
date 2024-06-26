
hook timer <spec> <callback>

  <spec>      specifier when this event should run
  <callback>  script proc

  callback arguments: {}

hook parse <command> <callback>

  <command>   can be any IRC command such as PRIVMSG, NOTICE, JOIN, PART, QUIT, etc..
  <callback>  script proc

  callback arguments: {from rest}

  If the callback returns 0 (zero), normal parsing is done by the EnergyMech after
  the script returns. If non-zero is returned, the bot will not parse the message.

hook dcc_complete - <callback>

  <callback>  script proc

  callback arguments: {to filename cps}

dcc_sendfile

debug "<text>"

to_file

to_server "<text>" [number]

  <text>    text to be sent to server
  [number]  max number of lines in the sendq

  * if number is not specified, the line will be sent immediately to the current bots server socket.
    to_server returns the number of bytes written or -1 on an error
    (in which case the server socket will have been closed)

  * if number is zero (0), the line will be added to the sendq for the current server socket.
    to_server returns the number of lines in the sendq

  * if number is a positive integer, the line will only be added to the sendq if the number
    of lines in the sendq is smaller than the given number.
    to_server returns the number of lines in the sendq

  ! when sending lines directly (number not specified), a newline must be supplied with the buffer.
    when sending through the sendq, the newline is optional

userlevel <nickuserhost> [channel]
