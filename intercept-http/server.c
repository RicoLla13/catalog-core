/* SPDX-License-Identifier: BSD-3-Clause */

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_PORT 8080
#define BUFLEN 2048

static const char reply[] = "HTTP/1.1 200 OK\r\n"
			    "Content-Type: text/plain\r\n"
			    "Content-Length: 32\r\n"
			    "Connection: close\r\n"
			    "\r\n"
			    "Hello from intercept-http on UK\n";
static char recvbuf[BUFLEN];

int main(int argc __attribute__((unused)),
	 char *argv[] __attribute__((unused)))
{
	int rc = 0;
	int srv;
	int client;
	ssize_t n;
	struct sockaddr_in srv_addr;

	/* Small intercept preflight before the sample switches to local guest sockets. */
	errno = 0;
	rc = access("/tmp", F_OK);
	if (rc == 0) {
		printf("intercept preflight: access(\"/tmp\", F_OK) rc=%d errno=%d\n",
		       rc, errno);
	} else {
		fprintf(stderr,
			"intercept preflight failed: access(\"/tmp\", F_OK) rc=%d errno=%d (%s)\n",
			rc, errno, strerror(errno));
	}

	srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv < 0) {
		fprintf(stderr, "Failed to create socket: %d\n", errno);
		goto out;
	}

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = INADDR_ANY;
	srv_addr.sin_port = htons(LISTEN_PORT);

	rc = bind(srv, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
	if (rc < 0) {
		fprintf(stderr, "Failed to bind socket: %d\n", errno);
		goto out_close_srv;
	}

	rc = listen(srv, 1);
	if (rc < 0) {
		fprintf(stderr, "Failed to listen on socket: %d\n", errno);
		goto out_close_srv;
	}

	/* The HTTP loop itself stays on the normal lwIP-backed socket path. */
	printf("Listening on port %d...\n", LISTEN_PORT);
	while (1) {
		client = accept(srv, NULL, 0);
		if (client < 0) {
			fprintf(stderr,
				"Failed to accept incoming connection: %d\n",
				errno);
			goto out_close_srv;
		}

		read(client, recvbuf, BUFLEN);

		/* Reply with a fixed body so the sample focuses on transport behavior. */
		n = write(client, reply, sizeof(reply) - 1);
		if (n < 0)
			fprintf(stderr, "Failed to send a reply\n");
		else
			printf("Sent a reply\n");

		close(client);
	}

out_close_srv:
	close(srv);
out:
	return rc;
}
