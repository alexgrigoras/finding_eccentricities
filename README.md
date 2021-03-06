# Distributed Processing - Algorithms on trees

## Description
Finding eccentricities in a tree

![Eccentricity of a tree image](https://mathworld.wolfram.com/images/eps-gif/GraphEccentricities_900.gif)

Image taken from [Wolfram MathWorld](https://mathworld.wolfram.com/GraphEccentricity.html).

## Implementation
The  is a distributed algorithm implemented in C with MPI.  It uses the saturation method to get a complexity of <i>O(n)</i>. It has 3 stages:
1) Activation
1) Saturation
1) Resolution

These are represented in the following image:

<img src="https://slideplayer.com/slide/13580230/82/images/7/1%29+2%29+3%29+leaf+internal+Saturated+node+init+WAKE-UP+WAKE-UP+WAKE-UP.jpg" width="500" alt="Saturation method image">

## Documentation
See [Documentation](documentation.pdf) for mode details.

## References
The algorithm is implemented from [N. Santoro, Design and Analysis of Distributed Algorithms, Ottawa: WILEY-INTERSCIENCE, 2006](http://www.iaushab.ac.ir/uploads/DESIGN%20AND%20ANALYSIS%20of%20Distributed%20_383.pdf).

## License
The application is licensed under the MIT License.
