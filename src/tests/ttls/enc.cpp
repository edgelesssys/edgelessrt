#include <arpa/inet.h>
#include <openenclave/enclave.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <sys/socket.h>
#include <ttls/ttls.h>
#include <unistd.h>
#include <string>

#include "test_t.h"

void test_ecall()
{
    const std::string kCACrt =
        "-----BEGIN CERTIFICATE-----\\r\\n"
        "MIIFqzCCA5OgAwIBAgIUbBY17peevr4MypRtXYzUiJ1qVEgwDQYJKoZIhvcNAQEL\\r\\n"
        "BQAwWjELMAkGA1UEBhMCVVMxDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5\\r\\n"
        "MQwwCgYDVQQKDANPcmcxDDAKBgNVBAsMA09yZzEQMA4GA1UEAwwHVGVzdCBDQTAe\\r\\n"
        "Fw0yMTAzMDgwNDAzMzFaFw0yMjAzMDgwNDAzMzFaMFoxCzAJBgNVBAYTAlVTMQ4w\\r\\n"
        "DAYDVQQIDAVTdGF0ZTENMAsGA1UEBwwEQ2l0eTEMMAoGA1UECgwDT3JnMQwwCgYD\\r\\n"
        "VQQLDANPcmcxEDAOBgNVBAMMB1Rlc3QgQ0EwggIiMA0GCSqGSIb3DQEBAQUAA4IC\\r\\n"
        "DwAwggIKAoICAQC9JZlQzti1uf+ayrAi1KZf/wjjgaDvHmFR5bVbGXhxT2woAQTk\\r\\n"
        "CSAslX1JLOKrijR5QLEN9K+2OX5ylRvk6CPaeamZRWW7kOlQVGZ6wGHrTZgADSGV\\r\\n"
        "qArHkA3oTlJrOkY3/wh/BQ7G7FIA10EzEG5VAqDAxsnsXGUP3FtckUubPktOGDDA\\r\\n"
        "oSpkLtwVGQPxcCWZt3MHH3iHYrNH6ClWaKCV5wWnuBWOFZtK5lyVMnZEvo4JCn/R\\r\\n"
        "yE0g46f5lF0cksP8+2D9og0StTru14+Mtf0mcFHO73w6O9UydKnjYPdagXrIB7P9\\r\\n"
        "VzTa68XrGnubarkRg9+WQ090Ud6/x3x0aZ8JIpLxBVdGQotQtJ8WSTkrdbHT4aIK\\r\\n"
        "A0AhgoAsSQ5iIPRQ1sMYeIzw+dtDoITdnLRszP9p8p6sKosUAu0yJFkkzucq1mY3\\r\\n"
        "kFRjf4axgtYVMBK8iDxmUcwyasuyRO+faLmlig8oAk3q69qIzXiy+ZXWPqfVkiDQ\\r\\n"
        "CD42S43cBzksLOR9LsPgOxT0oexlgpTnxgofzvxL/gH2ATZw4gZwOgtF8yw/eEps\\r\\n"
        "5HBVO+W8O8LTHhm+dIQL5d+jl9qAqMVYJB+yUnn+otLRdYRo4jGquL0eJbAhwZMK\\r\\n"
        "GhT/d8+E07wHY5nLUu1KYQC6xNVAKhBRKYWrkuSBwb/bHt0QPH5+lEF0+wIDAQAB\\r\\n"
        "o2kwZzAdBgNVHQ4EFgQUOa4Ai603WGFja/LhBJ28JI6ACfswHwYDVR0jBBgwFoAU\\r\\n"
        "Oa4Ai603WGFja/LhBJ28JI6ACfswDwYDVR0TAQH/BAUwAwEB/zAUBgNVHREEDTAL\\r\\n"
        "gglsb2NhbGhvc3QwDQYJKoZIhvcNAQELBQADggIBAGsGrFH3VA1FlA2humEfWhgw\\r\\n"
        "2B1a7gCjZdn5NYTPjjLZuJWR8+MGpuHmkxBMjaheKGx6yy9UfWRJV1horR14DRvf\\r\\n"
        "oV8yhNWE38lLuqqIQxQvXNr211xxav9QO3FEIV7/yOkRHPtt06gJnlm/eABREhBJ\\r\\n"
        "iOKNsLByExi6Sbracf0A9cDvjzt8HJEcuLuPgeTraO3bVehrH1f+5IKO1dNFaap+\\r\\n"
        "QIbtAf0ddI3tVm21CxgD1bLZ+y6imtyxZ2jvo4Ie8/rSrYQMSIoC9v4W9qVE/eZG\\r\\n"
        "QBoVHdbWquxlo0xHXyXdkjrloksrEQeAXdf1noKPg6/0n0n2LQsMyCynQtcaLcfU\\r\\n"
        "V5jdzKiAasg83qa+sc+uJ1daDD4zInrMc4WjX9/a0pXUOQdg/cbh3GlpECrOW0xz\\r\\n"
        "lMAx945HSbt6+YLaPwMEU1CBtNBRU040cUmEhYK9pQzZ6GLZYcvJ+2B5YVaEAW9U\\r\\n"
        "YJOtg14sF6rkCJ7TTHeMXgytZLK9vg6OwZt2M8gr3hty/S0k1EDz1MZqw/eXP36S\\r\\n"
        "UjuKPJ+A4G88GMAtx1Tfce6Rb+ecSjdopw2AQbdssIYjbkTjbHFP1Tl8YooBMDXF\\r\\n"
        "6e8E8mLPSC23bAYGfqSlHmZ0tI8UxnpoVpFTt803beDEF4z2dcpMRfcWmjZHO2zh\\r\\n"
        "7Oe8km7JBDiS8Av4cPe9\\r\\n"
        "-----END CERTIFICATE-----\\r\\n";

    constexpr std::string_view kRequest = "GET / HTTP/1.0\r\n\r\n";
    const auto ttls_cfg =
        "{\"tls\":{\"127.0.0.1:9000\": \" " + kCACrt + " \" }}";
    OE_TEST(oe_load_module_host_socket_interface() == OE_OK);

    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    OE_TEST(fd != -1);

    ert_init_ttls(ttls_cfg.data());

    auto t1 = edgeless::ttls::start_test_server();

    sockaddr_in sock_addr_in{};
    sock_addr_in.sin_family = AF_INET;
    sock_addr_in.sin_port = htons(9000);
    inet_aton("127.0.0.1", &sock_addr_in.sin_addr);
    sockaddr sock_addr = reinterpret_cast<sockaddr&>(sock_addr_in);

    OE_TEST(connect(fd, &sock_addr, sizeof(sock_addr)) == 0);
    OE_TEST(write(fd, kRequest.data(), kRequest.size()) == kRequest.size());

    std::string buf(4096, ' ');
    OE_TEST(read(fd, buf.data(), buf.size()) != -1);
    OE_TEST(buf.substr(9, 6) == "200 OK");

    OE_TEST(shutdown(fd, 2) == 0);
    OE_TEST(close(fd) == 0);
    t1.join();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    128,  /* NumHeapPages */
    64,   /* NumStackPages */
    2);   /* NumTCS */
