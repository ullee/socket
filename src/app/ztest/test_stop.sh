#!/bin/bash
# test server
kill -9 `ps -ef | grep './socket_client 52.79.111.101' | awk '{print $2}'`
kill -9 `ps -ef | grep './socket_client 13.124.165.208' | awk '{print $2}'`
kill -9 `ps -ef | grep './websocket_client 52.79.111.101' | awk '{print $2}'`
kill -9 `ps -ef | grep './websocket_client 13.124.165.208' | awk '{print $2}'`
