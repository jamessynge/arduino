// TODO(james): Describe this sketch.

/*
################################################################################

With Serial initialization and a delay in loop, but nothing else:

#define DO_LOG_SINK 0
#define DO_VOID_SINK 0

Sketch uses 1932 bytes (0%) of program storage space. Maximum is 253952 bytes.
Global variables use 184 bytes (2%) of dynamic memory, leaving 8008 bytes for local variables. Maximum is 8192 bytes.

################################################################################

With Serial initialization and a delay in loop, and with LogSink:

#define DO_LOG_SINK 1
#define DO_VOID_SINK 0

Sketch uses 2048 bytes (0%) of program storage space. Maximum is 253952 bytes.
Global variables use 194 bytes (2%) of dynamic memory, leaving 7998 bytes for local variables. Maximum is 8192 bytes.


################################################################################

With Serial initialization and a delay in loop, and with VoidSink:

#define DO_LOG_SINK 0
#define DO_VOID_SINK 1

Sketch uses 1932 bytes (0%) of program storage space. Maximum is 253952 bytes.
Global variables use 184 bytes (2%) of dynamic memory, leaving 8008 bytes for local variables. Maximum is 8192 bytes.

WOOT!!! Same size as without 
*/

#define DO_LOG_SINK 1
#define DO_VOID_SINK 0



#if DO_LOG_SINK
class LogSink {
  public:
    LogSink(Print& out) : out_(out) {}
    ~LogSink() {
      out_.println();
    }

    template <typename T>
    friend LogSink& operator<<(LogSink& sink, const T& value) {
      sink.out_.print(value);
      return sink;
    }

    template <typename T>
    friend LogSink& operator<<(LogSink& sink, T& value) {
      sink.out_.print(value);
      return sink;
    }

  private:
    Print& out_;
};
#define MakeSink LogSink(Serial)

#endif

#if DO_VOID_SINK
class VoidSink {
  public:

    template <typename T>
    friend VoidSink operator<<(VoidSink sink, const T&) {
      return sink;
    }
};
#define MakeSink VoidSink()
#endif




void setup() {
  // Setup serial, wait for it to be ready so that our logging messages can be
  // read.
  Serial.begin(9600);
  while (!Serial) {}

#ifdef MakeSink
//  auto sink = MakeSink;
//  sink << "Start";
  if (true) ; else MakeSink << "Start";
#endif
}

void loop() {
  // Code to run repeatedly.
  delay(1000);
  
#ifdef MakeSink
  if (true) ; else MakeSink << 1;
//  auto sink = MakeSink;  
//  MakeSink << 1;
#endif
}
