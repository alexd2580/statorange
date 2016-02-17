#ifndef __GMAILREQUIRESOAUTH___
#define __GMAILREQUIRESOAUTH___

#include "../Address.hpp"
#include "../JSON/jsonParser.hpp"
#include "../StateItem.hpp"
#include "../util.hpp"
#include <openssl/ssl.h>
#include <queue>
#include <resolv.h>
#include <string>

class GMail : public StateItem, public Logger
{

private:
  static std::string ca_cert;
  static std::string ca_path;

  static std::string hostname;
  static unsigned int port;
  static Address address;

  std::string username;
  std::string password;
  std::string mailbox;

  int unseen_mails;

  int server_fd;

  SSL_METHOD const* mth;
  SSL_CTX* ctx;
  SSL* ssl;

  bool connect_tcp(void);
  void log_SSL_errors(void);
  bool init_SSL(void);
  bool connect_ssl(void);
  bool show_certs(void);

  std::queue<std::string> read_line_queue;

  bool send_cmd(std::string id, std::string cmd);
  bool read_resp(std::string& res);
  bool expect_resp(std::string id, std::string& res);

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
