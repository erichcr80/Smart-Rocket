class rgb
{
  public:
    byte r, g, b;

    rgb() : r(0), g(0), b(0) {}
    rgb(byte r, byte g, byte b) : r(r), g(g), b(b) {}

    rgb dim(byte level)
    {
      float dim = (float)level / 255.0;
      return rgb((byte)(dim * (float)r), (byte)(dim * (float)g), (byte)(dim * (float)b));
    }

    static rgb purple;
    static rgb yellow;
    static rgb cyan;
    static rgb red;
    static rgb green;
    static rgb blue;
};

rgb rgb::purple = rgb(128, 0, 255);
rgb rgb::yellow = rgb(196, 255, 0);
rgb rgb::cyan = rgb(0, 255, 255);
rgb rgb::red = rgb(196, 0, 0);
rgb rgb::green = rgb(0, 255, 0);
rgb rgb::blue = rgb(0, 0, 255);

rgb all_colors[6] = {
  rgb::purple,
  rgb::yellow,
  rgb::cyan,
  rgb::red,
  rgb::green,
  rgb::blue
};

class led3
{
    rgb current;
    int r_pin, g_pin, b_pin, power_pin;

  public:
    led3(int r_pin, int g_pin, int b_pin, int power_pin)
      : r_pin(r_pin), g_pin(g_pin), b_pin(b_pin), power_pin(power_pin) {}

    void do_setup()
    {
      pinMode(power_pin, OUTPUT);
      digitalWrite(power_pin, HIGH);
    }

    rgb get_current() 
    {
      return current;
    }

    void set_rgb(const rgb & set)
    {
      set_rgb(set.r, set.g, set.b);
    }

    void set_rgb(byte r_in, byte g_in, byte b_in)
    {
      current.r = 255 - r_in;
      current.g = 255 - g_in;
      current.b = 255 - b_in;

      Serial.print(sizeof(led3));  Serial.print("\t");
      Serial.print(r_pin); Serial.print("\t");
      Serial.print(g_pin); Serial.print("\t");
      Serial.print(b_pin); Serial.print("\t");

      analogWrite(r_pin, current.r);
      analogWrite(g_pin, current.g);
      analogWrite(b_pin, current.b);
    }

    short get_max() 
    {
      return max(max(current.r, current.g), current.b);
    }
    
    short get_min() 
    {
      return min(min(current.r, current.g), current.b);
    }
};

led3 leds[2] =
{
  led3(11 /* r */, 10 /* g */, 9 /* b */, 12 /* power */),
  led3(6  /* r */, 5  /* g */, 3 /* b */, 4 /* power */)
};

enum rocket_mode
{
  flight,
  demo
} current_mode;

int demo_mode_wait = 5000;

int current_wait = 0; // how long has this direction been up?
int last_direction = 0; // 0 = x, 1 = y, 2 = z

const int xpin = A0;                  // x-axis of the accelerometer
const int ypin = A1;                  // y-axis
const int zpin = A2;                  // z-axis (only on 3-axis models)

void setup()
{
  leds[0].do_setup();
  leds[1].do_setup();

  current_mode = flight;

  // initialize the serial communications:
  Serial.begin(115200);
}

int defaultAccVal = 385;

float normxValue = 0.0;
float normyValue = 0.0;
float normzValue = 0.0;

void do_direction()
{
  int xValue = analogRead(xpin);
  int yValue = analogRead(ypin);
  int zValue = analogRead(zpin);

  Serial.print(xValue); Serial.print("\t");
  Serial.print(yValue); Serial.print("\t");
  Serial.print(zValue); Serial.print("\t");

  int dxValue = xValue - defaultAccVal;
  int dyValue = yValue - defaultAccVal;
  int dzValue = zValue - defaultAccVal;

  float fxValue = dxValue;
  float fyValue = dyValue;
  float fzValue = dzValue;

  float dLength = sqrt(fxValue * fxValue + fyValue * fyValue + fzValue * fzValue);

  normxValue = fxValue / dLength;
  normyValue = fyValue / dLength;
  normzValue = fzValue / dLength;

  Serial.print(normxValue); Serial.print("\t");
  Serial.print(normyValue); Serial.print("\t");
  Serial.print(normzValue); Serial.print("\t");
}

void do_flight()
{
  /* blend colors based on direction */
  int rOut = (1.0 - abs(normxValue)) * 255;
  int gOut = (1.0 - abs(normyValue)) * 255;
  int bOut = (1.0 - abs(normzValue)) * 255;

  /* single colors based on direction (current method) */
  rOut = get_current_direction() == 0 ? 255 : 0;
  gOut = get_current_direction() == 1 ? 255 : 0;
  bOut = get_current_direction() == 2 ? 255 : 0;

  leds[0].set_rgb(rOut, gOut, bOut);
  leds[1].set_rgb(rOut, gOut, bOut);
}

int demo_level = 0;
int demo_level_change = 3;

rgb_change demo_up(1, 0, 1);
rgb_change demo_down(1, 0, 1);

rgb_change demo_change = demo_up;

rgb color1 = rgb::yellow;
rgb color2 = rgb::purple;

void do_demo()
{
  leds[1].set_rgb(color1.dim(demo_level));
  leds[0].set_rgb(color2.dim(demo_level));

  if (demo_level >= 255)
  {
    demo_level_change = -5;
  }
  else if (demo_level <= 0)
  {
    demo_level_change = 5;
    
    color1 = all_colors[random(6)];
    color2 = all_colors[random(6)];
  }
  
  demo_level += demo_level_change;
}

int get_current_direction()
{
  if (abs(normxValue) > abs(normyValue) && abs(normxValue) > abs(normzValue))
    return 0;
  else if (abs(normyValue) > abs(normxValue) && abs(normyValue) > abs(normzValue))
    return 1;
  else if (abs(normzValue) > abs(normxValue) && abs(normzValue) > abs(normyValue))
    return 2;

  return 0;
}

void loop()
{
  do_direction();

  if (current_mode == flight)
    do_flight();
  else if (current_mode == demo)
    do_demo();

  int d = 5;
  delay(d);

  int current_direction = get_current_direction();

  if (current_direction != last_direction)
  {
    last_direction = current_direction;
    current_wait = 0;
  }

  current_wait += d;

  if (current_wait > demo_mode_wait)
  {
    current_mode = demo;
  }
  else
  {
    current_mode = flight;
  }

  Serial.println("");
}
