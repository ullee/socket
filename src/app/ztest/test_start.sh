#!/bin/bash

for ((i=0;i<2000;i++)); do
    # test server
    nohup /home/httpd/src/app/ztest/build/socket_client 52.79.111.101 &
    nohup /home/httpd/src/app/ztest/build/socket_client 13.124.165.208 &
    nohup /home/httpd/src/app/ztest/build/websocket_client 52.79.111.101 &
    nohup /home/httpd/src/app/ztest/build/websocket_client 13.124.165.208 &
    sleep 0.1
done