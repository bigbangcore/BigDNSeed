# BigDNSeed

# Ubuntu16.04/18.04
# Required tools and libraries

sudo apt install -y g++ libboost-all-dev cmake openssl libreadline-dev pkg-config libsodium-dev mysql-server libmysqld-dev<br>

#ubuntu16.04<br>
sudo apt install -y libssl-dev<br>
#ubuntu18.04<br>
sudo apt install -y libssl1.0-dev<br>
<br>
<br>
# git clone and build
git clone https://github.com/bigbangcore/BigDNSeed.git<br>
cd BigDNSeed<br>
./INSTALL.sh<br>
bigdnseed -help<br>
<br>
<br>
# Mysql setup
#Login mysql as root,run sql command as follows:<br>
<br>
create database bigdnseedmkf;<br>
grant all privileges on bigdnseedmkf.* to bigdnseed@localhost identified by 'bigdnseed';<br>
flush privileges;<br>
<br>
#then,quit from mysql prompt:<br>
quit<br>
<br>
#Finally,run command for restarting mysql service on terminal:<br>
service mysql restart<br>
<br>
# Run
bigdnseed

