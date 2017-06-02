#include <iostream>
#include <malloc.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "IMAPMail.hpp"

using namespace std;

bool IMAPMail::connect_tcp(void)
{
  if(!address.run_DNS_lookup())
    return false;
  server_fd = address.open_TCP_socket();
  return server_fd != -1;
}

void IMAPMail::log_SSL_errors(void)
{
  unsigned long err = ERR_get_error();
  while(err != 0)
  {
    char const* buf = ERR_error_string(err, nullptr);
    log() << buf << endl;
    err = ERR_get_error();
  }
}

bool IMAPMail::init_SSL(void)
{
  log() << "Initializing SSL" << endl;
  OpenSSL_add_ssl_algorithms();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  ctx = SSL_CTX_new(SSLv23_client_method());
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

bool IMAPMail::connect_ssl(void)
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

bool IMAPMail::show_certs(void)
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

bool IMAPMail::send_cmd(string id, string cmd)
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

bool IMAPMail::read_resp(string& res)
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

bool IMAPMail::expect_resp(string id, string& res)
{
  bool b = read_resp(res);
  if(!b)
    return false;
  string prefix = res.substr(0, id.length());
  if(prefix == id)
  {
    log() << "Prefix match: \"" << prefix << '"' << endl;
    res = res.substr(id.length() + 1);
    return true;
  }
  else
  {
    log() << "Prefix: \"" << prefix << "\"; Expected: \"" << id << '"' << endl;
    return expect_resp(id, res);
  }
}

void IMAPMail::disconnect_ssl(void)
{
  SSL_free(ssl);
  ssl = nullptr;
}

void IMAPMail::disconnect_tcp(void)
{
  close(server_fd);
  server_fd = -1;
}

bool IMAPMail::update(void)
{
  success = false;
  if(ctx == nullptr)
  {
    if(!init_SSL())
      return false; // failed to initialize ssl, which is mandatory
  }

  if(!connect_tcp())
    return true; // just no internet
  if(!connect_ssl())
  {
    disconnect_tcp();
    return false; // could not securely connect
  }

  bool done = true;
  string result;
  string cmd = "LOGIN " + username + " " + password;
  done &= send_cmd("a001", cmd);
  done &= expect_resp("a001", result);
  cmd = "STATUS " + mailbox + " (UNSEEN)";
  done &= send_cmd("a002", cmd);
  done &= expect_resp("* STATUS", result);
  // the length of the mailbox-name word w/wo quotes
  auto mb_length = mailbox.length();
  string mb_name = result.substr(0, mb_length);
  if(mb_name != mailbox)
  {
    mb_name = result.substr(1, mb_length);
    if(mb_name != mailbox)
    {
      log() << "Mailbox name mismatch: " << mb_name << " <-> " << mailbox
            << endl;
      done = false;
    }
    else
    {
      mb_length += 2;
      log() << "Mailbox name match" << endl;
    }
  }
  // MAILBOXNAME (UNSEEN 1)
  unsigned long index = mb_length + 1 + 1 + 6;
  string unseen = result.substr(index);

  try
  {
    unseen_mails = std::stoi(unseen);
    success = true;
    log() << "You have " << unseen_mails << " unseen mails" << endl;
  }
  catch(std::invalid_argument&)
  {
    done = false;
    log() << "Could not parse unseen mails: " << unseen << endl;
  }

  done &= expect_resp("a002", result);
  done &= send_cmd("a003", "LOGOUT");
  done &= expect_resp("a003", result);

  disconnect_ssl();
  disconnect_tcp();
  log() << "Done " << done << endl;
  return success;
}

void IMAPMail::print(void)
{
  if(unseen_mails != 0 && success)
  {
    separate(Direction::left, Color::neutral, cout);
    cout << icon << " " << tag << ": " << unseen_mails << " unseen mails ";
    separate(Direction::left, Color::white_on_black, cout);
  }
}

IMAPMail::IMAPMail(JSON const& item) : StateItem(item), Logger("[IMAPMail]")
{
  icon = parse_icon(item.get("icon").as_string_with_default(""));

  hostname.assign(item["hostname"]);
  port = item["port"];
  address = Address(hostname, port);

  ca_cert.assign(item.get("ca_file").as_string_with_default(""));
  ca_cert.assign(item.get("ca_path").as_string_with_default(""));

  username.assign(item["username"]);
  password.assign(item["password"]);
  mailbox.assign(item["mailbox"]);

  tag.assign(item.get("tag").as_string_with_default(hostname + ":" + username));

  ctx = nullptr;
  server_fd = -1;
  ssl = nullptr;

  unseen_mails = 0;
  success = false;
}

IMAPMail::~IMAPMail(void)
{
  if(ssl != nullptr)
    SSL_free(ssl);
  if(server_fd != -1)
    close(server_fd);
  if(ctx != nullptr)
    SSL_CTX_free(ctx);
}
