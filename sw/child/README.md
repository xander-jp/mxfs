# MXFS
Customized Logstrctured FileSystem as Using NAND/SPI Flash Memory in Directly

# Compile/Link

## RP2040

```
cd sw/child
mkdir -p ./_build
cd ./_build
cmake ../cmake/rp2040/
make -j
```


## BCM2711

```
cd sw/child
mkdir -p ./_build
cd ./_build
cmake ../cmake/bcm2711/
make -j
```


## OSX

```
cd sw/child
mkdir -p ./_build
cd ./_build
cmake ../cmake/macos/
make -j
```

# Dependencies

## RP2040

```
none
```

## BCM2711

```
sudo apt install libgtest-dev
sudo apt install googletest
```

## osx

```
brew install googletest
```



