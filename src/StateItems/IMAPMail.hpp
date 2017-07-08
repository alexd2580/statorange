#ifndef __GMAILREQUIRESOAUTH___
#define __GMAILREQUIRESOAUTH___

#include <functional>
#include <openssl/ssl.h>
#include <ostream>
#include <queue>
#include <resolv.h>
#include <string>

#include "../Address.hpp"
#include "../StateItem.hpp"
#include "../json_parser.hpp"
#include "../output.hpp"
#include "../util.hpp"

class IMAPMail final : public StateItem
{
  private:
    std::string const ca_cert;
    std::string const ca_path;

    std::string const hostname;
    uint32_t const port;
    Address address;

    std::string const username;
    std::string const password;
    std::string const mailbox;

    std::string const tag;
    int unseen_mails;
    bool success;

    int server_fd;

    SSL_CTX* ctx;
    SSL* ssl;

    std::queue<std::string> read_line_queue;

  public:
    IMAPMail(JSON::Node const& item);
    ~IMAPMail(void);

  private:
    bool with_tcp(std::function<bool(void)>);
    void log_SSL_errors(void);
    bool init_SSL(void);
    bool show_certs(void);
    bool with_ssl(std::function<bool(void)>);

    bool send_cmd(std::string id, std::string cmd);
    bool read_resp(std::string& res);
    bool expect_resp(std::string id, std::string& res);

    bool communicate(void);
    bool update(void) override;
    void print(std::ostream&, uint8_t) override;
};

#endif
