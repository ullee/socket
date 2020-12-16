# README #

### Boost Install ###

https://www.osetc.com/en/how-to-install-boost-on-ubuntu-16-04-18-04-linux.html
https://www.boost.org/users/download/

다운로드
wget https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.gz

```
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install cmake build-essential g++ python-dev autotools-dev libicu-dev build-essential libbz2-dev

tar xzvBpf boost_1_66_0.tar.gz
./bootstrap.sh


./b2 --prefix=/usr/local variant=release link=shared threading=multi runtime-link=shared
sudo ./b2 install
```


### Socket, WebSocket Home ###

* /home/httpd

### Socket, WebSocket Log Path ###

* /home/httpd/logs

### Socket, WebSocket Binary Path ###

* /home/httpd/bin

### Socket, WebSocket Config Path ###

* /home/httpd/conf
