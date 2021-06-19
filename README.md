# tachometer_esp32
A tachometer targeting orbital shakers


## Principle of operation
Orbital shakers perform a horizontal, circling movement of its table to mix the samples of interest.
By sampling the acceleration of the table one can extract the main frequency component using the Fourier Transform.
With this information it's easy to get the speed of rotation by simply multiplying the frequency in hertz by a factor of 60, thus resulting in rotations per minute (RPM).

The circuit must be firmly attached to the table in the horizontal position (with z axis perpendicular) so the accelerometer can be read properly.
Currently only the x axis is taken into account for the computation (the y component should provide the same information for a perfect orbital movement).

To report the data the ESP32 uses the BluetoothSerial interface. Any bluetooth enabled computer or smartphone can read the data by pairing with
the device named "Agitador Orbital" (Orbital Shaker). A string with only numbers will be sent each second.

## Tools
* Software
  * [kissfft](https://github.com/mborgerding/kissfft) - Using its FFT implementation
  * [Arduino core for the ESP32](https://github.com/espressif/arduino-esp32)
  * [Fritzing](https://fritzing.org/) - For the diagram below
* Hardware
  * ESP32 Devkit V1
  * ADXL345 Accelerometer

## Test circuit
![Example circuit](/circuit/circuit.png)
