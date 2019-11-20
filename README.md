Demo tun/tap device
==========================

## tun

```

  ----------------------------------------                      -----------------------------------------
  |10.10.10.14                           |                      |10.10.10.15                            |
  |                                      |                      |                                       |
  |                                      |                      |                                       |
  |      ip route 172.0.0.0/8 ---        |                      |  -----------------------------------  |
  |                             |        |                      |  |    test-dev(172.0.0.1/24)       |  |
  |                            \|/       |                      |  -----------------------------------  |
  |  ----------------------------------  |                      |  -----------------------------------  |
  |  |  tun0(192.168.0.1->192.168.0.2)|  |                      |  |  tun0(192.168.0.2->192.168.0.1) |  |
  |  ----------------------------------  |                      |  -----------------------------------  |
  |     /|\ |                            |                      |                   |   /|\             |
  | write|  |read                        |                      |               read|    |write         |
  |      | \|/                           |       UDP            |                  \|/   |              |
  |      xtun <--------------------------------------------------------------------> xtun               |
  |                                      |                      |                                       |
  ----------------------------------------                      -----------------------------------------


```

Having 2 nodes with ip `10.10.10.14` and `10.10.10.15`. On each node we run an `xtun` which creates a `tun` device named `tun0`. Then we configure `tun0` on `10.10.10.14` with ip `192.168.0.1` and destination `192.168.0.2`, and on `10.10.10.15` with ip `192.168.0.2` and destination `192.168.0.1`. Now, we have created 2 `point to point` interface on 2 nodes, `xtun` will read all data flow from `tun0` and forward to the other node with UDP tunnel, on reading data flow from UDP tunnel, the other node would write raw data to `tun0` too. Then, all following data flow will be controled by the kernel protocol stack.

In order to demonstrate a complete `VPN(Virtual private network)` situation, we create a `test-dev` interface on `10.10.10.15` with ip `172.0.0.1/24`. On `10.10.10.14` we route all trafic with destination to `172.0.0.0/8` to `tun0`. 

Then what it is? You can visit `172.0.0.1` on `10.10.10.14` just like you are in the same private network with `172.0.0.1`.


1. On `10.10.10.14`

```
## Launch a `xtun`
./xtun tun0 10.10.10.15

## Configure tun0
ifconfig tun0 192.168.0.1 pointopoint 192.168.0.2 mtu 1500

## Route trafic to tun0
ip route add 172.0.0.0/8 via 192.168.0.1
```

2. On `10.10.10.15`

```
## Luanch a `xtun`
./xtun tun0 10.10.10.14

## Configure tun0
ifconfig tun0 192.168.0.2 pointopoint 192.168.0.1 mtu 1500

## Add link and configure as 172.0.0.1/24
ip link add test-dev type bridge
ifconfig test 172.0.0.1 netmask 255.255.255.0
```

3. Test with `nc`

On `10.10.10.15`

```
nc -l 172.0.0.1 80
```

On `10.10.10.15`

```
nc 172.0.0.1 80
```

