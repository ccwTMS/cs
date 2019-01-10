
client/server sample code for transmission test.

** Both IPv4 and IPv6 connecting supported.

** UDP type connecting supported.

** using getifaddrs() to replace SIOCGIFADDR ioctl command for get IP addersses from ethernet interface.

** inet_pton and inet_ntop test.

** getnameinfo() test.




options:

-p port_number:

-i ip:

-s package_size:

-v　　　　　　　　: dump messages

-w write_path　　　: file path for writing test.

-f write_path　　　: write file test only.-u option for UDP protocol.

-b option for broadcast by IPv6.


ex:
server:

  #./ss -s 32768 -p 9999 &

client 1 (ipv6):

  #./cc -s 32768 -p 9999 -i 2002::1234:4321:1234:4321

or client 2 (ipv6):

  #./cc -s 32768 -p 9999 -i ::1

or client 3 (ipv4):

  #./cc -s 32768 -p 9999 -i 192.168.0.100
  



