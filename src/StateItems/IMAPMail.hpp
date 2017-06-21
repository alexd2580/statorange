#ifndef __GMAILREQUIRESOAUTH___
#define __GMAILREQUIRESOAUTH___

#include <functional>
#include <openssl/ssl.h>
#include <ostream>
#include <queue>
#include <resolv.h>
#include <string>

#include "../Address.hpp"
#include "../JSON/json_parser.hpp"
#include "../StateItem.hpp"
#include "../output.hpp"
#include "../util.hpp"

class IMAPMail : public StateItem
{
  private:
    std::string ca_cert;
    std::string ca_path;

    std::string hostname;
    unsigned int port;
    Address address;

    std::string username;
    std::string password;
    std::string mailbox;

    std::string tag;
    int unseen_mails;
    bool success;

    int server_fd;

    SSL_CTX* ctx;
    SSL* ssl;

    std::queue<std::string> read_line_queue;

  public:
    IMAPMail(JSON const& item);
    virtual ~IMAPMail();

  private:
    bool with_tcp(std::function<bool(void)>);
    void log_SSL_errors(void);
    bool init_SSL(void);
    bool show_certs(void);
    bool with_ssl(std::function<bool(void)>);

    bool send_cmd(std::string id, std::string cmd);
    bool read_resp(std::string& res);
    bool expect_resp(std::string id, std::string& res);

    bool update(void);
    bool communicate(void);
    void print(std::ostream&, uint8_t);
};

#endif
