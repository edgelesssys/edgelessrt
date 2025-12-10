#include <arpa/inet.h>
#include <netdb.h>
#include <openenclave/enclave.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <sys/socket.h>
#include <ttls/test_instances.h>
#include <unistd.h>

#include <string>

#include "test_t.h"

static void _test_client()
{
    constexpr std::string_view kRequest = "GET / HTTP/1.0\r\n\r\n";

    edgeless::ttls::TestCredentials credentials;
    const std::string ca_crt_encoded =
        edgeless::ttls::JSONescape(credentials.ca_crt);

    const std::string ttls_cfg =
        R"({
            "tls":
            {
                "Outgoing":
                {
                    "localhost:9000":
                        {
                            "cacrt": ")" +
        ca_crt_encoded + R"(",
         "clicrt":"",
          "clikey":""
                        }
                },
                "Incoming":
                {
                    "*:9999":
                    {
                        "cacrt": "","clicrt": "", "clikey": "", "clientAuth": false
                    }
                }
            }
            })";

    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    OE_TEST(fd != -1);

    ert_init_ttls(ttls_cfg.data());

    auto t1 = edgeless::ttls::StartTestServer(
        "9000",
        0, // MBEDTLS_SSL_VERIFY_NONE
        credentials.ca_crt,
        credentials.server_crt,
        credentials.server_key);

    sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(9000);
    inet_aton("127.0.0.1", &sock_addr_in.sin_addr);
    sockaddr sock_addr = reinterpret_cast<sockaddr&>(sock_addr_in);

    addrinfo* result = nullptr;
    OE_TEST(getaddrinfo("localhost", nullptr, nullptr, &result) == 0);
    OE_TEST(connect(fd, &sock_addr, sizeof(sock_addr)) == 0);
    OE_TEST(write(fd, kRequest.data(), kRequest.size()) == kRequest.size());

    std::string buf(4096, ' ');
    OE_TEST(read(fd, buf.data(), buf.size()) != -1);
    OE_TEST(buf.substr(9, 6) == "200 OK");

    OE_TEST(shutdown(fd, SHUT_RDWR) == 0);
    OE_TEST(close(fd) == 0);
    t1.join();
}

static void _test_server()
{
    constexpr std::string_view kResponse = "HTTP/1.0 200 OK\r\n\r\nBody\r\n";

    edgeless::ttls::TestCredentials credentials;
    const std::string ca_crt_encoded =
        edgeless::ttls::JSONescape(credentials.ca_crt);
    const std::string server_crt_encoded =
        edgeless::ttls::JSONescape(credentials.server_crt);
    const std::string server_key_encoded =
        edgeless::ttls::JSONescape(credentials.server_key);

    const std::string ttls_cfg =
        R"({
            "tls":
            {
                "Outgoing":
                {
                    "localhost:9000":
                        {
                            "cacrt": ")" +
        ca_crt_encoded + R"(",
                            "clicrt":"",
                            "clikey":""
                        }
                },
                "Incoming":
                {
                    "*:9010":
                    {
                        "cacrt": ")" +
        ca_crt_encoded + R"(",
        "clicrt": ")" +
        server_crt_encoded + R"(",
         "clikey": ")" +
        server_key_encoded + R"(",
        "clientAuth": true
                    }
                }
            }
            })";

    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    OE_TEST(fd != -1);

    ert_init_ttls(ttls_cfg.data());

    sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(9010);
    inet_aton("127.0.0.1", &sock_addr_in.sin_addr);
    sockaddr sock_addr = reinterpret_cast<sockaddr&>(sock_addr_in);

    OE_TEST(bind(fd, &sock_addr, sizeof(sockaddr)) == 0);
    OE_TEST(listen(fd, 10) == 0);

    auto t1 = edgeless::ttls::StartTestClient(
        "9010",
        true,
        credentials.ca_crt,
        credentials.cli_crt,
        credentials.cli_key);

    int client_fd = -1;
    do
    {
        client_fd = accept4(fd, nullptr, nullptr, 0);
    } while (client_fd == -1 && errno == EAGAIN);
    OE_TEST(client_fd > 0);

    std::string buf(4096, ' ');
    int ret = -1;
    do
    {
        ret = read(client_fd, buf.data(), buf.size());

    } while (ret == -1 && errno == EAGAIN);
    OE_TEST(ret > 0);
    OE_TEST(buf.substr(0, 3) == "GET");

    do
    {
        ret = write(client_fd, kResponse.data(), kResponse.size());
    } while (ret == -1 && errno == EAGAIN);
    OE_TEST(ret > 0);

    OE_TEST(shutdown(client_fd, SHUT_RDWR) == 0);
    OE_TEST(close(client_fd) == 0);

    t1.join();
    OE_TEST(close(fd) == 0);
}

void test_ecall()
{
    OE_TEST(oe_load_module_host_socket_interface() == OE_OK);
    _test_client();
    _test_server();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    256,  /* NumHeapPages */
    64,   /* NumStackPages */
    3);   /* NumTCS */
