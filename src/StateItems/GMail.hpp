#ifndef __GMAILREQUIRESOAUTH___
#define __GMAILREQUIRESOAUTH___

#include <string>
#include <resolv.h>
#include <openssl/ssl.h>
#include "../StateItem.hpp"
#include "../JSON/jsonParser.hpp"
#include "../util.hpp"
#include "../Address.hpp"

class GMail : public StateItem, public Logger
{

private:
  static std::string ca_cert;
  Address address;

  std::string username;
  std::string password;

  int server_fd;

  SSL_METHOD const* mth;
  SSL_CTX* ctx;
  SSL* ssl;

  void get_address(void);
  void connect_tcp(void);
  void log_SSL_errors(void);
  void init_SSL(void);
  void connect_ssl(void);
  void show_certs(void);
  void disconnect_ssl(void);
  void disconnect_tcp(void);

  bool update(void);
  void print(void);

public:
  static void settings(JSONObject&);
  GMail(JSONObject& item);
  virtual ~GMail();
};

#endif
