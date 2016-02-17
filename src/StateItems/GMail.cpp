#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "GMail.hpp"

using namespace std;

void GMail::get_address(void)
{
  if(!address.run_DNS_lookup())
  {
  }
}

void GMail::connect_tcp(void)
{
  server_fd = address.open_TCP_socket();
  if(server_fd == -1)
  {
  }
}

void GMail::log_SSL_errors(void)
{
  unsigned long err = ERR_get_error();
  while(err != 0)
  {
    char const* buf = ERR_error_string(err, nullptr);
    log() << buf << endl;
  }
}

void GMail::init_SSL(void)
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
    return;
  }

  int ret = SSL_CTX_load_verify_locations(ctx, ca_cert.c_str(), nullptr);
  if(ret == 0)
  {
    log_SSL_errors();
    return;
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
}

void GMail::connect_ssl(void)
{
  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, server_fd);
  if(SSL_connect(ssl) == -1)
  {
    log_SSL_errors();
    return;
  }
  else
  {
    log() << "Connected with " << SSL_get_cipher(ssl) << " encryption" << endl;
    show_certs();
    /*
    command(ssl, "a001 LOGIN user@gmail password\r\n");
    read_resp(ssl);
    command(ssl, "a002 STATUS INBOX (UNSEEN)\r\n"); // STATUS INBOX UNSEEN
    read_resp(ssl);*/
  }
}

void GMail::show_certs(void)
{
  X509* cert;
  char* line;

  cert = SSL_get_peer_certificate(ssl);
  if(cert != nullptr)
  {
    log() << "Server certificates:" << endl;
    line = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
    log() << "Subject: " << line << endl;
    free(line);
    line = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
    log() << "Issuer: " << line << endl;
    free(line);
    X509_free(cert);
  }
  else
    log() << "No certificates" << endl;
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

GMail::GMail(JSONObject& item) : StateItem(item), Logger("[GMail]", cerr) {}

GMail::~GMail(void)
{
  if(ssl != nullptr)
    SSL_free(ssl);
  if(server_fd != -1)
    close(server_fd);
  if(ctx != nullptr)
    SSL_CTX_free(ctx);
}

void command(SSL* ssl, string msg)
{
  SSL_write(ssl, msg.c_str(), msg.length());
  cout << "-> " << msg;
}
void read_resp(SSL* ssl)
{
  char buf[1024];
  memset(buf, 0, 1024);
  int bytes = SSL_read(ssl, buf, 1024); /* get reply & decrypt */
  buf[bytes] = '\0';
  cout << "<- " << buf;
}

int main()
{
  cout << "init" << endl;
  string hostname = "imap.gmail.com";
  int portnum = 993;
}
