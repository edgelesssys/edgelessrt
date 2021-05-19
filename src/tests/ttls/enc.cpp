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

const std::string kCACrt =
    "-----BEGIN CERTIFICATE-----\\r\\n"
    "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\\r\\n"
    "7Oe8km7JBDiS8Av4cPe9\\r\\n"
    "-----END CERTIFICATE-----\\r\\n";

const std::string kServerCert =
    "-----BEGIN CERTIFICATE-----\\r\\n"
    "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\\r\\n"
    "-----END CERTIFICATE-----\\r\\n";

const std::string kServerKey =
    "-----BEGIN PRIVATE KEY-----\\r\\n"
    "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\\r\\n"
    "FixD4ZKk1BloAg68xzWdqF6I28BAetSQ\\r\\n"
    "-----END PRIVATE KEY-----\\r\\n";

static void _test_client()
{
    constexpr std::string_view kRequest = "GET / HTTP/1.0\r\n\r\n";
    const std::string ttls_cfg =
        R"({
            "tls":
            {
                "Outgoing":
                {
                    "localhost:9000":
                        {
                            "cacrt": ")" +
        kCACrt + R"(",
         "clicert":"",
          "clikey":""
                        }
                },
                "Incoming":
                {
                    "*:9999":
                    {
                        "cacrt": "","clicert": "", "clikey": ""
                    }
                }
            }
            })";

    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    OE_TEST(fd != -1);

    ert_init_ttls(ttls_cfg.data());

    auto t1 = edgeless::ttls::StartTestServer(0); // MBEDTLS_SSL_VERIFY_NONE

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
    const std::string ttls_cfg =
        R"({
            "tls":
            {
                "Outgoing":
                {
                    "localhost:9000":
                        {
                            "cacrt": ")" +
        kCACrt + R"(",
                            "clicert":"",
                            "clikey":""
                        }
                },
                "Incoming":
                {
                    "*:9010":
                    {
                        "cacrt": ")" +
        kCACrt + R"(",
        "clicert": ")" +
        kServerCert + R"(",
         "clikey": ")" +
        kServerKey + R"("
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

    auto t1 = edgeless::ttls::StartTestClient(true);

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
    128,  /* NumHeapPages */
    64,   /* NumStackPages */
    3);   /* NumTCS */
