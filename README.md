# AltitudeControlSystem
Updated version of the Altitude Control System software for the venting of helium to achieve neutral buoyancy and prevent bursting during Adler Planetarium: Far Horizons balloon flights.

The Altitude Control System (ACS) is a mechanism controlled by a Teensy 3.5 and a Servo motor that extends and retracts to allow helium to vent out of high-altitude balloons (HABs) during nighttime imaging flights over Chicago.

The system is equipped with a custom-built pressure sensor and accelerometer that is used to calculate the system's current altitude and velocity, which is then used to determine the current stage of the flight (GROUND, ASCENT, VENTING, FLOATING, and DESCENT) as well as whether the Servo should be in a opened or closed position.

This system also collects and logs a swath of data, such as time, actuator extension, program mode and stage, pressure, altitude, velocity, as well as external and internal temperature. Error and status logs are created as well.

Currently, the system switches from GROUND to ASCENT once either someone has manually pushed the start button or the machine logs an altitude greater than 2000 m. Once in ASCENT mode, the system will switch to VENTING once it reaches a predetermined altitude declared globally (usually around 21000 m). Now, the most important part is that the system must determine when it has vented enough helium to achieve neutral buoyancy and reached the right altitude just above the ozone layer to clear turbulent winds. The user must declare a desired ASCENT_RATE_RANGE, the maximum change in altitude over change in time in m/s allowed (either positive or negative). A TIME_IN_RANGE must also be declared as the length of the timer that begins once this ASCENT_RATE_RANGE is achieved. Finally, once the timer has finished, the Teensy will calculate whether the number of times it has measured the ascent rate inside the range divided by the total number of measurements has met a desired CONFIDENCE_LEVEL.


For example,

Settings:
<code>#define ASCENT_RATE_RANGE 0.75</code> (m)
<code>#define TIME_IN_RANGE = 5000</code> (ms)
<code>#define CONFIDENCE_LEVEL 0.95</code>

<code>#define VENT_ALT 21000</code>
<code>#define FLOAT_TIME 60</code>

Balloon takes off and reaches 2000 m. It is now ASCENDING.

Balloon reached 21000 m. Actuator has initiated. It is now VENTING.

Ascent rate has been measured within the range of +/- 0.75 m/s. The ACS will now count the number of measurements both in and out of this range for the next 5 seconds.

5 seconds is up. 5 times out of range, 114 times in range.

96 / (5 + 96) = 0.9505

The calculation has satisfied our desired 95% confidence level. The system believes it has achieved neutral buoyancy. And closes the actuator. A timer begins for the predetermined amount of time to float over the city. It is now FLOATING.

 60 minutes has come and gone. It is time for the balloon to return to Earth. The actuator opens up and begins to release helium again. It is now DESCENDING.
