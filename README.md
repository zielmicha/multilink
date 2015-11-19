# Multilink

Multilink is an implementation of my [stream-based aggregation of unreliable heterogeneous network links](http://arxiv.org/abs/1509.08222) paper. That is, it allows you to create one stable and fast connection based on mutliple crappier links. It is not usable yet, but it soon will be.

## Build

Install dependencies

```
apt-get install libboost-program-options-dev libboost-filesystem-dev ninja-build cmake
```

or equivalent for your distro.

If build fails with linker error mentioning `boost::program_options`, it may be a problem with your compiler version.
