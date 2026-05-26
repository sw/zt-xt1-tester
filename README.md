# ZT-XT1 component tester alternative firmware
Alternative firmware for the component tester MCU in the Zotek/Zoyi ZT-XT1. Not affiliated with Zotek Instruments Co., Ltd.
Use at your own risk.

Note: this is intended to be used as a submodule of the [main DMM firmware](https://github.com/sw/zt-xt1-dmm).

## Required toolchain
- Assumes `arm-none-eabi-gcc` installed in `/usr/bin`. Otherwise, change `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER` in [CMakeLists.txt](CMakeLists.txt)
- Uses [picolibc](https://keithp.com/picolibc/) as the standard library
- Uses [pyOCD](https://pyocd.io/) for debugging. Install the required pack with `pyocd pack install N32G031`

## Tests
CTest is used for running unit tests in the host environment (i.e. not on the ARM bare-metal target). This uses the [ngspice shared library](https://ngspice.sourceforge.io/shared.html) to simulate components. Use the `Test` Cmake preset:

    cmake --workflow --preset Test

## License
See [LICENSE](LICENSE)
