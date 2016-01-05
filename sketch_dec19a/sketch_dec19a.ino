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
    static rgb orange;
};

rgb rgb::purple = rgb(128, 0, 255);
rgb rgb::yellow = rgb(196, 255, 0);
rgb rgb::cyan = rgb(0, 255, 255);
rgb rgb::red = rgb(196, 0, 0);
rgb rgb::green = rgb(0, 255, 0);
rgb rgb::blue = rgb(0, 0, 255);
rgb rgb::orange = rgb(128, 128, 0);

rgb all_colors[7] = {
  rgb::purple,
  rgb::yellow,
  rgb::cyan,
  rgb::red,
  rgb::green,
  rgb::blue,
  rgb::orange
};

int all_colors_count = 7;

class led3
{
    rgb output, current;
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
      //Serial.print(r_in); Serial.print("\t");
      //Serial.print(g_in); Serial.print("\t");
      //Serial.print(b_in); Serial.print("\t");
      current.r = r_in;
      current.g = g_in;
      current.b = b_in;

      /* flip for common anode */
      output = current;
      output.r = 255 - current.r;
      output.g = 255 - current.g;
      output.b = 255 - current.b;

      //Serial.print(sizeof(led3));  Serial.print("\t");
      //Serial.print(current.r); Serial.print("\t");
      //Serial.print(current.g); Serial.print("\t");
      //Serial.print(current.b); Serial.print("\t");

      analogWrite(r_pin, output.r);
      analogWrite(g_pin, output.g);
      analogWrite(b_pin, output.b);
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

class fader
{
    int max_level;

    int current_level;
    int step_value;

    bool done;

  public:
    fader() : current_level(0), step_value(0), done(true), max_level(255) {}
    fader(int step_value) : current_level(0), step_value(step_value), done(false), max_level(255) {}

    void next()
    {
      Serial.print("next: "); Serial.print(current_level); Serial.print("\t"); Serial.print(step_value); Serial.print("\t");
      current_level += step_value;

      if (current_level >= max_level)
      {
        step_value = -step_value;
        current_level = max_level;
      }
      else if (current_level <= 0)
      {
        step_value = -step_value;
        current_level = 0;

        done = true;
      }
    }

    int get_level()
    {
      return current_level;
    }

    void set_max_level(int value)
    {
      max_level = value;
    }

    void set_current_level(int value, int new_step_value)
    {
      current_level = value;
      done = false;

      step_value = new_step_value;

      if (current_level >= max_level && step_value > 0)
        step_value = -step_value;
    }

    bool is_done()
    {
      return done;
    }

    bool reset()
    {
      current_level = 0;
      done = false;
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
  demo,
  sleep
} current_mode;

long demo_mode_wait = 30L * 1000L; // 30 seconds
long sleep_mode_wait = 10L * 60L * 1000L; // 10 minutes

long current_wait = 0; // how long has this direction been up?
int last_direction = 0; // 0 = x, 1 = y, 2 = z

const int xpin = A0;                  // x-axis of the accelerometer
const int ypin = A1;                  // y-axis
const int zpin = A2;                  // z-axis (only on 3-axis models)

int defaultAccVal = 335;

void setup()
{
  leds[0].do_setup();
  leds[1].do_setup();

  current_mode = flight;

  // initialize the serial communications:
  //Serial.begin(115200);
}

/* parallel arrays for moving average */
int lastIndex = 0;
int lastCount = 5;
float lastX[5];
float lastY[5];
float lastZ[5];

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

  float normxValue = fxValue / dLength;
  float normyValue = fyValue / dLength;
  float normzValue = fzValue / dLength;

  Serial.print(normxValue); Serial.print("\t");
  Serial.print(normyValue); Serial.print("\t");
  Serial.print(normzValue); Serial.print("\t");

  lastX[lastIndex] = normxValue;
  lastY[lastIndex] = normyValue;
  lastZ[lastIndex] = normzValue;

  lastIndex++;

  if (lastIndex == lastCount)
    lastIndex = 0;
}

float getAverageValue(float * array)
{
  float acc = 0.0;

  for (int i = 0; i < lastCount; i++)
    acc += array[i];

  return acc / lastCount;
}

float getX() { return getAverageValue(lastX); }
float getY() { return getAverageValue(lastY); }
float getZ() { return getAverageValue(lastZ); }

void do_flight()
{
  /* blend colors based on direction */
  int rOut = abs(getX()) * 255;
  int gOut = abs(getY()) * 255;
  int bOut = abs(getZ()) * 255;

  leds[0].set_rgb(rOut, gOut, bOut);
  leds[1].set_rgb(rOut, gOut, bOut);
}

rgb colors[2];

fader faders[2];
int max_fader_step = 4;

void do_demo()
{
  for (int i = 0; i < 2; i++)
  {
    if (faders[i].is_done())
    {
      faders[i] = fader(random(max_fader_step) + 2);
      colors[i] = all_colors[random(all_colors_count)];
    }
    else
    {
      faders[i].next();
    }

    leds[i].set_rgb(colors[i].dim(faders[i].get_level()));
  }
}

fader sleep_fader(1);
rgb sleep_color = rgb::red;

void do_sleep()
{
  sleep_fader.set_max_level(64); // quarter brightness

  sleep_fader.next();

  if (sleep_fader.is_done())
    sleep_fader.reset();

  for (int i = 0; i < 2; i++)
    leds[i].set_rgb(sleep_color.dim(sleep_fader.get_level()));

  delay(100);
}

int get_current_direction()
{
  if (abs(getX()) > abs(getY()) && abs(getX()) > abs(getZ()))
    return 0;
  else if (abs(getY()) > abs(getX()) && abs(getY()) > abs(getZ()))
    return 1;
  else if (abs(getZ()) > abs(getX()) && abs(getZ()) > abs(getY()))
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
  else if (current_mode == sleep)
    do_sleep();

  int d = 5;
  delay(d);

  int current_direction = get_current_direction();

  if (current_direction != last_direction)
  {
    last_direction = current_direction;
    current_wait = 0;
  }
  else
  {
    current_wait += d;
  }

  if (current_wait > sleep_mode_wait)
  {
    if (current_mode != sleep)
      current_mode = sleep;
  }
  else if (current_wait > demo_mode_wait)
  {
    if (current_mode != demo)
    {
      faders[0].reset();

      faders[0].set_current_level(255, -2);
      faders[1].set_current_level(255, -2);

      colors[0] = leds[0].get_current();
      colors[1] = leds[1].get_current();
    }

    current_mode = demo;
  }
  else
  {
    current_mode = flight;
  }

  Serial.println("");
}
