#!/bin/bash
count=0
while :
do
    if [ -e /home/httpd/pid/socket.pid ]; then
        PID=`cat /home/httpd/pid/socket.pid`
        RES=`ps -p $PID`
        if [ $? -eq 1 ]; then
            echo "SocketServer terminated."
            echo "Run SocketServer"
            nohup /home/httpd/bin/socket > /dev/null &
        fi
    else
        echo "Run WebSocketServer"
        nohup /home/httpd/bin/websocket > /dev/null &
    fi

    if [ -e /home/httpd/pid/websocket.pid ]; then
        PID=`cat /home/httpd/pid/websocket.pid`
        RES=`ps -p $PID`
        if [ $? -eq 1 ]; then
            echo "WebSocketServer terminated."
            echo "Run WebSocketServer"
            nohup /home/httpd/bin/websocket > /dev/null &
        fi
    else
        echo "Run WebSocketServer"
        nohup /home/httpd/bin/websocket > /dev/null &
    fi
    sleep 1
    count=$count+1
done