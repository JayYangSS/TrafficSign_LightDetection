#include "math_utils.h"

/**
 * Return the greatest value of three passed arguments.
 *
 * Unsigned integer has been used, because RGB values cant be minus.
 */
unsigned int
get_maximum(unsigned int r, unsigned int g, unsigned int b)
{
  if (r >= g)
    {
      if (r >= b)
        {
          return r;
        }
      else
        {
          return b;
        }
    }
  else
    {
      if (g >= b)
        {
          return g;
        }
      else
        {
          return b;
        }
    }
}

/**
 * Return the lowest value of three passed arguments.
 *
 * Unsigned integer has been used, because RGB values cant be minus.
 */
unsigned int
get_minimum(unsigned int r, unsigned int g, unsigned int b)
{
  if (r <= g)
    {
      if (r <= b)
        {
          return r;
        }
      else
        {
          return b;
        }
    }
  else
    {
      if (g <= b)
        {
          return g;
        }
      else
        {
          return b;
        }
    }
}
