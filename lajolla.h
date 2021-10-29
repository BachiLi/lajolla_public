#pragma once

// We use double for most of our computation.
// Rendering is usually done in single precision floats.
// However, lajolla is a educational renderer with does not
// put emphasis on the absolute performance. 
// We choose double so that we do not need to worry about
// numerical accuracy as much when we render.
// Switching to floating point computation is easy --
// just set Real = float.
using Real = double;

