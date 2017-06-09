#ifndef __GMAILREQUIRESOAUTH___
#define __GMAILREQUIRESOAUTH___

#include <openssl/ssl.h>
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
    Icon icon;
    int unseen_mails;
    bool success;

    int server_fd;

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
    IMAPMail(JSON const& item);
    virtual ~IMAPMail();
};

#endif
