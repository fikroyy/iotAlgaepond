#include <pgmspace.h>

#define SECRET

const char ssid[] = "#After21";
const char pass[] = "duarrrrr";

#define THINGNAME "AlgaePond"

int8_t TIME_ZONE = +7; //NYC(USA): -5 UTC

const char MQTT_HOST[] = "a22eoammq4iudf-ats.iot.us-east-1.amazonaws.com";

// Obtain First CA certificate for Amazon AWS
// https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication
// Copy contents from CA certificate here ▼
static const char cacert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// Copy contents from XXXXXXXX-certificate.pem.crt here ▼
static const char client_cert[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUU1XwEGHrRdqhy0+lL6jqs18E+iowDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIwMDExNzA1NTUz
MFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAM5Vdny71TUwoCE1+oul
fLiiCK0Qggyhbx9WmyLJLc+0HGB+zukyxViDTobYqRm2RaSfgOuTBKp4sbqIFYUd
90UxoyZ4pJl2rO/Ai4Bm+ODZ1Fu3k89ZiKIHdNI5p8TYOdOJQtvU0rAV9EuDzHbr
jRM83R7OH/yIAf+q0m72j2nZhwk3A6DIkE+uF+QU2CQBjPebmUmBZe5J/U8S50M8
sazZjVaZ9ltxK0RXip7YxzE00TYBznn2U8n6AqZYoRXrLssgcF9h1RjAEuVPz3vk
5ZT2PGYb0DIymFxUnZ67uAFuYBUkVE5awKCRK3SNjfvhlVDrlV09yyhIsxaJ12zO
jgMCAwEAAaNgMF4wHwYDVR0jBBgwFoAUD39k7kdZn1lr4JXaTWsuuGRV0GYwHQYD
VR0OBBYEFOhZrx/Uz8FdxyGaM1RBUyOt+kjtMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAsgZpOsqLXPxuAlcerGcD6WWzx
AkflIF7Z65KHnngu2/741yoHESPsZ1bdrvn6cHUvKrhFh3FQmmJXzbV3ZMSmaU1D
uRPFI7DDmahr0+GLTERqEqzGrlgr2cbqaHglbi45ilf/fRaG6S0bz/4y/87Pii+f
93/B+S53GR/MrBcUwtl2JuXoxK5J6GqU4GUaYd8VhX9LOJrVkNdtTHLaoQlL06IV
fjUNZNT6V93qAvK8cokJmlK80AuU735ixIWto8W0EV4LaxseatTmqkfcqXuts5m7
eVjW37R2mde5oxCQfPZKN8xExdEN8XoBbBD34KJ8ObV4D8S6nEnNVKYV8f8w
-----END CERTIFICATE-----
)KEY";

// Copy contents from  XXXXXXXX-private.pem.key here ▼
static const char privkey[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAzlV2fLvVNTCgITX6i6V8uKIIrRCCDKFvH1abIsktz7QcYH7O
6TLFWINOhtipGbZFpJ+A65MEqnixuogVhR33RTGjJnikmXas78CLgGb44NnUW7eT
z1mIogd00jmnxNg504lC29TSsBX0S4PMduuNEzzdHs4f/IgB/6rSbvaPadmHCTcD
oMiQT64X5BTYJAGM95uZSYFl7kn9TxLnQzyxrNmNVpn2W3ErRFeKntjHMTTRNgHO
efZTyfoCplihFesuyyBwX2HVGMAS5U/Pe+TllPY8ZhvQMjKYXFSdnru4AW5gFSRU
TlrAoJErdI2N++GVUOuVXT3LKEizFonXbM6OAwIDAQABAoIBAQCh5d8dykhl/ux2
gqM/ta7iNIm4Oe7BOF436b26kr52bi2h7X4u7p54cDdKiXBN8347yMfgkDAmDqPt
599y3utpiKQqiJkmrT7OtHcBXkrEK0d8W9QH64nRESax2Xlxc/QTrm0ejqRudBuG
sTjaysKcKLKb/EKO5K0kzgVl+eLXuuRgzcL3y6HOGNejj9ol6ZfXQe4gowxw3kTj
9aesXFrMTtHaKIGq7Mii5vcOGpZbwFZi4af8VEunmGD8hXcLJFmaxY1SSiHjyVMm
c/99ko4Cfv7jPOSvaJQIH1Oj0kCzjS6YOyUDvUnrco0pq9zhppCJyL8Tl1RqCRBm
RQD4K1L5AoGBAPliqJVE7QGp5M7+mUCTucenO3Nq6LcVg8TzGIhXQU/1aS82hCQX
NvQ3mBGFsjeMJ85r9LBPOEZzec951u8wi8Fshlzl+pASwOmWn0glqWS/7iW40sSu
U1UUasi6hNFVQGuC08Wb4L5XRa5KKkI7/HV/iEkjxELVDH+VY8obqSv/AoGBANPO
e1Xc25gT8DJT47M23v5L6UJCDq2989uK2yneKDNhS8lEzvnNWlehJMMYebEVGCbU
yMaysFoS/+aaVL4sqk/1AEKzB7cEb/w61dYhfKgUuWJru2Cc6/SQtF7ie4eQE9af
+fpwQOY9YmfzrREbgPg1bP3BLTmPtZmq+C+DHe39AoGANgAd/wyIwZZ3PDc3Ghui
28+jeO+KarKW8Nuu0T8LnnPGjfmhDWuVc7ZiEFFB8PNBX1pTBqZwyhiRJNL+Lpb1
fDGyuoT2B6J38VyzKt002+MPf02RHhOdacvc/5Ab2HQ8Wctfbee995lQNlhK5EK2
/P5blXLQOaSGa03+6LGqpSMCgYBJnYhsbVi9YKCpPVL0pOuYQtYNu1vSsySgd4B4
6sfaI8TtbUbqt57guhdG36jcDVLZnDc0KEL264eZpKYzet6u755pkEPsLlSlu+U6
iWUEHj/Yn0z+5Ut1Mx4dpGX+1eqvO+bsTjugSlF7g8vxGDgZDKiXAZ8wVGxvvmaI
TPAIGQKBgCUg7Y4EKBPRSV+hHlxCLsKh/95H+bwfTvLRow0i1aaA4OHkFz0GBuBV
AvyBi/lOL09KBP3/BlhJ88wJrHadXSom7HSgAIEL5QQKCovzHdc2pHSVEoh5s4wa
0FoPUP+2ZVIfILR6Y3VgrbWJEts8PhJ+QvR+EjOSN3Qcat4vxzj9
-----END RSA PRIVATE KEY-----
)KEY";
