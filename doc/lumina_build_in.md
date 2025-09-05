# Built-In Features
## Scalar Types
### float

Single precision floating-point number.

#### Description:

  The `float` type represents a single-precision floating-point number, conforming to the IEEE 754 standard. It is used for representing decimal numbers with fractional parts.

#### Attributes:

  - *None*

#### Constructors:

  - `float()`: Default constructor that initializes the value to `0.0f`.
  - `float(float value)`: Copy constructor that initializes the value to `value`.
  - `float(int value)`: Converts an `int` to a `float`.
  - `float(uint value)`: Converts a `uint` to a `float`.

#### Implicit Conversions:

  - Implicitly convertible from `int`.
  - Implicitly convertible from `uint`.

#### Functions:

  - `float abs()`: Returns the absolute value of the float.
  - `float sign()`: Returns the sign of the float (`-1.0f`, `0.0f`, or `1.0f`).
  - `float floor()`: Returns the largest integer less than or equal to the float.
  - `float ceil()`: Returns the smallest integer greater than or equal to the float.
  - `float fract()`: Returns the fractional part of the float.
  - `float mod(float divisor)`: Returns the remainder after division of the float by `divisor`.
  - `float min(float other)`: Returns the smaller of the float and `other`.
  - `float max(float other)`: Returns the larger of the float and `other`.
  - `float clamp(float minVal, float maxVal)`: Clamps the float between `minVal` and `maxVal`.
  - `float mix(float other, float t)`: Performs a linear interpolation between the float and `other` based on `t`.
  - `float step(float edge)`: Returns `0.0f` if the float is less than `edge`, else `1.0f`.
  - `float smoothstep(float edge0, float edge1)`: Performs smooth Hermite interpolation between `0.0f` and `1.0f`.
  - `float pow(float exponent)`: Raises the float to the power of `exponent`.
  - `float exp()`: Returns the natural exponential of the float.
  - `float log()`: Returns the natural logarithm of the float.
  - `float exp2()`: Returns `2` raised to the power of the float.
  - `float log2()`: Returns the base-2 logarithm of the float.
  - `float sqrt()`: Returns the square root of the float.
  - `float inversesqrt()`: Returns the inverse square root of the float.
  - `float sin()`: Returns the sine of the float (angle in radians).
  - `float cos()`: Returns the cosine of the float (angle in radians).
  - `float tan()`: Returns the tangent of the float (angle in radians).
  - `float asin()`: Returns the arcsine of the float.
  - `float acos()`: Returns the arccosine of the float.
  - `float atan()`: Returns the arctangent of the float.

#### Operators:

##### Arithmetic Operators:

- `float operator+(float other)`: Adds two floats.
- `float operator-(float other)`: Subtracts `other` from the float.
- `float operator*(float other)`: Multiplies two floats.
- `float operator/(float other)`: Divides the float by `other`.
- `float operator%(float other)`: Computes the remainder of the float divided by `other`.
- `float operator+()`: Unary plus (returns the value of the float).
- `float operator-()`: Unary minus (negates the float).

##### Compound Assignment Operators:

- `float operator+=(float other)`: Adds `other` to the float and assigns the result.
- `float operator-=(float other)`: Subtracts `other` from the float and assigns the result.
- `float operator*=(float other)`: Multiplies the float by `other` and assigns the result.
- `float operator/=(float other)`: Divides the float by `other` and assigns the result.
- `float operator%=(float other)`: Computes the remainder and assigns the result.

##### Comparison Operators:

- `bool operator==(float other)`: Checks if the float is equal to `other`.
- `bool operator!=(float other)`: Checks if the float is not equal to `other`.
- `bool operator<(float other)`: Checks if the float is less than `other`.
- `bool operator>(float other)`: Checks if the float is greater than `other`.
- `bool operator<=(float other)`: Checks if the float is less than or equal to `other`.
- `bool operator>=(float other)`: Checks if the float is greater than or equal to `other`.

##### Operator Table:

  | Operator | float | int | uint | float (unary) |
  |----------|-------|-----|------|---------------|
  | `+`      |   X   |  X  |  X   |      X        |
  | `-`      |   X   |  X  |  X   |      X        |
  | `*`      |   X   |  X  |  X   |               |
  | `/`      |   X   |  X  |  X   |               |
  | `%`      |   X   |  X  |  X   |               |
  | `+=`     |   X   |  X  |  X   |               |
  | `-=`     |   X   |  X  |  X   |               |
  | `*=`     |   X   |  X  |  X   |               |
  | `/=`     |   X   |  X  |  X   |               |
  | `%=`     |   X   |  X  |  X   |               |
  | `==`     |   X   |  X  |  X   |               |
  | `!=`     |   X   |  X  |  X   |               |
  | `<`      |   X   |  X  |  X   |               |
  | `>`      |   X   |  X  |  X   |               |
  | `<=`     |   X   |  X  |  X   |               |
  | `>=`     |   X   |  X  |  X   |               |

### int

Signed integer type.

#### Description:

  The `int` type represents a 32-bit signed integer. It is used for representing whole numbers, both positive and negative.

#### Attributes:

  - *None*

#### Constructors:

  - `int()`: Default constructor that initializes the value to `0`.
  - `int(int value)`: Copy constructor that initializes the value to `value`.
  - `int(uint value)`: Converts a `uint` to an `int`.
  - `int(float value)`: Converts a `float` to an `int` by truncation.

#### Implicit Conversions:

  - Implicitly convertible from `uint`.
  - Implicitly convertible from `float`.

#### Functions:

  - `int abs()`: Returns the absolute value of the integer.
  - `int sign()`: Returns the sign of the integer (`-1`, `0`, or `1`).
  - `int mod(int divisor)`: Returns the remainder after division of the integer by `divisor`.
  - `int min(int other)`: Returns the smaller of the integer and `other`.
  - `int max(int other)`: Returns the larger of the integer and `other`.
  - `int clamp(int minVal, int maxVal)`: Clamps the integer between `minVal` and `maxVal`.

#### Operators:

##### Arithmetic Operators:

- `int operator+(int other)`: Adds two integers.
- `int operator-(int other)`: Subtracts `other` from the integer.
- `int operator*(int other)`: Multiplies two integers.
- `int operator/(int other)`: Divides the integer by `other`.
- `int operator%(int other)`: Computes the remainder of the integer divided by `other`.
- `int operator+()`: Unary plus (returns the value of the integer).
- `int operator-()`: Unary minus (negates the integer).

##### Compound Assignment Operators:

- `int operator+=(int other)`: Adds `other` to the integer and assigns the result.
- `int operator-=(int other)`: Subtracts `other` from the integer and assigns the result.
- `int operator*=(int other)`: Multiplies the integer by `other` and assigns the result.
- `int operator/=(int other)`: Divides the integer by `other` and assigns the result.
- `int operator%=(int other)`: Computes the remainder and assigns the result.

##### Increment and Decrement Operators:

- `int operator++()`: Prefix increment (increments the integer and returns the new value).
- `int operator--()`: Prefix decrement (decrements the integer and returns the new value).
- `int operator++(int)`: Postfix increment (returns the current value and then increments the integer).
- `int operator--(int)`: Postfix decrement (returns the current value and then decrements the integer).

##### Comparison Operators:

- `bool operator==(int other)`: Checks if the integer is equal to `other`.
- `bool operator!=(int other)`: Checks if the integer is not equal to `other`.
- `bool operator<(int other)`: Checks if the integer is less than `other`.
- `bool operator>(int other)`: Checks if the integer is greater than `other`.
- `bool operator<=(int other)`: Checks if the integer is less than or equal to `other`.
- `bool operator>=(int other)`: Checks if the integer is greater than or equal to `other`.

##### Operator Table:

  | Operator | int | uint | float | int (unary) |
  |----------|-----|------|-------|-------------|
  | `+`      |  X  |  X   |   X   |      X      |
  | `-`      |  X  |  X   |   X   |      X      |
  | `*`      |  X  |  X   |   X   |             |
  | `/`      |  X  |  X   |   X   |             |
  | `%`      |  X  |  X   |   X   |             |
  | `+=`     |  X  |  X   |   X   |             |
  | `-=`     |  X  |  X   |   X   |             |
  | `*=`     |  X  |  X   |   X   |             |
  | `/=`     |  X  |  X   |   X   |             |
  | `%=`     |  X  |  X   |   X   |             |
  | `==`     |  X  |  X   |   X   |             |
  | `!=`     |  X  |  X   |   X   |             |
  | `<`      |  X  |  X   |   X   |             |
  | `>`      |  X  |  X   |   X   |             |
  | `<=`     |  X  |  X   |   X   |             |
  | `>=`     |  X  |  X   |   X   |             |
  | `++`     |  X  |      |       |             |
  | `--`     |  X  |      |       |             |

### uint

Unsigned integer type.

#### Description:

  The `uint` type represents a 32-bit unsigned integer. It is used for representing whole numbers that are non-negative.

#### Attributes:

  - *None*

#### Constructors:

  - `uint()`: Default constructor that initializes the value to `0`.
  - `uint(uint value)`: Copy constructor that initializes the value to `value`.
  - `uint(int value)`: Converts an `int` to a `uint`.
  - `uint(float value)`: Converts a `float` to a `uint` by truncation.

#### Implicit Conversions:

  - Implicitly convertible from `int`.
  - Implicitly convertible from `float`.

#### Functions:

  - `uint mod(uint divisor)`: Returns the remainder after division of the unsigned integer by `divisor`.
  - `uint min(uint other)`: Returns the smaller of the unsigned integer and `other`.
  - `uint max(uint other)`: Returns the larger of the unsigned integer and `other`.
  - `uint clamp(uint minVal, uint maxVal)`: Clamps the unsigned integer between `minVal` and `maxVal`.

#### Operators:

##### Arithmetic Operators:

- `uint operator+(uint other)`: Adds two unsigned integers.
- `uint operator-(uint other)`: Subtracts `other` from the unsigned integer.
- `uint operator*(uint other)`: Multiplies two unsigned integers.
- `uint operator/(uint other)`: Divides the unsigned integer by `other`.
- `uint operator%(uint other)`: Computes the remainder of the unsigned integer divided by `other`.
- `uint operator+()`: Unary plus (returns the value of the unsigned integer).

##### Compound Assignment Operators:

- `uint operator+=(uint other)`: Adds `other` to the unsigned integer and assigns the result.
- `uint operator-=(uint other)`: Subtracts `other` from the unsigned integer and assigns the result.
- `uint operator*=(uint other)`: Multiplies the unsigned integer by `other` and assigns the result.
- `uint operator/=(uint other)`: Divides the unsigned integer by `other` and assigns the result.
- `uint operator%=(uint other)`: Computes the remainder and assigns the result.

##### Increment and Decrement Operators:

- `uint operator++()`: Prefix increment (increments the unsigned integer and returns the new value).
- `uint operator--()`: Prefix decrement (decrements the unsigned integer and returns the new value).
- `uint operator++(int)`: Postfix increment (returns the current value and then increments the unsigned integer).
- `uint operator--(int)`: Postfix decrement (returns the current value and then decrements the unsigned integer).

##### Comparison Operators:

- `bool operator==(uint other)`: Checks if the unsigned integer is equal to `other`.
- `bool operator!=(uint other)`: Checks if the unsigned integer is not equal to `other`.
- `bool operator<(uint other)`: Checks if the unsigned integer is less than `other`.
- `bool operator>(uint other)`: Checks if the unsigned integer is greater than `other`.
- `bool operator<=(uint other)`: Checks if the unsigned integer is less than or equal to `other`.
- `bool operator>=(uint other)`: Checks if the unsigned integer is greater than or equal to `other`.

##### Operator Table:

  | Operator | uint | int | float | uint (unary) |
  |----------|------|-----|-------|--------------|
  | `+`      |  X   |  X  |   X   |      X       |
  | `-`      |  X   |  X  |   X   |              |
  | `*`      |  X   |  X  |   X   |              |
  | `/`      |  X   |  X  |   X   |              |
  | `%`      |  X   |  X  |   X   |              |
  | `+=`     |  X   |  X  |   X   |              |
  | `-=`     |  X   |  X  |   X   |              |
  | `*=`     |  X   |  X  |   X   |              |
  | `/=`     |  X   |  X  |   X   |              |
  | `%=`     |  X   |  X  |   X   |              |
  | `==`     |  X   |  X  |   X   |              |
  | `!=`     |  X   |  X  |   X   |              |
  | `<`      |  X   |  X  |   X   |              |
  | `>`      |  X   |  X  |   X   |              |
  | `<=`     |  X   |  X  |   X   |              |
  | `>=`     |  X   |  X  |   X   |              |
  | `++`     |  X   |     |       |              |
  | `--`     |  X   |     |       |              |

### bool

Boolean type representing truth values.

#### Description:

  The `bool` type represents a boolean value, which can be either `true` or `false`.

#### Attributes:

  - *None*

#### Constructors:

  - `bool()`: Default constructor that initializes the value to `false`.
  - `bool(bool value)`: Copy constructor that initializes the value to `value`.
  - `bool(int value)`: Converts an `int` to a `bool` (`false` if `value` is `0`, else `true`).
  - `bool(uint value)`: Converts a `uint` to a `bool` (`false` if `value` is `0`, else `true`).

#### Implicit Conversions:

  - Implicitly convertible from `int`.
  - Implicitly convertible from `uint`.

#### Functions:

  - *None*

#### Operators:

##### Logical Operators:

- `bool operator&&(bool other)`: Logical AND.
- `bool operator||(bool other)`: Logical OR.
- `bool operator!()`: Logical NOT.

##### Assignment Operator:

- `bool operator=(bool value)`: Assigns `value` to the boolean.

##### Comparison Operators:

- `bool operator==(bool other)`: Checks if the boolean is equal to `other`.
- `bool operator!=(bool other)`: Checks if the boolean is not equal to `other`.

##### Operator Table:

  | Operator | bool |
  |----------|------|
  | `&&`     |  X   |
  | `||`     |  X   |
  | `!`      |  X   |
  | `=`      |  X   |
  | `==`     |  X   |
  | `!=`     |  X   |

  ## Vector Types
### Vector2

2D vector with floating-point components.

#### Description:

  The `Vector2` type represents a 2D vector with `float` components `X` and `y`. It is commonly used in graphics and mathematical computations involving 2D coordinates or directions.

#### Attributes:

  - `float X`: The X-component of the vector.
  - `float y`: The y-component of the vector.

#### Constructors:

  - `Vector2()`: Default constructor that initializes both `X` and `y` to `0.0f`.
  - `Vector2(float X, float y)`: Constructor that initializes the vector with specified `X` and `y` values.
  - `Vector2(float[2] values)`: Constructor that initializes the vector with an array of two `float` values.
  - `Vector2(Vector2Int vec)`: Converts a `Vector2Int` to a `Vector2`.
  - `Vector2(Vector2UInt vec)`: Converts a `Vector2UInt` to a `Vector2`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector2Int`.
  - Implicitly convertible from `Vector2UInt`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector.
  - `Vector2 normalize()`: Returns a normalized version of the vector.
  - `float dot(Vector2 other)`: Returns the dot product with `other`.
  - `Vector2 reflect(Vector2 normal)`: Reflects the vector around the given normal.
  - `Vector2 abs()`: Returns a vector with absolute values of each component.
  - `Vector2 floor()`: Returns a vector with each component rounded down.
  - `Vector2 ceil()`: Returns a vector with each component rounded up.
  - `Vector2 fract()`: Returns the fractional part of each component.
  - `Vector2 mod(Vector2 divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector2 min(Vector2 other)`: Returns the minimum of each component with `other`.
  - `Vector2 max(Vector2 other)`: Returns the maximum of each component with `other`.
  - `Vector2 clamp(Vector2 minVal, Vector2 maxVal)`: Clamps each component between `minVal` and `maxVal`.
  - `Vector2 lerp(Vector2 other, float t)`: Linearly interpolates between the vector and `other` by `t`.
  - `Vector2 step(Vector2 edge)`: Returns `0.0f` for components less than `edge`, else `1.0f`.
  - `Vector2 smoothstep(Vector2 edge0, Vector2 edge1)`: Smooth Hermite interpolation.
  - `Vector2 pow(Vector2 exponent)`: Raises components to the powers in `exponent`.
  - `Vector2 exp()`: Returns the exponential of each component.
  - `Vector2 log()`: Returns the natural logarithm of each component.
  - `Vector2 exp2()`: Returns `2` raised to each component.
  - `Vector2 log2()`: Returns the base-2 logarithm of each component.
  - `Vector2 sqrt()`: Returns the square root of each component.
  - `Vector2 inversesqrt()`: Returns the inverse square root of each component.
  - `Vector2 sin()`: Returns the sine of each component (in radians).
  - `Vector2 cos()`: Returns the cosine of each component (in radians).
  - `Vector2 tan()`: Returns the tangent of each component (in radians).
  - `Vector2 asin()`: Returns the arcsine of each component.
  - `Vector2 acos()`: Returns the arccosine of each component.
  - `Vector2 atan()`: Returns the arctangent of each component.

#### Operators:

##### Arithmetic Operators:

- `Vector2 operator+(Vector2 other)`: Adds two vectors component-wise.
- `Vector2 operator-(Vector2 other)`: Subtracts `other` from the vector component-wise.
- `Vector2 operator*(Vector2 other)`: Multiplies two vectors component-wise.
- `Vector2 operator/(Vector2 other)`: Divides the vector by `other` component-wise.
- `Vector2 operator+(float scalar)`: Adds `scalar` to both vector's components.
- `Vector2 operator-(float scalar)`: Subtracts `scalar` to both vector's components.
- `Vector2 operator*(float scalar)`: Multiplies the vector by a scalar.
- `Vector2 operator/(float scalar)`: Divides the vector by a scalar.
- `Vector2 operator+()`: Unary plus (returns the vector itself).
- `Vector2 operator-()`: Unary minus (negates each component).

##### Compound Assignment Operators:

- `Vector2 operator+=(Vector2 other)`: Adds `other` to the vector component-wise.
- `Vector2 operator-=(Vector2 other)`: Subtracts `other` from the vector component-wise.
- `Vector2 operator*=(Vector2 other)`: Multiplies the vector by `other` component-wise.
- `Vector2 operator/=(Vector2 other)`: Divides the vector by `other` component-wise.
- `Vector2 operator+=(float scalar)`: Adds `scalar` to both vector's components.
- `Vector2 operator-=(float scalar)`: Subtracts `scalar` to both vector's components.
- `Vector2 operator*=(float scalar)`: Multiplies the vector by `scalar`.
- `Vector2 operator/=(float scalar)`: Divides the vector by `scalar`.

##### Comparison Operators:

- `bool operator==(Vector2 other)`: Component-wise equality check.
- `bool operator!=(Vector2 other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector2 | Vector2Int | Vector2UInt | float | Vector2 (unary) |
  |----------|---------|------------|-------------|-------|-----------------|
  | `+`      |    X    |     X      |      X      |   X   |        X        |
  | `-`      |    X    |     X      |      X      |   X   |        X        |
  | `*`      |    X    |     X      |      X      |   X   |                 |
  | `/`      |    X    |     X      |      X      |   X   |                 |
  | `+=`     |    X    |     X      |      X      |   X   |                 |
  | `-=`     |    X    |     X      |      X      |   X   |                 |
  | `*=`     |    X    |     X      |      X      |   X   |                 |
  | `/=`     |    X    |     X      |      X      |   X   |                 |
  | `==`     |    X    |     X      |      X      |       |                 |
  | `!=`     |    X    |     X      |      X      |       |                 |

### Vector2Int

2D vector with integer components.

#### Description:

  The `Vector2Int` type represents a 2D vector with `int` components `X` and `y`. It is useful for integer-based calculations in 2D space.

#### Attributes:

  - `int X`: The X-component of the vector.
  - `int y`: The y-component of the vector.

#### Constructors:

  - `Vector2Int()`: Default constructor that initializes both `X` and `y` to `0`.
  - `Vector2Int(int X, int y)`: Constructor that initializes the vector with specified `X` and `y` values.
  - `Vector2Int(int[2] values)`: Constructor that initializes the vector with an array of two `int` values.
  - `Vector2Int(Vector2 vec)`: Converts a `Vector2` to a `Vector2Int` by truncation.
  - `Vector2Int(Vector2UInt vec)`: Converts a `Vector2UInt` to a `Vector2Int`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector2`.
  - Implicitly convertible from `Vector2UInt`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector as a `float`.
  - `Vector2Int abs()`: Returns a vector with absolute values of each component.
  - `Vector2Int mod(Vector2Int divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector2Int min(Vector2Int other)`: Returns the minimum of each component with `other`.
  - `Vector2Int max(Vector2Int other)`: Returns the maximum of each component with `other`.
  - `Vector2Int clamp(Vector2Int minVal, Vector2Int maxVal)`: Clamps each component between `minVal` and `maxVal`.
  - `Vector2Int sign()`: Returns a vector with the sign of each component (`-1`, `0`, or `1`).

#### Operators:

##### Arithmetic Operators:

- `Vector2Int operator+(Vector2Int other)`: Adds two vectors component-wise.
- `Vector2Int operator-(Vector2Int other)`: Subtracts `other` from the vector component-wise.
- `Vector2Int operator*(Vector2Int other)`: Multiplies two vectors component-wise.
- `Vector2Int operator/(Vector2Int other)`: Divides the vector by `other` component-wise.
- `Vector2Int operator+(int scalar)`: Adds `scalar` to both vector's components.
- `Vector2Int operator-(int scalar)`: Subtracts `scalar` to both vector's components.
- `Vector2Int operator*(int scalar)`: Multiplies the vector by a scalar.
- `Vector2Int operator/(int scalar)`: Divides the vector by a scalar.
- `Vector2Int operator%(Vector2Int other)`: Component-wise modulus with `other`.
- `Vector2Int operator+()`: Unary plus (returns the vector itself).
- `Vector2Int operator-()`: Unary minus (negates each component).

##### Compound Assignment Operators:

- `Vector2Int operator+=(Vector2Int other)`: Adds `other` to the vector component-wise.
- `Vector2Int operator-=(Vector2Int other)`: Subtracts `other` from the vector component-wise.
- `Vector2Int operator*=(Vector2Int other)`: Multiplies the vector by `other` component-wise.
- `Vector2Int operator/=(Vector2Int other)`: Divides the vector by `other` component-wise.
- `Vector2Int operator%=(Vector2Int other)`: Modulus of the vector by `other`.
- `Vector2Int operator+=(int scalar)`: Adds `scalar` to both vector's components.
- `Vector2Int operator-=(int scalar)`: Subtracts `scalar` to both vector's components.
- `Vector2Int operator*=(int scalar)`: Multiplies the vector by `scalar`.
- `Vector2Int operator/=(int scalar)`: Divides the vector by `scalar`.

##### Comparison Operators:

- `bool operator==(Vector2Int other)`: Component-wise equality check.
- `bool operator!=(Vector2Int other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector2Int | Vector2 | Vector2UInt | int | Vector2Int (unary) |
  |----------|------------|---------|-------------|-----|--------------------|
  | `+`      |     X      |    X    |      X      |  X  |         X          |
  | `-`      |     X      |    X    |      X      |  X  |         X          |
  | `*`      |     X      |    X    |      X      |  X  |                    |
  | `/`      |     X      |    X    |      X      |  X  |                    |
  | `%`      |     X      |    X    |      X      |  X  |                    |
  | `+=`     |     X      |    X    |      X      |  X  |                    |
  | `-=`     |     X      |    X    |      X      |  X  |                    |
  | `*=`     |     X      |    X    |      X      |  X  |                    |
  | `/=`     |     X      |    X    |      X      |  X  |                    |
  | `%=`     |     X      |    X    |      X      |  X  |                    |
  | `==`     |     X      |    X    |      X      |     |                    |
  | `!=`     |     X      |    X    |      X      |     |                    |

### Vector2UInt

2D vector with unsigned integer components.

#### Description:

  The `Vector2UInt` type represents a 2D vector with `uint` components `X` and `y`. It is useful for non-negative integer calculations in 2D space.

#### Attributes:

  - `uint X`: The X-component of the vector.
  - `uint y`: The y-component of the vector.

#### Constructors:

  - `Vector2UInt()`: Default constructor that initializes both `X` and `y` to `0`.
  - `Vector2UInt(uint X, uint y)`: Constructor that initializes the vector with specified `X` and `y` values.
  - `Vector2UInt(uint[2] values)`: Constructor that initializes the vector with an array of two `uint` values.
  - `Vector2UInt(Vector2 vec)`: Converts a `Vector2` to a `Vector2UInt` by truncation.
  - `Vector2UInt(Vector2Int vec)`: Converts a `Vector2Int` to a `Vector2UInt`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector2`.
  - Implicitly convertible from `Vector2Int`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector as a `float`.
  - `Vector2UInt mod(Vector2UInt divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector2UInt min(Vector2UInt other)`: Returns the minimum of each component with `other`.
  - `Vector2UInt max(Vector2UInt other)`: Returns the maximum of each component with `other`.
  - `Vector2UInt clamp(Vector2UInt minVal, Vector2UInt maxVal)`: Clamps each component between `minVal` and `maxVal`.

#### Operators:

##### Arithmetic Operators:

- `Vector2UInt operator+(Vector2UInt other)`: Adds two vectors component-wise.
- `Vector2UInt operator-(Vector2UInt other)`: Subtracts `other` from the vector component-wise.
- `Vector2UInt operator*(Vector2UInt other)`: Multiplies two vectors component-wise.
- `Vector2UInt operator/(Vector2UInt other)`: Divides the vector by `other` component-wise.
- `Vector2UInt operator+(uint scalar)`: Adds `scalar` to both vector's components.
- `Vector2UInt operator-(uint scalar)`: Subtracts `scalar` to both vector's components.
- `Vector2UInt operator*(uint scalar)`: Multiplies the vector by a scalar.
- `Vector2UInt operator/(uint scalar)`: Divides the vector by a scalar.
- `Vector2UInt operator%(Vector2UInt other)`: Component-wise modulus with `other`.
- `Vector2UInt operator+()`: Unary plus (returns the vector itself).

##### Compound Assignment Operators:

- `Vector2UInt operator+=(Vector2UInt other)`: Adds `other` to the vector component-wise.
- `Vector2UInt operator-=(Vector2UInt other)`: Subtracts `other` from the vector component-wise.
- `Vector2UInt operator*=(Vector2UInt other)`: Multiplies the vector by `other` component-wise.
- `Vector2UInt operator/=(Vector2UInt other)`: Divides the vector by `other` component-wise.
- `Vector2UInt operator+=(uint scalar)`: Adds `scalar` to both vector's components.
- `Vector2UInt operator-=(uint scalar)`: Subtracts `scalar` to both vector's components.
- `Vector2UInt operator*=(uint scalar)`: Multiplies the vector by `scalar`.
- `Vector2UInt operator/=(uint scalar)`: Divides the vector by `scalar`.
- `Vector2UInt operator%=(Vector2UInt other)`: Modulus of the vector by `other`.

##### Comparison Operators:

- `bool operator==(Vector2UInt other)`: Component-wise equality check.
- `bool operator!=(Vector2UInt other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector2UInt | Vector2 | Vector2Int | uint | Vector2UInt (unary) |
  |----------|-------------|---------|------------|------|---------------------|
  | `+`      |      X      |    X    |     X      |  X   |          X          |
  | `-`      |      X      |    X    |     X      |  X   |          X          |
  | `*`      |      X      |    X    |     X      |  X   |                     |
  | `/`      |      X      |    X    |     X      |  X   |                     |
  | `%`      |      X      |    X    |     X      |  X   |                     |
  | `+=`     |      X      |    X    |     X      |  X   |                     |
  | `-=`     |      X      |    X    |     X      |  X   |                     |
  | `*=`     |      X      |    X    |     X      |  X   |                     |
  | `/=`     |      X      |    X    |     X      |  X   |                     |
  | `%=`     |      X      |    X    |     X      |  X   |                     |
  | `==`     |      X      |    X    |     X      |      |                     |
  | `!=`     |      X      |    X    |     X      |      |                     |

  ### Vector3

3D vector with floating-point components.

#### Description:

  The `Vector3` type represents a 3D vector with `float` components `x`, `y`, and `z`. It is commonly used in graphics and mathematical computations involving 3D coordinates or directions.

#### Attributes:

  - `float x`: The x-component of the vector.
  - `float y`: The y-component of the vector.
  - `float z`: The z-component of the vector.

#### Constructors:

  - `Vector3()`: Default constructor that initializes `x`, `y`, and `z` to `0.0f`.
  - `Vector3(float x, float y, float z)`: Constructor that initializes the vector with specified `x`, `y`, and `z` values.
  - `Vector3(float[3] values)`: Constructor that initializes the vector with an array of three `float` values.
  - `Vector3(Vector2 vec, float z)`: Constructs a `Vector3` from a `Vector2` and a `float`.
  - `Vector3(float x, Vector2 vec)`: Constructs a `Vector3` from a `float` and a `Vector2`.
  - `Vector3(Vector3Int vec)`: Converts a `Vector3Int` to a `Vector3`.
  - `Vector3(Vector3UInt vec)`: Converts a `Vector3UInt` to a `Vector3`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector3Int`.
  - Implicitly convertible from `Vector3UInt`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector.
  - `Vector3 normalize()`: Returns a normalized version of the vector.
  - `float dot(Vector3 other)`: Returns the dot product with `other`.
  - `Vector3 cross(Vector3 other)`: Returns the cross product with `other`.
  - `Vector3 reflect(Vector3 normal)`: Reflects the vector around the given normal.
  - `Vector3 abs()`: Returns a vector with absolute values of each component.
  - `Vector3 floor()`: Returns a vector with each component rounded down.
  - `Vector3 ceil()`: Returns a vector with each component rounded up.
  - `Vector3 fract()`: Returns the fractional part of each component.
  - `Vector3 mod(Vector3 divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector3 min(Vector3 other)`: Returns the minimum of each component with `other`.
  - `Vector3 max(Vector3 other)`: Returns the maximum of each component with `other`.
  - `Vector3 clamp(Vector3 minVal, Vector3 maxVal)`: Clamps each component between `minVal` and `maxVal`.
  - `Vector3 lerp(Vector3 other, float t)`: Linearly interpolates between the vector and `other` by `t`.
  - `Vector3 step(Vector3 edge)`: Returns `0.0f` for components less than `edge`, else `1.0f`.
  - `Vector3 smoothstep(Vector3 edge0, Vector3 edge1)`: Smooth Hermite interpolation.
  - `Vector3 pow(Vector3 exponent)`: Raises components to the powers in `exponent`.
  - `Vector3 exp()`: Returns the exponential of each component.
  - `Vector3 log()`: Returns the natural logarithm of each component.
  - `Vector3 exp2()`: Returns `2` raised to each component.
  - `Vector3 log2()`: Returns the base-2 logarithm of each component.
  - `Vector3 sqrt()`: Returns the square root of each component.
  - `Vector3 inversesqrt()`: Returns the inverse square root of each component.
  - `Vector3 sin()`: Returns the sine of each component (in radians).
  - `Vector3 cos()`: Returns the cosine of each component (in radians).
  - `Vector3 tan()`: Returns the tangent of each component (in radians).
  - `Vector3 asin()`: Returns the arcsine of each component.
  - `Vector3 acos()`: Returns the arccosine of each component.
  - `Vector3 atan()`: Returns the arctangent of each component.

#### Operators:

##### Arithmetic Operators:

- `Vector3 operator+(Vector3 other)`: Adds two vectors component-wise.
- `Vector3 operator-(Vector3 other)`: Subtracts `other` from the vector component-wise.
- `Vector3 operator*(Vector3 other)`: Multiplies two vectors component-wise.
- `Vector3 operator/(Vector3 other)`: Divides the vector by `other` component-wise.
- `Vector3 operator+(float scalar)`: Adds a scalar to each component of the vector.
- `Vector3 operator-(float scalar)`: Subtracts a scalar from each component of the vector.
- `Vector3 operator*(float scalar)`: Multiplies the vector by a scalar.
- `Vector3 operator/(float scalar)`: Divides the vector by a scalar.
- `Vector3 operator+()`: Unary plus (returns the vector itself).
- `Vector3 operator-()`: Unary minus (negates each component).

##### Compound Assignment Operators:

- `Vector3 operator+=(Vector3 other)`: Adds `other` to the vector component-wise.
- `Vector3 operator-=(Vector3 other)`: Subtracts `other` from the vector component-wise.
- `Vector3 operator*=(Vector3 other)`: Multiplies the vector by `other` component-wise.
- `Vector3 operator/=(Vector3 other)`: Divides the vector by `other` component-wise.
- `Vector3 operator+=(float scalar)`: Adds `scalar` to each component of the vector.
- `Vector3 operator-=(float scalar)`: Subtracts `scalar` from each component of the vector.
- `Vector3 operator*=(float scalar)`: Multiplies the vector by `scalar`.
- `Vector3 operator/=(float scalar)`: Divides the vector by `scalar`.

##### Comparison Operators:

- `bool operator==(Vector3 other)`: Component-wise equality check.
- `bool operator!=(Vector3 other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector3 | Vector3Int | Vector3UInt | float | Vector3 (unary) |
  |----------|---------|------------|-------------|-------|-----------------|
  | `+`      |    X    |     X      |      X      |   X   |        X        |
  | `-`      |    X    |     X      |      X      |   X   |        X        |
  | `*`      |    X    |     X      |      X      |   X   |                 |
  | `/`      |    X    |     X      |      X      |   X   |                 |
  | `+=`     |    X    |     X      |      X      |   X   |                 |
  | `-=`     |    X    |     X      |      X      |   X   |                 |
  | `*=`     |    X    |     X      |      X      |   X   |                 |
  | `/=`     |    X    |     X      |      X      |   X   |                 |
  | `==`     |    X    |     X      |      X      |       |                 |
  | `!=`     |    X    |     X      |      X      |       |                 |

### Vector3Int

3D vector with integer components.

#### Description:

  The `Vector3Int` type represents a 3D vector with `int` components `x`, `y`, and `z`. It is useful for integer-based calculations in 3D space.

#### Attributes:

  - `int x`: The x-component of the vector.
  - `int y`: The y-component of the vector.
  - `int z`: The z-component of the vector.

#### Constructors:

  - `Vector3Int()`: Default constructor that initializes `x`, `y`, and `z` to `0`.
  - `Vector3Int(int x, int y, int z)`: Constructor that initializes the vector with specified `x`, `y`, and `z` values.
  - `Vector3Int(int[3] values)`: Constructor that initializes the vector with an array of three `int` values.
  - `Vector3Int(Vector2Int vec, int z)`: Constructs a `Vector3Int` from a `Vector2Int` and an `int`.
  - `Vector3Int(int x, Vector2Int vec)`: Constructs a `Vector3Int` from an `int` and a `Vector2Int`.
  - `Vector3Int(Vector3 vec)`: Converts a `Vector3` to a `Vector3Int` by truncation.
  - `Vector3Int(Vector3UInt vec)`: Converts a `Vector3UInt` to a `Vector3Int`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector3`.
  - Implicitly convertible from `Vector3UInt`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector as a `float`.
  - `Vector3Int abs()`: Returns a vector with absolute values of each component.
  - `Vector3Int mod(Vector3Int divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector3Int min(Vector3Int other)`: Returns the minimum of each component with `other`.
  - `Vector3Int max(Vector3Int other)`: Returns the maximum of each component with `other`.
  - `Vector3Int clamp(Vector3Int minVal, Vector3Int maxVal)`: Clamps each component between `minVal` and `maxVal`.
  - `Vector3Int sign()`: Returns a vector with the sign of each component (`-1`, `0`, or `1`).

#### Operators:

##### Arithmetic Operators:

- `Vector3Int operator+(Vector3Int other)`: Adds two vectors component-wise.
- `Vector3Int operator-(Vector3Int other)`: Subtracts `other` from the vector component-wise.
- `Vector3Int operator*(Vector3Int other)`: Multiplies two vectors component-wise.
- `Vector3Int operator/(Vector3Int other)`: Divides the vector by `other` component-wise.
- `Vector3Int operator+(int scalar)`: Adds a scalar to each component of the vector.
- `Vector3Int operator-(int scalar)`: Subtracts a scalar from each component of the vector.
- `Vector3Int operator*(int scalar)`: Multiplies the vector by a scalar.
- `Vector3Int operator/(int scalar)`: Divides the vector by a scalar.
- `Vector3Int operator%(Vector3Int other)`: Component-wise modulus with `other`.
- `Vector3Int operator+()`: Unary plus (returns the vector itself).
- `Vector3Int operator-()`: Unary minus (negates each component).

##### Compound Assignment Operators:

- `Vector3Int operator+=(Vector3Int other)`: Adds `other` to the vector component-wise.
- `Vector3Int operator-=(Vector3Int other)`: Subtracts `other` from the vector component-wise.
- `Vector3Int operator*=(Vector3Int other)`: Multiplies the vector by `other` component-wise.
- `Vector3Int operator/=(Vector3Int other)`: Divides the vector by `other` component-wise.
- `Vector3Int operator+=(int scalar)`: Adds `scalar` to each component of the vector.
- `Vector3Int operator-=(int scalar)`: Subtracts `scalar` from each component of the vector.
- `Vector3Int operator*=(int scalar)`: Multiplies the vector by `scalar`.
- `Vector3Int operator/=(int scalar)`: Divides the vector by `scalar`.
- `Vector3Int operator%=(Vector3Int other)`: Modulus of the vector by `other`.

##### Comparison Operators:

- `bool operator==(Vector3Int other)`: Component-wise equality check.
- `bool operator!=(Vector3Int other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector3Int | Vector3 | Vector3UInt | int | Vector3Int (unary) |
  |----------|------------|---------|-------------|-----|--------------------|
  | `+`      |     X      |    X    |      X      |  X  |         X          |
  | `-`      |     X      |    X    |      X      |  X  |         X          |
  | `*`      |     X      |    X    |      X      |  X  |                    |
  | `/`      |     X      |    X    |      X      |  X  |                    |
  | `%`      |     X      |    X    |      X      |     |                    |
  | `+=`     |     X      |    X    |      X      |  X  |                    |
  | `-=`     |     X      |    X    |      X      |  X  |                    |
  | `*=`     |     X      |    X    |      X      |  X  |                    |
  | `/=`     |     X      |    X    |      X      |  X  |                    |
  | `%=`     |     X      |    X    |      X      |     |                    |
  | `==`     |     X      |    X    |      X      |     |                    |
  | `!=`     |     X      |    X    |      X      |     |                    |

### Vector3UInt

3D vector with unsigned integer components.

#### Description:

  The `Vector3UInt` type represents a 3D vector with `uint` components `x`, `y`, and `z`. It is useful for non-negative integer calculations in 3D space.

#### Attributes:

  - `uint x`: The x-component of the vector.
  - `uint y`: The y-component of the vector.
  - `uint z`: The z-component of the vector.

#### Constructors:

  - `Vector3UInt()`: Default constructor that initializes `x`, `y`, and `z` to `0`.
  - `Vector3UInt(uint x, uint y, uint z)`: Constructor that initializes the vector with specified `x`, `y`, and `z` values.
  - `Vector3UInt(uint[3] values)`: Constructor that initializes the vector with an array of three `uint` values.
  - `Vector3UInt(Vector2UInt vec, uint z)`: Constructs a `Vector3UInt` from a `Vector2UInt` and a `uint`.
  - `Vector3UInt(uint x, Vector2UInt vec)`: Constructs a `Vector3UInt` from a `uint` and a `Vector2UInt`.
  - `Vector3UInt(Vector3 vec)`: Converts a `Vector3` to a `Vector3UInt` by truncation.
  - `Vector3UInt(Vector3Int vec)`: Converts a `Vector3Int` to a `Vector3UInt`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector3`.
  - Implicitly convertible from `Vector3Int`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector as a `float`.
  - `Vector3UInt mod(Vector3UInt divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector3UInt min(Vector3UInt other)`: Returns the minimum of each component with `other`.
  - `Vector3UInt max(Vector3UInt other)`: Returns the maximum of each component with `other`.
  - `Vector3UInt clamp(Vector3UInt minVal, Vector3UInt maxVal)`: Clamps each component between `minVal` and `maxVal`.

#### Operators:

##### Arithmetic Operators:

- `Vector3UInt operator+(Vector3UInt other)`: Adds two vectors component-wise.
- `Vector3UInt operator-(Vector3UInt other)`: Subtracts `other` from the vector component-wise.
- `Vector3UInt operator*(Vector3UInt other)`: Multiplies two vectors component-wise.
- `Vector3UInt operator/(Vector3UInt other)`: Divides the vector by `other` component-wise.
- `Vector3UInt operator+(uint scalar)`: Adds a scalar to each component of the vector.
- `Vector3UInt operator-(uint scalar)`: Subtracts a scalar from each component of the vector.
- `Vector3UInt operator*(uint scalar)`: Multiplies the vector by a scalar.
- `Vector3UInt operator/(uint scalar)`: Divides the vector by a scalar.
- `Vector3UInt operator%(Vector3UInt other)`: Component-wise modulus with `other`.
- `Vector3UInt operator+()`: Unary plus (returns the vector itself).

##### Compound Assignment Operators:

- `Vector3UInt operator+=(Vector3UInt other)`: Adds `other` to the vector component-wise.
- `Vector3UInt operator-=(Vector3UInt other)`: Subtracts `other` from the vector component-wise.
- `Vector3UInt operator*=(Vector3UInt other)`: Multiplies the vector by `other` component-wise.
- `Vector3UInt operator/=(Vector3UInt other)`: Divides the vector by `other` component-wise.
- `Vector3UInt operator+=(uint scalar)`: Adds `scalar` to each component of the vector.
- `Vector3UInt operator-=(uint scalar)`: Subtracts `scalar` from each component of the vector.
- `Vector3UInt operator*=(uint scalar)`: Multiplies the vector by `scalar`.
- `Vector3UInt operator/=(uint scalar)`: Divides the vector by `scalar`.
- `Vector3UInt operator%=(Vector3UInt other)`: Modulus of the vector by `other`.

##### Comparison Operators:

- `bool operator==(Vector3UInt other)`: Component-wise equality check.
- `bool operator!=(Vector3UInt other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector3UInt | Vector3 | Vector3Int | uint | Vector3UInt (unary) |
  |----------|-------------|---------|------------|------|---------------------|
  | `+`      |      X      |    X    |     X      |  X   |          X          |
  | `-`      |      X      |    X    |     X      |  X   |                     |
  | `*`      |      X      |    X    |     X      |  X   |                     |
  | `/`      |      X      |    X    |     X      |  X   |                     |
  | `%`      |      X      |    X    |     X      |      |                     |
  | `+=`     |      X      |    X    |     X      |  X   |                     |
  | `-=`     |      X      |    X    |     X      |  X   |                     |
  | `*=`     |      X      |    X    |     X      |  X   |                     |
  | `/=`     |      X      |    X    |     X      |  X   |                     |
  | `%=`     |      X      |    X    |     X      |      |                     |
  | `==`     |      X      |    X    |     X      |      |                     |
  | `!=`     |      X      |    X    |     X      |      |                     |

  ### Vector4

4D vector with floating-point components.

#### Description:

  The `Vector4` type represents a 4D vector with `float` components `x`, `y`, `z`, and `w`. It is commonly used in graphics and mathematical computations involving 4D coordinates or homogeneous coordinates in 3D graphics.

#### Attributes:

  - `float x`: The x-component of the vector.
  - `float y`: The y-component of the vector.
  - `float z`: The z-component of the vector.
  - `float w`: The w-component of the vector.

#### Constructors:

  - `Vector4()`: Default constructor that initializes `x`, `y`, `z`, and `w` to `0.0f`.
  - `Vector4(float x, float y, float z, float w)`: Constructor that initializes the vector with specified `x`, `y`, `z`, and `w` values.
  - `Vector4(float[4] values)`: Constructor that initializes the vector with an array of four `float` values.
  - `Vector4(Vector2 vec1, Vector2 vec2)`: Constructs a `Vector4` from two `Vector2` vectors.
  - `Vector4(Vector3 vec, float w)`: Constructs a `Vector4` from a `Vector3` and a `float`.
  - `Vector4(float x, Vector3 vec)`: Constructs a `Vector4` from a `float` and a `Vector3`.
  - `Vector4(Vector4Int vec)`: Converts a `Vector4Int` to a `Vector4`.
  - `Vector4(Vector4UInt vec)`: Converts a `Vector4UInt` to a `Vector4`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector4Int`.
  - Implicitly convertible from `Vector4UInt`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector.
  - `Vector4 normalize()`: Returns a normalized version of the vector.
  - `float dot(Vector4 other)`: Returns the dot product with `other`.
  - `Vector4 reflect(Vector4 normal)`: Reflects the vector around the given normal.
  - `Vector4 abs()`: Returns a vector with absolute values of each component.
  - `Vector4 floor()`: Returns a vector with each component rounded down.
  - `Vector4 ceil()`: Returns a vector with each component rounded up.
  - `Vector4 fract()`: Returns the fractional part of each component.
  - `Vector4 mod(Vector4 divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector4 min(Vector4 other)`: Returns the minimum of each component with `other`.
  - `Vector4 max(Vector4 other)`: Returns the maximum of each component with `other`.
  - `Vector4 clamp(Vector4 minVal, Vector4 maxVal)`: Clamps each component between `minVal` and `maxVal`.
  - `Vector4 lerp(Vector4 other, float t)`: Linearly interpolates between the vector and `other` by `t`.
  - `Vector4 step(Vector4 edge)`: Returns `0.0f` for components less than `edge`, else `1.0f`.
  - `Vector4 smoothstep(Vector4 edge0, Vector4 edge1)`: Smooth Hermite interpolation.
  - `Vector4 pow(Vector4 exponent)`: Raises components to the powers in `exponent`.
  - `Vector4 exp()`: Returns the exponential of each component.
  - `Vector4 log()`: Returns the natural logarithm of each component.
  - `Vector4 exp2()`: Returns `2` raised to each component.
  - `Vector4 log2()`: Returns the base-2 logarithm of each component.
  - `Vector4 sqrt()`: Returns the square root of each component.
  - `Vector4 inversesqrt()`: Returns the inverse square root of each component.
  - `Vector4 sin()`: Returns the sine of each component (in radians).
  - `Vector4 cos()`: Returns the cosine of each component (in radians).
  - `Vector4 tan()`: Returns the tangent of each component (in radians).
  - `Vector4 asin()`: Returns the arcsine of each component.
  - `Vector4 acos()`: Returns the arccosine of each component.
  - `Vector4 atan()`: Returns the arctangent of each component.

#### Operators:

##### Arithmetic Operators:

- `Vector4 operator+(Vector4 other)`: Adds two vectors component-wise.
- `Vector4 operator-(Vector4 other)`: Subtracts `other` from the vector component-wise.
- `Vector4 operator*(Vector4 other)`: Multiplies two vectors component-wise.
- `Vector4 operator/(Vector4 other)`: Divides the vector by `other` component-wise.
- `Vector4 operator+(float scalar)`: Adds a scalar to each component of the vector.
- `Vector4 operator-(float scalar)`: Subtracts a scalar from each component of the vector.
- `Vector4 operator*(float scalar)`: Multiplies the vector by a scalar.
- `Vector4 operator/(float scalar)`: Divides the vector by a scalar.
- `Vector4 operator+()`: Unary plus (returns the vector itself).
- `Vector4 operator-()`: Unary minus (negates each component).

##### Compound Assignment Operators:

- `Vector4 operator+=(Vector4 other)`: Adds `other` to the vector component-wise.
- `Vector4 operator-=(Vector4 other)`: Subtracts `other` from the vector component-wise.
- `Vector4 operator*=(Vector4 other)`: Multiplies the vector by `other` component-wise.
- `Vector4 operator/=(Vector4 other)`: Divides the vector by `other` component-wise.
- `Vector4 operator+=(float scalar)`: Adds `scalar` to each component of the vector.
- `Vector4 operator-=(float scalar)`: Subtracts `scalar` from each component of the vector.
- `Vector4 operator*=(float scalar)`: Multiplies the vector by `scalar`.
- `Vector4 operator/=(float scalar)`: Divides the vector by `scalar`.

##### Comparison Operators:

- `bool operator==(Vector4 other)`: Component-wise equality check.
- `bool operator!=(Vector4 other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector4 | Vector4Int | Vector4UInt | float | Vector4 (unary) |
  |----------|---------|------------|-------------|-------|-----------------|
  | `+`      |    X    |     X      |      X      |   X   |        X        |
  | `-`      |    X    |     X      |      X      |   X   |        X        |
  | `*`      |    X    |     X      |      X      |   X   |                 |
  | `/`      |    X    |     X      |      X      |   X   |                 |
  | `+=`     |    X    |     X      |      X      |   X   |                 |
  | `-=`     |    X    |     X      |      X      |   X   |                 |
  | `*=`     |    X    |     X      |      X      |   X   |                 |
  | `/=`     |    X    |     X      |      X      |   X   |                 |
  | `==`     |    X    |     X      |      X      |       |                 |
  | `!=`     |    X    |     X      |      X      |       |                 |

### Vector4Int

4D vector with integer components.

#### Description:

  The `Vector4Int` type represents a 4D vector with `int` components `x`, `y`, `z`, and `w`. It is useful for integer-based calculations in 4D space or when working with homogeneous coordinates in an integer context.

#### Attributes:

  - `int x`: The x-component of the vector.
  - `int y`: The y-component of the vector.
  - `int z`: The z-component of the vector.
  - `int w`: The w-component of the vector.

#### Constructors:

  - `Vector4Int()`: Default constructor that initializes `x`, `y`, `z`, and `w` to `0`.
  - `Vector4Int(int x, int y, int z, int w)`: Constructor that initializes the vector with specified `x`, `y`, `z`, and `w` values.
  - `Vector4Int(int[4] values)`: Constructor that initializes the vector with an array of four `int` values.
  - `Vector4Int(Vector2Int vec1, Vector2Int vec2)`: Constructs a `Vector4Int` from two `Vector2Int` vectors.
  - `Vector4Int(Vector3Int vec, int w)`: Constructs a `Vector4Int` from a `Vector3Int` and an `int`.
  - `Vector4Int(int x, Vector3Int vec)`: Constructs a `Vector4Int` from an `int` and a `Vector3Int`.
  - `Vector4Int(Vector4 vec)`: Converts a `Vector4` to a `Vector4Int` by truncation.
  - `Vector4Int(Vector4UInt vec)`: Converts a `Vector4UInt` to a `Vector4Int`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector4`.
  - Implicitly convertible from `Vector4UInt`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector as a `float`.
  - `Vector4Int abs()`: Returns a vector with absolute values of each component.
  - `Vector4Int mod(Vector4Int divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector4Int min(Vector4Int other)`: Returns the minimum of each component with `other`.
  - `Vector4Int max(Vector4Int other)`: Returns the maximum of each component with `other`.
  - `Vector4Int clamp(Vector4Int minVal, Vector4Int maxVal)`: Clamps each component between `minVal` and `maxVal`.
  - `Vector4Int sign()`: Returns a vector with the sign of each component (`-1`, `0`, or `1`).

#### Operators:

##### Arithmetic Operators:

- `Vector4Int operator+(Vector4Int other)`: Adds two vectors component-wise.
- `Vector4Int operator-(Vector4Int other)`: Subtracts `other` from the vector component-wise.
- `Vector4Int operator*(Vector4Int other)`: Multiplies two vectors component-wise.
- `Vector4Int operator/(Vector4Int other)`: Divides the vector by `other` component-wise.
- `Vector4Int operator+(int scalar)`: Adds a scalar to each component of the vector.
- `Vector4Int operator-(int scalar)`: Subtracts a scalar from each component of the vector.
- `Vector4Int operator*(int scalar)`: Multiplies the vector by a scalar.
- `Vector4Int operator/(int scalar)`: Divides the vector by a scalar.
- `Vector4Int operator%(Vector4Int other)`: Component-wise modulus with `other`.
- `Vector4Int operator+()`: Unary plus (returns the vector itself).
- `Vector4Int operator-()`: Unary minus (negates each component).

##### Compound Assignment Operators:

- `Vector4Int operator+=(Vector4Int other)`: Adds `other` to the vector component-wise.
- `Vector4Int operator-=(Vector4Int other)`: Subtracts `other` from the vector component-wise.
- `Vector4Int operator*=(Vector4Int other)`: Multiplies the vector by `other` component-wise.
- `Vector4Int operator/=(Vector4Int other)`: Divides the vector by `other` component-wise.
- `Vector4Int operator+=(int scalar)`: Adds `scalar` to each component of the vector.
- `Vector4Int operator-=(int scalar)`: Subtracts `scalar` from each component of the vector.
- `Vector4Int operator*=(int scalar)`: Multiplies the vector by `scalar`.
- `Vector4Int operator/=(int scalar)`: Divides the vector by `scalar`.
- `Vector4Int operator%=(Vector4Int other)`: Modulus of the vector by `other`.

##### Comparison Operators:

- `bool operator==(Vector4Int other)`: Component-wise equality check.
- `bool operator!=(Vector4Int other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector4Int | Vector4 | Vector4UInt | int | Vector4Int (unary) |
  |----------|------------|---------|-------------|-----|--------------------|
  | `+`      |     X      |    X    |      X      |  X  |         X          |
  | `-`      |     X      |    X    |      X      |  X  |         X          |
  | `*`      |     X      |    X    |      X      |  X  |                    |
  | `/`      |     X      |    X    |      X      |  X  |                    |
  | `%`      |     X      |    X    |      X      |     |                    |
  | `+=`     |     X      |    X    |      X      |  X  |                    |
  | `-=`     |     X      |    X    |      X      |  X  |                    |
  | `*=`     |     X      |    X    |      X      |  X  |                    |
  | `/=`     |     X      |    X    |      X      |  X  |                    |
  | `%=`     |     X      |    X    |      X      |     |                    |
  | `==`     |     X      |    X    |      X      |     |                    |
  | `!=`     |     X      |    X    |      X      |     |                    |

### Vector4UInt

4D vector with unsigned integer components.

#### Description:

  The `Vector4UInt` type represents a 4D vector with `uint` components `x`, `y`, `z`, and `w`. It is useful for non-negative integer calculations in 4D space or when working with homogeneous coordinates in an unsigned integer context.

#### Attributes:

  - `uint x`: The x-component of the vector.
  - `uint y`: The y-component of the vector.
  - `uint z`: The z-component of the vector.
  - `uint w`: The w-component of the vector.

#### Constructors:

  - `Vector4UInt()`: Default constructor that initializes `x`, `y`, `z`, and `w` to `0`.
  - `Vector4UInt(uint x, uint y, uint z, uint w)`: Constructor that initializes the vector with specified `x`, `y`, `z`, and `w` values.
  - `Vector4UInt(uint[4] values)`: Constructor that initializes the vector with an array of four `uint` values.
  - `Vector4UInt(Vector2UInt vec1, Vector2UInt vec2)`: Constructs a `Vector4UInt` from two `Vector2UInt` vectors.
  - `Vector4UInt(Vector3UInt vec, uint w)`: Constructs a `Vector4UInt` from a `Vector3UInt` and a `uint`.
  - `Vector4UInt(uint x, Vector3UInt vec)`: Constructs a `Vector4UInt` from a `uint` and a `Vector3UInt`.
  - `Vector4UInt(Vector4 vec)`: Converts a `Vector4` to a `Vector4UInt` by truncation.
  - `Vector4UInt(Vector4Int vec)`: Converts a `Vector4Int` to a `Vector4UInt`.

#### Implicit Conversions:

  - Implicitly convertible from `Vector4`.
  - Implicitly convertible from `Vector4Int`.

#### Functions:

  - `float length()`: Returns the length (magnitude) of the vector as a `float`.
  - `Vector4UInt mod(Vector4UInt divisor)`: Returns the component-wise modulus with `divisor`.
  - `Vector4UInt min(Vector4UInt other)`: Returns the minimum of each component with `other`.
  - `Vector4UInt max(Vector4UInt other)`: Returns the maximum of each component with `other`.
  - `Vector4UInt clamp(Vector4UInt minVal, Vector4UInt maxVal)`: Clamps each component between `minVal` and `maxVal`.

#### Operators:

##### Arithmetic Operators:

- `Vector4UInt operator+(Vector4UInt other)`: Adds two vectors component-wise.
- `Vector4UInt operator-(Vector4UInt other)`: Subtracts `other` from the vector component-wise.
- `Vector4UInt operator*(Vector4UInt other)`: Multiplies two vectors component-wise.
- `Vector4UInt operator/(Vector4UInt other)`: Divides the vector by `other` component-wise.
- `Vector4UInt operator+(uint scalar)`: Adds a scalar to each component of the vector.
- `Vector4UInt operator-(uint scalar)`: Subtracts a scalar from each component of the vector.
- `Vector4UInt operator*(uint scalar)`: Multiplies the vector by a scalar.
- `Vector4UInt operator/(uint scalar)`: Divides the vector by a scalar.
- `Vector4UInt operator%(Vector4UInt other)`: Component-wise modulus with `other`.
- `Vector4UInt operator+()`: Unary plus (returns the vector itself).

##### Compound Assignment Operators:

- `Vector4UInt operator+=(Vector4UInt other)`: Adds `other` to the vector component-wise.
- `Vector4UInt operator-=(Vector4UInt other)`: Subtracts `other` from the vector component-wise.
- `Vector4UInt operator*=(Vector4UInt other)`: Multiplies the vector by `other` component-wise.
- `Vector4UInt operator/=(Vector4UInt other)`: Divides the vector by `other` component-wise.
- `Vector4UInt operator+=(uint scalar)`: Adds `scalar` to each component of the vector.
- `Vector4UInt operator-=(uint scalar)`: Subtracts `scalar` from each component of the vector.
- `Vector4UInt operator*=(uint scalar)`: Multiplies the vector by `scalar`.
- `Vector4UInt operator/=(uint scalar)`: Divides the vector by `scalar`.
- `Vector4UInt operator%=(Vector4UInt other)`: Modulus of the vector by `other`.

##### Comparison Operators:

- `bool operator==(Vector4UInt other)`: Component-wise equality check.
- `bool operator!=(Vector4UInt other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Vector4UInt | Vector4 | Vector4Int | uint | Vector4UInt (unary) |
  |----------|-------------|---------|------------|------|---------------------|
  | `+`      |      X      |    X    |     X      |  X   |          X          |
  | `-`      |      X      |    X    |     X      |  X   |                     |
  | `*`      |      X      |    X    |     X      |  X   |                     |
  | `/`      |      X      |    X    |     X      |  X   |                     |
  | `%`      |      X      |    X    |     X      |      |                     |
  | `+=`     |      X      |    X    |     X      |  X   |                     |
  | `-=`     |      X      |    X    |     X      |  X   |                     |
  | `*=`     |      X      |    X    |     X      |  X   |                     |
  | `/=`     |      X      |    X    |     X      |  X   |                     |
  | `%=`     |      X      |    X    |     X      |      |                     |
  | `==`     |      X      |    X    |     X      |      |                     |
  | `!=`     |      X      |    X    |     X      |      |                     |

### Color

Color representation with floating-point components.

#### Description:

  The `Color` type represents a color with `float` components `r`, `g`, `b`, and `a`, corresponding to red, green, blue, and alpha (transparency). Each component typically ranges from `0.0f` to `1.0f`.

#### Attributes:

  - `float r`: The red component of the color.
  - `float g`: The green component of the color.
  - `float b`: The blue component of the color.
  - `float a`: The alpha (transparency) component of the color.

#### Constructors:

  - `Color()`: Default constructor that initializes `r`, `g`, `b`, and `a` to `0.0f`.
  - `Color(float r, float g, float b, float a)`: Constructor that initializes the color with specified `r`, `g`, `b`, and `a` values.
  - `Color(float[4] values)`: Constructor that initializes the color with an array of four `float` values.

#### Functions:

  - `Color lerp(Color other, float t)`: Linearly interpolates between the color and `other` by `t`.
  - `Color clamp(Color minVal, Color maxVal)`: Clamps each component between `minVal` and `maxVal`.
  - `Color saturate()`: Clamps each component between `0.0f` and `1.0f`.
  - `Color min(Color other)`: Returns the minimum of each component with `other`.
  - `Color max(Color other)`: Returns the maximum of each component with `other`.
  - `Color abs()`: Returns a color with absolute values of each component.
  - `Color smoothstep(Color edge0, Color edge1)`: Smooth Hermite interpolation between two colors.
  - `Color step(Color edge)`: Returns `0.0f` for components less than `edge`, else `1.0f`.

#### Operators:

##### Arithmetic Operators:

- `Color operator+(Color other)`: Adds two colors component-wise.
- `Color operator-(Color other)`: Subtracts `other` from the color component-wise.
- `Color operator*(Color other)`: Multiplies two colors component-wise.
- `Color operator/(Color other)`: Divides the color by `other` component-wise.
- `Color operator+(float scalar)`: Adds a scalar to each component of the color.
- `Color operator-(float scalar)`: Subtracts a scalar from each component of the color.
- `Color operator*(float scalar)`: Multiplies the color by a scalar.
- `Color operator/(float scalar)`: Divides the color by a scalar.
- `Color operator+()`: Unary plus (returns the color itself).
- `Color operator-()`: Unary minus (negates each component).

##### Compound Assignment Operators:

- `Color operator+=(Color other)`: Adds `other` to the color component-wise.
- `Color operator-=(Color other)`: Subtracts `other` from the color component-wise.
- `Color operator*=(Color other)`: Multiplies the color by `other` component-wise.
- `Color operator/=(Color other)`: Divides the color by `other` component-wise.
- `Color operator+=(float scalar)`: Adds `scalar` to each component of the color.
- `Color operator-=(float scalar)`: Subtracts `scalar` from each component of the color.
- `Color operator*=(float scalar)`: Multiplies the color by `scalar`.
- `Color operator/=(float scalar)`: Divides the color by `scalar`.

##### Comparison Operators:

- `bool operator==(Color other)`: Component-wise equality check.
- `bool operator!=(Color other)`: Component-wise inequality check.

##### Operator Table:

  | Operator | Color | Vector4 | float | Color (unary) |
  |----------|-------|---------|-------|---------------|
  | `+`      |   X   |    X    |   X   |       X       |
  | `-`      |   X   |    X    |   X   |       X       |
  | `*`      |   X   |    X    |   X   |               |
  | `/`      |   X   |    X    |   X   |               |
  | `+=`     |   X   |    X    |   X   |               |
  | `-=`     |   X   |    X    |   X   |               |
  | `*=`     |   X   |    X    |   X   |               |
  | `/=`     |   X   |    X    |   X   |               |
  | `==`     |   X   |    X    |       |               |
  | `!=`     |   X   |    X    |       |               |

## Texture Type

### Texture

Texture object for sampling colors.

#### Description:
  The `Texture` type represents a texture resource that can be sampled to retrieve color information. Textures are sampled using UV coordinates, where `(0.0f, 0.0f)` corresponds to the bottom-left corner and `(1.0f, 1.0f)` corresponds to the top-right corner.

#### Constructors:
	None.

#### Functions:
	None.

##### Member Functions:
- `Color getPixel(Vector2 UVs)`: Samples the texture at the given UV coordinates and returns the color.
- `Vector2 size()`: Returns the dimensions of the texture.

#### Operators:
	None.