# mxfs-tool(Parent processing)
Parent Processing(formatter) on BCM2711(buster or bullseye 32bit)


# formatter

## compile/link

```
cd mother
mkdir -p _build
cd _build
cmake ../cmake/format/
make -j
```

## run/debug



### config instruction

Erase first

```
./formatter -t erase -s 0 -e 1 -S 0 -E 1
```

```
./formatter -t config -c \
'{wifi:{ssid:0024A5DAA935,password:ri34k2mc8y7m9}, \
    cplane:{host:192.168.11.111,port:50012}, \
    ntp:{host:192.168.11.111,port:123, \
    device:{id:45678, \
    sensors:{ \
        speed:{id:0}, \
        power:{id:0}, \
        cadence:{id:0} \
        }, \
    sleep:{mode:0} \
}'
```

### Reads, verify configuration

```
./formatter -t read -s 0 -e 1 -S 0 -E 1 -p 16

...
<<<<
{wifi:{ssid:0024A5DAA935,password:ri34k2mc8y7m9}, \
    cplane:{host:192.168.11.111,port:50012}, \
    ntp:{host:192.168.11.111,port:123, \
    device:{id:45678, \
    sensors:{ \
        speed:{id:0}, \
        power:{id:0}, \
        cadence:{id:0} \
        }, \
    sleep:{mode:0} \
}
```

### read instruction by block and page

|opt|desc||
|---|---|---|
|s|start index of block||
|e|end index of block||
|S|start index of page||
|E|end index of page||
||||

```
./formatter -t read -s 1 -e 2 -S 0 -E 1
start ./formatter
reset            [1 , 1]ff 
device id        [2 , 2]9f 00 
device(EF AA 21)
un-protect       [3 , 3]1f a0 00 
write enable     [1 , 1]06 
command : read/Page: 1 -> 2/Block: 0 -> 1
>> block : 1/ page : 0,magic: deadc0de, flags: 00000010, time: 00000007, fcs: 3ff90850
>>>> read >>>>
DE C0 AD DE 10 00 00 00 :07 00 00 00 50 08 F9 3F :FE 16 F4 0B FC FF FB 0E :F9 ED 11 15 EC F0 16 F7 

....

<<<<
fin ./formatter
```

### erase instruction by block and page

```
./formatter -t erase -s 1 -e 2 -S 0 -E 1
start ./formatter
reset            [1 , 1]ff 
device id        [2 , 2]9f 00 
device(EF AA 21)
un-protect       [3 , 3]1f a0 00 
write enable     [1 , 1]06 
command : erase/Page: 1 -> 2/Block: 0 -> 1
fin ./formatter
```

