#ifndef __STATIC_ELECTRICITY__
#define __STATIC_ELECTRICITY__

class Application
{
  private:
    Application() = delete;

  public:
    static bool dead;
    static bool force_update;
    static int exit_status;
    static int argc;
    static char** argv;
};

#endif
