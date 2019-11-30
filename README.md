# BigDNSeed

# Ubuntu16.04/18.04
# Required tools and libraries

sudo apt install -y g++ libboost-all-dev cmake openssl libreadline-dev pkg-config libsodium-dev mysql-server libmysqld-dev

#ubuntu16.04
sudo apt install -y libssl-dev
#ubuntu18.04
sudo apt install -y libssl1.0-dev


# git clone and build
git clone https://github.com/bigbangcore/BigDNSeed.git
cd BigDNSeed
./INSTALL.sh
bigdnseed -help


# Mysql setup
#Login mysql as root,run sql command as follows:

create database bigdnseed;
grant all privileges on bigdnseed.* to bigdnseed@localhost identified by 'bigdnseed';
flush privileges;

#then,quit from mysql prompt:
quit

#Finally,run command for restarting mysql service on terminal:
service mysql restart

# Run
bigdnseed

