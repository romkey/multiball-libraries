# Multiball Library

[![Build Status](https://travis-ci.com/romkey/multiball-libraries.svg?branch=master)](https://travis-ci.com/romkey/multiball-libraries)

This library contains code that's used in several hardware projects:
- the [furball](https://github.com/HomeBusProjects/furball) environmental sensor, dustball (merged into furball)
- [butterball](https://github.com/romkey/butterball) high temperature sensor
- the  [vendo](https://github.com/romkey/vendo) firmware for the discoball LED controller,
- [waterball](https://github.com/romkey/waterball)
- hydroponics sensor
- [powerball](https://github.com/romkey/powerball) power consumption sensor and
- [dirtball](https://github.com/romkey/dirtball) garden sensor

Splitting it into its own library allows it to be easily reused and allows the projects to be very modular.

See also the [Multiball Sensor]() library, which contains highly uniform and modular sensor code that is also reused among projects.

Unless you're working with one of the "ball" projects you almost certainly don't want to use this library. It's designed to accomodate the specific needs of these projects and not for general use.
