# simulation
```
brew install icarus-verilog gtkwave
```
# run
```
iverilog -s mxfs_fast_tb -o sim.out fsm_fast.v fsm_slow.v fsm_tb.v && vvp sim.out && \
iverilog -s mxfs_slow_tb -o sim.out fsm_fast.v fsm_slow.v fsm_tb.v && vvp sim.out && \
iverilog -s mxfs_fast_slow_pattern_a_tb -o sim.out fsm_fast.v fsm_slow.v fsm_tb.v && vvp sim.out
```

# Related

https://www.j-platpat.inpit.go.jp/c1801/PU/JP-2024-094699/10/ja
