#include <iostream>
#include <malloc.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../output.hpp"
#include "GMail.hpp"

using namespace std;

string GMail::ca_cert = "";
string GMail::ca_path = "";

string GMail::hostname = "localhost";
unsigned int GMail::port = 993;
Address GMail::address;

bool GMail::connect_tcp(void)
{
  if(!address.run_DNS_lookup())
    return false;
  server_fd = address.open_TCP_socket();
  return server_fd != -1;
}

void GMail::log_SSL_errors(void)
{
  unsigned long err = ERR_get_error();
  while(err != 0)
  {
    char const* buf = ERR_error_string(err, nullptr);
    log() << buf << endl;
    err = ERR_get_error();
  }
}

bool GMail::init_SSL(void)
{
  log() << "Initializing SSL" << endl;
  OpenSSL_add_ssl_algorithms();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  mth = TLSv1_method();
  ctx = SSL_CTX_new(mth);
  if(ctx == nullptr)
  {
    log_SSL_errors();
    return false;
  }

  log() << "Loading root certificates:" << endl;
  log() << "CA_File: " << ca_cert << endl;
  log() << "CA_Path: " << ca_path << endl;

  int ret = SSL_CTX_load_verify_locations(
      ctx,
      ca_cert.size() == 0 ? nullptr : ca_cert.c_str(),
      ca_path.size() == 0 ? nullptr : ca_path.c_str());
  if(ret == 0)
  {
    log() << "Failed to load certificates" << endl;
    log_SSL_errors();
    return false;
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
  return true;
}

bool GMail::connect_ssl(void)
{
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, server_fd);
  if(SSL_connect(ssl) == -1)
  {
    log() << "Failed to establish SSL session" << endl;
    log_SSL_errors();
    return false;
  }
  else
  {
    log() << "Connected with " << SSL_get_cipher(ssl) << " encryption" << endl;
    return show_certs();
  }
}

bool GMail::show_certs(void)
{

  X509* cert = SSL_get_peer_certificate(ssl);
  if(cert != nullptr)
  {
    char* line;
    log() << "Server certificates:" << endl;
    line = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
    log() << "Subject: " << line << endl;
    free(line);
    line = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
    log() << "Issuer: " << line << endl;
    free(line);
    X509_free(cert);
    return true;
  }
  else
    log() << "No certificates" << endl;
  return false;
}

bool GMail::send_cmd(string id, string cmd)
{
  cmd = id + " " + cmd + "\r\n";
  log() << "-> " << cmd.substr(0, 10) << "..." << endl;
  int bytes = SSL_write(ssl, cmd.c_str(), (int)cmd.length());
  if(bytes <= 0)
  {
    log_SSL_errors();
    return false;
  }
  else if(bytes < (int)cmd.length())
  {
    log() << "Only a part of the message has been transmitted!" << endl;
    return false;
  }
  return true;
}

bool GMail::read_resp(string& res)
{
  if(read_line_queue.size() == 0)
  {
    char buf[1024]; // assuming one line fits in 1023 characters
    int bytes = SSL_read(ssl, buf, 1024);
    if(bytes <= 0)
    {
      log_SSL_errors();
      return false;
    }
    buf[bytes] = '\0';

    int start = 0;
    for(int i = 0; i < bytes; i++)
    {
      if(buf[i] == '\n')
      {
        string line(buf + start, (uint32_t)(i - start));
        log() << "<- " << line << endl;
        read_line_queue.push(line);
        start = i + 1;
      }
    }

    if(start != bytes)
    {
      log() << "Received partial line (missing newline): " << endl
            << '\t' << string(buf + start) << endl;
    }
  }

  res = read_line_queue.front();
  read_line_queue.pop();
  return true;
}

bool GMail::expect_resp(string id, string& res)
{
  bool b = read_resp(res);
  if(!b)
    return false;
  string prefix = res.substr(0, id.length());
  log() << "Prefix: \"" << prefix << "\"; Expected: \"" << id << '"' << endl;
  if(prefix == id)
  {
    res = res.substr(id.length() + 1);
    return true;
  }
  else
    return expect_resp(id, res);
}

void GMail::disconnect_ssl(void)
{
  SSL_free(ssl);
  ssl = nullptr;
}

void GMail::disconnect_tcp(void)
{
  close(server_fd);
  server_fd = -1;
}

bool GMail::update(void)
{
  if(ctx == nullptr)
    if(!init_SSL())
      return false;

  if(!connect_tcp())
    return false;
  if(!connect_ssl())
  {
    disconnect_tcp();
    return false;
  }

  bool done = true;
  string result;
  string cmd = "LOGIN " + username + " " + password;
  done &= send_cmd("a001", cmd);
  done &= expect_resp("a001", result);
  cmd = "STATUS " + mailbox + " (UNSEEN)";
  done &= send_cmd("a002", cmd);
  done &= expect_resp("* STATUS", result);
  if(result.substr(1, mailbox.length()) != mailbox)
    done = false;
  //"INBOX" (UNSEEN 1)
  unsigned long index = 1 + mailbox.length() + 1 + 1 + 1 + 6;
  string unseen = result.substr(index);
  unseen_mails = std::stoi(unseen);
  log() << "You have " << unseen_mails << " unseen mails" << endl;
  done &= expect_resp("a002", result);
  done &= send_cmd("a003", "LOGOUT");
  done &= expect_resp("a003", result);

  disconnect_ssl();
  disconnect_tcp();

  return done;
}

void GMail::print(void)
{
  if(unseen_mails != 0)
  {
    separate(Left, neutral_colors, cout);
    print_icon(icon_mail, cout);
    cout << " You have " << unseen_mails << " unseen mails ";
    separate(Left, white_on_black, cout);
  }
}

void GMail::settings(JSONObject& config)
{
  hostname = config["hostname"].string();
  port = config["port"].number();
  address = Address(hostname, port);

  JSON* ca_file_ptr = config.has("ca_file");
  if(ca_file_ptr != nullptr)
    ca_cert = ca_file_ptr->string();
  JSON* ca_path_ptr = config.has("ca_path");
  if(ca_path_ptr != nullptr)
    ca_path = ca_path_ptr->string();
}

GMail::GMail(JSONObject& item) : StateItem(item), Logger("[GMail]", cerr)
{
  username = item["username"].string();
  password = item["password"].string();
  mailbox = item["mailbox"].string();

  ctx = nullptr;
  server_fd = -1;
  ssl = nullptr;
}

GMail::~GMail(void)
{
  if(ssl != nullptr)
    SSL_free(ssl);
  if(server_fd != -1)
    close(server_fd);
  if(ctx != nullptr)
    SSL_CTX_free(ctx);
}
