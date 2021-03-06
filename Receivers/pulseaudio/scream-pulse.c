#include <stdio.h>
#include <string.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define MULTICAST_TARGET "239.255.77.77"
#define MULTICAST_PORT 4010
#define MAX_SO_PACKETSIZE 1764
#define MIN_PA_PACKETSIZE MAX_SO_PACKETSIZE
#define BUFFER_SIZE MIN_PA_PACKETSIZE * 2

int main(int argc, char*argv[]) {
  int sockfd, error;
  ssize_t n;
  int offset;
  struct sockaddr_in servaddr;
  struct ip_mreq imreq;
  pa_simple *s;
  pa_sample_spec ss;
  unsigned char buf[BUFFER_SIZE];

  ss.format = PA_SAMPLE_S16NE;
  ss.channels = 2;
  ss.rate = 44100;
  s = pa_simple_new(NULL,
    "Scream",
    PA_STREAM_PLAYBACK,
    NULL,
    "Audio",
    &ss,
    NULL,
    NULL,
    NULL
  );
  if (!s) goto BAIL;

  sockfd = socket(AF_INET,SOCK_DGRAM,0);

  memset((void *)&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(MULTICAST_PORT);
  bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  imreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_TARGET);
  imreq.imr_interface.s_addr = INADDR_ANY;

  setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
            (const void *)&imreq, sizeof(struct ip_mreq));

  offset = 0;
  for (;;) {
    n = recvfrom(sockfd, &buf[offset], MAX_SO_PACKETSIZE, 0, NULL, 0);
    if (n > 0) {
      offset += n;
      if (offset >= MIN_PA_PACKETSIZE) {
        if (pa_simple_write(s, buf, offset, &error) < 0) {
          printf("pa_simple_write() failed: %s\n", pa_strerror(error));
          goto BAIL;
        }
        offset = 0;
      }
    }
  }

  BAIL:
  if (s) pa_simple_free(s);
};
